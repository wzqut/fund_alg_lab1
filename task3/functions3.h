#ifndef FUNCTIONS3_H
#define FUNCTIONS3_H

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define EPS_MIN 1e-9L
#define EPS_MAX 1.0L
#define COEF_COUNT 3
#define PERM_COUNT 6

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_ARG_COUNT,
    STATUS_ERR_FLAG,
    STATUS_ERR_NAN,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_EPS_RANGE,
    STATUS_ERR_ZERO,
    STATUS_ERR_NOT_POSITIVE
} status_t;

typedef enum {
    ROOTS_TWO,
    ROOTS_ONE,
    ROOTS_NO_REAL,
    ROOTS_ANY,
    ROOTS_NONE
} roots_kind_t;

typedef struct {
    roots_kind_t kind;
    long double x1;
    long double x2;
} roots_t;


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
    errno = 0;
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

static status_t parse_integer(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    size_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    if (str[i] == '\0')
        return STATUS_ERR_NAN;
    for (; str[i] != '\0'; i++)
        if (!isdigit((unsigned char)str[i]))
            return STATUS_ERR_NAN;

    char *end = NULL;
    errno = 0;
    const long long value = strtoll(str, &end, 10);
    if (end == str || *end != '\0')
        return STATUS_ERR_NAN;
    if (errno == ERANGE)
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}


static bool same_triple(const long double x[COEF_COUNT], const long double y[COEF_COUNT],
                        const long double eps)
{
    for (int i = 0; i < COEF_COUNT; i++)
        if (fabsl(x[i] - y[i]) > eps)
            return false;
    return true;
}


static status_t unique_permutations(const long double eps, const long double coefs[COEF_COUNT],
                                    long double perms[PERM_COUNT][COEF_COUNT], int *count)
{
    if (coefs == NULL || perms == NULL || count == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    const int order[PERM_COUNT][COEF_COUNT] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    int found = 0;
    for (int p = 0; p < PERM_COUNT; p++) {
        long double candidate[COEF_COUNT];
        for (int i = 0; i < COEF_COUNT; i++)
            candidate[i] = coefs[order[p][i]];

        bool duplicate = false;
        for (int q = 0; q < found && !duplicate; q++)
            duplicate = same_triple(perms[q], candidate, eps);

        if (!duplicate) {
            for (int i = 0; i < COEF_COUNT; i++)
                perms[found][i] = candidate[i];
            found++;
        }
    }
    *count = found;
    return STATUS_OK;
}

static status_t solve_equation(const long double eps, const long double a, const long double b,
                               const long double c, roots_t *roots)
{
    if (roots == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    if (fabsl(a) <= eps) {
        if (fabsl(b) <= eps) {
            roots->kind = (fabsl(c) <= eps) ? ROOTS_ANY : ROOTS_NONE;
            return STATUS_OK;
        }
        const long double x = -c / b;
        if (!isfinite(x))
            return STATUS_ERR_OVERFLOW;
        roots->kind = ROOTS_ONE;
        roots->x1 = x;
        return STATUS_OK;
    }

    const long double d = b * b - 4.0L * a * c;
    if (!isfinite(d))
        return STATUS_ERR_OVERFLOW;

    if (d < -eps) {
        roots->kind = ROOTS_NO_REAL;
        return STATUS_OK;
    }
    if (d <= eps) {
        const long double x = -b / (2.0L * a);
        if (!isfinite(x))
            return STATUS_ERR_OVERFLOW;
        roots->kind = ROOTS_ONE;
        roots->x1 = x;
        return STATUS_OK;
    }

    const long double sq = sqrtl(d);
    const long double x1 = (-b - sq) / (2.0L * a);
    const long double x2 = (-b + sq) / (2.0L * a);
    if (!isfinite(x1) || !isfinite(x2))
        return STATUS_ERR_OVERFLOW;
    roots->kind = ROOTS_TWO;
    roots->x1 = x1;
    roots->x2 = x2;
    return STATUS_OK;
}


static status_t check_multiple(const long long a, const long long b, bool *is_multiple)
{
    if (is_multiple == NULL)
        return STATUS_ERR_NULL_PTR;
    if (a == 0 || b == 0)
        return STATUS_ERR_ZERO;

    *is_multiple = (b == -1) || (a % b == 0);
    return STATUS_OK;
}


static status_t check_right_triangle(const long double eps, const long double a, const long double b,
                                     const long double c, bool *is_right)
{
    if (is_right == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;
    if (a <= eps || b <= eps || c <= eps)
        return STATUS_ERR_NOT_POSITIVE;

    const long double a2 = a * a, b2 = b * b, c2 = c * c;
    if (!isfinite(a2) || !isfinite(b2) || !isfinite(c2) || !isfinite(a2 + b2 + c2))
        return STATUS_ERR_OVERFLOW;

    *is_right = fabsl(a2 + b2 - c2) <= eps
             || fabsl(a2 + c2 - b2) <= eps
             || fabsl(b2 + c2 - a2) <= eps;
    return STATUS_OK;
}

#endif