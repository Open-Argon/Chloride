/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "Argon.h"
#include "thread.h"
#include <gc/gc.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum thread_state { UNCHANGED, JOINED, DETACHED } allocation_status;

struct thread_arg {
  ArgonObject *target;
  ArgonError *err;
  ArgonNativeAPI *api;
  mt_thread_t thread;
  atomic_int status;
  atomic_int finished;
};

void *thread_fn(void *arg) {
  struct thread_arg *args = (struct thread_arg *)arg;
  ArgonObject *registers;

  ArgonState *state = args->api->new_state(&registers);

  args->api->call(args->target, 0, NULL, args->err, state);
  int status = atomic_load(&args->status);
  atomic_store(&args->finished, 1);
  if (status == DETACHED) {
    printf("free?\n");
    GC_free(arg);
  }
  return NULL;
}

ArgonObject *Argon_Thread(size_t argc, ArgonObject **argv, ArgonError *err,
                          ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err)) {
    return api->ARGON_NULL;
  }

  ArgonObject *GC_managed_args_object =
      api->create_argon_buffer(sizeof(struct thread_arg *));
  struct buffer GC_managed_args_buffer =
      api->argon_buffer_to_buffer(GC_managed_args_object, err);

  struct thread_arg **gc_args_ptr = GC_managed_args_buffer.data;

  struct thread_arg *gc_args =
      GC_MALLOC_UNCOLLECTABLE(sizeof(struct thread_arg));

  *gc_args_ptr = gc_args;

  ArgonError *err_obj = api->err_object_to_err(argv[1], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  gc_args->target = argv[0];
  gc_args->api = api;
  gc_args->err = err_obj;
  atomic_init(&gc_args->status, UNCHANGED);
  atomic_init(&gc_args->finished, 0);
  if (mt_thread_start(&gc_args->thread, thread_fn, gc_args) != 0) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "Failed to create thread");
  }
  return GC_managed_args_object;
}

ArgonObject *Argon_Thread_join(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err)) {
    return api->ARGON_NULL;
  }

  ArgonObject *thread_object = argv[0];

  /* Convert Argon buffer → C buffer */
  struct buffer buf = api->argon_buffer_to_buffer(thread_object, err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  if (buf.size != sizeof(struct thread_arg *)) {
    return api->throw_argon_error(err, "Type Error", "Invalid Thread object");
  }

  struct thread_arg **thread_ptr = buf.data;

  struct thread_arg *thread = *thread_ptr;

  int expected = UNCHANGED;
  if (atomic_compare_exchange_strong(&thread->status, &expected, JOINED)) {
    mt_thread_join(&thread->thread);
    /* Block until thread exits */
    api->set_err(argv[1], err);
    GC_FREE(thread);
    return api->ARGON_NULL;
  }

  return api->throw_argon_error(err, "Runtime Error",
                                "Thread already joined or detached.");
}

ArgonObject *Argon_Thread_detach(size_t argc, ArgonObject **argv,
                                 ArgonError *err, ArgonState *state,
                                 ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }

  ArgonObject *thread_object = argv[0];

  /* Convert Argon buffer → C buffer */
  struct buffer buf = api->argon_buffer_to_buffer(thread_object, err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  if (buf.size != sizeof(struct thread_arg *)) {
    return api->throw_argon_error(err, "Type Error", "Invalid Thread object");
  }

  struct thread_arg **thread_ptr = buf.data;

  struct thread_arg *thread = *thread_ptr;

  if (atomic_load(&thread->finished)) {
    GC_FREE(thread);
    return api->ARGON_NULL;
  }
  int expected = UNCHANGED;
  if (atomic_compare_exchange_strong(&thread->status, &expected, DETACHED)) {
    mt_thread_detach(&thread->thread);
    return api->ARGON_NULL;
  }

  return api->throw_argon_error(err, "Runtime Error",
                                "Thread already joined or detached.");
}

ArgonObject *Argon_Err_object(size_t argc, ArgonObject **argv, ArgonError *err,
                              ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err)) {
    return api->ARGON_NULL;
  }

  /* Free native resources */
  return api->create_err_object();
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  api->register_ArgonObject(
      reg, "Thread", api->create_argon_native_function("Thread", Argon_Thread));
  api->register_ArgonObject(
      reg, "Join",
      api->create_argon_native_function("Join", Argon_Thread_join));
  api->register_ArgonObject(
      reg, "Detach",
      api->create_argon_native_function("Detach", Argon_Thread_detach));
  api->register_ArgonObject(
      reg, "Err_object",
      api->create_argon_native_function("Err_object", Argon_Err_object));
}