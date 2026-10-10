#include <stdio.h>
#include <stdlib.h>
#include "functions4.h"

static void print_hint(const char *prog)
{
    fprintf(stderr, "usage: %s <flag> <input_file> [output_file]\n", prog);
    fprintf(stderr, "flags: -d -i -s -a (or /); with 'n' (-nd -ni -ns -na) the output file is the 3rd argument\n");
    fprintf(stderr, "  -d  remove digits\n");
    fprintf(stderr, "  -i  count latin letters per line\n");
    fprintf(stderr, "  -s  count symbols that are not latin letters, digits or spaces, per line\n");
    fprintf(stderr, "  -a  replace characters other than digits/space/'\\n' with ASCII hex code\n");
    fprintf(stderr, "without 'n' the output file is out_<input_file>\n");
}

static void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:                 printf("all good\n"); break;
        case STATUS_ERR_ARGC:           fprintf(stderr, "error: invalid number of arguments\n"); break;
        case STATUS_ERR_FLAG_UNKNOWN:   fprintf(stderr, "error: invalid flag\n"); break;
        case STATUS_ERR_OPEN_INPUT:     fprintf(stderr, "error: can't open input file\n"); break;
        case STATUS_ERR_OPEN_OUTPUT:    fprintf(stderr, "error: can't open output file\n"); break;
        case STATUS_ERR_SAME_FILE:      fprintf(stderr, "error: input and output files must be different\n"); break;
        case STATUS_ERR_READ:           fprintf(stderr, "error: failed to read the input file\n"); break;
        case STATUS_ERR_WRITE:          fprintf(stderr, "error: failed to write the output file\n"); break;
        case STATUS_ERR_OVERFLOW:       fprintf(stderr, "error: overflow\n"); break;
        case STATUS_ERR_ALLOC:          fprintf(stderr, "error: failed to allocate memory\n"); break;
        case STATUS_ERR_NULL_PTR:       fprintf(stderr, "error: null pointer\n"); break;
        default:                        fprintf(stderr, "error: unknown status\n"); break;
    }
}

int main(int argc, char *argv[])
{
    const char *prog = (argc > 0 && argv != NULL && argv[0] != NULL) ? argv[0] : "task4";

    if (argv == NULL || argc < 3 || argc > 4) {
        print_hint(prog);
        print_status(STATUS_ERR_ARGC);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (argv[i] == NULL) {
            print_status(STATUS_ERR_NULL_PTR);
            return 1;
        }
    }

    char flag = 0;
    int has_n = 0;
    status_t st = parse_flag(argv[1], &flag, &has_n);
    if (st != STATUS_OK) {
        print_hint(prog);
        print_status(st);
        return 1;
    }

    const int need_argc = has_n ? 4 : 3;
    if (argc != need_argc) {
        print_hint(prog);
        print_status(STATUS_ERR_ARGC);
        return 1;
    }

    char *output_path = NULL;
    int output_allocated = 0;

    if (has_n) {
        output_path = argv[3];
    } else {
        st = build_output_path(argv[2], &output_path);
        if (st != STATUS_OK) {
            print_status(st);
            return 1;
        }
        output_allocated = 1;
    }

    switch (flag) {
        case 'd': st = flag_d(argv[2], output_path); break;
        case 'i': st = flag_i(argv[2], output_path); break;
        case 's': st = flag_s(argv[2], output_path); break;
        case 'a': st = flag_a(argv[2], output_path); break;
        default:  st = STATUS_ERR_FLAG_UNKNOWN; break;
    }

    print_status(st);

    if (output_allocated)
        free(output_path);

    return (st == STATUS_OK) ? 0 : 1;
}