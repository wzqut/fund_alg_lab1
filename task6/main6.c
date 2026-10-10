#include <math.h>
#include <stdio.h>
#include "functions6.h"


static void print_error(const status_code code)
{
    switch (code) {
        case ERROR_INVALID_ARGS:    printf("Ошибка: неверные аргументы функции\n"); break;
        case ERROR_NULL_POINTER:    printf("Ошибка: нулевой указатель\n"); break;
        case ERROR_INVALID_NUMBER:  printf("Ошибка: некорректная строка с числом\n"); break;
        case ERROR_OVERFLOW:        printf("Ошибка: переполнение\n"); break;
        case ERROR_DOMAIN:          printf("Ошибка: значение вне области определения\n"); break;
        case ERROR_NO_ROOT:         printf("Ошибка: на интервале нет корня (знаки на концах совпадают)\n"); break;
        case ERROR_ITERATIONS:      printf("Ошибка: превышено число итераций\n"); break;
        case SUCCESS:               break;
        default:                    printf("Неизвестная ошибка\n"); break;
    }
}

static int report(const status_code status)
{
    if (status == SUCCESS)
        return 0;
    print_error(status);
    return 1;
}

static int expect_error(const status_code status, const status_code expected)
{
    if (status == expected) {
        print_error(status);
        return 0;
    }
    printf("НЕОЖИДАННЫЙ РЕЗУЛЬТАТ: код %d, ожидался %d\n", (int)status, (int)expected);
    return 1;
}

static void print_convex(const char *name, const int convex)
{
    printf("%s: %s\n", name, convex ? "выпуклый" : "невыпуклый");
}

static void print_found(const char *title, const char **found, const int count)
{
    printf("%s:", title);
    if (count == 0)
        printf(" нет");
    for (int i = 0; i < count; i++)
        printf(" %s", found[i]);
    printf("\n");
}

static double f1(const double x) { return x * x - 2.0; }
static double f2(const double x) { return cos(x) - x; }
static double f3(const double x) { return x * x * x - x - 2.0; }
static double f4(const double x) { return x - 1.0; }
static double f_log(const double x) { return log(x); }

