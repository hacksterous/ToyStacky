/* From http://www.hanshq.net/bigint.html */

#include "bigint.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Add n-place integers u and v into (n + 1)-place w. */
static void algorithm_a(int n, const uint32_t *u, const uint32_t *v, uint32_t *w) {
	bool carry, carry_a, carry_b;
	uint32_t sum_a, sum_b;
	int j;

	carry = false;

	for (j = 0; j < n; j++) {
	sum_a = u[j] + carry;
	carry_a = (sum_a < u[j]);

	sum_b = sum_a + v[j];
	carry_b = (sum_b < sum_a);

	w[j] = sum_b;
	carry = carry_a + carry_b;
	}

	w[j] = carry;
}

/* Compare u with v, returning -1 if u < v, 1 if u > v, and 0 otherwise. */
static int cmp(int u_len, const uint32_t *u, int v_len, const uint32_t *v) {
	int i;

	if (u_len != v_len) {
		return u_len < v_len ? -1 : 1;
	}

	for (i = u_len - 1; i >= 0; i--) {
		if (u[i] != v[i]) {
			return u[i] < v[i] ? -1 : 1;
		}
	}

	return 0;
}

/* Compute w = u - v, where u, v, w are n-place integers, and u >= v. */
static void algorithm_s(int n, const uint32_t *u, const uint32_t *v, uint32_t *w) {
	bool borrow, borrow_a, borrow_b;
	uint32_t diff_a, diff_b;
	int j;

	borrow = false;

	for (j = 0; j < n; j++) {
		diff_a = u[j] - borrow;
		borrow_a = (diff_a > u[j]);

		diff_b = diff_a - v[j];
		borrow_b = (diff_b > diff_a);

		w[j] = diff_b;
		borrow = borrow_a + borrow_b;
	}

}

/*
   Multiply 32-bit numbers x and y into 64-bit (z_hi:z_lo).

   This maps to a single instruction on most 32-bit CPUs,
   including x86 and ARM.
 */
static void mul_32_by_32(uint32_t x, uint32_t y, uint32_t *z_hi, uint32_t *z_lo) {
	uint64_t prod;

	prod = (uint64_t)x * y;

	*z_hi = (uint32_t)(prod >> 32);
	*z_lo = (uint32_t)prod;
}

/* Multiply m-place u with n-place v, yielding (m + n)-place w. */
static void algorithm_m(int m, int n, const uint32_t *u, const uint32_t *v, uint32_t *w) {
	int i, j;
	uint32_t k, hi_prod, lo_prod;
	bool carry_a, carry_b;

	for (i = 0; i < m; i++) {
		w[i] = 0;
	}

	for (j = 0; j < n; j++) {
		if (v[j] == 0) {
			w[j + m] = 0;
			continue;
		}

		k = 0;
		for (i = 0; i < m; i++) {
			mul_32_by_32(u[i], v[j], &hi_prod, &lo_prod);

			lo_prod += k;
			carry_a = (lo_prod < k);

			w[i + j] += lo_prod;
			carry_b = (w[i + j] < lo_prod);

			k = hi_prod + carry_a + carry_b;
		}

		w[j + m] = k;
	}
}

/* Divide (u_hi:u:lo) by v, setting q and r to the quotient and remainder. */
static void div_32_by_16(uint16_t u_hi, uint16_t u_lo, uint16_t v, uint16_t *q, uint16_t *r) {
	uint32_t u = ((uint32_t)u_hi << 16) | u_lo;

	*q = (uint16_t)(u / v);
	*r = (uint16_t)(u % v);
}

/* Divide n-place u by v, yielding n-place quotient q and scalar remainder r. */
static void short_division(int n, uint16_t *u, uint16_t v, uint16_t *q, uint16_t *r) {
	uint16_t k;
	int i;

	k = 0;
	for (i = n - 1; i >= 0; i--) {
		div_32_by_16(k, u[i], v, &q[i], &k);
	}
	*r = k;
}

/* Count leading zeros in x. x must not be zero. */
static int leading_zeros(uint16_t x) {
	int n;

	n = 0;
	while (x <= UINT16_MAX / 2) {
		x <<= 1;
		n++;
	}

	return n;
}

/* Shift n-place u m positions to the left. */
static void shift_left(int n, uint16_t *u, int m) {
	uint16_t k, t;
	int i;

	k = 0;
	for (i = 0; i < n; i++) {
		t = u[i] >> (16 - m);
		u[i] = (u[i] << m) | k;
		k = t;
	}
}

/* Shift n-place u m positions to the right. */
static void shift_right(int n, uint16_t *u, int m)
{
	uint16_t k, t;
	int i;

	k = 0;
	for (i = n - 1; i >= 0; i--) {
		t = u[i] << (16 - m);
		u[i] = (u[i] >> m) | k;
		k = t;
	}
}

