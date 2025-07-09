#include "returnTypes.h"

extern const ArErr no_err;

ArErr create_err(int64_t line, int64_t column, int length, char *path, const char *type,
                 const char *fmt, ...);
void output_err(ArErr err);