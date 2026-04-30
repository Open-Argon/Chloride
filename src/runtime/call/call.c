/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "call.h"
#include "../../err.h"
#include "../../memory.h"
#include "../api/api.h"
#include "../objects/array/array.h"
#include "../objects/dictionary/dictionary.h"
#include "../objects/exceptions/exceptions.h"
#include "../objects/string/string.h"
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_C_STACK_LIMIT 100

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#include <psapi.h>
#include <windows.h>

double get_memory_usage_mb() {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    return pmc.WorkingSetSize / (1024.0 * 1024.0); // in MB
  }
  return 0.0;
}

#elif defined(__APPLE__)
#include <mach/mach.h>

double get_memory_usage_mb() {
  struct task_basic_info info;
  mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) ==
      KERN_SUCCESS) {
    return info.resident_size / (1024.0 * 1024.0); // in MB
  }
  return 0.0;
}

#elif defined(__linux__)
#include <stdlib.h>
#include <string.h>

double get_memory_usage_mb() {
  FILE *fp = fopen("/proc/self/status", "r");
  if (!fp)
    return 0.0;

  char line[256];
  size_t memory_kb = 0;

  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      sscanf(line + 6, "%zu", &memory_kb);
      break;
    }
  }

  fclose(fp);
  return memory_kb / 1024.0; // Convert KB to MB
}

#else
double get_memory_usage_mb() {
  // Unsupported platform
  return 0.0;
}
#endif

ArgonObject *argon_call(ArgonObject *original_object, size_t argc,
                        ArgonObject **argv, ArgonHashmap *kwargs, ArErr *err,
                        RuntimeState *state) {
  run_call(original_object, argc, argv, kwargs, state, true, err);
  return state->registers[0];
}