/*
   Divide (m + n)-place u by n-place v, yielding (m + 1)-place quotient q and
   n-place remainder u. u must have room for an (m + n + 1)th element.
 */
static void algorithm_d(int m, int n, uint16_t *u, uint16_t *v, uint16_t *q) {
	int shift;
	int j, i;
	uint32_t qhat, rhat, p, t;
	uint16_t k, k2, d;

	if (n == 1) {
		short_division(m + n, u, v[0], q, &u[0]);
		return;
	}

	/* Normalize. */
	u[m + n] = 0;
	shift = leading_zeros(v[n-1]);
	if (shift) {
		shift_left(n, v, shift);
		shift_left(m + n + 1, u, shift);
	}

	for (j = m; j >= 0; j--) {
		/* Calculate qhat. */
		t = ((uint32_t)u[j + n] << 16) | u[j + n - 1];
		qhat = t / v[n - 1];
		rhat = t % v[n - 1];

		while (true) {
			if (qhat > UINT16_MAX ||
				qhat * v[n - 2] > ((rhat << 16) | u[j + n - 2])) {
				qhat--;
				rhat += v[n - 1];
				if (rhat <= UINT16_MAX) {
					continue;
				}
			}
			break;
		}

		/* Multiply and subtract. */
		k = 0;
		for (i = 0; i <= n; i++) {
			p = qhat * (i == n ? 0 : v[i]);
			k2 = (p >> 16);

			d = u[j + i] - (uint16_t)p;
			k2 += (d > u[j + i]);

			u[j + i] = d - k;
			k2 += (u[j + i] > d);

			k = k2;
		}

		/* Test remainder. */
		q[j] = qhat;
		if (k != 0) {
			/* Add back. */
			q[j]--;
			k = 0;
			for (i = 0; i < n; i++) {
				t = u[j + i] + v[i] + k;
				u[j + i] = (uint16_t)t;
				k = t >> 16;
			}
			u[j + n] += k;
		}
	}

	/* Unnormalize. */
	if (shift) {
		shift_right(n, u, shift);
	}
}

/* Copy n uint32_t values from u32 to u16. */
static void u32_to_u16(int n, const uint32_t *u32, uint16_t *u16) {
	int i;

	for (i = 0; i < n; i++) {
		u16[i * 2 + 0] = (uint16_t)u32[i];
		u16[i * 2 + 1] = (uint16_t)(u32[i] >> 16);
	}
}

/* Copy n uint16_t values from u16 to u32. */
static void u16_to_u32(int n, const uint16_t *u16, uint32_t *u32) {
	int i;

	for (i = 0; i < n; i += 2) {
		u32[i / 2] = u16[i] | ((uint32_t)u16[i + 1] << 16);
	}
}

/*
   Divide (m + n)-place u with n-place v, yielding (m + 1)-place quotient q and
   n-place remainder r.
 */
static void algorithm_d_wrapper(int m, int n, const uint32_t *u,
				const uint32_t *v, uint32_t *q, uint32_t *r) {
	/* To avoid having to do 64-bit divisions, convert to uint16_t. Also
	   extend the dividend one place, as that is required for the
	   normalization step. */

	uint16_t u16[(m + n) * 2 + 1];
	uint16_t v16[n * 2];
	uint16_t q16[(m + 1) * 2];
	bool v_zero;

	u32_to_u16(m + n, u, u16);
	u32_to_u16(n, v, v16);

	/* If v16 has a leading zero, treat it as one place shorter. */
	v_zero = (v16[n * 2 - 1] == 0);

	algorithm_d(m * 2 + v_zero, n * 2 - v_zero, u16, v16, q16);

	if (v_zero) {
		/* If v16 was short, pad the remainder. */
		u16[n * 2 - 1] = 0;
	} else {
		/* Pad the quotient. */
		q16[(m + 1) * 2 - 1] = 0;
	}

	u16_to_u32((m + 1) * 2, q16, q);
	u16_to_u32(n * 2, u16, r);
}

/* Multiply m-place integer u by x and add y to it; set m to the new size. */
static void multiply_add(uint32_t *u, int *m, uint32_t x, uint32_t y) {
	int i;
	uint32_t k, hi_prod, lo_prod;

	k = y;

	for (i = 0; i < *m; i++) {
		mul_32_by_32(u[i], x, &hi_prod, &lo_prod);
		lo_prod += k;
		k = hi_prod + (lo_prod < k);
		u[i] = lo_prod;
	}

	if (k) {
		u[*m] = k;
		(*m)++;
	}
}

bool isBigIntDecString (const char *str) {
	if (str == NULL || str[0] == '\0') return false; 
	for (size_t i = 0; i < strlen(str); i++)
		if (str[i] < '0' || str[i] > '9') return false;
	return true;
}

