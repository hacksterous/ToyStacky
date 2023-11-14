/* From http://www.hanshq.net/bigint.html */

#ifndef BIGINT_H
#define BIGINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
        int32_t length : 31;
        uint32_t negative : 1;
        uint32_t data[129];
} bigint_t;

/* Create a bigint from n-character string str, consisting of one or more
   decimal characters, with an optional preceding hyphen. */
bool bigint_create_str(char *str, bigint_t *res);

/* Get the maximum required string length for x. */
size_t bigint_max_stringlen(const bigint_t *x);

/* Convert bigint to decimal string. */
bool bigint_tostring(const bigint_t *x, char *str);

/* string is a bigint */
bool isBigIntDecString(const char *str);

/* Print a bigint to stdout in decimal. */
void bigint_print(bigint_t *x);

/* Arithmetic. Division does truncation towards zero, and the remainder will
   have the same sign as the divisor. Division by zero is not allowed. */
void bigint_max(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_min(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_add(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_sub(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_mul(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_div(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_rem(const bigint_t *x, const bigint_t *y, bigint_t *res);
void bigint_neg(const bigint_t *x, bigint_t *res);

/* Comparison: returns -1 if x < y, 1 if x > y, and 0 if they are equal. */
int bigint_cmp(const bigint_t *x, const bigint_t *y);
int bigint_gt(const bigint_t *x, const bigint_t *y);
int bigint_lt(const bigint_t *x, const bigint_t *y);
int bigint_gte(const bigint_t *x, const bigint_t *y);
int bigint_lte(const bigint_t *x, const bigint_t *y);
int bigint_eq(const bigint_t *x, const bigint_t *y);
int bigint_neq(const bigint_t *x, const bigint_t *y);

/* Check whether x is zero. */
bool bigint_is_zero(const bigint_t *x);

#endif