int main(void)
{
    const double eps1 = 1e-3;
    const double eps2 = 1e-9;
    int convex = 0;
    double value = 0.0;
    const char *found[8];
    int found_count = 0;
    status_code st;
    int failed = 0;

    printf("--- 1. Выпуклость ---\n");
    st = is_convex(&convex, eps2, 4, 0.0, 0.0, 2.0, 0.0, 2.0, 2.0, 0.0, 2.0);
    if (report(st)) failed = 1; else print_convex("Квадрат", convex);

    st = is_convex(&convex, eps2, 4, 0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0);
    if (report(st)) failed = 1; else print_convex("Квадрат (по часовой стрелке)", convex);

    st = is_convex(&convex, eps2, 3, 0.0, 0.0, 4.0, 0.0, 0.0, 3.0);
    if (report(st)) failed = 1; else print_convex("Треугольник", convex);

    st = is_convex(&convex, eps2, 5, 0.0, 0.0, 2.0, 0.0, 1.0, 1.0, 2.0, 2.0, 0.0, 2.0);
    if (report(st)) failed = 1; else print_convex("Стрела", convex);

    st = is_convex(&convex, eps2, 5, 0.0, 1.0, 0.5878, -0.8090, -0.9511, 0.3090,
                   0.9511, 0.3090, -0.5878, -0.8090);
    if (report(st)) failed = 1; else print_convex("Пентаграмма (самопересечение)", convex);

    st = is_convex(&convex, eps2, 3, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0);
    if (report(st)) failed = 1; else print_convex("Три точки на одной прямой", convex);

    printf("\n--- 2. Многочлен ---\n");
    st = poly_value(&value, 2.0, 2, 2.0, -3.0, 1.0);
    if (report(st)) failed = 1; else printf("2x^2 - 3x + 1 при x=2: %g\n", value);

    st = poly_value(&value, 0.5, 3, 1.0, 0.0, 0.0, 0.0);
    if (report(st)) failed = 1; else printf("x^3 при x=0.5: %g\n", value);

    st = poly_value(&value, -1.5, 4, 1.0, -2.0, 0.0, 4.0, -5.0);
    if (report(st)) failed = 1; else printf("x^4 - 2x^3 + 4x - 5 при x=-1.5: %g\n", value);

    st = poly_value(&value, 100.0, 0, 7.0);
    if (report(st)) failed = 1; else printf("константа 7 (степень 0): %g\n", value);

    printf("\n--- 3. Числа Капрекара ---\n");
    st = find_kaprekar(found, &found_count, 10, 6, "9", "45", "297", "10", "55", "7");
    if (report(st)) failed = 1; else print_found("Основание 10 из (9 45 297 10 55 7)", found, found_count);

    st = find_kaprekar(found, &found_count, 10, 4, "1", "99", "100", "4950");
    if (report(st)) failed = 1; else print_found("Основание 10 из (1 99 100 4950)", found, found_count);

    st = find_kaprekar(found, &found_count, 16, 5, "FF", "55", "A", "b", "1F");
    if (report(st)) failed = 1; else print_found("Основание 16 из (FF 55 A b 1F)", found, found_count);

    st = find_kaprekar(found, &found_count, 2, 4, "1", "10", "11", "110");
    if (report(st)) failed = 1; else print_found("Основание 2 из (1 10 11 110)", found, found_count);

    printf("\n--- 4. Среднее геометрическое ---\n");
    st = geom_mean(&value, 4, 1.0, 3.0, 9.0, 27.0);
    if (report(st)) failed = 1; else printf("gm(1, 3, 9, 27) = %g\n", value);

    st = geom_mean(&value, 2, 2.0, 8.0);
    if (report(st)) failed = 1; else printf("gm(2, 8) = %g\n", value);

    st = geom_mean(&value, 3, 0.0, 4.0, 9.0);
    if (report(st)) failed = 1; else printf("gm(0, 4, 9) = %g\n", value);

    st = geom_mean(&value, 1, 5.0);
    if (report(st)) failed = 1; else printf("gm(5) = %g\n", value);

    printf("\n--- 5. Быстрое возведение в степень ---\n");
    st = fast_pow(&value, 2.0, 10, eps2);
    if (report(st)) failed = 1; else printf("2^10 = %g\n", value);

    st = fast_pow(&value, 2.0, -3, eps2);
    if (report(st)) failed = 1; else printf("2^-3 = %g\n", value);

    st = fast_pow(&value, -2.0, 3, eps2);
    if (report(st)) failed = 1; else printf("(-2)^3 = %g\n", value);

    st = fast_pow(&value, 1.5, 0, eps2);
    if (report(st)) failed = 1; else printf("1.5^0 = %g\n", value);

    st = fast_pow(&value, 10.0, -2, eps2);
    if (report(st)) failed = 1; else printf("10^-2 = %g\n", value);

    printf("\n--- 6. Дихотомия ---\n");
    st = bisection(&value, 0.0, 2.0, eps1, f1);
    if (report(st)) failed = 1; else printf("x^2-2 на [0,2], eps=1e-3: %.6f\n", value);

    st = bisection(&value, 0.0, 2.0, eps2, f1);
    if (report(st)) failed = 1; else printf("x^2-2 на [0,2], eps=1e-9: %.9f\n", value);

    st = bisection(&value, 0.0, 1.0, eps2, f2);
    if (report(st)) failed = 1; else printf("cos(x)-x на [0,1], eps=1e-9: %.9f\n", value);

    st = bisection(&value, 1.0, 2.0, eps2, f3);
    if (report(st)) failed = 1; else printf("x^3-x-2 на [1,2], eps=1e-9: %.9f\n", value);

    st = bisection(&value, -5.0, 5.0, eps1, f3);
    if (report(st)) failed = 1; else printf("x^3-x-2 на [-5,5], eps=1e-3: %.6f\n", value);

    st = bisection(&value, 0.0, 2.0, eps2, f4);
    if (report(st)) failed = 1; else printf("x-1 на [0,2] (корень в середине): %.9f\n", value);

    printf("\n--- Ошибки ---\n");
    failed |= expect_error(is_convex(NULL, eps2, 3, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0), ERROR_NULL_POINTER);
    failed |= expect_error(is_convex(&convex, eps2, 2, 0.0, 0.0, 1.0, 1.0), ERROR_INVALID_ARGS);
    failed |= expect_error(is_convex(&convex, eps2, 3, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0), ERROR_INVALID_ARGS);
    failed |= expect_error(is_convex(&convex, eps2, 3, 0.0, 0.0, NAN, 0.0, 1.0, 1.0), ERROR_INVALID_ARGS);
    failed |= expect_error(poly_value(&value, 2.0, -1), ERROR_INVALID_ARGS);
    failed |= expect_error(poly_value(&value, 1e200, 2, 1.0, 0.0, 0.0), ERROR_OVERFLOW);
    failed |= expect_error(find_kaprekar(found, &found_count, 10, 1, "29ц8е40цк349"), ERROR_INVALID_NUMBER);
    failed |= expect_error(find_kaprekar(found, &found_count, 10, 1, ""), ERROR_INVALID_NUMBER);
    failed |= expect_error(find_kaprekar(found, &found_count, 8, 1, "189"), ERROR_INVALID_NUMBER);
    failed |= expect_error(find_kaprekar(found, &found_count, 10, 1, "99999999999x"), ERROR_INVALID_NUMBER);
    failed |= expect_error(find_kaprekar(found, &found_count, 10, 1, "294849295899"), ERROR_OVERFLOW);
    failed |= expect_error(find_kaprekar(found, &found_count, 10, 1, (const char *)NULL), ERROR_NULL_POINTER);
    failed |= expect_error(find_kaprekar(found, &found_count, 1, 1, "9"), ERROR_INVALID_ARGS);
    failed |= expect_error(geom_mean(&value, 2, 4.0, -1.0), ERROR_DOMAIN);
    failed |= expect_error(geom_mean(&value, 0), ERROR_INVALID_ARGS);
    failed |= expect_error(geom_mean(&value, 2, 4.0, NAN), ERROR_INVALID_ARGS);
    failed |= expect_error(fast_pow(&value, 0.0, -1, eps2), ERROR_DOMAIN);
    failed |= expect_error(fast_pow(&value, 1e200, 2, eps2), ERROR_OVERFLOW);
    failed |= expect_error(fast_pow(&value, 2.0, 3, 0.0), ERROR_INVALID_ARGS);
    failed |= expect_error(fast_pow(NULL, 2.0, 3, eps2), ERROR_NULL_POINTER);
    failed |= expect_error(bisection(&value, 3.0, 4.0, eps2, f1), ERROR_NO_ROOT);
    failed |= expect_error(bisection(&value, 0.0, 2.0, 1e-30, f1), ERROR_ITERATIONS);
    failed |= expect_error(bisection(&value, 2.0, 0.0, eps2, f1), ERROR_INVALID_ARGS);
    failed |= expect_error(bisection(&value, 0.0, 2.0, eps2, NULL), ERROR_NULL_POINTER);
    failed |= expect_error(bisection(&value, -1.0, 2.0, eps2, f_log), ERROR_DOMAIN);

    return failed;
}