/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "string.h"
#include "../../lexer/token.h"

#include "../../err.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Helper: Encode a Unicode codepoint as UTF-8, write to *out_ptr, return bytes
// written
static int utf8_encode(uint32_t codepoint, char **out_ptr) {
  char *p = *out_ptr;
  if (codepoint <= 0x7F) {
    *p++ = (char)codepoint;
    *out_ptr = p;
    return 1;
  } else if (codepoint <= 0x7FF) {
    *p++ = (char)(0xC0 | (codepoint >> 6));
    *p++ = (char)(0x80 | (codepoint & 0x3F));
    *out_ptr = p;
    return 2;
  } else if (codepoint <= 0xFFFF) {
    *p++ = (char)(0xE0 | (codepoint >> 12));
    *p++ = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    *p++ = (char)(0x80 | (codepoint & 0x3F));
    *out_ptr = p;
    return 3;
  } else if (codepoint <= 0x10FFFF) {
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
 *  returning a malloc'ed buffer with the decoded string and its length
 * (including embedded nulls).
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
char *unquote(char *input, size_t *out_len, char quote_char, bool is_quoted) {
  if (!input)
    return NULL;

  const char *src = input;
  const char *src_end = NULL;

  if (is_quoted) {
    if (input[0] != quote_char)
      return NULL;

    // Find the closing quote
    const char *p = input + 1;
    const char *end = NULL;
    while (*p) {
      if (*p == quote_char) {
        end = p;
        break;
      }
      if (*p == '\\') {
        p++;
        if (*p == '\0')
          return NULL; // invalid escape at end
      }
      p++;
    }
    if (!end)
      return NULL; // no closing quote

    src = input + 1;
    src_end = end;
  } else {
    // treat whole input as unquoted
    src_end = input + strlen(input);
  }

  size_t input_len = src_end - src;
  char *outbuf = (char *)checked_malloc(input_len + 1);
  if (!outbuf)
    return NULL;

  char *dst = outbuf;

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
      case '\\':
        *dst++ = '\\';
        src++;
        break;
      case '/':
        *dst++ = '/';
        src++;
        break;
      case 'b':
        *dst++ = '\b';
        src++;
        break;
      case 'f':
        *dst++ = '\f';
        src++;
        break;
      case 'n':
        *dst++ = '\n';
        src++;
        break;
      case 'r':
        *dst++ = '\r';
        src++;
        break;
      case 't':
        *dst++ = '\t';
        src++;
        break;
      case 'u': {
        if (src + 5 > src_end) {
          free(outbuf);
          return NULL;
        }
        uint16_t code_unit1 = 0;
        if (!parse_hex4(src + 1, &code_unit1)) {
          free(outbuf);
          return NULL;
        }
        src += 5;

        // handle surrogate pairs
        if (code_unit1 >= 0xD800 && code_unit1 <= 0xDBFF) {
          if (src + 6 <= src_end && src[0] == '\\' && src[1] == 'u') {
            uint16_t code_unit2 = 0;
            if (!parse_hex4(src + 2, &code_unit2)) {
              free(outbuf);
              return NULL;
            }
            if (code_unit2 >= 0xDC00 && code_unit2 <= 0xDFFF) {
              uint32_t codepoint = 0x10000 + (((code_unit1 - 0xD800) << 10) |
                                              (code_unit2 - 0xDC00));
              utf8_encode(codepoint, &dst);
              src += 6;
              break;
            } else {
              free(outbuf);
              return NULL;
            }
          } else {
            free(outbuf);
            return NULL;
          }
        } else if (code_unit1 >= 0xDC00 && code_unit1 <= 0xDFFF) {
          free(outbuf);
          return NULL;
        } else {
          utf8_encode(code_unit1, &dst);
        }
        break;
      }
      default:
        if (is_quoted) {
          *dst++ = quote_char;
          src++;
          break;
        }
        free(outbuf);
        return NULL;
      }
    }
  }

  *dst = '\0';
  if (out_len)
    *out_len = dst - outbuf;

  return outbuf;
}

