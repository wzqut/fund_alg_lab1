#ifndef FUNCTIONS2_H
#define FUNCTIONS2_H

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#define EPS_MIN 1e-4L
#define EPS_MAX 1e-1L
#define MAX_ITER 100000000LL
#define MAX_SIEVE 200000000LL
#define NEWTON_MAX_ITER 1000
#define DERIVATIVE_MIN 1e-12L

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_INPUT,
    STATUS_ERR_RANGE,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_MATH,
    STATUS_ERR_CONVERGENCE,
    STATUS_ERR_MEMORY
} status_t;

typedef status_t (*calc_func_t)(const long double epsilon, long double *result);
typedef long double (*real_func_t)(const long double x);


static status_t check_args(const long double epsilon, const long double *result)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (!isfinite(epsilon) || epsilon < EPS_MIN || epsilon > EPS_MAX)
        return STATUS_ERR_RANGE;
    return STATUS_OK;
}

static status_t store_result(const long double value, long double *result)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (!isfinite(value))
        return STATUS_ERR_MATH;
    *result = value;
    return STATUS_OK;
}

static status_t limit_steps(const long double epsilon, long long *steps)
{
    if (steps == NULL)
        return STATUS_ERR_NULL_PTR;

    const long double n = ceill(2.0L / epsilon);
    if (!isfinite(n) || n < 1.0L || n > (long double)MAX_ITER)
        return STATUS_ERR_OVERFLOW;

    *steps = (long long)n;
    return STATUS_OK;
}

static long double fast_pow(long double base, long long power)
{
    long double result = 1.0L;
    while (power > 0) {
        if (power % 2 == 1)
            result *= base;
        base *= base;
        power /= 2;
    }
    return result;
}

static status_t newton(const real_func_t f, const real_func_t df, long double x,
                       const long double epsilon, long double *result)
{
    if (f == NULL || df == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    for (int i = 0; i < NEWTON_MAX_ITER; i++) {
        const long double d = df(x);
        if (!isfinite(d) || fabsl(d) < DERIVATIVE_MIN)
            return STATUS_ERR_MATH;
        const long double next = x - f(x) / d;
        if (!isfinite(next))
            return STATUS_ERR_MATH;
        if (fabsl(next - x) <= epsilon)
            return store_result(next, result);
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}


static status_t parse_epsilon(const char *str, long double *epsilon)
{
    if (str == NULL || epsilon == NULL)
        return STATUS_ERR_NULL_PTR;
    if (str[0] == '\0')
        return STATUS_ERR_INPUT;

    for (const char *p = str; *p != '\0'; ++p) {
        const int digit = (*p >= '0' && *p <= '9');
        const int allowed = (*p == '.' || *p == 'e' || *p == 'E' || *p == '+' || *p == '-');
        if (!digit && !allowed)
            return STATUS_ERR_INPUT;
    }
    if (!((str[0] >= '0' && str[0] <= '9') || str[0] == '.'))
        return STATUS_ERR_INPUT;

    char *end = NULL;
    errno = 0;
    const long double value = strtold(str, &end);
    if (end == str || *end != '\0')
        return STATUS_ERR_INPUT;
    if (errno == ERANGE)
        return fabsl(value) > 1.0L ? STATUS_ERR_OVERFLOW : STATUS_ERR_RANGE;
    if (!isfinite(value))
        return STATUS_ERR_INPUT;
    if (value < EPS_MIN || value > EPS_MAX)
        return STATUS_ERR_RANGE;

    *epsilon = value;
    return STATUS_OK;
}

static status_t e_limit(const long double epsilon, long double *result)
{
    status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long long n = 0;
    st = limit_steps(epsilon, &n);
    if (st != STATUS_OK)
        return st;

    return store_result(fast_pow(1.0L + 1.0L / n, n), result);
}

static status_t e_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 1.0L, term = 1.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        term /= n;
        sum += term;
        if (term <= epsilon)
            return store_result(sum, result);
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double e_f(const long double x)  { return logl(x) - 1.0L; }
static long double e_df(const long double x) { return 1.0L / x; }

static status_t e_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(e_f, e_df, 2.0L, epsilon, result);
}


static status_t pi_limit(const long double epsilon, long double *result)
{
    status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long long n = 0;
    st = limit_steps(epsilon, &n);
    if (st != STATUS_OK)
        return st;

    long double a = 4.0L;
    for (long long k = 2; k <= n; k++)
        a *= 4.0L * k * (k - 1) / ((2.0L * k - 1) * (2.0L * k - 1));

    return store_result(a, result);
}

static status_t pi_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 0.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        const long double term = 4.0L / (2.0L * n - 1.0L);
        sum += (n % 2 == 1) ? term : -term;
        if (term <= epsilon)
            return store_result(sum, result);
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double pi_f(const long double x)  { return cosl(x / 2.0L); }
static long double pi_df(const long double x) { return -sinl(x / 2.0L) / 2.0L; }

static status_t pi_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(pi_f, pi_df, 3.0L, epsilon, result);
}

