#include "string.h"
#include "../../lexer/token.h"

#include "../../memory.h"
#include <cjson/cJSON.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Helper: Convert 4 hex digits from input to a uint16_t value
static int parse_hex4(const char *in, uint16_t *out) {
    uint16_t val = 0;
    for (int i = 0; i < 4; i++) {
        char c = in[i];
        val <<= 4;
        if (c >= '0' && c <= '9')
            val |= (c - '0');
        else if (c >= 'a' && c <= 'f')
            val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            val |= (c - 'A' + 10);
        else
            return 0; // invalid hex digit
    }
    *out = val;
    return 1;
}

// Helper: Encode a Unicode codepoint as UTF-8, write to *out_ptr, return bytes written
static int utf8_encode(uint32_t codepoint, char **out_ptr) {
    char *p = *out_ptr;
    if (codepoint <= 0x7F) {
        *p++ = (char)codepoint;
        *out_ptr = p;
        return 1;
    }
    else if (codepoint <= 0x7FF) {
        *p++ = (char)(0xC0 | (codepoint >> 6));
        *p++ = (char)(0x80 | (codepoint & 0x3F));
        *out_ptr = p;
        return 2;
    }
    else if (codepoint <= 0xFFFF) {
        *p++ = (char)(0xE0 | (codepoint >> 12));
        *p++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        *p++ = (char)(0x80 | (codepoint & 0x3F));
        *out_ptr = p;
        return 3;
    }
    else if (codepoint <= 0x10FFFF) {
        *p++ = (char)(0xF0 | (codepoint >> 18));
        *p++ = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        *p++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        *p++ = (char)(0x80 | (codepoint & 0x3F));
        *out_ptr = p;
        return 4;
    }
    return 0; // invalid codepoint
}

/**
 * unquote_json_string:
 *  Parses and unescapes a JSON string literal including quotes,
 *  returning a malloc'ed buffer with the decoded string and its length (including embedded nulls).
 *  
 * Parameters:
 *  input: const char* JSON string literal (must start and end with quotes)
 *  out_len: pointer to size_t to receive decoded length
 * 
 * Returns:
 *  malloc'ed buffer with decoded string (not necessarily null-terminated)
 *  NULL on error (invalid input)
 * 
 * Caller must free() returned buffer.
 */
char *unquote_json_string(const char *input, size_t *out_len) {
    if (!input || input[0] != '"') return NULL;

    // Find the closing quote
    const char *p = input + 1;
    const char *end = NULL;
    while (*p) {
        if (*p == '"') {
            end = p;
            break;
        }
        // Skip escaped quotes and escapes
        if (*p == '\\') {
            p++;
            if (*p == '\0') return NULL; // invalid escape at end
        }
        p++;
    }
    if (!end) return NULL; // no closing quote

    size_t input_len = end - (input + 1); // length inside quotes
    const char *src = input + 1;
    // Allocate max output size = input_len, decoded string cannot be longer than input_len
    char *outbuf = (char *)malloc(input_len + 1);
    if (!outbuf) return NULL;

    char *dst = outbuf;
    const char *src_end = src + input_len;

    while (src < src_end) {
        if (*src != '\\') {
            *dst++ = *src++;
        } else {
            // Escape sequence
            src++;
            if (src >= src_end) {
                free(outbuf);
                return NULL; // invalid escape at end
            }
            switch (*src) {
                case '"': *dst++ = '"'; src++; break;
                case '\\': *dst++ = '\\'; src++; break;
                case '/': *dst++ = '/'; src++; break;
                case 'b': *dst++ = '\b'; src++; break;
                case 'f': *dst++ = '\f'; src++; break;
                case 'n': *dst++ = '\n'; src++; break;
                case 'r': *dst++ = '\r'; src++; break;
                case 't': *dst++ = '\t'; src++; break;

                case 'u': {
                    // Unicode escape \uXXXX
                    if (src + 5 > src_end) {
                        free(outbuf);
                        return NULL; // not enough chars for \uXXXX
                    }
                    uint16_t code_unit1 = 0;
                    if (!parse_hex4(src + 1, &code_unit1)) {
                        free(outbuf);
                        return NULL; // invalid hex digits
                    }
                    src += 5; // consume uXXXX

                    // Check for surrogate pair
                    if (code_unit1 >= 0xD800 && code_unit1 <= 0xDBFF) {
                        // high surrogate, expect another \uXXXX
                        if (src + 6 <= src_end && src[0] == '\\' && src[1] == 'u') {
                            uint16_t code_unit2 = 0;
                            if (!parse_hex4(src + 2, &code_unit2)) {
                                free(outbuf);
                                return NULL;
                            }
                            if (code_unit2 >= 0xDC00 && code_unit2 <= 0xDFFF) {
                                // valid low surrogate, combine to codepoint
                                uint32_t codepoint = 0x10000 + (((code_unit1 - 0xD800) << 10) | (code_unit2 - 0xDC00));
                                utf8_encode(codepoint, &dst);
                                src += 6; // consume \uXXXX low surrogate
                                break;
                            } else {
                                free(outbuf);
                                return NULL; // invalid low surrogate
                            }
                        } else {
                            free(outbuf);
                            return NULL; // expected low surrogate missing
                        }
                    } else if (code_unit1 >= 0xDC00 && code_unit1 <= 0xDFFF) {
                        free(outbuf);
                        return NULL; // unexpected low surrogate without high surrogate
                    } else {
                        // normal BMP codepoint
                        utf8_encode(code_unit1, &dst);
                    }
                    break;
                }

                default:
                    free(outbuf);
                    return NULL; // invalid escape char
            }
        }
    }
    // decoded length:
    size_t decoded_len = dst - outbuf;

    // Optionally null terminate (not required)
    *dst = '\0';

    if (out_len)
        *out_len = decoded_len;

    return outbuf;
}



char *swap_quotes(char *input, char quote) {
  size_t len = strlen(input);
  char *result = checked_malloc(len + 1);
  if (!result)
    return NULL;

  for (size_t i = 0; i < len; ++i) {
    if (input[i] == '"')
      result[i] = quote;
    else if (input[i] == quote)
      result[i] = '"';
    else
      result[i] = input[i];
  }
  result[len] = '\0';
  return result;
}


char *unquote(char *str, size_t *decoded_len) {
  if (*str == '\0')
    return NULL;

  char quote = str[0];
  char *swapped = NULL;
  char *unescaped = NULL;

  if (quote != '"') {
    swapped = swap_quotes(str, quote);
    if (!swapped)
      return NULL;
    str = swapped;
  }

  unescaped = unquote_json_string(str, decoded_len);
  if (!unescaped) {
    if (swapped)
      free(swapped);
    return NULL;
  }

  if (swapped)
    free(swapped);

  if (quote != '"') {
    char *final = swap_quotes(unescaped, quote);
    free(unescaped);
    return final;
  }

  return unescaped;
}

ParsedValue *parse_string(char*file,Token* token) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_STRING;
  ParsedString *parsedString = checked_malloc(sizeof(ParsedString));
  parsedValue->data = parsedString;
  parsedString->length = 0;
  parsedString->string = unquote(token->value, &parsedString->length);
  return parsedValue;
}