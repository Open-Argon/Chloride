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
#include <stdio.h>
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

void run_call(Translated *translated, RuntimeState *state) {
  uint8_t from_register = pop_byte(translated, state);
  uint8_t source_location_index = pop_bytecode(translated, state);
  ArgonObject *object = state->registers[from_register];
  if (object->type == TYPE_FUNCTION) {
    Stack *scope = create_scope(object->value.argon_fn.stack);
    for (size_t i = 0; i < *state->call_args_length; i++) {
      struct string_struct key = object->value.argon_fn.parameters[i];
      ArgonObject *value = (*state->call_args)[i];
      uint64_t hash = siphash64_bytes(key.data, key.length, siphash_key);
      hashmap_insert_GC(scope->scope, hash,
                        new_string_object(key.data, key.length), value, 0);
    }
    DArray bytecode_darray = {object->value.argon_fn.bytecode, sizeof(uint8_t),
                              object->value.argon_fn.bytecode_length,
                              object->value.argon_fn.bytecode_length, false};
    StackFrame new_stackFrame = {
        (Translated){translated->registerCount, NULL, bytecode_darray,
                     translated->source_locations, translated->constants,
                     object->value.argon_fn.path},
        (RuntimeState){state->registers, 0, state->path,
                       state->currentStackFramePointer, state->call_args,
                       state->call_args_length},
        scope,
        *state->currentStackFramePointer,
        no_err,
        (*state->currentStackFramePointer)->depth + 1};
    if (((*state->currentStackFramePointer)->depth+1) % STACKFRAME_CHUNKS == 0) {
      *state->currentStackFramePointer = checked_malloc(sizeof(StackFrame) * STACKFRAME_CHUNKS);
    } else {
      *state->currentStackFramePointer = *state->currentStackFramePointer + 1;
    }
      **state->currentStackFramePointer = new_stackFrame;
    if ((*state->currentStackFramePointer)->depth >= 10000) {
      double logval = log10((double)(*state->currentStackFramePointer)->depth);
      if (floor(logval) == logval) {
        SourceLocation *source_location =
            darray_get(&translated->source_locations, source_location_index);
        double memoryUsage = get_memory_usage_mb();
        fprintf(stderr,
                "Warning: %s:%" PRIu64 ":%" PRIu64
                " the call stack depth has exceeded %" PRIu64,
                state->path, source_location->line, source_location->column,
                (*state->currentStackFramePointer)->depth);
        if (memoryUsage) {
          fprintf(stderr, ", memory usage at %f MB\n", memoryUsage);

        } else {
          fprintf(stderr, "\n");
        }
      }
    };
  }
}