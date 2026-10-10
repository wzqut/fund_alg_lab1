#ifndef FUNCTIONS9_H
#define FUNCTIONS9_H

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIXED_SIZE 20
#define RAND_SIZE_MIN 10
#define RAND_SIZE_MAX 10000
#define RAND_VALUE_MIN (-1000)
#define RAND_VALUE_MAX 1000

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_ARGC,
    STATUS_ERR_NAN,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_RANGE_ORDER,
    STATUS_ERR_SIZE,
    STATUS_ERR_ALLOC
} status_t;

status_t parse_integer(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(str);
    if (len == 0)
        return STATUS_ERR_NAN;

    size_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    if (i == len)
        return STATUS_ERR_NAN;
    for (; i < len; i++)
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

status_t check_int_range(const long long value)
{
    if (value < INT_MIN || value > INT_MAX)
        return STATUS_ERR_OVERFLOW;
    return STATUS_OK;
}


unsigned long long random_bits(void)
{
    unsigned long long value = 0;
    for (int i = 0; i < 5; i++)
        value = (value << 15) ^ (unsigned long long)(rand() & 0x7FFF);
    return value;
}

status_t random_in_range(const int a, const int b, int *result)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (a > b)
        return STATUS_ERR_RANGE_ORDER;

    const unsigned long long width = (unsigned long long)((long long)b - (long long)a) + 1ULL;
    const unsigned long long threshold = (0ULL - width) % width;
    unsigned long long bits = random_bits();
    while (bits < threshold)
        bits = random_bits();

    *result = (int)((long long)a + (long long)(bits % width));
    return STATUS_OK;
}

status_t fill_array_rand(const int a, const int b, int *array, const int size)
{
    if (array == NULL)
        return STATUS_ERR_NULL_PTR;
    if (size <= 0)
        return STATUS_ERR_SIZE;
    if (a > b)
        return STATUS_ERR_RANGE_ORDER;

    for (int i = 0; i < size; i++) {
        const status_t st = random_in_range(a, b, &array[i]);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

status_t random_size(const int min_size, const int max_size, int *size)
{
    if (min_size <= 0)
        return STATUS_ERR_SIZE;
    return random_in_range(min_size, max_size, size);
}

status_t find_min_max_indices(const int *array, const int size, int *min_idx, int *max_idx)
{
    if (array == NULL || min_idx == NULL || max_idx == NULL)
        return STATUS_ERR_NULL_PTR;
    if (size <= 0)
        return STATUS_ERR_SIZE;

    int mi = 0, ma = 0;
    for (int i = 1; i < size; i++) {
        if (array[i] < array[mi]) mi = i;
        if (array[i] > array[ma]) ma = i;
    }
    *min_idx = mi;
    *max_idx = ma;
    return STATUS_OK;
}

status_t swap_min_max(int *array, const int size)
{
    int min_idx = 0, max_idx = 0;
    const status_t st = find_min_max_indices(array, size, &min_idx, &max_idx);
    if (st != STATUS_OK)
        return st;

    const int tmp = array[min_idx];
    array[min_idx] = array[max_idx];
    array[max_idx] = tmp;
    return STATUS_OK;
}


int compare_int(const void *x, const void *y)
{
    const int a = *(const int *)x;
    const int b = *(const int *)y;
    return (a > b) - (a < b);
}

status_t nearest_in_sorted(const int *sorted, const int size, const int target, int *result)
{
    if (sorted == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (size <= 0)
        return STATUS_ERR_SIZE;

    int lo = 0, hi = size;
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        if (sorted[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    if (lo == 0) {
        *result = sorted[0];
        return STATUS_OK;
    }
    if (lo == size) {
        *result = sorted[size - 1];
        return STATUS_OK;
    }

    const long long lower = sorted[lo - 1];
    const long long upper = sorted[lo];
    *result = (int)(((long long)target - lower <= upper - (long long)target) ? lower : upper);
    return STATUS_OK;
}

status_t generate_c(const int *a, const int size_a, const int *sorted_b, const int size_b, int *c)
{
    if (a == NULL || sorted_b == NULL || c == NULL)
        return STATUS_ERR_NULL_PTR;
    if (size_a <= 0 || size_b <= 0)
        return STATUS_ERR_SIZE;

    for (int i = 0; i < size_a; i++) {
        int near = 0;
        const status_t st = nearest_in_sorted(sorted_b, size_b, a[i], &near);
        if (st != STATUS_OK)
            return st;

        const long long sum = (long long)a[i] + (long long)near;
        if (sum < INT_MIN || sum > INT_MAX)
            return STATUS_ERR_OVERFLOW;
        c[i] = (int)sum;
    }
    return STATUS_OK;
}

#endif