#include <stdio.h>
#include "functions10.h"


void print_hint(void)
{
    printf("usage: no arguments, input from console\n");
    printf("1) enter base [2..36]\n");
    printf("2) enter integers in this base (digits above 9 - uppercase latin letters),\n");
    printf("   separated by spaces or newlines, end of input - %s\n", STOP_WORD);
    printf("prints number with max abs value and sum of all numbers\n");
    printf("(in entered base and in bases 9, 18, 27, 36)\n\n");
}

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:               printf("all good\n"); break;
        case STATUS_ERR_NULL_PTR:     printf("error: null pointer\n"); break;
        case STATUS_ERR_ALLOC:        printf("error: failed to allocate memory\n"); break;
        case STATUS_ERR_READ:         printf("error: can't read the input\n"); break;
        case STATUS_ERR_BASE:         printf("error: base must be an integer in [%d..%d]\n", MIN_BASE, MAX_BASE); break;
        case STATUS_ERR_OVERFLOW:     printf("error: overflow\n"); break;
        case STATUS_ERR_SUM_OVERFLOW: printf("error: sum of numbers doesn't fit in long long\n"); break;
        case STATUS_ERR_BUFFER:       printf("error: buffer is too small\n"); break;
        case STATUS_END:              printf("error: input ended unexpectedly (%s expected)\n", STOP_WORD); break;
        case STATUS_ERR_NO_NUMBERS:   printf("error: no numbers were entered\n"); break;
        default:                      printf("error: unknown status\n"); break;
    }
}

void print_representations(const char *title, const repr_t *repr)
{
    if (title == NULL || repr == NULL)
        return;
    printf("%s:\n", title);
    for (int i = 0; i < REPR_COUNT; i++)
        printf("  base %2d: %s\n", repr->base[i], repr->str[i]);
}

status_t read_base(int *base)
{
    char *lexeme = NULL;
    status_t st = read_lexeme(stdin, &lexeme);
    if (st == STATUS_END)
        return STATUS_ERR_BASE;
    if (st != STATUS_OK)
        return st;

    st = parse_base(lexeme, base);
    free(lexeme);
    return st;
}

status_t read_numbers(const int base, stats_t *stats)
{
    while (1) {
        char *lexeme = NULL;
        const status_t st = read_lexeme(stdin, &lexeme);
        if (st != STATUS_OK)
            return st;

        if (is_stop(lexeme)) {
            free(lexeme);
            return STATUS_OK;
        }

        long long value = 0;
        const status_t parsed = parse_number(lexeme, base, &value);
        if (parsed == STATUS_OK) {
            const status_t add = add_number(stats, value);
            if (add != STATUS_OK) {
                free(lexeme);
                return add;
            }
        } else if (parsed == STATUS_ERR_LEXEME) {
            printf("invalid number <%s> for base %d, skipped\n", lexeme, base);
        } else if (parsed == STATUS_ERR_OVERFLOW) {
            printf("number <%s> is too large, skipped\n", lexeme);
        } else {
            free(lexeme);
            return parsed;
        }
        free(lexeme);
    }
}

int main(int argc, char *argv[])
{
    (void)argv;
    print_hint();

    if (argc != 1) {
        printf("error: no arguments expected, got %d\n", argc - 1);
        return 1;
    }

    int base = 0;
    printf("enter base:\n");
    status_t st = read_base(&base);
    if (st != STATUS_OK) {
        print_status(st);
        return 1;
    }

    stats_t stats;
    init_stats(&stats);

    printf("enter numbers, %s to finish:\n", STOP_WORD);
    st = read_numbers(base, &stats);
    if (st != STATUS_OK) {
        print_status(st);
        return 1;
    }

    if (stats.count == 0) {
        print_status(STATUS_ERR_NO_NUMBERS);
        return 1;
    }

    repr_t max_repr, sum_repr;
    st = build_representations(stats.max_abs, base, &max_repr);
    if (st == STATUS_OK)
        st = build_representations(stats.sum, base, &sum_repr);
    if (st != STATUS_OK) {
        print_status(st);
        return 1;
    }

    print_representations("max by abs", &max_repr);
    print_representations("sum", &sum_repr);
    return 0;
}