static status_t ln2_limit(const long double epsilon, long double *result)
{
    status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long long n = 0;
    st = limit_steps(epsilon, &n);
    if (st != STATUS_OK)
        return st;

    return store_result(n * (powl(2.0L, 1.0L / n) - 1.0L), result);
}

static status_t ln2_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 0.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        const long double term = 1.0L / n;
        sum += (n % 2 == 1) ? term : -term;
        if (term <= epsilon)
            return store_result(sum, result);
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double ln2_f(const long double x)  { return expl(x) - 2.0L; }
static long double ln2_df(const long double x) { return expl(x); }

static status_t ln2_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(ln2_f, ln2_df, 0.5L, epsilon, result);
}


static status_t sqrt2_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double x = -0.5L;
    for (long long i = 0; i < MAX_ITER; i++) {
        const long double next = x - x * x / 2.0L + 1.0L;
        if (!isfinite(next))
            return STATUS_ERR_MATH;
        if (fabsl(next - x) <= epsilon)
            return store_result(next, result);
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}

static status_t sqrt2_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double product = 1.0L;
    for (long long k = 2; k <= 64; k++) {
        const long double factor = powl(2.0L, powl(2.0L, (long double)-k));
        product *= factor;
        if (product * (factor - 1.0L) <= epsilon)
            return store_result(product, result);
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double sqrt2_f(const long double x)  { return x * x - 2.0L; }
static long double sqrt2_df(const long double x) { return 2.0L * x; }

static status_t sqrt2_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(sqrt2_f, sqrt2_df, 1.0L, epsilon, result);
}


static status_t gamma_limit(const long double epsilon, long double *result)
{
    status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long long n = 0;
    st = limit_steps(epsilon, &n);
    if (st != STATUS_OK)
        return st;

    long double harmonic = 0.0L;
    for (long long k = 1; k <= n; k++)
        harmonic += 1.0L / k;

    return store_result(harmonic - logl((long double)n), result);
}

static status_t gamma_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = -powl(acosl(-1.0L), 2.0L) / 6.0L;
    long long k = 2;
    for (long long r = 1; k <= MAX_ITER; r++) {
        long double block = 0.0L;
        const long long end = (r + 1) * (r + 1);
        for (; k < end; k++)
            block += 1.0L / (r * r) - 1.0L / k;
        sum += block;
        if (block <= epsilon)
            return store_result(sum + 2.0L / (r + 1), result);
    }
    return STATUS_ERR_CONVERGENCE;
}

static long double gamma_eq_f(const long double x, const long double limit)  { return expl(-x) - limit; }
static long double gamma_eq_df(const long double x)                          { return -expl(-x); }

static status_t gamma_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long double t_ld = ceill(1.0L / (epsilon * epsilon));
    if (!isfinite(t_ld) || t_ld < 2.0L || t_ld > (long double)MAX_SIEVE)
        return STATUS_ERR_OVERFLOW;
    const long long t = (long long)t_ld;

    unsigned char *composite = calloc((size_t)(t / 8 + 1), sizeof *composite);
    if (composite == NULL)
        return STATUS_ERR_MEMORY;

    long double log_product = 0.0L;
    for (long long i = 2; i <= t; i++) {
        if (composite[i / 8] & (1 << (i % 8)))
            continue;
        log_product += log1pl(-1.0L / i);
        if (i > t / i)
            continue;
        for (long long j = i * i; j <= t; j += i)
            composite[j / 8] |= (unsigned char)(1 << (j % 8));
    }
    free(composite);

    const long double limit = expl(logl(logl((long double)t)) + log_product);

    long double x = 0.5L;
    for (int i = 0; i < NEWTON_MAX_ITER; i++) {
        const long double d = gamma_eq_df(x);
        if (!isfinite(d) || fabsl(d) < DERIVATIVE_MIN)
            return STATUS_ERR_MATH;
        const long double next = x - gamma_eq_f(x, limit) / d;
        if (!isfinite(next))
            return STATUS_ERR_MATH;
        if (fabsl(next - x) <= epsilon)
            return store_result(next, result);
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}

#endif