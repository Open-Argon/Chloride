/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARAM_LIST_H
#define PARAM_LIST_H

#include <stdlib.h>
#include <stdint.h>
#include "../../dynamic_array/darray.h"
#include "../../arobject.h"

typedef struct {
  struct hashmap *seen; // duplicate-detection
  DArray positional;    // char *
  DArray *defaults;     // default_value_parameter, lazy-allocated
  char *v_param;        // *args name, or NULL
  char *kw_param;       // **kwargs name, or NULL
  uint8_t stage;        // 0=positional 1=defaults 2=*args 3=**kwargs
} ParamState;

void param_state_init(ParamState *ps);
void param_state_free(ParamState *ps);

ArErr parse_param_list(char *file, DArray *tokens, size_t *index, ParamState *ps);

#endif // PARAM_LIST_H