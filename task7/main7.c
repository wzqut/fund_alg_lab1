#include <stdio.h>
#include <string.h>
#include "functions7.h"


void print_hint(void)
{
    printf("usage:\n");
    printf("  -r <file1> <file2> <out>\n");
    printf("  -a <file> <out>\n");
    printf("the flag can start with '-' or '/'\n\n");
}

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:             printf("done\n"); break;
        case STATUS_ERR_NULL_PTR:   printf("error: null pointer\n"); break;
        case STATUS_ERR_EMPTY_PATH: printf("error: empty file path\n"); break;
        case STATUS_ERR_ALLOC:      printf("error: no memory\n"); break;
        case STATUS_ERR_READ:       printf("error: can't read file\n"); break;
        case STATUS_ERR_WRITE:      printf("error: can't write file\n"); break;
        case STATUS_ERR_BASE:       printf("error: wrong base\n"); break;
        case STATUS_ERR_OVERFLOW:   printf("error: overflow\n"); break;
        case STATUS_ERR_SAME_FILE:  printf("error: files must be different\n"); break;
        case STATUS_ERR_OPEN_IN:    printf("error: can't open input file\n"); break;
        case STATUS_ERR_OPEN_OUT:   printf("error: can't open output file\n"); break;
        default:                    printf("error: unknown\n"); break;
    }
}

int parse_flag(const char *str, char *flag)
{
    if (str == NULL || flag == NULL)
        return 0;
    if (strlen(str) != 2 || (str[0] != '-' && str[0] != '/'))
        return 0;
    if (str[1] != 'r' && str[1] != 'a')
        return 0;
    *flag = str[1];
    return 1;
}

status_t check_different(const char *in1, const char *in2, const char *out)
{
    if (same_file(in1, out))
        return STATUS_ERR_SAME_FILE;
    if (in2 != NULL && same_file(in2, out))
        return STATUS_ERR_SAME_FILE;
    return STATUS_OK;
}

status_t run(const char flag, char *argv[])
{
    const char *path1 = argv[2];
    const char *path2 = (flag == 'r') ? argv[3] : NULL;
    const char *path_out = (flag == 'r') ? argv[4] : argv[3];

    status_t st = check_path(path1);
    if (st == STATUS_OK && flag == 'r')
        st = check_path(path2);
    if (st == STATUS_OK)
        st = check_path(path_out);
    if (st != STATUS_OK)
        return st;

    FILE *in1 = fopen(path1, "r");
    if (in1 == NULL)
        return STATUS_ERR_OPEN_IN;

    FILE *in2 = NULL;
    if (flag == 'r') {
        in2 = fopen(path2, "r");
        if (in2 == NULL) {
            fclose(in1);
            return STATUS_ERR_OPEN_IN;
        }
    }

    FILE *out = NULL;
    st = check_different(path1, path2, path_out);
    if (st == STATUS_OK) {
        out = fopen(path_out, "w");
        if (out == NULL)
            st = STATUS_ERR_OPEN_OUT;
    }

    if (st == STATUS_OK)
        st = (flag == 'r') ? flag_r(in1, in2, out) : flag_a(in1, out);

    if (st == STATUS_OK && out != NULL && fputc('\n', out) == EOF)
        st = STATUS_ERR_WRITE;
    if (st == STATUS_OK && (ferror(in1) || (in2 != NULL && ferror(in2))))
        st = STATUS_ERR_READ;
    if (st == STATUS_OK && out != NULL && ferror(out))
        st = STATUS_ERR_WRITE;

    fclose(in1);
    if (in2 != NULL)
        fclose(in2);
    if (out != NULL && fclose(out) != 0 && st == STATUS_OK)
        st = STATUS_ERR_WRITE;
    return st;
}

int main(int argc, char *argv[])
{
    if (argv == NULL || argc < 2 || argv[1] == NULL) {
        print_hint();
        printf("error: no flag\n");
        return 1;
    }

    char flag = 0;
    if (!parse_flag(argv[1], &flag)) {
        print_hint();
        printf("error: wrong flag\n");
        return 1;
    }

    const int need_argc = (flag == 'r') ? 5 : 4;
    if (argc != need_argc) {
        print_hint();
        printf("error: wrong number of arguments\n");
        return 1;
    }
    for (int i = 2; i < argc; i++) {
        if (argv[i] == NULL) {
            print_status(STATUS_ERR_NULL_PTR);
            return 1;
        }
    }

    const status_t st = run(flag, argv);
    print_status(st);
    return (st == STATUS_OK) ? 0 : 1;
}