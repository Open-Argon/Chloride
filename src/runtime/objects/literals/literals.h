#ifndef RUNTIME_LITERALS_H
#define RUNTIME_LITERALS_H
#include "../object.h"

extern ArgonObject *ARGON_NULL;
extern ArgonObject *ARGON_FALSE;
extern ArgonObject *ARGON_TRUE;

void init_literals();


#endif // RUNTIME_LITERALS_H