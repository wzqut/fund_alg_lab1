#ifndef FUNCTIONS5_H
#define FUNCTIONS5_H

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define EPS_MIN 1e-7L
#define EPS_MAX 0.1L
#define MAX_ITER 100000000LL
#define MAX_PARTS (1LL << 30)

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_NAN,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_EPS_RANGE,
    STATUS_ERR_CONVERGENCE
} status_t;

typedef long double (*ratio_func_t)(const long double x, const long long n);
typedef long double (*real_func_t)(const long double x);



static bool is_real_format(const char *str)
{
    size_t i = 0;
    if (str[i] == '+' || str[i] == '-')
        i++;

    size_t mantissa_digits = 0;
    while (isdigit((unsigned char)str[i])) {
        i++;
        mantissa_digits++;
    }
    if (str[i] == '.') {
        i++;
        while (isdigit((unsigned char)str[i])) {
            i++;
            mantissa_digits++;
        }
    }
    if (mantissa_digits == 0)
        return false;

    if (str[i] == 'e' || str[i] == 'E') {
        i++;
        if (str[i] == '+' || str[i] == '-')
            i++;
        size_t exp_digits = 0;
        while (isdigit((unsigned char)str[i])) {
            i++;
            exp_digits++;
        }
        if (exp_digits == 0)
            return false;
    }
    return str[i] == '\0';
}

static status_t parse_real(const char *str, long double *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (!is_real_format(str))
        return STATUS_ERR_NAN;

    char *end = NULL;
    const long double value = strtold(str, &end);
    if (end == str || *end != '\0')
        return STATUS_ERR_NAN;
    if (!isfinite(value))
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}

static status_t check_eps(const long double eps)
{
    if (!isfinite(eps) || eps < EPS_MIN || eps > EPS_MAX)
        return STATUS_ERR_EPS_RANGE;
    return STATUS_OK;
}

static status_t parse_eps(const char *str, long double *eps)
{
    if (eps == NULL)
        return STATUS_ERR_NULL_PTR;

    long double value = 0.0L;
    const status_t st = parse_real(str, &value);
    if (st != STATUS_OK)
        return st;
    if (check_eps(value) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    *eps = value;
    return STATUS_OK;
}



static status_t sum_series(const long double x, const long double eps, const long double first,
                           const long long n_start, const ratio_func_t ratio, long double *result)
{
    if (ratio == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;
    if (!isfinite(x) || !isfinite(first))
        return STATUS_ERR_OVERFLOW;

    long double term = first, sum = first;
    for (long long n = n_start; fabsl(term) > eps; n++) {
        if (n - n_start >= MAX_ITER)
            return STATUS_ERR_CONVERGENCE;
        term *= ratio(x, n);
        sum += term;
        if (!isfinite(term) || !isfinite(sum))
            return STATUS_ERR_OVERFLOW;
    }
    *result = sum;
    return STATUS_OK;
}

static long double ratio_a(const long double x, const long long n)
{
    return x / (n + 1);
}

static long double ratio_b(const long double x, const long long n)
{
    return -x * x / ((2.0L * n + 1) * (2.0L * n + 2));
}

static long double ratio_c(const long double x, const long long n)
{
    return 9.0L * (n + 1) * (n + 1) * x * x / ((3.0L * n + 1) * (3.0L * n + 2));
}

static long double ratio_d(const long double x, const long long n)
{
    return -(2.0L * n + 1) * x * x / (2.0L * n + 2);
}

static status_t sum_a(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_a, result);
}

static status_t sum_b(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_b, result);
}

static status_t sum_c(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_c, result);
}

static status_t sum_d(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, -x * x / 2.0L, 1, ratio_d, result);
}



static long double midpoint(const real_func_t f, const long long n)
{
    const long double h = 1.0L / n;
    long double sum = 0.0L;
    for (long long i = 0; i < n; i++)
        sum += f((i + 0.5L) * h);
    return sum * h;
}

static status_t integrate(const real_func_t f, const long double eps, long double *result)
{
    if (f == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    long double prev = midpoint(f, 1);
    if (!isfinite(prev))
        return STATUS_ERR_OVERFLOW;

    for (long long n = 2; n <= MAX_PARTS; n *= 2) {
        const long double cur = midpoint(f, n);
        if (!isfinite(cur))
            return STATUS_ERR_OVERFLOW;
        if (fabsl(cur - prev) <= eps / 2.0L) {
            *result = cur;
            return STATUS_OK;
        }
        prev = cur;
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double f_a(const long double x) { return log1pl(x) / x; }
static long double f_b(const long double x) { return expl(-x * x / 2.0L); }
static long double f_c(const long double x) { return -log1pl(-x); }
static long double f_d(const long double x) { return powl(x, x); }

static status_t integral_a(const long double eps, long double *result) { return integrate(f_a, eps, result); }
static status_t integral_b(const long double eps, long double *result) { return integrate(f_b, eps, result); }
static status_t integral_c(const long double eps, long double *result) { return integrate(f_c, eps, result); }
static status_t integral_d(const long double eps, long double *result) { return integrate(f_d, eps, result); }

#endif