/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "import.h"
#include "../../import.h"
#include "../../../external/cwalk/include/cwalk.h"
#include "../api/api.h"
#include "../objects/dictionary/dictionary.h"
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

void runtime_import(Translated *translated, RuntimeState *state,
                    struct Stack *stack, ArErr *err) {
  struct string path = native_api.argon_to_string(state->registers[0], err);
  if (native_api.is_error(err))
    return;
  char path_c[PATH_MAX];
  if (path.length >= sizeof(path_c)) {
    native_api.throw_argon_error(err, "Import Error", "path is too big");
    return;
  }
  memcpy(path_c, path.data, path.length);
  path_c[path.length] = '\0';
  memcpy(path_c, path.data, path.length);
  path_c[path.length] = '\0';
  size_t current_directory_length;
  cwk_path_get_dirname(state->path, &current_directory_length);
  char current_directory[PATH_MAX];
  if (current_directory_length >= sizeof(current_directory)) {
    native_api.throw_argon_error(err, "Import Error", "current directory is too big");
    return;
  }
  memcpy(current_directory, state->path, current_directory_length);
  current_directory[current_directory_length]='\0';
  
  Stack *result = ar_import(current_directory, path_c, err, false);
  if (!result)
    return;
  state->registers[0] = create_dictionary(result->scope);
}