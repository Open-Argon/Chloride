/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "call.h"
#include "../../hash_data/hash_data.h"
#include "../api/api.h"
#include "../objects/string/string.h"
#include "../../err.h"
#include "../../memory.h"
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#include <windows.h>
#include <psapi.h>

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
                        ArgonObject **argv, ArErr *err, RuntimeState *state) {
  run_call(original_object, argc, argv, state, true, err);
  return state->registers[0];
}

void run_call(ArgonObject *original_object, size_t argc, ArgonObject **argv,
              RuntimeState *state, bool CStackFrame, ArErr *err) {
  ArgonObject *object = original_object;
  if (object->type != TYPE_FUNCTION && object->type != TYPE_NATIVE_FUNCTION &&
      object->type != TYPE_METHOD) {
    ArgonObject *call_method = get_builtin_field_for_class(
        get_builtin_field(object, __class__), __call__, original_object);
    if (call_method) {
      object = call_method;
    }
  }
  if (object->type == TYPE_METHOD) {
    ArgonObject *binding_object = get_builtin_field(object, __binding__);
    if (binding_object) {
      ArgonObject **new_call_args =
          ar_alloc(sizeof(ArgonObject *) * (argc + 1));
      new_call_args[0] = binding_object;
      if (argc > 0) {
        memcpy(new_call_args + 1, argv, argc * sizeof(ArgonObject *));
      }
      argv = new_call_args;
      argc++;
    }
    ArgonObject *function_object = get_builtin_field(object, __function__);
    if (function_object)
      object = function_object;
  }
  if (object->type == TYPE_FUNCTION) {
    if (argc != object->value.argon_fn->number_of_parameters) {
      ArgonObject *type_object_name = get_builtin_field_for_class(
          get_builtin_field(object, __class__), __name__, original_object);
      ArgonObject *object_name =
          get_builtin_field_for_class(object, __name__, original_object);
      *err = create_err(
          state->source_location.line, state->source_location.column,
          state->source_location.length, state->path, "Type Error",
          "%.*s %.*s takes %" PRIu64 " argument(s) but %" PRIu64 " was given",
          (int)type_object_name->value.as_str->length,
          type_object_name->value.as_str->data,
          (int)object_name->value.as_str->length,
          object_name->value.as_str->data,
          object->value.argon_fn->number_of_parameters, argc);
      return;
    }
    Stack *scope = create_scope(object->value.argon_fn->stack, true);
    for (size_t i = 0; i < argc; i++) {
      struct string_struct key = object->value.argon_fn->parameters[i];
      ArgonObject *value = argv[i];
      uint64_t hash = siphash64_bytes(key.data, key.length, siphash_key);
      hashmap_insert_GC(scope->scope, hash,
                        new_string_object(key.data, key.length, 0, hash), value,
                        0);
    }
    if (CStackFrame) {
      ArgonObject
          *registers[MAX_REGISTERS]; // fixed on the stack for speed purposes
      StackFrame new_stackFrame = {
          {object->value.argon_fn->translated.registerCount,
           object->value.argon_fn->translated.registerAssignment,
           NULL,
           {object->value.argon_fn->bytecode, sizeof(uint8_t),
            object->value.argon_fn->bytecode_length,
            object->value.argon_fn->bytecode_length, false},
           object->value.argon_fn->translated.constants,
           object->value.argon_fn->translated.path},
          {registers,
           0,
           object->value.argon_fn->translated.path,
           NULL,
           state->currentStackFramePointer,
           {},
           {},
           state->load_number_cache},
          scope,
          *state->currentStackFramePointer,
          (*state->currentStackFramePointer)->depth + 1};
      for (size_t i = 0; i < new_stackFrame.translated.registerCount; i++) {
        new_stackFrame.state.registers[i] = NULL;
      }
      runtime(new_stackFrame.translated, new_stackFrame.state,
              new_stackFrame.stack, err);
      state->registers[0] = new_stackFrame.state.registers[0];
      return;
    } else {
      StackFrame *currentStackFrame =
          ar_alloc(sizeof(StackFrame) +
                   object->value.argon_fn->translated.registerCount *
                       sizeof(ArgonObject *));
      *currentStackFrame = (StackFrame){
          {object->value.argon_fn->translated.registerCount,
           object->value.argon_fn->translated.registerAssignment,
           NULL,
           {object->value.argon_fn->bytecode, sizeof(uint8_t),
            object->value.argon_fn->bytecode_length,
            object->value.argon_fn->bytecode_length, false},
           object->value.argon_fn->translated.constants,
           object->value.argon_fn->translated.path},
          {(ArgonObject **)((char *)currentStackFrame + sizeof(StackFrame)),
           0,
           object->value.argon_fn->translated.path,
           NULL,
           state->currentStackFramePointer,
           {},
           {},
           state->load_number_cache},
          scope,
          *state->currentStackFramePointer,
          (*state->currentStackFramePointer)->depth + 1};
      for (size_t i = 0; i < (*currentStackFrame).translated.registerCount;
           i++) {
        (*currentStackFrame).state.registers[i] = NULL;
      }
      *state->currentStackFramePointer = currentStackFrame;
      if ((*state->currentStackFramePointer)->depth >= 10000) {
        double logval =
            log10((double)(*state->currentStackFramePointer)->depth);
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
  } else if (object->type == TYPE_NATIVE_FUNCTION) {
    state->registers[0] =
        object->value.native_fn(argc, argv, err, state, &native_api);
    if (err->exists && strlen(err->path) == 0) {
      err->line = state->source_location.line;
      err->column = state->source_location.column;
      err->length = state->source_location.length;
      strcpy(err->path, state->path);
    }
    return;
  }
  ArgonObject *type_object_name = get_builtin_field_for_class(
      get_builtin_field(original_object, __class__), __name__, original_object);
  *err = create_err(state->source_location.line, state->source_location.column,
                    state->source_location.length, state->path, "Type Error",
                    "'%.*s' object is not callable",
                    (int)type_object_name->value.as_str->length,
                    type_object_name->value.as_str->data);
}