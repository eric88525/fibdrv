#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "bn.h"
#include "fibonacci.h"
#include "mybignum.h"


MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 500

#define TIME_PROXY(fib_f, result, k, timer)             \
    ({                                                  \
        timer = ktime_get();                            \
        result = fib_f(k);                              \
        timer = (size_t) ktime_sub(ktime_get(), timer); \
    });

#define BN_TIME_PROXY(fib_f, result, k, timer)          \
    ({                                                  \
        timer = ktime_get();                            \
        fib_f(*offset, result);                         \
        timer = (size_t) ktime_sub(ktime_get(), timer); \
    });

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;

#define MUTEX

#ifdef MUTEX
static DEFINE_MUTEX(fib_mutex);
#endif

// the timer to caculate lernel fib runtime
static ktime_t timer;

static void escape(void *p)
{
    __asm__ volatile("" : : "g"(p) : "memory");
}

// the normal version of fib
static long long fib_sequence(long long k)
{
    if (k == 0)
        return 0;
    else if (k <= 2)
        return 1;

    unsigned long long a, b;
    a = 0;
    b = 1;

    for (int i = 2; i <= k; i++) {
        unsigned long long temp = a + b;
        a = b;
        b = temp;
    }

    return b;
}

static long long fib_fastdoubling(long long n)
{
    if (n == 0)
        return 0;
    else if (n <= 2)
        return 1;
    // The position of the highest bit of n.
    // So we need to loop `h` times to get the answer.
    // Example: n = (Dec)50 = (Bin)00110010, then h = 6.
    //                               ^ 6th bit from right side
    unsigned int h = 0;
    for (unsigned int i = n; i; ++h, i >>= 1)
        ;

    uint64_t a = 0;  // F(0) = 0
    uint64_t b = 1;  // F(1) = 1
    // There is only one `1` in the bits of `mask`. The `1`'s position is same
    // as the highest bit of n(mask = 2^(h-1) at first), and it will be shifted
    // right iteratively to do `AND` operation with `n` to check `n_j` is odd or
    // even, where n_j is defined below.
    for (unsigned int mask = 1 << (h - 1); mask; mask >>= 1) {  // Run h times!
        // Let j = h-i (looping from i = 1 to i = h), n_j = floor(n / 2^j) = n
        // >> j (n_j = n when j = 0), k = floor(n_j / 2), then a = F(k), b =
        // F(k+1) now.
        uint64_t c = a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k+1) – F(k) ]
        uint64_t d = a * a + b * b;    // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & n) {  // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            a = d;       //   F(n_j) = F(2k + 1)
            b = c + d;   //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {         // n_j is even: k = n_j/2 => n_j = 2k
            a = c;       //   F(n_j) = F(2k)
            b = d;       //   F(n_j + 1) = F(2k + 1)
        }
    }

    return a;
}


// the normal version of fib
static long long fib_clz_fastdoubling(long long k)
{
    if (k == 0)
        return 0;
    else if (k <= 2)
        return 1;

    uint64_t a = 0;  // F(0) = 0
    uint64_t b = 1;  // F(1) = 1
    // There is only one `1` in the bits of `mask`. The `1`'s position is same
    // as the highest bit of n(mask = 2^(h-1) at first), and it will be shifted
    // right iteratively to do `AND` operation with `n` to check `n_j` is odd or
    // even, where n_j is defined below.
    for (unsigned int mask = 1 << (31 - __builtin_clz(k)); mask;
         mask >>= 1) {  // Run h times!
        // Let j = h-i (looping from i = 1 to i = h), n_j = floor(n / 2^j) =
        // n >> j (n_j = n when j = 0), k = floor(n_j / 2), then a = F(k),
        // b = F(k+1) now.
        uint64_t c = a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k+1) – F(k) ]
        uint64_t d = a * a + b * b;    // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & k) {  // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            a = d;       //   F(n_j) = F(2k + 1)
            b = c + d;   //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {         // n_j is even: k = n_j/2 => n_j = 2k
            a = c;       //   F(n_j) = F(2k)
            b = d;       //   F(n_j + 1) = F(2k + 1)
        }
    }
    return a;
}

static int fib_open(struct inode *inode, struct file *file)
{
#ifdef MUTEX
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
#endif
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
#ifdef MUTEX
    mutex_unlock(&fib_mutex);
#endif
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    if (size == 0) {
        return (ssize_t) fib_sequence(*offset);
    } else if (size == 1) {
        bignum *fib = my_bn_init(1);
        my_bn_fib_sequence(*offset, fib);
        char *p = my_bn_to_str(fib);
        size_t len = strlen(p) + 1;
        size_t left = copy_to_user(buf, p, len);
        my_bn_free(fib);
        kfree(p);
        return left;
    } else if (size == 2) {
        bn_t fib;
        bn_init(fib);
        // ref_fd_fibonacci(*offset, fib);

        ref_fibonacci(*offset, fib);
        char *p = kmalloc(100, GFP_KERNEL);
        bn_snprint(fib, 10, p, 99);

        size_t left = copy_to_user(buf, p, strlen(p) + 1);

        bn_free(fib);
        kfree(p);

        return left;
    }
    return 0;
}

/*
 * @ param size : the fibonacci mode
 *
 *
 */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t mode,
                         loff_t *offset)
{
    long long result = 0;
    bignum *fib = my_bn_init(1);
    bn_t ref_fib = BN_INITIALIZER;


    escape(fib);
    escape(&result);
    escape(ref_fib);

    switch (mode) {
    case 0: /* noraml */
        TIME_PROXY(fib_sequence, result, *offset, timer)
        break;
    case 1: /* fast doubling*/
        TIME_PROXY(fib_fastdoubling, result, *offset, timer)
        break;
    case 2: /*clz + fast doubling*/
        TIME_PROXY(fib_clz_fastdoubling, result, *offset, timer)
        break;
    case 3: /* my implementaion of bignum*/
        BN_TIME_PROXY(my_bn_fib_sequence, fib, *offset, timer)
        break;
    case 4: /* teacher's implementaion bn + fib*/
        BN_TIME_PROXY(ref_fibonacci, ref_fib, *offset, timer);
        break;
    case 5: /* teacher's implementaion bn +  fast doubling*/
        BN_TIME_PROXY(ref_fd_fibonacci, ref_fib, *offset, timer);
        break;
    default:
        my_bn_free(fib);
        bn_free(ref_fib);
        return (ssize_t) 0;
        break;
    }

    my_bn_free(fib);
    bn_free(ref_fib);
    return (ssize_t) ktime_to_ns(timer);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;
#ifdef MUTEX
    mutex_init(&fib_mutex);
#endif
    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
#ifdef MUTEX
    mutex_destroy(&fib_mutex);
#endif
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
