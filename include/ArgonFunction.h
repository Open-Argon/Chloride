#define ARGON_FUNCTION(NAME, ...)                                              \
  ArgonObject *NAME(size_t argc, ArgonObject **argv, ArErr *err,               \
                    RuntimeState *state, ArgonNativeAPI *api) {                \
    (void)argc;                                                                \
    (void)argv;                                                                \
    (void)err;                                                                 \
    (void)state;                                                               \
    (void)api;                                                                 \
    __VA_ARGS__                                                                \
  }
#define EXPOSE_ARGON_FUNCTION(NAME)                                            \
  ArgonObject *NAME(size_t argc, ArgonObject **argv, ArErr *err,               \
                    RuntimeState *state, ArgonNativeAPI *api);