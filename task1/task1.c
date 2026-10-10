#include <limits.h>
#include <stddef.h>
#include <stdio.h>

typedef unsigned long long ull;

#define MULTIPLES_LIMIT 100
#define POWER_MAX       10
#define HEX_BUF_SIZE    17

typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_INVALID_ARG_COUNT,
    STATUS_EMPTY_INPUT,
    STATUS_INVALID_NUMBER,
    STATUS_NEGATIVE_NUMBER,
    STATUS_NOT_NATURAL,
    STATUS_INVALID_FLAG,
    STATUS_OUT_OF_RANGE,
    STATUS_OVERFLOW,
    STATUS_BUFFER_TOO_SMALL
} Status;

typedef enum {
    FLAG_H, FLAG_P, FLAG_S, FLAG_E, FLAG_A, FLAG_F
} Flag;

typedef enum {
    NUM_NEITHER, NUM_PRIME, NUM_COMPOSITE
} PrimeKind;

typedef struct {
    ull cell[POWER_MAX][POWER_MAX];
} PowerTable;

static Status validate_and_parse_natural(const char *str, ull *result)
{
    if (str == NULL || result == NULL) return STATUS_NULL_POINTER;
    if (str[0] == '\0') return STATUS_EMPTY_INPUT;
    if (str[0] == '-') return STATUS_NEGATIVE_NUMBER;

    for (const char *p = str; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return STATUS_INVALID_NUMBER;
    }

    ull value = 0;
    for (const char *p = str; *p != '\0'; ++p) {
        ull digit = (ull)(*p - '0');
        if (value > (ULLONG_MAX - digit) / 10) return STATUS_OVERFLOW;
        value = value * 10 + digit;
    }

    if (value == 0) return STATUS_NOT_NATURAL;
    *result = value;
    return STATUS_OK;
}

static Status parse_flag(const char *str, Flag *flag)
{
    if (str == NULL || flag == NULL) return STATUS_NULL_POINTER;
    if (str[0] != '-' && str[0] != '/') return STATUS_INVALID_FLAG;
    if (str[1] == '\0' || str[2] != '\0') return STATUS_INVALID_FLAG;

    switch (str[1]) {
        case 'h': *flag = FLAG_H; break;
        case 'p': *flag = FLAG_P; break;
        case 's': *flag = FLAG_S; break;
        case 'e': *flag = FLAG_E; break;
        case 'a': *flag = FLAG_A; break;
        case 'f': *flag = FLAG_F; break;
        default:  return STATUS_INVALID_FLAG;
    }
    return STATUS_OK;
}


static Status find_multiples(ull x, unsigned *out, size_t out_size, size_t *count)
{
    if (out == NULL || count == NULL) return STATUS_NULL_POINTER;
    if (x == 0) return STATUS_NOT_NATURAL;
    if (out_size < MULTIPLES_LIMIT) return STATUS_BUFFER_TOO_SMALL;

    size_t n = 0;
    if (x <= MULTIPLES_LIMIT) {
        for (ull m = x; m <= MULTIPLES_LIMIT; m += x) {
            out[n++] = (unsigned)m;
        }
    }
    *count = n;
    return STATUS_OK;
}

static Status check_prime(ull x, PrimeKind *kind)
{
    if (kind == NULL) return STATUS_NULL_POINTER;
    if (x == 0) return STATUS_NOT_NATURAL;

    if (x == 1) { *kind = NUM_NEITHER; return STATUS_OK; }
    if (x < 4)  { *kind = NUM_PRIME;   return STATUS_OK; }
    if (x % 2 == 0 || x % 3 == 0) { *kind = NUM_COMPOSITE; return STATUS_OK; }
    
    for (ull i = 5; i <= x / i; i += 6) {
        if (x % i == 0 || x % (i + 2) == 0) {
            *kind = NUM_COMPOSITE;
            return STATUS_OK;
        }
    }
    *kind = NUM_PRIME;
    return STATUS_OK;
}