/* Convert n-character decimal string str into integer u. */
static void from_string(int n, const char *str, int *u_len, uint32_t *u) {
	uint32_t chunk;
	int i;

	static const uint32_t pow10s[] = {1000000000,
					  10,
					  100,
					  1000,
					  10000,
					  100000,
					  1000000,
					  10000000,
					  100000000};

	*u_len = 0;
	chunk = 0;

	/* Process the string in chunks of up to 9 characters, as 10**9 is the
	   largest power of 10 that fits in uint32_t. */

	for (i = 1; i <= n; i++) {
		chunk = chunk * 10 + *str++ - '0';

		if (i % 9 == 0 || i == n) {
			multiply_add(u, u_len, pow10s[i % 9], chunk);
			chunk = 0;
		}
	}
}

/* Turn n-place integer u into decimal string str. */
static void to_string(int n, const uint32_t *u, char *str) {
	uint16_t v[n * 2];
	uint16_t k;
	char *s, t;
	int i;

	/* Make a scratch copy that's easy to do division on. */
	u32_to_u16(n, u, v);
	n *= 2;

	/* Skip leading zeros. */
	while (n && v[n - 1] == 0) {
		n--;
	}

	/* Special case for zero to avoid generating an empty string. */
	if (n == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	s = str;
	while (n != 0) {
		/* Divide by 10**4 to get the 4 least significant decimals. */
		short_division(n, v, 10000, v, &k);

		/* Skip leading zeros. */
		while (n && v[n - 1] == 0) {
			n--;
		}

		/* Add the digits to the string in reverse, with padding unless
		   this is the most significant group of digits (n == 0). */
		for (i = 0; (n != 0 && i < 4) || k; i++) {
			*s++ = '0' + (k % 10);
			k /= 10;
		}
	}

	/* Terminate and reverse the string. */
	*s-- = '\0';
	while (str < s) {
		t = *str;
		*str++ = *s;
		*s-- = t;
	}
}


/* Create a bigint from n-length array u. Leading zeros or n = 0 are allowed. */
static void bigint_create(int n, const uint32_t *u, bool negative, bigint_t *res) {
	/* Strip leading zeros. A bigint_t will never contain leading zeros. */
	while (n > 0 && u[n - 1] == 0) {
		n--;
	}

	res->length = n;

	if (n == 0) {
		res->negative = false;
	} else {
		res->negative = negative;
		memcpy(res->data, u, sizeof(res->data[0]) * n);
	}
}

bool bigint_create_str(char *str, bigint_t *res) {
	//printf("bigint_create_str: str is: %s\n", str);
	int32_t len = strlen(str);
	if (len == 0) { //Empty string is not a valid number
		res->length = -1;
		return false;
	}

	uint32_t u[len / 9 + 1];  /* A uint32_t holds at least 9 decimals. */
	bool negative = false;
	int u_length;

	if (str[0] == '-') {
		if (len <= 1) { //Just '-' is not a valid number
			res->length = -1;
			return false;
		}
		negative = true;
		str++;
		len--;
	}

	from_string(len, str, &u_length, u);

	bigint_create(u_length, u, negative, res);
	//printf("bigint_create_str: res is:");
	//bigint_print(res);
	return true;
}

size_t bigint_max_stringlen(const bigint_t *x) {
	if (x->length == 0) {
		return 1;
	}

	/* 10 digits per uint32_t, one more for '-'. */
	return x->length * 10 + x->negative;
}

bool bigint_tostring(const bigint_t *x, char *str) {
	if (x->negative) {
		*str++ = '-';
	}

	to_string(x->length, x->data, str);
	return true;
}

void bigint_print(bigint_t *x) {
	char str[bigint_max_stringlen(x) + 1];

	if (bigint_tostring(x, str))
		puts(str);
}

static void add(int x_len, const uint32_t *x, int y_len, const uint32_t *y, bigint_t *res) {
	if (x_len < y_len) {
		add(y_len, y, x_len, x, res);
	}

	int w_len = x_len + 1;
	uint32_t w[w_len];
	int i;

	/* Copy y into w, and pad it to the same length as x. */
	for (i = 0; i < y_len; i++) {
		w[i] = y[i];
	}
	for (i = y_len; i < x_len; i++) {
		w[i] = 0;
	}

	/* w = x + w */
	algorithm_a(x_len, x, w, w);

	bigint_create(w_len, w, false, res);
}

static void sub(int x_len, const uint32_t *x, int y_len, const uint32_t *y, bigint_t *res) {
	if (cmp(x_len, x, y_len, y) < 0) {
		/* x - y = -(y - x) */
		sub(y_len, y, x_len, x, res);
		res->negative = true;
		return;
	}

	uint32_t w[x_len];
	int i;

	/* Copy y into w, and pad to the same length as x. */
	for (i = 0; i < y_len; i++) {
		w[i] = y[i];
	}
	for (i = y_len; i < x_len; i++) {
		w[i] = 0;
	}

	/* w = x - w */
	algorithm_s(x_len, x, w, w);

	bigint_create(x_len, w, false, res);
}

void bigint_max(const bigint_t *x, const bigint_t *y, bigint_t *res) {
	if (bigint_cmp(x, y) > 0)
		bigint_create(x->length, x->data, x->negative, res);
	else
		bigint_create(y->length, y->data, y->negative, res);
}

void bigint_min(const bigint_t *x, const bigint_t *y, bigint_t *res) {
	if (bigint_cmp(x, y) > 0)
		bigint_create(y->length, y->data, y->negative, res);
	else
		bigint_create(x->length, x->data, x->negative, res);
}

void bigint_add(const bigint_t *x, const bigint_t *y, bigint_t *res) {

	if (x->negative && y->negative) {
		/* (-x) + (-y) = -(x + y) */
		add(x->length, x->data, y->length, y->data, res);
		res->negative = true;
		return;
	}

	if (x->negative) {
		/* (-x) + y = y - x */
		sub(y->length, y->data, x->length, x->data, res);
		return;
	}

	if (y->negative) {
		/* x + (-y) = x - y */
		sub(x->length, x->data, y->length, y->data, res);
		return;
	}

	add(x->length, x->data, y->length, y->data, res);
}

void bigint_sub(const bigint_t *x, const bigint_t *y, bigint_t *res) {

	if (x->negative && y->negative) {
		/* (-x) - (-y) = y - x */
		sub(y->length, y->data, x->length, x->data, res);
		return;
	}

	if (x->negative) {
		/* (-x) - y = -(x + y) */
		add(x->length, x->data, y->length, y->data, res);
		res->negative = true;
		return;
	}

	if (y->negative) {
		/* x - (-y) = x + y */
		return add(x->length, x->data, y->length, y->data, res);
	}

	return sub(x->length, x->data, y->length, y->data, res);
}

void bigint_mul(const bigint_t *x, const bigint_t *y, bigint_t *res) {
	uint32_t w[x->length + y->length];

	algorithm_m(x->length, y->length, x->data, y->data, w);

	bigint_create(x->length + y->length, w, x->negative ^ y->negative, res);
}

static void divrem(int x_len, const uint32_t *x, int y_len, const uint32_t *y, bool remainder, bigint_t *res) {
	uint32_t q[x_len - y_len + 1];
	uint32_t r[y_len];

	algorithm_d_wrapper(x_len - y_len, y_len, x, y, q, r);

	if (remainder) {
		bigint_create(y_len, r, false, res);
		return;
	}

	bigint_create(x_len - y_len + 1, q, false, res);
}

void bigint_div(const bigint_t *x, const bigint_t *y, bigint_t *res) {
	if (y->length == 0) {
		res->length = -1; //Division by zero! error
		return;
	}

	if (x->length < y->length) {
		bigint_create(0, NULL, false, res);
		return;
	}

	divrem(x->length, x->data, y->length, y->data, false, res);
	res->negative = x->negative ^ y->negative;
}

void bigint_rem(const bigint_t *x, const bigint_t *y, bigint_t *res) { 
	if (x->length < y->length) {
		bigint_create(x->length, x->data, false, res);
	} else {
		divrem(x->length, x->data, y->length, y->data, true, res);
	}
	res->negative = x->negative;
}

void bigint_neg(const bigint_t *x, bigint_t *res) {
	bigint_create(x->length, x->data, x->negative ^ 1, res);
}

int bigint_cmp(const bigint_t *x, const bigint_t *y) {
	if (x->negative != y->negative) {
		return x->negative ? -1 : 1;
	}

	if (x->negative) {
		return cmp(y->length, y->data, x->length, x->data);
	}

	return cmp(x->length, x->data, y->length, y->data);
}

int bigint_gt(const bigint_t *x, const bigint_t *y) {
	int gt = bigint_cmp(x, y);
	return gt > 0 ? 1: 0;
}

int bigint_lt(const bigint_t *x, const bigint_t *y) {
	int lt = bigint_cmp(x, y);
	return lt < 0 ? 1: 0;
}

int bigint_gte(const bigint_t *x, const bigint_t *y) {
	return (!bigint_lt(x, y));
}

int bigint_lte(const bigint_t *x, const bigint_t *y) {
	return (!bigint_gt(x, y));
}

int bigint_eq(const bigint_t *x, const bigint_t *y) {
	int eq = bigint_cmp(x, y);
	return eq == 0 ? 1: 0;
}

int bigint_neq(const bigint_t *x, const bigint_t *y) {
	return (!bigint_eq(x, y));
}

bool bigint_is_zero(const bigint_t *x) {
	return x->length == 0;
}
