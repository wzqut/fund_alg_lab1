#ifndef FUNCTIONS10_H
#define FUNCTIONS10_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_CAP 16
#define MIN_BASE 2
#define MAX_BASE 36
#define STOP_WORD "Stop"
#define OUT_BASES_COUNT 4
#define REPR_COUNT (OUT_BASES_COUNT + 1)
#define STR_SIZE 66

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_ALLOC,
    STATUS_ERR_READ,
    STATUS_ERR_BASE,
    STATUS_ERR_LEXEME,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_SUM_OVERFLOW,
    STATUS_ERR_BUFFER,
    STATUS_END,
    STATUS_ERR_NO_NUMBERS
} status_t;


typedef struct {
    long long max_abs;
    long long sum;
    int count;
} stats_t;

typedef struct {
    char str[REPR_COUNT][STR_SIZE];
    int base[REPR_COUNT];
} repr_t;

int digit_value(const char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

int is_space(const int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int skip_spaces(FILE *in){
    int c;
    do {
        c = fgetc(in);
    } while (c != EOF && is_space(c));
    return c;
}

status_t read_lexeme(FILE *in, char **lexeme){
    if (in == NULL || lexeme == NULL)
        return STATUS_ERR_NULL_PTR;

    int c = skip_spaces(in);
    if (c == EOF)
        return ferror(in) ? STATUS_ERR_READ : STATUS_END;

    size_t cap = START_CAP, len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;

    while (c != EOF && !is_space(c)) {
        if (c == '\0') {
            free(buf);
            return STATUS_ERR_READ;
        }
        if (len + 1 >= cap) {
            if (cap > SIZE_MAX / 2) {
                free(buf);
                return STATUS_ERR_OVERFLOW;
            }
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap);
            if (tmp == NULL) {
                free(buf);
                return STATUS_ERR_ALLOC;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
        c = fgetc(in);
    }
    if (ferror(in)) {
        free(buf);
        return STATUS_ERR_READ;
    }
    buf[len] = '\0';
    *lexeme = buf;
    return STATUS_OK;
}

int is_stop(const char *lexeme){
    return lexeme != NULL && strcmp(lexeme, STOP_WORD) == 0;
}

status_t parse_base(const char *lexeme, int *base)
{
    if (lexeme == NULL || base == NULL)
        return STATUS_ERR_NULL_PTR;
    if (*lexeme == '\0')
        return STATUS_ERR_BASE;

    int value = 0;
    for (; *lexeme != '\0'; lexeme++) {
        if (*lexeme < '0' || *lexeme > '9')
            return STATUS_ERR_BASE;
        value = value * 10 + (*lexeme - '0');
        if (value > MAX_BASE)
            return STATUS_ERR_BASE;
    }
    if (value < MIN_BASE)
        return STATUS_ERR_BASE;

    *base = value;
    return STATUS_OK;
}

status_t parse_number(const char *lexeme, const int base, long long *result)
{
    if (lexeme == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_ERR_BASE;

    int negative = 0;
    if (*lexeme == '-' || *lexeme == '+') {
        negative = (*lexeme == '-');
        lexeme++;
    }
    if (*lexeme == '\0')
        return STATUS_ERR_LEXEME;

    const unsigned long long limit = negative ? (unsigned long long)LLONG_MAX + 1ULL
                                              : (unsigned long long)LLONG_MAX;
    unsigned long long value = 0;
    int overflow = 0;
    for (; *lexeme != '\0'; lexeme++) {
        const int d = digit_value(*lexeme);
        if (d < 0 || d >= base)
            return STATUS_ERR_LEXEME;
        if (!overflow) {
            if (value > (limit - (unsigned long long)d) / (unsigned long long)base)
                overflow = 1;
            else
                value = value * (unsigned long long)base + (unsigned long long)d;
        }
    }
    if (overflow)
        return STATUS_ERR_OVERFLOW;

    if (!negative)
        *result = (long long)value;
    else if (value == (unsigned long long)LLONG_MAX + 1ULL)
        *result = LLONG_MIN;
    else
        *result = -(long long)value;
    return STATUS_OK;
}

unsigned long long abs_value(const long long x){
    return (x < 0) ? 0ULL - (unsigned long long)x : (unsigned long long)x;
}

void init_stats(stats_t *stats){
    if (stats == NULL)
        return;
    stats->max_abs = 0;
    stats->sum = 0;
    stats->count = 0;
}

status_t add_number(stats_t *stats, const long long value)
{
    if (stats == NULL)
        return STATUS_ERR_NULL_PTR;
    if (stats->count == INT_MAX)
        return STATUS_ERR_OVERFLOW;

    if ((value > 0 && stats->sum > LLONG_MAX - value) ||
        (value < 0 && stats->sum < LLONG_MIN - value))
        return STATUS_ERR_SUM_OVERFLOW;

    stats->sum += value;
    if (stats->count == 0 || abs_value(value) > abs_value(stats->max_abs))
        stats->max_abs = value;
    stats->count++;
    return STATUS_OK;
}


status_t to_base(const long long value, const int base, char *buf, const size_t size, size_t *start)
{
    if (buf == NULL || start == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_ERR_BASE;
    if (size < 2)
        return STATUS_ERR_BUFFER;

    size_t pos = size - 1;
    buf[pos] = '\0';

    unsigned long long n = abs_value(value);
    if (n == 0) {
        buf[--pos] = '0';
    }
    while (n > 0) {
        if (pos == 0)
            return STATUS_ERR_BUFFER;
        const int d = (int)(n % (unsigned long long)base);
        buf[--pos] = (char)((d < 10) ? '0' + d : 'A' + d - 10);
        n /= (unsigned long long)base;
    }
    if (value < 0) {
        if (pos == 0)
            return STATUS_ERR_BUFFER;
        buf[--pos] = '-';
    }

    *start = pos;
    return STATUS_OK;
}

status_t build_representations(const long long value, const int input_base, repr_t *repr)
{
    if (repr == NULL)
        return STATUS_ERR_NULL_PTR;
    if (input_base < MIN_BASE || input_base > MAX_BASE)
        return STATUS_ERR_BASE;

    const int bases[REPR_COUNT] = {input_base, 9, 18, 27, 36};
    for (int i = 0; i < REPR_COUNT; i++) {
        char buf[STR_SIZE];
        size_t start = 0;
        const status_t st = to_base(value, bases[i], buf, STR_SIZE, &start);
        if (st != STATUS_OK)
            return st;
        strcpy(repr->str[i], buf + start);
        repr->base[i] = bases[i];
    }
    return STATUS_OK;
}

#endif