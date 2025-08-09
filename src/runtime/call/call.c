/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "call.h"
#include "../../hash_data/hash_data.h"
#include "../objects/string/string.h"
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
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
                        ArgonObject **argv, ArErr *err, RuntimeState *state) {
  *err = run_call(original_object, argc, argv, state, true);
  return state->registers[0];
}

ArErr run_call(ArgonObject *original_object, size_t argc, ArgonObject **argv,
               RuntimeState *state, bool CStackFrame) {
  ArgonObject *object = original_object;
  if (object->type != TYPE_FUNCTION && object->type != TYPE_NATIVE_FUNCTION &&
      object->type != TYPE_METHOD) {
    ArgonObject *call_method =
        get_field_for_class(get_field(object, "__class__", false, false),
                            "__call__", original_object);
    if (call_method) {
      object = call_method;
    }
  }
  bool to_free_args = false;
  if (object->type == TYPE_METHOD) {
    ArgonObject *binding_object =
        get_field(object, "__binding__", false, false);
    if (binding_object) {
      ArgonObject **new_call_args = malloc(sizeof(ArgonObject *) * (argc + 1));
      new_call_args[0] = binding_object;
      if (argc > 0) {
        memcpy(new_call_args + 1, argv, argc * sizeof(ArgonObject *));
      }
      argv = new_call_args;
      argc++;
      to_free_args = true;
    }
    ArgonObject *function_object =
        get_field(object, "__function__", false, false);
    if (function_object)
      object = function_object;
  }
  if (object->type == TYPE_FUNCTION) {
    if (argc != object->value.argon_fn.number_of_parameters) {
      ArgonObject *type_object_name =
          get_field_for_class(get_field(object, "__class__", false, false),
                              "__name__", original_object);
      ArgonObject *object_name =
          get_field_for_class(object, "__name__", original_object);
      return create_err(
          state->source_location.line, state->source_location.column,
          state->source_location.length, state->path, "Type Error",
          "%.*s %.*s takes %" PRIu64 " argument(s) but %" PRIu64 " was given",
          (int)type_object_name->value.as_str.length,
          type_object_name->value.as_str.data,
          (int)object_name->value.as_str.length, object_name->value.as_str.data,
          object->value.argon_fn.number_of_parameters, argc);
    }
    Stack *scope = create_scope(object->value.argon_fn.stack);
    for (size_t i = 0; i < argc; i++) {
      struct string_struct key = object->value.argon_fn.parameters[i];
      ArgonObject *value = argv[i];
      uint64_t hash = siphash64_bytes(key.data, key.length, siphash_key);
      hashmap_insert_GC(scope->scope, hash,
                        new_string_object(key.data, key.length), value, 0);
    }
    if (to_free_args)
      free(argv);
    StackFrame new_stackFrame = {
        {object->value.argon_fn.translated.registerCount,
         NULL,
         {object->value.argon_fn.bytecode, sizeof(uint8_t),
          object->value.argon_fn.bytecode_length,
          object->value.argon_fn.bytecode_length, false},
         object->value.argon_fn.translated.constants,
         object->value.argon_fn.translated.path},
        {state->registers,
         0,
         object->value.argon_fn.translated.path,
         NULL,
         state->currentStackFramePointer,
         {},
         {}},
        scope,
        *state->currentStackFramePointer,
        (*state->currentStackFramePointer)->depth + 1};
    if (CStackFrame) {
      return runtime(new_stackFrame.translated, new_stackFrame.state, new_stackFrame.stack);
    } else {
      if (((*state->currentStackFramePointer)->depth + 1) % STACKFRAME_CHUNKS ==
          0) {
        *state->currentStackFramePointer =
            checked_malloc(sizeof(StackFrame) * STACKFRAME_CHUNKS);
      } else {
        *state->currentStackFramePointer = *state->currentStackFramePointer + 1;
      }
      **state->currentStackFramePointer = new_stackFrame;
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
      return no_err;
    }
  } else if (object->type == TYPE_NATIVE_FUNCTION) {
    ArErr err = no_err;
    state->registers[0] = object->value.native_fn(argc, argv, &err, state);
    if (to_free_args)
      free(argv);
    if (err.exists && strlen(err.path) == 0) {
      err.line = state->source_location.line;
      err.column = state->source_location.column;
      err.length = state->source_location.length;
      err.path = state->path;
    }
    return err;
  }
  if (to_free_args)
    free(argv);
  ArgonObject *type_object_name =
      get_field_for_class(get_field(original_object, "__class__", false, false),
                          "__name__", original_object);
  return create_err(state->source_location.line, state->source_location.column,
                    state->source_location.length, state->path, "Type Error",
                    "'%.*s' object is not callable",
                    (int)type_object_name->value.as_str.length,
                    type_object_name->value.as_str.data);
}