#include "number.h"
#include "../string/string.h"

#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

regex_t numberCompile;

void initNumber()
{
    int compileError;
    compileError = regcomp(&numberCompile, "^( *)(-)?(((([0-9]+(\\.[0-9]+)?)|(\\.[0-9]+))(e((\\-|\\+)?([0-9]+)))?))( *)$", REG_EXTENDED);
    if (compileError)
    {
        char errorBuffer[1024];
        regerror(compileError, &numberCompile, errorBuffer, sizeof(errorBuffer));
        fprintf(stderr, "Error compiling regex: %s\n", errorBuffer);
        exit(1);
    }
}

void cleanupNumber()
{
    regfree(&numberCompile);
}

int gcd(int64_t a, int64_t b)
{
    while (b != 0)
    {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void simplifyFraction(int64_t *numerator, int64_t *denominator)
{
    int common_divisor = gcd(*numerator, *denominator);
    *numerator /= common_divisor;
    *denominator /= common_divisor;
}

void doubleToFraction(double num, int64_t *numerator, uint64_t *denominator) {
    int currentSign = (num < 0) ? -1 : 1;
    num = fabs(num);

    double tolerance = 1.0e-10;
    double h1 = 1, h2 = 0, k1 = 0, k2 = 1;
    double b = num;
    do {
        double a = floor(b);
        double aux = h1;
        h1 = a * h1 + h2;
        h2 = aux;
        aux = k1;
        k1 = a * k1 + k2;
        k2 = aux;
        b = 1 / (b - a);
    } while (fabs(num - h1 / k1) > num * tolerance);

    *numerator = (int64_t)(h1 * currentSign);
    *denominator = (uint64_t)k1;
}

struct number translateNumber(char *code)
{
    char *codeClone = cloneString(code);
    stripString(codeClone, WHITE_SPACE);
    int reti = regexec(&numberCompile, codeClone, 0, NULL, 0);
    if (reti == REG_NOMATCH)
    {
        return (struct number){
            .numerator = 0,
            .denominator = 0
        };
    }
    struct number num;
    num.numerator = 0;
    num.denominator = 1;

    double coefficient = 0;
    int exponent = 0;

    char *e = strchr(codeClone, 'e');
    if (e) {
        *e = '\0';
        e++;
        if (*e == '+') e++;
        exponent = atoi(e);
    }

    coefficient = atof(codeClone);

    doubleToFraction(coefficient, &num.numerator, &num.denominator);

    if (exponent > 0) {
        num.numerator *= (int64_t)pow(10, exponent);
    } else if (exponent < 0) {
        num.denominator *= (int64_t)pow(10, -exponent);
    }

    return num;
}
