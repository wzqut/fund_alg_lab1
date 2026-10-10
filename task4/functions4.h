#ifndef FUNCTIONS4_H
#define FUNCTIONS4_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_PREFIX "out_"

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_ARGC,
    STATUS_ERR_FLAG_UNKNOWN,
    STATUS_ERR_ALLOC,
    STATUS_ERR_OPEN_INPUT,
    STATUS_ERR_OPEN_OUTPUT,
    STATUS_ERR_SAME_FILE,
    STATUS_ERR_READ,
    STATUS_ERR_WRITE,
    STATUS_ERR_OVERFLOW
} status_t;

typedef int (*char_pred_t)(const int ch);


static status_t parse_flag(const char *str, char *out_flag, int *out_has_n)
{
    if (str == NULL || out_flag == NULL || out_has_n == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(str);
    if (len < 2 || (str[0] != '-' && str[0] != '/'))
        return STATUS_ERR_FLAG_UNKNOWN;

    char c;
    int has_n;
    if (len == 2) {
        c = str[1];
        has_n = 0;
    } else if (len == 3 && str[1] == 'n') {
        c = str[2];
        has_n = 1;
    } else {
        return STATUS_ERR_FLAG_UNKNOWN;
    }

    if (c != 'd' && c != 'i' && c != 's' && c != 'a')
        return STATUS_ERR_FLAG_UNKNOWN;

    *out_flag = c;
    *out_has_n = has_n;
    return STATUS_OK;
}

static status_t build_output_path(const char *input_path, char **out_path)
{
    if (input_path == NULL || out_path == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(input_path);
    const size_t prefix_len = strlen(OUT_PREFIX);
    if (len > SIZE_MAX - prefix_len - 1)
        return STATUS_ERR_OVERFLOW;

    const char *name = input_path;
    for (const char *p = input_path; *p != '\0'; ++p) {
        if (*p == '/'
#ifdef _WIN32
            || *p == '\\'
#endif
        )
            name = p + 1;
    }
    const size_t dir_len = (size_t)(name - input_path);

    char *path = (char *)malloc(len + prefix_len + 1);
    if (path == NULL)
        return STATUS_ERR_ALLOC;

    memcpy(path, input_path, dir_len);
    memcpy(path + dir_len, OUT_PREFIX, prefix_len);
    memcpy(path + dir_len + prefix_len, name, len - dir_len + 1);

    *out_path = path;
    return STATUS_OK;
}


static status_t open_files(const char *input_path, const char *output_path, FILE **in, FILE **out)
{
    if (input_path == NULL || output_path == NULL || in == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;

    if (strcmp(input_path, output_path) == 0)
        return STATUS_ERR_SAME_FILE;

    FILE *input_file = fopen(input_path, "r");
    if (input_file == NULL)
        return STATUS_ERR_OPEN_INPUT;

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        fclose(input_file);
        return STATUS_ERR_OPEN_OUTPUT;
    }

    *in = input_file;
    *out = output_file;
    return STATUS_OK;
}

static status_t close_files(FILE *in, FILE *out, status_t st)
{
    if (st == STATUS_OK && ferror(in))
        st = STATUS_ERR_READ;
    if (st == STATUS_OK && ferror(out))
        st = STATUS_ERR_WRITE;

    if (fclose(in) != 0 && st == STATUS_OK)
        st = STATUS_ERR_READ;
    if (fclose(out) != 0 && st == STATUS_OK)
        st = STATUS_ERR_WRITE;
    return st;
}

static status_t flag_d(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if (ch >= '0' && ch <= '9')
            continue;
        if (fputc(ch, out) == EOF) {
            st = STATUS_ERR_WRITE;
            break;
        }
    }
    return close_files(in, out, st);
}

static status_t flag_a(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if ((ch >= '0' && ch <= '9') || ch == ' ' || ch == '\n') {
            if (fputc(ch, out) == EOF) {
                st = STATUS_ERR_WRITE;
                break;
            }
            continue;
        }
        if (fprintf(out, "%X", (unsigned int)(unsigned char)ch) < 0) {
            st = STATUS_ERR_WRITE;
            break;
        }
    }
    return close_files(in, out, st);
}

static int is_latin_letter(const int ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static int is_other_symbol(const int ch)
{
    return !is_latin_letter(ch) && !(ch >= '0' && ch <= '9') && ch != ' ';
}

static status_t count_per_line(const char *input_path, const char *output_path,
                               const char_pred_t pred)
{
    if (pred == NULL)
        return STATUS_ERR_NULL_PTR;

    FILE *in = NULL, *out = NULL;
    status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    unsigned long long line_no = 1, count = 0;
    int has_pending = 0;
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        has_pending = 1;
        if (ch == '\n') {
            if (fprintf(out, "%llu: %llu\n", line_no, count) < 0) {
                st = STATUS_ERR_WRITE;
                break;
            }
            if (line_no == ULLONG_MAX) {
                st = STATUS_ERR_OVERFLOW;
                break;
            }
            line_no++;
            count = 0;
            has_pending = 0;
            continue;
        }
        if (pred(ch)) {
            if (count == ULLONG_MAX) {
                st = STATUS_ERR_OVERFLOW;
                break;
            }
            count++;
        }
    }
    if (st == STATUS_OK && has_pending && fprintf(out, "%llu: %llu\n", line_no, count) < 0)
        st = STATUS_ERR_WRITE;

    return close_files(in, out, st);
}

static status_t flag_i(const char *input_path, const char *output_path)
{
    return count_per_line(input_path, output_path, is_latin_letter);
}

static status_t flag_s(const char *input_path, const char *output_path)
{
    return count_per_line(input_path, output_path, is_other_symbol);
}

#endif