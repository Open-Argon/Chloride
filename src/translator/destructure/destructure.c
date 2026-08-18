/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "destructure.h"
#include "../../hash_data/hash_data.h"
#include "../../memory.h"
#include "../../parser/assignable/identifier/identifier.h"
#include "../translator.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

size_t translate_destructure(Translated *translated, Destructure *destructure,
                             uint8_t value_register, ArErr *err,
                             OperationType opcode) {
  if (!destructure) {
    return translated->bytecode.size;
  }

  switch (destructure->type) {
  case DESTRUCTURE_IDENTIFIER: {
    char *name = destructure->identifier.name;
    size_t length = strlen(name);
    push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, destructure->identifier.line);
    push_instruction_code(translated, destructure->identifier.column);
    push_instruction_code(translated, length);

    size_t identifier_pos = arena_push(&translated->constants, name, length);

    size_t first = push_instruction_byte(translated, opcode);

    push_instruction_code(translated, length);

    push_instruction_code(translated, identifier_pos);

    push_instruction_code(translated,
                          siphash64_bytes(name, length, siphash_key_fixed));

    push_instruction_byte(translated, value_register);

    return first;
  }

  case DESTRUCTURE_INDEX: {
    size_t first = translated->bytecode.size;
    if (destructure->index.length) {
      uint8_t iterator_or_error_register = translated->registerAssignment++;
      uint8_t next_register = translated->registerAssignment++;
      uint8_t counter_register = translated->registerAssignment++;

      if (value_register != 0) {
        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, value_register);
        push_instruction_byte(translated, 0);
      }

      set_registers(translated, translated->registerAssignment);
      push_instruction_byte(translated, OP_LOAD_ITER_METHOD);
      push_instruction_byte(translated, OP_INIT_CALL);
      push_instruction_code(translated, 0);
      push_instruction_byte(translated, OP_CALL);
      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, 0);
      push_instruction_byte(translated, iterator_or_error_register);
      push_instruction_byte(translated, OP_LOAD_NEXT_METHOD);
      if (destructure->index.length > 1 || destructure->index.rest) {
        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, 0);
        push_instruction_byte(translated, next_register);
      }
      translated->exception_handler_depth++;
      push_instruction_byte(translated, OP_EXCEPTION_CATCHER_PUSH);
      uint64_t jump_index = push_instruction_code(translated, 0);

      for (size_t i = 0; i < destructure->index.length; i++) {
        push_instruction_byte(translated, OP_LOAD_NUMBER);
        push_instruction_byte(translated, counter_register);
        push_instruction_byte(translated, 1);
        push_instruction_code(translated, i);

        if (i != 0) {
          push_instruction_byte(translated, OP_COPY_TO_REGISTER);
          push_instruction_byte(translated, next_register);
          push_instruction_byte(translated, 0);
        }

        push_instruction_byte(translated, OP_INIT_CALL);
        push_instruction_code(translated, 0);

        push_instruction_byte(translated, OP_CALL);
        if (destructure->index.items[i]) {
          translate_destructure(translated, destructure->index.items[i], 0, err,
                                opcode);
        }
      }

      if (destructure->index.rest) {
        push_instruction_byte(translated, OP_LOAD_CREATE_ARRAY);
        push_instruction_byte(translated, OP_INIT_CALL);
        push_instruction_code(translated, 0);
        push_instruction_byte(translated, OP_UNPACK_ITERATOR);
        push_instruction_byte(translated, iterator_or_error_register);
        push_instruction_byte(translated, next_register);
        push_instruction_byte(translated, OP_CALL);

        translate_destructure(translated, destructure->index.rest, 0, err,
                              opcode);
      }

      translated->exception_handler_depth--;
      push_instruction_byte(translated, OP_EXCEPTION_CATCHER_POP);

      push_instruction_byte(translated, OP_JUMP);
      size_t skip_exception_handler_pos = push_instruction_code(translated, 0);

      set_instruction_code(translated, jump_index, translated->bytecode.size);

      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, 0);
      push_instruction_byte(translated, iterator_or_error_register);

      push_instruction_byte(translated, OP_LOAD_IS_INSTANCE_FUNCTION);

      push_instruction_byte(translated, OP_INIT_CALL);
      push_instruction_code(translated, 2);

      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, iterator_or_error_register);
      push_instruction_byte(translated, 0);

      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, 0);

      push_instruction_byte(translated, OP_LOAD_STOPITERATION_CLASS);

      push_instruction_byte(translated, OP_INSERT_ARG);
      push_instruction_code(translated, 1);

      push_instruction_byte(translated, OP_CALL);
      push_instruction_byte(translated, OP_NOT);

      push_instruction_byte(translated, OP_JUMP_IF_FALSE);
      push_instruction_byte(translated, 0);

      size_t is_stop_iteration = push_instruction_code(translated, 0);

      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, iterator_or_error_register);
      push_instruction_byte(translated, 0);

      push_instruction_byte(translated, OP_QUIET_THROW);

      set_instruction_code(translated, is_stop_iteration,
                           translated->bytecode.size);

      push_instruction_byte(translated, OP_DESTRUCTURE_ERROR);
      push_instruction_code(translated, destructure->index.length);
      push_instruction_byte(translated, counter_register);

      set_instruction_code(translated, skip_exception_handler_pos,
                           translated->bytecode.size);

      translated->registerAssignment -= 3;
    }
    return first;
  }

  case DESTRUCTURE_KEY: {
    size_t first = translated->bytecode.size;

    if (destructure->key.length || destructure->key.rest) {
      /*
       * Hold the source dictionary in its own scratch register, since
       * register 0 gets clobbered by every __getitem__ call (and, for
       * *rest, by the dictionary(...) copy + __delitem__ calls too).
       */
      uint8_t dictionary_register = translated->registerAssignment++;
      set_registers(translated, translated->registerAssignment);

      push_instruction_byte(translated, OP_COPY_TO_REGISTER);
      push_instruction_byte(translated, value_register);
      push_instruction_byte(translated, dictionary_register);

      /*
       * If there's a *rest, each matched key's value is needed again later
       * to __delitem__ it off the rest copy. Evaluating the key expression
       * twice would re-run any side effects it has (e.g. {foo(): x}), so
       * when rest is present we cache each key's evaluated value in its own
       * scratch register the first time and reuse that register instead of
       * re-emitting translate_parsed.
       */
      uint8_t *key_registers = NULL;
      if (destructure->key.rest && destructure->key.length) {
        key_registers =
            checked_malloc(sizeof(uint8_t) * destructure->key.length);
      }

      for (size_t i = 0; i < destructure->key.length; i++) {
        DestructureKeyItem *item = &destructure->key.items[i];

        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, dictionary_register);
        push_instruction_byte(translated, 0);

        push_instruction_byte(translated, OP_LOAD_GETITEM_METHOD);

        push_instruction_byte(translated, OP_INIT_CALL);
        push_instruction_code(translated, 1);

        if (item->key->type == AST_IDENTIFIER) {
          /*
           * Shorthand: {name} is equivalent to {"name": name}, so the key
           * is the literal string "name", not a lookup of a variable
           * called `name`.
           */
          char *key_name = ((ParsedIdentifier *)item->key->data)->name;
          size_t key_name_length = strlen(key_name);

          size_t key_string_pos =
              arena_push(&translated->constants, key_name, key_name_length);

          set_registers(translated, 1);
          push_instruction_byte(translated, OP_LOAD_STRING);
          push_instruction_byte(translated, 0);
          push_instruction_code(translated, key_name_length);
          push_instruction_code(translated, key_string_pos);
          push_instruction_code(
              translated,
              siphash64_bytes(key_name, key_name_length, siphash_key_fixed));
        } else {
          translate_parsed(translated, item->key, err);
          if (is_error(err)) {
            if (key_registers) {
              free(key_registers);
            }
            return first;
          }
        }

        if (key_registers) {
          uint8_t key_register = translated->registerAssignment++;
          set_registers(translated, translated->registerAssignment);
          key_registers[i] = key_register;

          push_instruction_byte(translated, OP_COPY_TO_REGISTER);
          push_instruction_byte(translated, 0);
          push_instruction_byte(translated, key_register);

          push_instruction_byte(translated, OP_COPY_TO_REGISTER);
          push_instruction_byte(translated, key_register);
          push_instruction_byte(translated, 0);
        }

        push_instruction_byte(translated, OP_INSERT_ARG);
        push_instruction_code(translated, 0);

        push_instruction_byte(translated, OP_CALL);

        translate_destructure(translated, item->destructure, 0, err, opcode);
        if (is_error(err)) {
          if (key_registers) {
            free(key_registers);
          }
          return first;
        }
      }

      if (destructure->key.rest) {
        /*
         * rest = dictionary(source); for each matched key, del rest[key]
         */
        uint8_t rest_register = translated->registerAssignment++;
        set_registers(translated, translated->registerAssignment);

        push_instruction_byte(translated, OP_LOAD_DICTIONARY_CLASS);
        push_instruction_byte(translated, OP_INIT_CALL);
        push_instruction_code(translated, 1);

        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, dictionary_register);
        push_instruction_byte(translated, 0);

        push_instruction_byte(translated, OP_INSERT_ARG);
        push_instruction_code(translated, 0);

        push_instruction_byte(translated, OP_CALL);

        push_instruction_byte(translated, OP_COPY_TO_REGISTER);
        push_instruction_byte(translated, 0);
        push_instruction_byte(translated, rest_register);

        for (size_t i = 0; i < destructure->key.length; i++) {
          push_instruction_byte(translated, OP_COPY_TO_REGISTER);
          push_instruction_byte(translated, rest_register);
          push_instruction_byte(translated, 0);

          push_instruction_byte(translated, OP_LOAD_DELITEM_METHOD);

          push_instruction_byte(translated, OP_INIT_CALL);
          push_instruction_code(translated, 1);

          push_instruction_byte(translated, OP_COPY_TO_REGISTER);
          push_instruction_byte(translated, key_registers[i]);
          push_instruction_byte(translated, 0);

          push_instruction_byte(translated, OP_INSERT_ARG);
          push_instruction_code(translated, 0);

          push_instruction_byte(translated, OP_CALL);
        }

        translate_destructure(translated, destructure->key.rest, rest_register,
                              err, opcode);
        if (is_error(err)) {
          if (key_registers) {
            translated->registerAssignment -= destructure->key.length;
            free(key_registers);
          }
          translated->registerAssignment--;
          return first;
        }

        translated->registerAssignment--;
      }

      if (key_registers) {
        translated->registerAssignment -= destructure->key.length;
        free(key_registers);
      }

      translated->registerAssignment--;
    }

    return first;
  }
  }
  return 0;
}