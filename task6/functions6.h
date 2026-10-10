#ifndef FUNCTIONS6_H
#define FUNCTIONS6_H

#include <math.h>
#include <stdarg.h>
#include <stddef.h>

#define MAX_ITER 1000
#define MAX_KAPREKAR 4294967295ULL
#define MAX_VARARGS 1000

typedef enum {
    SUCCESS = 0,
    ERROR_INVALID_ARGS,
    ERROR_NULL_POINTER,
    ERROR_INVALID_NUMBER,
    ERROR_OVERFLOW,
    ERROR_DOMAIN,
    ERROR_NO_ROOT,
    ERROR_ITERATIONS
} status_code;

typedef struct {
    double x, y;
} point;



static status_code read_point(va_list *args, point *p)
{
    const double x = va_arg(*args, double);
    const double y = va_arg(*args, double);
    if (!isfinite(x) || !isfinite(y))
        return ERROR_INVALID_ARGS;
    p->x = x;
    p->y = y;
    return SUCCESS;
}

typedef struct {
    int pos;
    int neg;
    double total;
} turn_info;

static status_code add_turn(const point a, const point b, const point c, const double eps,
                            turn_info *t)
{
    const double v1x = b.x - a.x, v1y = b.y - a.y;
    const double v2x = c.x - b.x, v2y = c.y - b.y;

    if ((fabs(v1x) <= eps && fabs(v1y) <= eps) || (fabs(v2x) <= eps && fabs(v2y) <= eps))
        return ERROR_INVALID_ARGS;

    const double cross = v1x * v2y - v1y * v2x;
    const double dot = v1x * v2x + v1y * v2y;
    if (!isfinite(cross) || !isfinite(dot))
        return ERROR_OVERFLOW;

    double angle = 0.0;
    if (cross > eps) {
        t->pos = 1;
        angle = atan2(cross, dot);
    } else if (cross < -eps) {
        t->neg = 1;
        angle = atan2(cross, dot);
    } else if (dot < 0.0) {
        t->pos = 1;
        t->neg = 1;
    }
    t->total += angle;
    return SUCCESS;
}

static status_code is_convex(int *result, const double eps, const int n, ...)
{
    if (result == NULL)
        return ERROR_NULL_POINTER;
    if (n < 3 || n > MAX_VARARGS || !isfinite(eps) || eps < 0.0)
        return ERROR_INVALID_ARGS;

    va_list args;
    va_start(args, n);

    point first = {0.0, 0.0}, second = {0.0, 0.0};
    status_code status = read_point(&args, &first);
    if (status == SUCCESS)
        status = read_point(&args, &second);

    turn_info t = {0, 0, 0.0};
    point a = first, b = second;
    for (int i = 2; i < n && status == SUCCESS; i++) {
        point c = {0.0, 0.0};
        status = read_point(&args, &c);
        if (status == SUCCESS)
            status = add_turn(a, b, c, eps, &t);
        a = b;
        b = c;
    }
    va_end(args);

    if (status == SUCCESS)
        status = add_turn(a, b, first, eps, &t);
    if (status == SUCCESS)
        status = add_turn(b, first, second, eps, &t);
    if (status != SUCCESS)
        return status;

    const double two_pi = 8.0 * atan(1.0);
    const int one_direction = (t.pos != t.neg);
    const int one_turn = fabs(fabs(t.total) - two_pi) < 1.0;
    *result = one_direction && one_turn;
    return SUCCESS;
}



static status_code poly_value(double *result, const double x, const int n, ...)
{
    if (result == NULL)
        return ERROR_NULL_POINTER;
    if (n < 0 || n > MAX_VARARGS || !isfinite(x))
        return ERROR_INVALID_ARGS;

    va_list args;
    va_start(args, n);

    double value = 0.0;
    status_code status = SUCCESS;
    for (int i = 0; i <= n; i++) {
        const double coef = va_arg(args, double);
        if (!isfinite(coef)) {
            status = ERROR_INVALID_ARGS;
            break;
        }
        value = value * x + coef;
        if (!isfinite(value)) {
            status = ERROR_OVERFLOW;
            break;
        }
    }
    va_end(args);

    if (status != SUCCESS)
        return status;
    *result = value;
    return SUCCESS;
}



static status_code parse_number(const char *s, const int base, unsigned long long *n)
{
    if (s == NULL || n == NULL)
        return ERROR_NULL_POINTER;
    if (*s == '\0')
        return ERROR_INVALID_NUMBER;

    unsigned long long value = 0;
    int overflow = 0;
    for (; *s != '\0'; s++) {
        int d;
        if (*s >= '0' && *s <= '9')
            d = *s - '0';
        else if (*s >= 'A' && *s <= 'Z')
            d = *s - 'A' + 10;
        else if (*s >= 'a' && *s <= 'z')
            d = *s - 'a' + 10;
        else
            return ERROR_INVALID_NUMBER;
        if (d >= base)
            return ERROR_INVALID_NUMBER;

        if (!overflow) {
            value = value * (unsigned long long)base + (unsigned long long)d;
            if (value > MAX_KAPREKAR)
                overflow = 1;
        }
    }
    if (overflow)
        return ERROR_OVERFLOW;
    *n = value;
    return SUCCESS;
}

