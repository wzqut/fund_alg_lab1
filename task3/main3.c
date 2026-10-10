#include <stdio.h>
#include <string.h>
#include "functions3.h"

static void print_hint(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -q <eps> <a> <b> <c>  solve a*x^2 + b*x + c = 0 for all unique permutations of a, b, c\n", prog);
    fprintf(stderr, "  %s -m <a> <b>            check if a is a multiple of b (non-zero integers)\n", prog);
    fprintf(stderr, "  %s -t <eps> <a> <b> <c>  check if a, b, c can be sides of a right triangle\n", prog);
    fprintf(stderr, "Flags may start with '-' or '/'. eps must be in [1e-9, 1].\n");
}

static void print_status_error(FILE *out, const status_t status, const char *arg)
{
    if (arg == NULL)
        arg = "";

    switch (status) {
        case STATUS_ERR_NAN:
            fprintf(out, "Error: <%s> is not a valid number\n", arg);
            break;
        case STATUS_ERR_OVERFLOW:
            if (arg[0] != '\0')
                fprintf(out, "Error: number <%s> is too large (overflow)\n", arg);
            else
                fprintf(out, "Error: overflow during calculation\n");
            break;
        case STATUS_ERR_EPS_RANGE:
            fprintf(out, "Error: eps <%s> must be in [1e-9, 1]\n", arg);
            break;
        case STATUS_ERR_ZERO:
            fprintf(out, "Error: numbers must be non-zero\n");
            break;
        case STATUS_ERR_NOT_POSITIVE:
            fprintf(out, "Error: sides must be > eps (positive)\n");
            break;
        case STATUS_ERR_NULL_PTR:
            fprintf(out, "Error: null pointer\n");
            break;
        case STATUS_ERR_ARG_COUNT:
            fprintf(out, "Error: wrong number of parameters\n");
            break;
        case STATUS_ERR_FLAG:
            fprintf(out, "Error: unknown flag <%s>\n", arg);
            break;
        case STATUS_OK:
        default:
            fprintf(out, "Error: unknown status\n");
            break;
    }
}

static status_t parse_flag(const char *str, char *flag)
{
    if (str == NULL || flag == NULL)
        return STATUS_ERR_NULL_PTR;
    if (strlen(str) != 2 || (str[0] != '-' && str[0] != '/'))
        return STATUS_ERR_FLAG;
    if (str[1] != 'q' && str[1] != 'm' && str[1] != 't')
        return STATUS_ERR_FLAG;

    *flag = str[1];
    return STATUS_OK;
}

static long double no_neg_zero(const long double x) { return x + 0.0L; }

static void print_equation(const long double coefs[COEF_COUNT])
{
    printf("[%.9Lgx^2 %+.9Lgx %+.9Lg = 0]",
           no_neg_zero(coefs[0]), no_neg_zero(coefs[1]), no_neg_zero(coefs[2]));
}

static void print_roots(const roots_t *roots)
{
    switch (roots->kind) {
        case ROOTS_TWO:
            printf("solution: [%.9Lf, %.9Lf]\n", no_neg_zero(roots->x1), no_neg_zero(roots->x2));
            break;
        case ROOTS_ONE:
            printf("solution: [%.9Lf]\n", no_neg_zero(roots->x1));
            break;
        case ROOTS_NO_REAL:
            printf("solution: [no real roots]\n");
            break;
        case ROOTS_ANY:
            printf("solution: [any x]\n");
            break;
        case ROOTS_NONE:
            printf("solution: [no solutions]\n");
            break;
    }
}

static status_t parse_eps_and_three(char *argv[], long double *eps,
                                    long double values[COEF_COUNT], const char **bad)
{
    if (argv == NULL || eps == NULL || values == NULL || bad == NULL)
        return STATUS_ERR_NULL_PTR;

    *bad = argv[2];
    status_t st = parse_eps(argv[2], eps);
    if (st != STATUS_OK)
        return st;

    for (int i = 0; i < COEF_COUNT; i++) {
        *bad = argv[3 + i];
        st = parse_real(argv[3 + i], &values[i]);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

static int run_q(char *argv[])
{
    long double eps = 0.0L, coefs[COEF_COUNT];
    const char *bad = "";
    status_t st = parse_eps_and_three(argv, &eps, coefs, &bad);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, bad);
        return 1;
    }

    long double perms[PERM_COUNT][COEF_COUNT];
    int count = 0;
    st = unique_permutations(eps, coefs, perms, &count);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, "");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        roots_t roots = {ROOTS_NONE, 0.0L, 0.0L};
        st = solve_equation(eps, perms[i][0], perms[i][1], perms[i][2], &roots);
        print_equation(perms[i]);
        printf(", ");
        if (st == STATUS_OK)
            print_roots(&roots);
        else
            print_status_error(stdout, st, "");
    }
    return 0;
}

static int run_m(char *argv[])
{
    long long a = 0, b = 0;
    status_t st = parse_integer(argv[2], &a);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, argv[2]);
        return 1;
    }
    st = parse_integer(argv[3], &b);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, argv[3]);
        return 1;
    }

    bool is_multiple = false;
    st = check_multiple(a, b, &is_multiple);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, "");
        return 1;
    }
    printf("%lld %s a multiple of %lld\n", a, is_multiple ? "is" : "is not", b);
    return 0;
}

static int run_t(char *argv[])
{
    long double eps = 0.0L, sides[COEF_COUNT];
    const char *bad = "";
    status_t st = parse_eps_and_three(argv, &eps, sides, &bad);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, bad);
        return 1;
    }

    bool is_right = false;
    st = check_right_triangle(eps, sides[0], sides[1], sides[2], &is_right);
    if (st != STATUS_OK) {
        print_status_error(stderr, st, "");
        return 1;
    }
    printf("%.9Lg, %.9Lg, %.9Lg %s be sides of a right triangle\n",
           sides[0], sides[1], sides[2], is_right ? "can" : "can not");
    return 0;
}

int main(int argc, char *argv[])
{
    const char *prog = (argc > 0 && argv != NULL && argv[0] != NULL) ? argv[0] : "task3";

    if (argc < 2 || argv == NULL || argv[1] == NULL) {
        print_hint(prog);
        print_status_error(stderr, STATUS_ERR_ARG_COUNT, "");
        return 1;
    }

    char flag = 0;
    const status_t st = parse_flag(argv[1], &flag);
    if (st != STATUS_OK) {
        print_hint(prog);
        print_status_error(stderr, st, argv[1]);
        return 1;
    }

    const int need_argc = (flag == 'm') ? 4 : 6;
    if (argc != need_argc) {
        fprintf(stderr, "Error: flag -%c needs %d parameters, got %d\n", flag, need_argc - 2, argc - 2);
        return 1;
    }
    for (int i = 2; i < argc; i++) {
        if (argv[i] == NULL) {
            print_status_error(stderr, STATUS_ERR_NULL_PTR, "");
            return 1;
        }
    }

    switch (flag) {
        case 'q': return run_q(argv);
        case 'm': return run_m(argv);
        case 't': return run_t(argv);
        default:  return 1;
    }
}