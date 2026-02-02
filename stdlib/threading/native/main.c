/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "Argon.h"
#include "thread.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

struct thread_arg {
  ArgonObject *target;
  ArgonError *err;
  ArgonNativeAPI *api;
  mt_thread_t thread;
};

ArgonNativeAPI *api;

void *thread_fn(void *arg) {
  struct thread_arg args = *((struct thread_arg *)arg);
  ArgonObject *registers;

  ArgonState *state = api->new_state(&registers);

  api->call(args.target, 0, NULL, args.err, state);

  atomic_store(&args.thread.finished, 1);
  return NULL;
}

ArgonObject *Argon_Init(size_t argc, ArgonObject **argv, ArgonError *err,
                        ArgonState *state, ArgonNativeAPI *api_passed) {
  if (api_passed->fix_to_arg_size(0, argc, err)) {
    return api_passed->ARGON_NULL;
  }
  api = api_passed;
  return api_passed->ARGON_NULL;
}

ArgonObject *Argon_Thread(size_t argc, ArgonObject **argv, ArgonError *err,
                          ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err)) {
    return api->ARGON_NULL;
  }

  ArgonObject *GC_managed_args_object =
      api->create_argon_buffer(sizeof(struct thread_arg));
  struct buffer GC_managed_args_buffer =
      api->argon_buffer_to_buffer(GC_managed_args_object, err);

  struct thread_arg *gc_args = GC_managed_args_buffer.data;

  ArgonError *err_obj = api->err_object_to_err(argv[1], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  gc_args->target = argv[0];
  gc_args->api = api;
  gc_args->err = err_obj;

  mt_thread_init(&gc_args->thread);
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

  if (buf.size != sizeof(struct thread_arg)) {
    return api->throw_argon_error(err, "Type Error", "Invalid Thread object");
  }

  struct thread_arg *thread = buf.data;

  if (thread == NULL) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "Thread already joined");
  }

  /* Block until thread exits */
  mt_thread_join(&thread->thread, NULL);

  api->set_err(argv[1], err);

  /* Free native resources */
  return api->ARGON_NULL;
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
      reg, "Init", api->create_argon_native_function("Init", Argon_Init));
  api->register_ArgonObject(
      reg, "Thread", api->create_argon_native_function("Thread", Argon_Thread));
  api->register_ArgonObject(
      reg, "Join",
      api->create_argon_native_function("Join", Argon_Thread_join));
  api->register_ArgonObject(
      reg, "Err_object",
      api->create_argon_native_function("Err_object", Argon_Err_object));
}