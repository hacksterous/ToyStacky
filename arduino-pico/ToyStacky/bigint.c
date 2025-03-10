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
static void shift_right(int n, uint16_t *u, int m) {
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
  shift = leading_zeros(v[n - 1]);
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
      if (qhat > UINT16_MAX || qhat * v[n - 2] > ((rhat << 16) | u[j + n - 2])) {
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

bool isBigIntDecString(const char *str) {
  if (str == NULL || str[0] == '\0') return false;
  for (size_t i = 0; i < strlen(str); i++)
    if (i == 0) {
      if ((str[i] < '0' || str[i] > '9') && (str[i] != '+') && (str[i] != '-')) return false;
    } else if (str[i] < '0' || str[i] > '9') return false;
  return true;
}

/* Convert n-character decimal string str into integer u. */
static void from_string(int n, const char *str, int *u_len, uint32_t *u) {
  uint32_t chunk;
  int i;

  static const uint32_t pow10s[] = { 1000000000,
                                     10,
                                     100,
                                     1000,
                                     10000,
                                     100000,
                                     1000000,
                                     10000000,
                                     100000000 };

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

//copy from one bigint to another
void bigint_copy(bigint_t *dest, const bigint_t *src) {
  if (src->length > 0) {
    memcpy(dest->data, src->data, sizeof(src->data[0]) * src->length);
    dest->length = src->length;
    dest->negative = src->negative;
  } else {
    dest->length = 0;
    dest->negative = false;
  }
}

bool bigint_from_str(bigint_t *res, const char *str) {
  //printf("bigint_from_str: str is: %s\n", str);
  int32_t len = strlen(str);
  if (len == 0) {  //Empty string is not a valid number
    res->length = -1;
    return false;
  }
  if (!isBigIntDecString(str)) return false;

  uint32_t u[len / 9 + 1]; /* A uint32_t holds at least 9 decimals. */
  bool negative = false;
  int u_length;

  if (str[0] == '-') {
    if (len <= 1) {  //Just '-' is not a valid number
      res->length = -1;
      return false;
    }
    negative = true;
    str++;
    len--;
  }

  from_string(len, str, &u_length, u);

  bigint_create(u_length, u, negative, res);
  return true;
}

size_t bigint_max_stringlen(const bigint_t *x) {
  if (x->length == 0) {
    return 1;
  } else if (x->length == -1) {
    return 0;
  }

  /* 10 digits per uint32_t, one more for '-'. */
  return x->length * 10 + x->negative;
}

bool bigint_tostring(const bigint_t *x, char *str, int negate) {
  if (x->length == -1) {
    *str = '\0';
    return false;
  }
  if (x->negative ^ negate) {
    *str++ = '-';
  }

  to_string(x->length, x->data, str);
  return true;
}

void bigint_print(bigint_t *x) {
  char str[bigint_max_stringlen(x) + 1];
  if (bigint_tostring(x, str, 0))
    puts(str);
  else
    puts("\n");
}

static void add(int x_len, const uint32_t *x, int y_len, const uint32_t *y, bigint_t *res) {
  if (x_len < y_len) {
    add(y_len, y, x_len, x, res);
    return;
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
    res->length = -1;  //Division by zero! error
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

void bigint_negative(const bigint_t *x, bigint_t *res) {
  bigint_create(x->length, x->data, x->negative ^ 1, res);
}

long long powll(long long a, long long b) {
  long long res = 1;
  while (b > 0) {
    if (b & 1)
      res = res * a;
    a = a * a;
    b >>= 1;
  }
  return res;
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
  return gt > 0 ? 1 : 0;
}

int bigint_lt(const bigint_t *x, const bigint_t *y) {
  int lt = bigint_cmp(x, y);
  return lt < 0 ? 1 : 0;
}

int bigint_gte(const bigint_t *x, const bigint_t *y) {
  return (!bigint_lt(x, y));
}

int bigint_lte(const bigint_t *x, const bigint_t *y) {
  return (!bigint_gt(x, y));
}

int bigint_eq(const bigint_t *x, const bigint_t *y) {
  int eq = bigint_cmp(x, y);
  return eq == 0 ? 1 : 0;
}

int bigint_neq(const bigint_t *x, const bigint_t *y) {
  return (!bigint_eq(x, y));
}

bool bigint_is_zero(const bigint_t *x) {
  return x->length == 0;
}

void bigint_from_uint32(bigint_t *x, const uint32_t n) {
  uint32_t u[1];
  u[0] = n;
  bigint_create(1, u, false, x);
}

void bigint_from_int(bigint_t *x, const long int n) {
  uint32_t u[1];
  if (n < 0) {
    u[0] = -n;
    bigint_create(1, u, true, x);
  } else {
    u[0] = n;
    bigint_create(1, u, false, x);
  }
}

void bigint_to_hex(const bigint_t *x, char *hexstr) {
  int i, j = 0;
  char tempstr[bigint_max_stringlen(x) + 1];
  int start = 0;  // Flag to handle leading zeros

  // Add "x" prefix to the output string
  strcpy(hexstr, "x");
  // Convert each uint32_t in the data array to hex
  for (i = x->length - 1; i >= 0; i--) {
    if (x->data[i] == 0 && !start) {
      continue;  // Skip leading zeros
    }
    start = 1;
    sprintf(tempstr + j, "%08x", (unsigned int)x->data[i]);
    //printf("i = %d j = %d tempstr = %s\n", i, j, tempstr);
    j += 8;
  }

  // If the number is zero, it will become 0x0
  if (!start) {
    strcpy(tempstr, "0");
  }

  //remove leading zeros
  i = 0;
  while (tempstr[i] == '0') i++;

  // add after "x"
  if (x->negative) strcat(hexstr, "-");
  strcat(hexstr, tempstr + i);
}

bool bigint_from_hex(bigint_t *res, char *hexstr) {
  size_t len = strlen(hexstr);
  if (len < 2 || hexstr[0] != 'x') {
    return false;  // Invalid hex string
  }

  // Initialize bigint
  res->length = 0;
  res->negative = 0;
  memset(res->data, 0, sizeof(res->data));

  size_t hexlen = len - 1;               // Length of the hex digits
  size_t num_digits = (hexlen + 7) / 8;  // Number of uint32_t required

  for (size_t i = 0; i < hexlen; i++) {
    uint32_t value;
    char c = hexstr[hexlen - i];  // Get the hex digit from the end
    if (c == '-' && i == (hexlen - 1)) {
      res->negative = 1;
      continue;
    } else if (isdigit(c)) {
      value = c - '0';
    } else if (isxdigit(c)) {
      value = tolower(c) - 'a' + 10;
    } else {
      return false;  // Invalid character in hex string
    }
    size_t pos = i / 8;
    res->data[pos] |= value << (4 * (i % 8));
  }

  res->length = num_digits;
  return true;
}

// Function to convert a 4-bit binary string to a single hex digit
char bin_to_hexit(const char *binstr) {
  int value = 0;
  for (int i = 0; i < 4; i++) {
    value <<= 1;
    if (binstr[i] == '1') {
      value += 1;
    } else if (binstr[i] != '0') return 0;  //error
  }
  if (value < 10) {
    return '0' + value;
  } else {
    return 'a' + (value - 10);
  }
}

// Function to convert a binary string to a hexadecimal string
bool bin_to_hex(char *binstr, char *hexstr) {
  int len = strlen(binstr);
  int hexlen = (len + 3) / 4;  // Calculate length of hex string
  hexstr[hexlen] = '\0';       // Null-terminate the hex string

  // Pad the binary string with leading zeros if necessary
  char paddedbinstr[hexlen * 4 + 1];
  int padding = hexlen * 4 - len;
  for (int i = 0; i < padding; i++) {
    paddedbinstr[i] = '0';
  }
  strcpy(paddedbinstr + padding, binstr);

  // Convert each 4-bit group to a hex digit
  for (int i = 0; i < hexlen; i++) {
    hexstr[i] = bin_to_hexit(paddedbinstr + 4 * i);
    if (hexstr[i] == 0) {
      //error
      return false;
    }
  }
  return true;
}

bool bigint_from_bin(bigint_t *res, char *binstr) {
  if (binstr[0] != 'b') return false;
  char hexstr[BIGINT_STRING_SIZE];
  hexstr[0] = 'x';
  bool success;
  if (binstr[1] == '-') {
    success = bin_to_hex(&binstr[2], &hexstr[2]);
    hexstr[1] = '-';
  } else {
    success = bin_to_hex(&binstr[1], &hexstr[1]);
  }
  if (!success) {
    hexstr[0] = '\0';
    return false;
  }
  //printf("bigint_from_bin: hexstr = %s\n", hexstr);
  bigint_from_hex(res, hexstr);
  return true;
}

void bigint_hex(const bigint_t *x, char *res) {
  bigint_to_hex(x, res);
}

void bigint_dec(const bigint_t *x, char *res) {
  bigint_tostring(x, res, 0);
}

void h2bsprintf(char *buf, char hexdigit) {
  //convert a nibble from hex to bin
  switch (hexdigit) {
    case '0': strcpy(buf, "0000"); break;
    case '1': strcpy(buf, "0001"); break;
    case '2': strcpy(buf, "0010"); break;
    case '3': strcpy(buf, "0011"); break;
    case '4': strcpy(buf, "0100"); break;
    case '5': strcpy(buf, "0101"); break;
    case '6': strcpy(buf, "0110"); break;
    case '7': strcpy(buf, "0111"); break;
    case '8': strcpy(buf, "1000"); break;
    case '9': strcpy(buf, "1001"); break;
    case 'a': strcpy(buf, "1010"); break;
    case 'b': strcpy(buf, "1011"); break;
    case 'c': strcpy(buf, "1100"); break;
    case 'd': strcpy(buf, "1101"); break;
    case 'e': strcpy(buf, "1110"); break;
    case 'f': strcpy(buf, "1111"); break;
    default: strcpy(buf, "0000"); break;  // Invalid hex digit
  }
}

void bigint_bin(const bigint_t *x, char *res) {
  char tempstr[BIGINT_STRING_SIZE];
  bigint_to_hex(x, res);
  int i;
  int j = 0;
  int len = strlen(res);
  if (len > 25) {
    *res = '\0';
    return;
  }
  //printf("bigint_bin: after conv to hex, res = %s\n", res);
  for (i = 1; i < len; i++) {
    //skip the first 'x'
    if (res[i] == '-') {
      strcpy(tempstr, "-");
      j++;
      continue;
    }
    h2bsprintf(tempstr + j, res[i]);
    j += 4;
  }
  //remove leading zeros
  i = 0;
  while (tempstr[i] == '0') i++;
  strcpy(res, "b");
  strcat(res, tempstr + i);
}

void bigint_oct(const bigint_t *x, char *res) {
}

void bigint_neg(const bigint_t *x, char *res) {
  bigint_tostring(x, res, 1);
}

void bigint_lcm(const bigint_t *a, const bigint_t *b, bigint_t *lcm) {
}

void bigint_gcd(const bigint_t *a, const bigint_t *b, bigint_t *gcd, bigint_t *x) {
  //GCD and modular inversion using Extended Euclidean Algorithm
  bigint_t s;
  bigint_t t;
  bigint_t r;
  bigint_t oldr;
  bigint_t q;
  bigint_t tempmul;
  bigint_t newr;
  bigint_t y;
  bigint_t newx;
  bigint_t newy;

  /*
	s = 0
	x = 1
	t = 1
	y = 0
	r = int (b)
	oldr = int(a)
	*/

  bigint_from_int(&s, 0);
  bigint_from_int(x, 1);
  bigint_from_int(&t, 1);
  bigint_from_int(&y, 0);
  bigint_copy(&r, b);
  bigint_copy(&oldr, a);

  while (!bigint_is_zero(&r)) {
    /*
		q = oldr // r

		newr = oldr - q*r
		oldr = r
		r = newr

		newx = x - q*s
		x = s
		s = newx

		newy = y - q*t
		y = t
		t = newy
		*/

    //using tuple assignments

    //oldr, r = r, oldr - q*r
    //x, s = s, x - q*s
    //y, t = t, y - q*t

    bigint_div(&oldr, &r, &q);

    bigint_mul(&q, &r, &tempmul);
    bigint_sub(&oldr, &tempmul, &newr);
    bigint_copy(&oldr, &r);
    bigint_copy(&r, &newr);

    bigint_mul(&q, &s, &tempmul);
    bigint_sub(x, &tempmul, &newx);
    bigint_copy(x, &s);
    bigint_copy(&s, &newx);

    bigint_mul(&q, &t, &tempmul);
    bigint_sub(&y, &tempmul, &newy);
    bigint_copy(&y, &t);
    bigint_copy(&t, &newy);
  }
  //At this point, x and y have been found such that
  // a * x + b * y = gcd (a, b)
  // If a and b are relatively prime, then gcd = 1 and
  // x is the inverse of a modulo b.
  // If gcd > 1, then a does not have an inverse module b.
  // reuse q and tempmul
  bigint_mul(a, x, &q);
  bigint_mul(b, &y, &tempmul);
  bigint_add(&q, &tempmul, gcd);

  //reuse t as bigint-1
  bigint_from_int(&t, 1);
  if (bigint_eq(gcd, &t)) {
    //if gcd = 1, adjust inverse value
    if (x->negative) {
      //if x is negative add b (x is the modular inverse)
      bigint_add(x, b, &q);
      bigint_copy(x, &q);
    }
  } else {
    //if gcd > 1, set inverse as 0
    bigint_from_int(x, 0);
  }
}

void bigint_mod_inv(const bigint_t *base, const bigint_t *mod, bigint_t *res) {
  bigint_t gcd;
  bigint_gcd(base, mod, &gcd, res);
}

void bigint_mod_exp(const bigint_t *base, const bigint_t *exp, const bigint_t *mod, bigint_t *res) {
  bigint_t exp_copy, base_mod;
  bigint_t two, temp;

  // Initialize bigints
  bigint_from_int(res, 1);
  bigint_from_int(&base_mod, 0);
  bigint_from_int(&exp_copy, 0);
  bigint_from_int(&two, 0);
  bigint_from_int(&temp, 0);

  // base_mod = base % mod
  bigint_rem(base, mod, &base_mod);

  // exp_copy = exp
  bigint_copy(&exp_copy, exp);

  // two = 2
  bigint_from_int(&two, 2);

  while (!bigint_is_zero(&exp_copy)) {
    // If exp_copy is odd
    if (exp_copy.data[0] & 1) {
      // res = (res * base_mod) % mod
      bigint_mul(res, &base_mod, &temp);
      bigint_rem(&temp, mod, res);
    }
    // exp_copy = exp_copy / 2
    bigint_div(&exp_copy, &two, &exp_copy);

    // base_mod = (base_mod * base_mod) % mod
    bigint_mul(&base_mod, &base_mod, &temp);
    bigint_rem(&temp, mod, &base_mod);
  }
}

void bigint_pow(const bigint_t *x, const bigint_t *y, bigint_t *res) {
  //printf("bigint_pow: entered with big x = ");
  //bigint_print((bigint_t*) x);
  //printf("bigint_pow: and big y = ");
  //bigint_print((bigint_t*) y);
  bigint_t mod;
  bigint_from_str(&mod, "18446744073709551616");  //2^64
  bigint_mod_exp(x, y, &mod, res);                //reuse mod exp with mod = 1
}