static Status to_hex_digits(ull x, char *buf, size_t buf_size, size_t *len)
{
    static const char DIGITS[] = "0123456789ABCDEF";
    if (buf == NULL || len == NULL) return STATUS_NULL_POINTER;

    char tmp[HEX_BUF_SIZE];
    size_t n = 0;
    do {
        if (n >= sizeof tmp) return STATUS_OVERFLOW;
        tmp[n++] = DIGITS[x % 16];
        x /= 16;
    } while (x > 0);

    if (buf_size < n + 1) return STATUS_BUFFER_TOO_SMALL;

    for (size_t i = 0; i < n; ++i) buf[i] = tmp[n - 1 - i];
    buf[n] = '\0';
    *len = n;
    return STATUS_OK;
}

static Status build_power_table(ull x, PowerTable *table)
{
    if (table == NULL) return STATUS_NULL_POINTER;
    if (x < 1 || x > POWER_MAX) return STATUS_OUT_OF_RANGE;

    for (ull base = 1; base <= POWER_MAX; ++base) {
        ull value = 1;
        for (ull e = 1; e <= x; ++e) {
            if (value > ULLONG_MAX / base) return STATUS_OVERFLOW;
            value *= base;
            table->cell[base - 1][e - 1] = value;
        }
    }
    return STATUS_OK;
}

static Status sum_to(ull x, ull *result)
{
    if (result == NULL) return STATUS_NULL_POINTER;
    if (x == 0) return STATUS_NOT_NATURAL;
    if (x == ULLONG_MAX) return STATUS_OVERFLOW;

    ull a = x, b = x + 1;
    if (a % 2 == 0) a /= 2; else b /= 2;

    if (a > ULLONG_MAX / b) return STATUS_OVERFLOW;
    *result = a * b;
    return STATUS_OK;
}

static Status factorial(ull x, ull *result)
{
    if (result == NULL) return STATUS_NULL_POINTER;
    if (x == 0) return STATUS_NOT_NATURAL;

    ull value = 1;
    for (ull i = 2; i <= x; ++i) {
        if (value > ULLONG_MAX / i) return STATUS_OVERFLOW;
        value *= i;
    }
    *result = value;
    return STATUS_OK;
}


static void print_error(Status status)
{
    switch (status) {
        case STATUS_INVALID_ARG_COUNT:
            fprintf(stderr, "Ошибка: нужно ровно 2 аргумента: <x> <-h|-p|-s|-e|-a|-f>\n");
            break;
        case STATUS_EMPTY_INPUT:
            fprintf(stderr, "Ошибка: x не может быть пустой строкой\n");
            break;
        case STATUS_INVALID_NUMBER:
            fprintf(stderr, "Ошибка: x должно состоять только из десятичных цифр "
                            "(без пробелов, букв и знаков)\n");
            break;
        case STATUS_NEGATIVE_NUMBER:
            fprintf(stderr, "Ошибка: x не может быть отрицательным "
                            "(значение не должно начинаться с '-'), "
                            "нужно натуральное число (>= 1)\n");
            break;
        case STATUS_NOT_NATURAL:
            fprintf(stderr, "Ошибка: x должно быть натуральным числом (>= 1)\n");
            break;
        case STATUS_INVALID_FLAG:
            fprintf(stderr, "Ошибка: неверный флаг (допустимы -h -p -s -e -a -f, "
                            "а также с символом '/')\n");
            break;
        case STATUS_OUT_OF_RANGE:
            fprintf(stderr, "Ошибка: x вне допустимого диапазона для этого флага "
                            "(для -e: от 1 до %d)\n", POWER_MAX);
            break;
        case STATUS_OVERFLOW:
            fprintf(stderr, "Ошибка: переполнение при вводе или вычислении\n");
            break;
        case STATUS_NULL_POINTER:
            fprintf(stderr, "Внутренняя ошибка: нулевой указатель\n");
            break;
        case STATUS_BUFFER_TOO_SMALL:
            fprintf(stderr, "Внутренняя ошибка: буфер слишком мал\n");
            break;
        case STATUS_OK:
            break;
    }
}

