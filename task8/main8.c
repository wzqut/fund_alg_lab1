#include <stdio.h>
#include <string.h>
#include "functions8.h"


void print_hint(void)
{
    printf("usage: <input_file> <output_file>\n");
    printf("input: numbers in bases 2..36 separated by spaces, tabs or newlines\n");
    printf("output: number without leading zeros, its minimal base, decimal value\n");
    printf("invalid numbers (bad symbol, overflow) are skipped\n\n");
}

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:             printf("all good\n"); break;
        case STATUS_ERR_NULL_PTR:   printf("error: null pointer\n"); break;
        case STATUS_ERR_EMPTY_PATH: printf("error: empty file path\n"); break;
        case STATUS_ERR_ALLOC:      printf("error: failed to allocate memory\n"); break;
        case STATUS_ERR_READ:       printf("error: can't read file\n"); break;
        case STATUS_ERR_WRITE:      printf("error: can't write file\n"); break;
        case STATUS_ERR_BASE:       printf("error: wrong base\n"); break;
        case STATUS_ERR_OVERFLOW:   printf("error: overflow\n"); break;
        case STATUS_ERR_SAME_FILE:  printf("error: input and output files must be different\n"); break;
        case STATUS_ERR_OPEN_IN:    printf("error: can't open input file\n"); break;
        case STATUS_ERR_OPEN_OUT:   printf("error: can't open output file\n"); break;
        default:                    printf("error: unknown status\n"); break;
    }
}

status_t run(const char *path_in, const char *path_out, size_t *invalid, size_t *overflow)
{
    status_t st = check_path(path_in);
    if (st == STATUS_OK)
        st = check_path(path_out);
    if (st != STATUS_OK)
        return st;

    FILE *in = fopen(path_in, "r");
    if (in == NULL)
        return STATUS_ERR_OPEN_IN;

    if (same_file(path_in, path_out)) {
        fclose(in);
        return STATUS_ERR_SAME_FILE;
    }

    FILE *out = fopen(path_out, "w");
    if (out == NULL) {
        fclose(in);
        return STATUS_ERR_OPEN_OUT;
    }

    st = process_file(in, out, invalid, overflow);
    if (st == STATUS_OK && ferror(in))
        st = STATUS_ERR_READ;
    if (st == STATUS_OK && ferror(out))
        st = STATUS_ERR_WRITE;

    fclose(in);
    if (fclose(out) != 0 && st == STATUS_OK)
        st = STATUS_ERR_WRITE;
    return st;
}

int main(int argc, char *argv[])
{
    if (argv == NULL || argc != 3 || argv[1] == NULL || argv[2] == NULL) {
        print_hint();
        printf("error: invalid number of arguments (2 expected)\n");
        return 1;
    }

    size_t invalid = 0, overflow = 0;
    const status_t st = run(argv[1], argv[2], &invalid, &overflow);
    print_status(st);
    if (st == STATUS_OK && (invalid > 0 || overflow > 0))
        printf("skipped: %zu invalid, %zu overflow\n", invalid, overflow);
    return (st == STATUS_OK) ? 0 : 1;
}