void run_call(ArgonObject *original_object, size_t argc, ArgonObject **argv,
              ArgonHashmap *kwargs, RuntimeState *state, bool CStackFrame,
              ArErr *err) {
  ArgonObject *object = original_object;
  if (object->type != TYPE_FUNCTION && object->type != TYPE_NATIVE_FUNCTION &&
      object->type != TYPE_METHOD) {
    ArgonObject *call_method = get_builtin_field_for_class(
        get_builtin_field(object, __class__), __call__, original_object);
    if (call_method) {
      object = call_method;
    }
  }
  ArgonObject *binding_object = NULL;
  int binding_object_exists = 0;
  if (object->type == TYPE_METHOD) {
    binding_object = get_builtin_field(object, __binding__);
    if (binding_object)
      binding_object_exists = 1;
    ArgonObject *function_object = get_builtin_field(object, __function__);
    if (function_object)
      object = function_object;
  }
  switch (object->type) {
  case TYPE_FUNCTION: {
    // ── build a "bound" bitset to track which parameters have been filled ──
    size_t n_params = object->value.argon_fn->number_of_parameters;
    bool *bound = checked_malloc(n_params * sizeof(bool));
    memset(bound, 0, n_params * sizeof(bool));

    // ── bind self / binding_object ────────────────────────────────────────
    Stack *scope = create_scope(object->value.argon_fn->stack, true);
    if (binding_object) {
      struct string_struct key = object->value.argon_fn->parameters[0];
      hashmap_insert_GC(scope->scope, key.hash,
                        new_string_object(key.data, key.length, key.hash),
                        binding_object, 0);
      bound[0] = true;
    }

    // ── bind positional args left to right ────────────────────────────────
    size_t positional_start = binding_object_exists;
    size_t next_positional = positional_start;
    size_t next_default = 0; // tracks position in default_parameters
    for (size_t i = 0; i < argc; i++) {
      if (next_positional < n_params) {
        // bind to a normal parameter slot
        struct string_struct key =
            object->value.argon_fn->parameters[next_positional];
        hashmap_insert_GC(scope->scope, key.hash,
                          new_string_object(key.data, key.length, key.hash),
                          argv[i], 0);
        bound[next_positional] = true;
        next_positional++;
      } else if (next_default <
                 object->value.argon_fn->number_of_default_parameters) {
        // bind to a default parameter slot (overrides the default)
        struct string_struct key =
            object->value.argon_fn->default_parameters[next_default].key;
        hashmap_insert_GC(scope->scope, key.hash,
                          new_string_object(key.data, key.length, key.hash),
                          argv[i], 0);
        next_default++;
      } else {
        // genuinely too many args
        if (object->value.argon_fn->vargs.data == NULL) {
          ArgonObject *type_object_name = get_builtin_field_for_class(
              get_builtin_field(object, __class__), __name__, original_object);
          ArgonObject *object_name =
              get_builtin_field_for_class(object, __name__, original_object);
          *err = create_err(
              TypeError,
              "%.*s %.*s takes %" PRIu64 " argument(s) but too many were given",
              (int)type_object_name->value.as_str->length,
              type_object_name->value.as_str->data,
              (int)object_name->value.as_str->length,
              object_name->value.as_str->data,
              n_params + object->value.argon_fn->number_of_default_parameters);
          free(bound);
          return;
        }
        break;
      }
    }

    // ── bind keyword args ─────────────────────────────────────────────────
    struct hashmap_GC *leftover_kwargs = NULL; // for **kw_arg

    if (kwargs != NULL) {
      size_t kwargs_array_length;
      struct node_GC **kwargs_array =
          hashmap_GC_to_array(kwargs, &kwargs_array_length);

      for (size_t i = 0; i < kwargs_array_length; i++) {
        struct string_struct *name = kwargs_array[i]->key;
        ArgonObject *value = kwargs_array[i]->val;

        // find matching parameter by name
        bool found = false;
        for (size_t j = positional_start; j < n_params; j++) {
          struct string_struct key = object->value.argon_fn->parameters[j];
          if (key.hash == name->hash && key.length == name->length &&
              memcmp(key.data, name->data, name->length) == 0) {
            if (bound[j]) {
              ArgonObject *object_name = get_builtin_field_for_class(
                  object, __name__, original_object);
              *err = create_err(TypeError,
                                "%.*s got multiple values for argument '%.*s'",
                                (int)object_name->value.as_str->length,
                                object_name->value.as_str->data,
                                (int)name->length, name->data);
              free(bound);
              return;
            }
            hashmap_insert_GC(scope->scope, key.hash,
                              new_string_object(key.data, key.length, key.hash),
                              value, 0);
            bound[j] = true;
            found = true;
            break;
          }
        }

        if (!found) {
          // unexpected kwarg — goes to **kw_arg if available, else error
          if (object->value.argon_fn->kwargs.data == NULL) {
            ArgonObject *object_name =
                get_builtin_field_for_class(object, __name__, original_object);
            *err = create_err(
                TypeError, "%.*s got an unexpected keyword argument '%.*s'",
                (int)object_name->value.as_str->length,
                object_name->value.as_str->data, (int)name->length, name->data);
            free(bound);
            return;
          }
          if (leftover_kwargs == NULL)
            leftover_kwargs = createHashmap_GC();
          hashmap_insert_GC(
              leftover_kwargs, name->hash,
              new_string_object(name->data, name->length, name->hash), value,
              0);
        }
      }
    }

    // ── fill defaults for unbound params ──────────────────────────────────
    for (size_t i = 0; i < object->value.argon_fn->number_of_default_parameters;
         i++) {
      struct default_value dv = object->value.argon_fn->default_parameters[i];
      if (hashmap_lookup_GC(scope->scope, dv.key.hash) == NULL) {
        hashmap_insert_GC(
            scope->scope, dv.key.hash,
            new_string_object(dv.key.data, dv.key.length, dv.key.hash),
            dv.value, 0);
      }
    }

    // ── collect leftover positionals into vargs ───────────────────────────
    if (object->value.argon_fn->vargs.data != NULL) {
      // count how many positionals were consumed by normal params
      size_t consumed = 0;
      for (size_t i = positional_start; i < n_params; i++)
        if (bound[i])
          consumed++;
      size_t n_vargs = (argc > consumed) ? argc - consumed : 0;
      ArgonObject *array_obj = new_instance(ARRAY_TYPE, sizeof(darray_armem));
      array_obj->type = TYPE_ARRAY;

      array_obj->value.as_array = darray_armem_create();

      darray_armem_init(array_obj->value.as_array, sizeof(ArgonObject *),
                        n_vargs);
      size_t vi = 0;
      for (size_t i = next_positional; i < argc && vi < n_vargs; i++)
        ((ArgonObject **)array_obj->value.as_array->data)[vi++] = argv[i];

      struct string_struct vkey = object->value.argon_fn->vargs;

      hashmap_insert_GC(scope->scope, vkey.hash,
                        new_string_object(vkey.data, vkey.length, vkey.hash),
                        array_obj, 0);
    }

    // ── bind leftover kwargs to **kw_arg ──────────────────────────────────
    if (object->value.argon_fn->kwargs.data != NULL) {
      if (leftover_kwargs == NULL)
        leftover_kwargs = createHashmap_GC();
      struct string_struct kwkey = object->value.argon_fn->kwargs;
      // leftover_kwargs is your raw hashmap — wrap into dict as needed
      (void)leftover_kwargs; // TODO: wrap into ArgonObject dict
      hashmap_insert_GC(scope->scope, kwkey.hash,
                        new_string_object(kwkey.data, kwkey.length, kwkey.hash),
                        create_dictionary(leftover_kwargs), 0);
    }

    // ── check all required params are bound ───────────────────────────────
    for (size_t i = positional_start; i < n_params; i++) {
      if (!bound[i]) {
        struct string_struct key = object->value.argon_fn->parameters[i];
        ArgonObject *object_name =
            get_builtin_field_for_class(object, __name__, original_object);
        *err = create_err(TypeError, "%.*s missing required argument '%.*s'",
                          (int)object_name->value.as_str->length,
                          object_name->value.as_str->data, (int)key.length,
                          key.data);
        free(bound);
        return;
      }
    }

    free(bound);
    if (CStackFrame) {
      if (state->c_depth >= MAX_C_STACK_LIMIT) {
        *err = create_err(
            InternalError,
            "C stack limit exceeded (this usually indicates a builtin calling "
            "itself indirectly)",
            argc);
        return;
      }
      ArgonObject **registers =
          ar_alloc(object->value.argon_fn->translated.registerCount *
                   sizeof(ArgonObject *));
      runtime(
          (Translated){object->value.argon_fn->translated.registerCount,
                       object->value.argon_fn->translated.registerAssignment,
                       0,
                       0,
                       {-1, 0, 0},
                       {NULL, 0, 0},
                       {NULL, 0, 0},
                       {object->value.argon_fn->bytecode, sizeof(uint8_t),
                        object->value.argon_fn->bytecode_length,
                        object->value.argon_fn->bytecode_length, false},
                       object->value.argon_fn->translated.constants,
                       object->value.argon_fn->translated.path},
          (RuntimeState){registers,
                         0,
                         NULL,
                         NULL,
                         {},
                         {},
                         state->load_number_cache,
                         object->value.argon_fn->translated.path,
                         state->c_depth + 1},
          scope, err);
      state->registers[0] = registers[0];
      return;
    }
    StackFrame *currentStackFrame = ar_alloc(
        sizeof(StackFrame) + object->value.argon_fn->translated.registerCount *
                                 sizeof(ArgonObject *));
    *currentStackFrame = (StackFrame){
        {object->value.argon_fn->translated.registerCount,
         object->value.argon_fn->translated.registerAssignment,
         0,
         0,
         {-1, 0, 0},
         {NULL, 0, 0},
         {NULL, 0, 0},
         {object->value.argon_fn->bytecode, sizeof(uint8_t),
          object->value.argon_fn->bytecode_length,
          object->value.argon_fn->bytecode_length, false},
         object->value.argon_fn->translated.constants,
         object->value.argon_fn->translated.path},
        {(ArgonObject **)((char *)currentStackFrame + sizeof(StackFrame)),
         0,
         NULL,
         state->currentStackFramePointer,
         {},
         {},
         state->load_number_cache,
         object->value.argon_fn->translated.path,
         state->c_depth},
        scope,
        *state->currentStackFramePointer,
        (*state->currentStackFramePointer)->depth + 1,
        {}};
    *state->currentStackFramePointer = currentStackFrame;
    if ((*state->currentStackFramePointer)->depth >= 10000) {
      double logval = log10((double)(*state->currentStackFramePointer)->depth);
      if (floor(logval) == logval) {
        double memoryUsage = get_memory_usage_mb();
        fprintf(stderr,
                "Warning: %s:%" PRIu64 ":%" PRIu64
                " the call stack depth has exceeded %" PRIu64,
                state->path, state->source_location.line,
                state->source_location.column,
                (*state->currentStackFramePointer)->depth);
        if (memoryUsage) {
          fprintf(stderr, ", memory usage at %f MB\n", memoryUsage);
        } else {
          fprintf(stderr, "\n");
        }
      }
    };
    return;
  }
  case TYPE_NATIVE_FUNCTION: {
    if (binding_object) {
      if (argc > 0) {
        ArgonObject **new_call_args =
            ar_alloc(sizeof(ArgonObject *) * (argc + 1));
        new_call_args[0] = binding_object;
        memcpy(new_call_args + 1, argv, argc * sizeof(ArgonObject *));

        argv = new_call_args;
        argc++;
      } else {
        argv = &binding_object;
        argc = 1;
      }
    }
    state->registers[0] =
        object->value.native_fn(argc, argv, kwargs, err, state, &native_api);
    if (KeyboardInterrupted) {
      err->ptr = KeyboardInterrupt_instance;
      KeyboardInterrupted = 0;
    }
    return;
  }
  default: {
    ArgonObject *type_object_name = get_builtin_field_for_class(
        get_builtin_field(original_object, __class__), __name__,
        original_object);
    *err = create_err(TypeError, "'%.*s' object is not callable",
                      (int)type_object_name->value.as_str->length,
                      type_object_name->value.as_str->data);
    return;
  }
  }
}