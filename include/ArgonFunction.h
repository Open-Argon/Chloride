#ifndef Argon_NATIVE_FUNCTION_H
#define Argon_NATIVE_FUNCTION_H

#define ARGON_FUNCTION_ARGS size_t argc, ArgonObject **argv, ArgonHashmap *kwargs,     \
                    ArErr *err, RuntimeState *state, ArgonNativeAPI *api

#define ARGON_FUNCTION(NAME, ...)                                              \
  ArgonObject *NAME(ARGON_FUNCTION_ARGS) {    \
    (void)argc;                                                                \
    (void)argv;                                                                \
    (void)kwargs;                                                              \
    (void)err;                                                                 \
    (void)state;                                                               \
    (void)api;                                                                 \
    __VA_ARGS__                                                                \
  }

#define EXPOSE_ARGON_FUNCTION(NAME)                                            \
  ArgonObject *NAME(ARGON_FUNCTION_ARGS);

#endif // Argon_NATIVE_FUNCTION_H