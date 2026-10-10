#ifndef FUNCTIONS7_H
#define FUNCTIONS7_H

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
#define BASE_R4 4
#define BASE_R8 8

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_EMPTY_PATH,
    STATUS_ERR_ALLOC,
    STATUS_ERR_READ,
    STATUS_ERR_WRITE,
    STATUS_ERR_BASE,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_SAME_FILE,
    STATUS_ERR_OPEN_IN,
    STATUS_ERR_OPEN_OUT,
    STATUS_END
} status_t;

int is_space(const int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

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

status_t read_lexeme(FILE *in, char **lexeme)
{
    if (in == NULL || lexeme == NULL)
        return STATUS_ERR_NULL_PTR;

    int c = fgetc(in);
    while (c != EOF && is_space(c))
        c = fgetc(in);
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

void to_lower_str(char *s)
{
    if (s == NULL)
        return;
    for (; *s != '\0'; s++)
        if (*s >= 'A' && *s <= 'Z')
            *s = (char)(*s - 'A' + 'a');
}

status_t ascii_in_base(const char *s, const int base, char **out)
{
    if (s == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < 4 || base > 10)
        return STATUS_ERR_BASE;
    if (strlen(s) > (SIZE_MAX - 1) / 4)
        return STATUS_ERR_OVERFLOW;

    char *res = (char *)malloc(strlen(s) * 4 + 1);
    if (res == NULL)
        return STATUS_ERR_ALLOC;

    size_t pos = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        unsigned int v = (unsigned char)s[i];
        char tmp[8];
        int k = 0;
        if (v == 0)
            tmp[k++] = '0';
        while (v > 0) {
            tmp[k++] = (char)('0' + v % (unsigned int)base);
            v /= (unsigned int)base;
        }
        while (k > 0)
            res[pos++] = tmp[--k];
    }
    res[pos] = '\0';

    *out = res;
    return STATUS_OK;
}

status_t put_word(FILE *out, const char *word, int *first)
{
    if (out == NULL || word == NULL || first == NULL)
        return STATUS_ERR_NULL_PTR;

    if (!*first && fputc(' ', out) == EOF)
        return STATUS_ERR_WRITE;
    if (fputs(word, out) == EOF)
        return STATUS_ERR_WRITE;
    *first = 0;
    return STATUS_OK;
}

status_t take_word(FILE *in, FILE *out, int *end, int *first)
{
    if (end == NULL)
        return STATUS_ERR_NULL_PTR;

    char *w = NULL;
    status_t st = read_lexeme(in, &w);
    if (st == STATUS_END) {
        *end = 1;
        return STATUS_OK;
    }
    if (st != STATUS_OK)
        return st;

    st = put_word(out, w, first);
    free(w);
    return st;
}

status_t flag_r(FILE *in1, FILE *in2, FILE *out)
{
    if (in1 == NULL || in2 == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;

    int end1 = 0, end2 = 0, first = 1;
    while (!end1 || !end2) {
        if (!end1) {
            const status_t st = take_word(in1, out, &end1, &first);
            if (st != STATUS_OK)
                return st;
        }
        if (!end2) {
            const status_t st = take_word(in2, out, &end2, &first);
            if (st != STATUS_OK)
                return st;
        }
    }
    return STATUS_OK;
}

status_t convert_by_number(char *word, const long long num, char **res)
{
    if (word == NULL || res == NULL)
        return STATUS_ERR_NULL_PTR;

    *res = NULL;
    if (num % 10 == 0) {
        to_lower_str(word);
        return ascii_in_base(word, BASE_R4, res);
    }
    if (num % 2 == 0) {
        to_lower_str(word);
        return STATUS_OK;
    }
    if (num % 5 == 0)
        return ascii_in_base(word, BASE_R8, res);
    return STATUS_OK;
}

status_t flag_a(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;

    long long num = 0;
    int first = 1;
    while (1) {
        char *w = NULL;
        status_t st = read_lexeme(in, &w);
        if (st == STATUS_END)
            return STATUS_OK;
        if (st != STATUS_OK)
            return st;

        if (num == LLONG_MAX) {
            free(w);
            return STATUS_ERR_OVERFLOW;
        }
        num++;
        char *conv = NULL;
        st = convert_by_number(w, num, &conv);
        if (st == STATUS_OK)
            st = put_word(out, conv != NULL ? conv : w, &first);
        free(conv);
        free(w);
        if (st != STATUS_OK)
            return st;
    }
}

#endif