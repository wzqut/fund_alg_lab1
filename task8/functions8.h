#ifndef FUNCTIONS8_H
#define FUNCTIONS8_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#define HAVE_FILE_ID 1
#endif

#define START_CAP 16
#define MIN_BASE 2
#define MAX_BASE 36

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_EMPTY_PATH,
    STATUS_ERR_ALLOC,
    STATUS_ERR_READ,
    STATUS_ERR_WRITE,
    STATUS_ERR_BASE,
    STATUS_ERR_LEXEME,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_SAME_FILE,
    STATUS_ERR_OPEN_IN,
    STATUS_ERR_OPEN_OUT,
    STATUS_END
} status_t;


status_t check_path(const char *path)
{
    if (path == NULL)
        return STATUS_ERR_NULL_PTR;
    if (*path == '\0')
        return STATUS_ERR_EMPTY_PATH;
    return STATUS_OK;
}

int same_file(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
        return 0;
    if (strcmp(a, b) == 0)
        return 1;
#ifdef HAVE_FILE_ID
    struct stat sa, sb;
    if (stat(a, &sa) == 0 && stat(b, &sb) == 0)
        return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
#endif
    return 0;
}

int digit_value(const char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

int is_space(const int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void to_upper_str(char *s){
    if (s == NULL)
        return;
    for (; *s != '\0'; s++)
        if (*s >= 'a' && *s <= 'z')
            *s = (char)(*s - 'a' + 'A');
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

status_t analyze_lexeme(const char *lexeme, int *negative, const char **digits, int *min_base)
{
    if (lexeme == NULL || negative == NULL || digits == NULL || min_base == NULL)
        return STATUS_ERR_NULL_PTR;

    const char *p = lexeme;
    int neg = 0;
    if (*p == '-' || *p == '+') {
        neg = (*p == '-');
        p++;
    }
    if (*p == '\0')
        return STATUS_ERR_LEXEME;

    while (*p == '0' && p[1] != '\0')
        p++;

    int max_digit = 0;
    for (const char *q = p; *q != '\0'; q++) {
        const int d = digit_value(*q);
        if (d < 0)
            return STATUS_ERR_LEXEME;
        if (d > max_digit)
            max_digit = d;
    }

    *negative = neg;
    *digits = p;
    *min_base = (max_digit + 1 < MIN_BASE) ? MIN_BASE : max_digit + 1;
    return STATUS_OK;
}


status_t to_decimal(const char *digits, const int base, const int negative, long long *result)
{
    if (digits == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_ERR_BASE;

    const unsigned long long limit = negative ? (unsigned long long)LLONG_MAX + 1ULL
                                              : (unsigned long long)LLONG_MAX;
    unsigned long long value = 0;
    for (; *digits != '\0'; digits++) {
        const int dv = digit_value(*digits);
        if (dv < 0 || dv >= base)
            return STATUS_ERR_LEXEME;
        const unsigned long long d = (unsigned long long)dv;
        if (value > (limit - d) / (unsigned long long)base)
            return STATUS_ERR_OVERFLOW;
        value = value * (unsigned long long)base + d;
    }

    if (!negative)
        *result = (long long)value;
    else if (value == (unsigned long long)LLONG_MAX + 1ULL)
        *result = LLONG_MIN;
    else
        *result = -(long long)value;
    return STATUS_OK;
}

status_t convert_lexeme(const char *lexeme, int *negative, const char **digits, int *base, long long *value){
    if (lexeme == NULL || negative == NULL || digits == NULL || base == NULL || value == NULL)
        return STATUS_ERR_NULL_PTR;

    const status_t st = analyze_lexeme(lexeme, negative, digits, base);
    if (st != STATUS_OK)
        return st;

    const status_t st_dec = to_decimal(*digits, *base, *negative, value);
    if (st_dec != STATUS_OK)
        return st_dec;

    if (*value == 0)
        *negative = 0;
    return STATUS_OK;
}


status_t print_number(FILE *out, const int negative, const char *digits, const int base, const long long value){
    if (out == NULL || digits == NULL)
        return STATUS_ERR_NULL_PTR;
    if (fprintf(out, "%s%s base %d decimal %lld\n", negative ? "-" : "", digits, base, value) < 0)
        return STATUS_ERR_WRITE;
    return STATUS_OK;
}


status_t process_file(FILE *in, FILE *out, size_t *invalid, size_t *overflow){
    if (in == NULL || out == NULL || invalid == NULL || overflow == NULL)
        return STATUS_ERR_NULL_PTR;

    *invalid = 0;
    *overflow = 0;
    while (1) {
        char *lexeme = NULL;
        status_t st = read_lexeme(in, &lexeme);
        if (st == STATUS_END)
            return STATUS_OK;
        if (st != STATUS_OK)
            return st;

        to_upper_str(lexeme);

        int negative = 0, base = 0;
        const char *digits = NULL;
        long long value = 0;

        st = convert_lexeme(lexeme, &negative, &digits, &base, &value);
        if (st == STATUS_OK)
            st = print_number(out, negative, digits, base, value);
        else if (st == STATUS_ERR_LEXEME) {
            (*invalid)++;
            st = STATUS_OK;
        } else if (st == STATUS_ERR_OVERFLOW) {
            (*overflow)++;
            st = STATUS_OK;
        }
        free(lexeme);
        if (st != STATUS_OK)
            return st;
    }
}

#endif