static int is_kaprekar(const unsigned long long n, const int base)
{
    if (n == 0)
        return 0;
    if (n == 1)
        return 1;

    const unsigned long long b = (unsigned long long)base;
    const unsigned long long sq = n * n;
    unsigned long long pw = b;

    while (pw <= sq) {
        const unsigned long long right = sq % pw, left = sq / pw;
        if (right > 0 && left + right == n)
            return 1;
        if (pw > sq / b)
            break;
        pw *= b;
    }
    return 0;
}

static status_code find_kaprekar(const char **found, int *found_count, const int base,
                                 const int count, ...)
{
    if (found == NULL || found_count == NULL)
        return ERROR_NULL_POINTER;
    if (base < 2 || base > 36 || count < 0 || count > MAX_VARARGS)
        return ERROR_INVALID_ARGS;

    *found_count = 0;

    va_list args;
    va_start(args, count);

    status_code status = SUCCESS;
    int amount = 0;
    for (int i = 0; i < count; i++) {
        const char *s = va_arg(args, const char *);
        unsigned long long n = 0;
        status = parse_number(s, base, &n);
        if (status != SUCCESS)
            break;
        if (is_kaprekar(n, base))
            found[amount++] = s;
    }
    va_end(args);

    if (status != SUCCESS)
        return status;
    *found_count = amount;
    return SUCCESS;
}



static status_code geom_mean(double *result, const int count, ...)
{
    if (result == NULL)
        return ERROR_NULL_POINTER;
    if (count <= 0 || count > MAX_VARARGS)
        return ERROR_INVALID_ARGS;

    va_list args;
    va_start(args, count);

    double sum_log = 0.0;
    int has_zero = 0;
    status_code status = SUCCESS;
    for (int i = 0; i < count; i++) {
        const double x = va_arg(args, double);
        if (!isfinite(x)) {
            status = ERROR_INVALID_ARGS;
            break;
        }
        if (x < 0.0) {
            status = ERROR_DOMAIN;
            break;
        }
        if (x > 0.0)
            sum_log += log(x);
        else
            has_zero = 1;
    }
    va_end(args);

    if (status != SUCCESS)
        return status;

    const double value = has_zero ? 0.0 : exp(sum_log / count);
    if (!isfinite(value))
        return ERROR_OVERFLOW;
    *result = value;
    return SUCCESS;
}



static double pow_rec(const double x, const unsigned long long n)
{
    if (n == 0)
        return 1.0;
    const double half = pow_rec(x, n / 2);
    return (n % 2 == 0) ? half * half : half * half * x;
}

static status_code fast_pow(double *result, const double x, const int n, const double eps)
{
    if (result == NULL)
        return ERROR_NULL_POINTER;
    if (!isfinite(x) || !isfinite(eps) || eps <= 0.0)
        return ERROR_INVALID_ARGS;

    double value;
    if (n >= 0) {
        value = pow_rec(x, (unsigned long long)n);
    } else {
        if (fabs(x) < eps)
            return ERROR_DOMAIN;
        value = 1.0 / pow_rec(x, (unsigned long long)(-(long long)n));
    }

    if (!isfinite(value))
        return ERROR_OVERFLOW;
    *result = value;
    return SUCCESS;
}



static int sign_of(const double v)
{
    if (v > 0.0)
        return 1;
    if (v < 0.0)
        return -1;
    return 0;
}

static status_code bisection(double *root, const double a, const double b, const double eps,
                             double (*f)(double))
{
    if (root == NULL || f == NULL)
        return ERROR_NULL_POINTER;
    if (!isfinite(a) || !isfinite(b) || !isfinite(eps) || eps <= 0.0 || a >= b)
        return ERROR_INVALID_ARGS;
    if (!isfinite(b - a))
        return ERROR_OVERFLOW;

    double lo = a, hi = b;
    const double f_lo = f(lo), f_hi = f(hi);
    if (!isfinite(f_lo) || !isfinite(f_hi))
        return ERROR_DOMAIN;

    const int s_lo = sign_of(f_lo), s_hi = sign_of(f_hi);
    if (s_lo == 0) {
        *root = lo;
        return SUCCESS;
    }
    if (s_hi == 0) {
        *root = hi;
        return SUCCESS;
    }
    if (s_lo == s_hi)
        return ERROR_NO_ROOT;

    for (int i = 0; i < MAX_ITER; i++) {
        const double mid = lo + (hi - lo) / 2.0;
        const double f_mid = f(mid);
        if (!isfinite(f_mid))
            return ERROR_DOMAIN;

        const int s_mid = sign_of(f_mid);
        if (s_mid == 0 || (hi - lo) / 2.0 < eps) {
            *root = mid;
            return SUCCESS;
        }
        if (s_mid == s_lo)
            lo = mid;
        else
            hi = mid;
    }
    return ERROR_ITERATIONS;
}

#endif