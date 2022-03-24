#ifndef BIGNUM_H
#define BIGNUM_H

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

typedef struct __bignum {
    unsigned char *number;  // store num by 1byte int
    unsigned int size;      // data length
    int sign;               // 0:negative ,1:positive
} bignum;

// create bignum
bignum *bn_init(size_t size);

// create bignum from int
bignum *bn_from_int(int x);

// resize bignum to target size
int bn_resize(bignum *src, size_t size);

// convert bignum to string
char *to_string(bignum *a);

// add two bignum
void bn_add(bignum *a, bignum *b, bignum *result);

// bn to string
char *bn_to_str(bignum *src);

// free bignum
int bn_free(bignum *src);

// copy src to dest
int bn_cpy(bignum *dest, bignum *src);

void bn_swap(bignum *a, bignum *b);

void bn_fib_sequence(bignum *dest, long long k);


#endif