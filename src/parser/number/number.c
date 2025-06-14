#include "number.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include "../../memory.h"
#include <gmp.h>
#include <stdio.h>
#include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <ctype.h>


// int parse_exponent(const char *exp_str, long *exp_val) {
//     char *endptr;
//     long val = strtol(exp_str, &endptr, 10);
//     if (*endptr != '\0') {
//         // exponent contains invalid chars or decimal point → reject
//         return -1;
//     }
//     *exp_val = val;
//     return 0;
// }

// int mpq_set_decimal_str_exp(mpq_t r, const char *str) {
//     // Skip leading whitespace
//     while (isspace(*str)) str++;

//     // Handle sign
//     int negative = 0;
//     if (*str == '-') {
//         negative = 1;
//         str++;
//     } else if (*str == '+') {
//         str++;
//     }

//     // Copy input to a buffer for manipulation
//     size_t len = strlen(str);
//     char *buf = malloc(len + 1);
//     if (!buf) return -1;
//     strcpy(buf, str);

//     // Find 'e' or 'E'
//     char *e_ptr = strchr(buf, 'e');
//     if (!e_ptr) e_ptr = strchr(buf, 'E');

//     char *exp_str = NULL;
//     if (e_ptr) {
//         *e_ptr = '\0';
//         exp_str = e_ptr + 1;
//     }

//     // Validate decimal part (digits and one dot)
//     int dot_count = 0;
//     for (char *p = buf; *p; p++) {
//         if (*p == '.') {
//             if (++dot_count > 1) { free(buf); return -1; }
//             continue;
//         }
//         if (!isdigit((unsigned char)*p)) { free(buf); return -1; }
//     }

//     // Extract integer and fractional parts
//     char *dot = strchr(buf, '.');
//     size_t int_len = dot ? (size_t)(dot - buf) : strlen(buf);
//     size_t frac_len = dot ? strlen(dot + 1) : 0;

//     // Validate exponent if present
//     int exp_negative = 0;
//     long exp_val = 0;
//     if (exp_str) {
//         // Skip leading spaces in exponent (not in regex but safe)
//         while (isspace(*exp_str)) exp_str++;

//         if (*exp_str == '-') {
//             exp_negative = 1;
//             exp_str++;
//         } else if (*exp_str == '+') {
//             exp_str++;
//         }

//         if (!isdigit((unsigned char)*exp_str)) {
//             free(buf);
//             return -1;
//         }

//         char *endptr;
//         exp_val = strtol(exp_str, &endptr, 10);
//         if (*endptr != '\0') {
//             free(buf);
//             return -1;
//         }
//         if (exp_negative) exp_val = -exp_val;
//     }

//     // Build numerator string (integer part + fractional part)
//     size_t num_len = int_len + frac_len;
//     if (num_len == 0) { free(buf); return -1; }

//     char *num_str = malloc(num_len + 1);
//     if (!num_str) { free(buf); return -1; }

//     if (int_len > 0) memcpy(num_str, buf, int_len);
//     if (frac_len > 0) memcpy(num_str + int_len, dot + 1, frac_len);
//     num_str[num_len] = '\0';

//     // Calculate denominator exponent considering exponent part
//     long denom_exp = frac_len - exp_val;

//     mpz_t numerator, denominator;
//     mpz_init(numerator);
//     mpz_init(denominator);

//     if (mpz_set_str(numerator, num_str, 10) != 0) {
//         free(num_str);
//         free(buf);
//         mpz_clear(numerator);
//         mpz_clear(denominator);
//         return -1;
//     }
//     free(num_str);
//     free(buf);

//     if (denom_exp >= 0) {
//         mpz_ui_pow_ui(denominator, 10, (unsigned long)denom_exp);
//     } else {
//         // denom_exp < 0 means multiply numerator by 10^(-denom_exp)
//         mpz_ui_pow_ui(denominator, 10, 0);
//         mpz_ui_pow_ui(numerator, 10, (unsigned long)(-denom_exp));
//     }

//     if (denom_exp < 0) {
//         mpz_t temp;
//         mpz_init(temp);
//         mpz_ui_pow_ui(temp, 10, (unsigned long)(-denom_exp));
//         mpz_mul(numerator, numerator, temp);
//         mpz_clear(temp);
//         mpz_set_ui(denominator, 1);
//     }

//     mpq_set_num(r, numerator);
//     mpq_set_den(r, denominator);
//     mpq_canonicalize(r);

//     if (negative) mpq_neg(r, r);

//     mpz_clear(numerator);
//     mpz_clear(denominator);

//     return 0;
// }

ParsedValue *parse_number(Token *token) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_NUMBER;
  parsedValue->data = strdup(token->value);
  return parsedValue;
}