ParsedValueReturn parse_string(Token *token, bool to_unquote) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_STRING;
  ParsedString *parsedString = checked_malloc(sizeof(ParsedString));
  parsedValue->data = parsedString;
  if (to_unquote) {
    parsedString->string =
        unquote(token->value, &parsedString->length, token->value[0], true);
    if (!parsedString->string) {
      free(parsedValue);
      free(parsedString);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   NULL, InternalError,
                                   "failed to unquote string %s", token->value),
          NULL};
    }
  } else {
    parsedString->string = checked_malloc(token->length);
    memcpy(parsedString->string, token->value, token->length);
    parsedString->length = token->length;
  }
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_string(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedString *parsedString = parsedValue->data;
  if (parsedString->string)
    free(parsedString->string);
  free(parsedString);
}

void free_parsed_template_string(void *ptr) {
  ParsedString *parsedString = ptr;
  if (parsedString->string)
    free(parsedString->string);
}

void free_parsed_value_with_nulls(void *ptr) {
  ParsedValue *parsedValue = (ParsedValue *)ptr;
  if (parsedValue)
    free_parsed(parsedValue);
  free(parsedValue);
}

void free_template_value(void *ptr) {
  TemplateValue *template_value = (TemplateValue *)ptr;
  if (template_value->is_string) {
    free_parsed_template_string(&template_value->value.string);
    return;
  }
  free_parsed_value_with_nulls(template_value->value.value);
  return;
}

ParsedValueReturn parse_template(char *file, DArray *tokens, size_t *index,
                                 ParsedValue *templater) {
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    return (ParsedValueReturn){err, NULL};
  }
  DArray template;
  darray_init(&template, sizeof(TemplateValue));

  Token *token = darray_get(tokens, *index);

  while (token->type != TOKEN_TEMPLATE_END) {
    switch (token->type) {
    case TOKEN_TEMPLATE_TEXT: {
      if (template.size > 0) {
        TemplateValue *value = darray_get(&template, template.size - 1);
        if (value->is_string) {
          size_t length;
          char *unquoted = unquote(token->value, &length, '`', false);

          value->value.string.string = realloc(
              value->value.string.string, value->value.string.length + length);
          memcpy(value->value.string.string + value->value.string.length,
                 unquoted, length);
          value->value.string.length += length;
          break;
        }
      }
      TemplateValue value;
      value.is_string = true;
      value.value.string.string =
          unquote(token->value, &value.value.string.length, '`', false);
      darray_push(&template, &value);
      break;
    }
    case TOKEN_TEMPLATE_EXPR_START: {
      (*index)++;
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        darray_free(&template, free_template_value);
        return (ParsedValueReturn){err, NULL};
      }
      ParsedValueReturn parsed = parse_token(file, tokens, index, true);
      if (is_error(&parsed.err)) {
        darray_free(&template, free_template_value);
        return parsed;
      } else if (!parsed.value) {
        darray_free(&template, free_template_value);
        return (ParsedValueReturn){
            path_specific_create_err(token->line, token->column, token->length,
                                     file, SyntaxError, "expected value"),
            NULL};
      }
      TemplateValue value;
      value.is_string = false;
      value.value.value = parsed.value;
      darray_push(&template, &value);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        return (ParsedValueReturn){err, NULL};
      }
      skip_newlines_and_indents(tokens, index);
      token = darray_get(tokens, *index);

      if (token->type != TOKEN_TEMPLATE_EXPR_END) {
        darray_free(&template, free_template_value);
        return (ParsedValueReturn){
            path_specific_create_err(token->line, token->column, token->length,
                                     file, SyntaxError,
                                     "expected end of template value"),
            NULL};
      }

      break;
    }
    default: {
      darray_free(&template, free_template_value);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError, "unexpected token"),
          NULL};
    }
    }
    (*index)++;
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      darray_free(&template, free_template_value);
      return (ParsedValueReturn){err, NULL};
    }
    token = darray_get(tokens, *index);
  }
  (*index)++;
  ParsedValue *Parsedvalue = checked_malloc(sizeof(ParsedValue));
  Parsedvalue->type = AST_TEMPLATE;
  ParsedTemplate *Parsed_template = checked_malloc(sizeof(ParsedTemplate));
  Parsedvalue->data = Parsed_template;
  Parsed_template->values = template;
  Parsed_template->templater = templater;
  return (ParsedValueReturn){no_err, Parsedvalue};
}

void free_parsed_template(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedTemplate *parsedTemplate = parsedValue->data;
  darray_free(&parsedTemplate->values, free_template_value);
  if (parsedTemplate->templater) {
    free_parsed(parsedTemplate->templater);
    free(parsedTemplate->templater);
  }
  free(parsedTemplate);
}