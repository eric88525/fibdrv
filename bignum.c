#include "bignum.h"



#define MAX(x, y) ((x) > (y) ? (x) : (y))

bignum *bn_init(size_t size)
{
    // create bn obj
    bignum *new_bn = kmalloc(sizeof(bignum), GFP_KERNEL);

    // init data
    new_bn->number = kmalloc(size, GFP_KERNEL);
    memset(new_bn->number, 0, size);

    // init size and sign
    new_bn->size = size;
    new_bn->sign = 0;

    return new_bn;
}

// create bignum from int
bignum *bn_from_int(int x)
{
    if (x < 10) {
        bignum *result = bn_init(1);
        result->number[0] = x;
        return result;
    }

    bignum *result = bn_init(10);

    size_t size = 0;

    while (x) {
        result->number[size] = x % 10;
        x /= 10;
        size++;
    }
    bn_resize(result, size);
    return result;
}

// resize bignum to target size
int bn_resize(bignum *src, size_t size)
{
    if (!src)
        return -1;

    if (size == src->size)
        return 0;

    if (size == 0)  // prevent krealloc(0) = kfree, which will cause problem
        return bn_free(src);

    src->number = krealloc(src->number, size, GFP_KERNEL);

    if (!src->number)  // realloc fails
        return -1;

    if (size > src->size)
        memset(src->number + src->size, 0, size - src->size);

    src->size = size;
    return 0;
}

// add two bignum and store at result
void bn_add(bignum *a, bignum *b, bignum *result)
{
    if (a->sign && !b->sign) {  // a neg, b pos, do b-a
        return;
    } else if (!a->sign && b->sign) {  // a pos, b neg, do a-b
        return;
    }

    // pre caculate how many digits that result need
    int digit_width = MAX(a->size, b->size) + 1;
    bn_resize(result, digit_width);

    unsigned char carry = 0;  // store add carry

    for (int i = 0; i < digit_width - 1; i++) {
        unsigned char temp_a = i < a->size ? a->number[i] : 0;
        unsigned char temp_b = i < b->size ? b->number[i] : 0;

        carry += temp_a + temp_b;

        result->number[i] = carry - (10 & -(carry > 9));
        carry = carry > 9;
    }

    if (carry) {
        result->number[digit_width - 1] = 1;
    } else {
        bn_resize(result, digit_width - 1);
    }

    result->sign = a->sign;
}

// bn to string
char *bn_to_str(bignum *src)
{
    size_t len = sizeof(char) * src->size + 1;
    char *p = kmalloc(len, GFP_KERNEL);
    memset(p, 0, len - 1);
    int n = src->size - 1;
    for (int i = 0; i < src->size; i++) {
        p[i] = src->number[n - i] + '0';
    }

    return p;
}

// free bignum
int bn_free(bignum *src)
{
    if (src == NULL)
        return -1;
    kfree(src->number);
    kfree(src);
    return 0;
}

void bn_swap(bignum *a, bignum *b)
{
    bignum tmp = *a;
    *a = *b;
    *b = tmp;
}

/*int bn_cpy(bignum *dest, bignum *src)
{
    if (bn_resize(dest, src->size) < 0)
        return -1;
    dest->sign = src->sign;
    memcpy(dest->number, src->number, src->size * sizeof(int));
    return 0;
}*/

void bn_fib_sequence(bignum *dest, long long k)
{
    bn_resize(dest, 1);

    if (k < 2) {
        dest->number[0] = k;
        return;
    }

    bignum *a = bn_from_int(0);
    bignum *b = bn_from_int(0);
    dest->number[0] = 1;

    for (int i = 2; i <= k; i++) {
        bn_swap(b, dest);
        bn_add(a, b, dest);
        bn_swap(a, b);
    }

    bn_free(a);
    bn_free(b);
}
