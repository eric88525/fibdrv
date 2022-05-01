#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>

/* TODO: implement custom memory allocator which fits arbitrary precision
 * operations
 */
static inline void *xmalloc(size_t size)
{
    void *p;
    if (!(p = kzalloc(size, GFP_KERNEL))) {
        printk("Out of memory.\n");
        return NULL;
    }
    return p;
}

static inline void *xrealloc(void *ptr, size_t size)
{
    void *p;
    if (!(p = krealloc(ptr, size, GFP_KERNEL)) && size != 0) {
        printk("Out of memory.\n");
        return NULL;
    }
    return p;
}

static inline void xfree(void *ptr)
{
    kfree(ptr);
}

#define MALLOC(n) xmalloc(n)
#define REALLOC(p, n) xrealloc(p, n)
#define FREE(p) xfree(p)

#endif /* !_MEMORY_H_ */