static Status print_multiples(const unsigned *items, size_t count)
{
    if (items == NULL && count > 0) return STATUS_NULL_POINTER;

    if (count == 0) {
        printf("Натуральных чисел в пределах 100, кратных x, нет\n");
        return STATUS_OK;
    }
    for (size_t i = 0; i < count; ++i) {
        printf("%u%c", items[i], (i + 1 < count) ? ' ' : '\n');
    }
    return STATUS_OK;
}

static void print_prime_kind(PrimeKind kind)
{
    switch (kind) {
        case NUM_PRIME:     printf("Число простое\n"); break;
        case NUM_COMPOSITE: printf("Число составное\n"); break;
        case NUM_NEITHER:   printf("Число не является ни простым, ни составным\n"); break;
    }
}

static Status print_spaced(const char *digits, size_t len)
{
    if (digits == NULL) return STATUS_NULL_POINTER;

    for (size_t i = 0; i < len; ++i) {
        printf("%c%c", digits[i], (i + 1 < len) ? ' ' : '\n');
    }
    return STATUS_OK;
}

static Status print_power_table(ull x, const PowerTable *table)
{
    if (table == NULL) return STATUS_NULL_POINTER;
    if (x < 1 || x > POWER_MAX) return STATUS_OUT_OF_RANGE;

    printf("Строки - основания 1..10, столбцы - показатели 1..%llu\n", x);
    for (size_t b = 0; b < POWER_MAX; ++b) {
        printf("%2zu:", b + 1);
        for (ull e = 0; e < x; ++e) {
            printf(" %llu", table->cell[b][e]);
        }
        printf("\n");
    }
    return STATUS_OK;
}


static Status run_action(Flag flag, ull x)
{
    Status st = STATUS_OK;

    switch (flag) {
        case FLAG_H: {
            unsigned items[MULTIPLES_LIMIT];
            size_t count = 0;
            st = find_multiples(x, items, MULTIPLES_LIMIT, &count);
            if (st == STATUS_OK) st = print_multiples(items, count);
            break;
        }
        case FLAG_P: {
            PrimeKind kind = NUM_NEITHER;
            st = check_prime(x, &kind);
            if (st == STATUS_OK) print_prime_kind(kind);
            break;
        }
        case FLAG_S: {
            char buf[HEX_BUF_SIZE];
            size_t len = 0;
            st = to_hex_digits(x, buf, sizeof buf, &len);
            if (st == STATUS_OK) st = print_spaced(buf, len);
            break;
        }
        case FLAG_E: {
            PowerTable table;
            st = build_power_table(x, &table);
            if (st == STATUS_OK) st = print_power_table(x, &table);
            break;
        }
        case FLAG_A: {
            ull sum = 0;
            st = sum_to(x, &sum);
            if (st == STATUS_OK) printf("%llu\n", sum);
            break;
        }
        case FLAG_F: {
            ull f = 0;
            st = factorial(x, &f);
            if (st == STATUS_OK) printf("%llu\n", f);
            break;
        }
    }
    return st;
}


int main(int argc, char *argv[])
{
    Status st = STATUS_OK;

    if (argc != 3) {
        st = STATUS_INVALID_ARG_COUNT;
    } else if (argv == NULL || argv[1] == NULL || argv[2] == NULL) {
        st = STATUS_NULL_POINTER;
    }

    ull x = 0;
    Flag flag = FLAG_H;
    if (st == STATUS_OK) st = validate_and_parse_natural(argv[1], &x);
    if (st == STATUS_OK) st = parse_flag(argv[2], &flag);
    if (st == STATUS_OK) st = run_action(flag, x);

    if (st != STATUS_OK) {
        print_error(st);
        return 1;
    }
    return 0;
}
