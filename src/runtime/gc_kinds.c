/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// What every managed type points at, for the garbage collector.
//
// Each traverse function must visit exactly the references the type *counts*
// (see the rules in src/gc/README.md). When you add a field that holds an
// ArgonObject or a Stack to any of these structs, add it here too, or the
// collector won't know the object keeps it alive.

#include "gc_kinds.h"
#include "../arobject.h"
#include "../gc/gc.h"
#include "objects/object.h"
#include <stdio.h>

uint8_t ARGON_OBJECT_GC_KIND;
uint8_t STACK_GC_KIND;

// The GCHeader is the first member of every managed struct, so a pointer to
// the struct is a pointer to its header (and NULL stays NULL).
#define VISIT(ptr) visit((GCHeader *)(ptr), ctx)

static void visit_object_array(ArgonObject **items, size_t count,
                               gc_visit_fn visit, void *ctx) {
  for (size_t i = 0; i < count; i++)
    VISIT(items[i]);
}

// ---------------------------------------------------------------------------
// ArgonObject
// ---------------------------------------------------------------------------

static void object_traverse(GCHeader *h, gc_visit_fn visit, void *ctx) {
  ArgonObject *o = (ArgonObject *)h;

  // Every object: its built-in slots (__class__, __base__, __name__, ...)
  // and its attribute dictionary.
  for (size_t i = 0; i < o->built_in_slot_length; i++)
    VISIT(o->built_in_slot[i].value);
  hashmap_GC_traverse(o->dict, visit, ctx);

  // Then whatever its type keeps in `value`.
  switch (o->type) {
  case TYPE_FUNCTION: {
    struct argon_function_struct *fn = o->value.argon_fn;
    if (!fn)
      break;
    VISIT(fn->stack); // the closure's scope
    for (size_t i = 0; i < fn->number_of_default_parameters; i++)
      VISIT(fn->default_parameters[i].value);
    break;
  }
  case TYPE_DICTIONARY:
    hashmap_GC_traverse(o->value.as_hashmap, visit, ctx);
    break;
  case TYPE_ARRAY: {
    darray_armem *a = o->value.as_array;
    if (a && a->data)
      visit_object_array((ArgonObject **)a->data + a->offset, a->size, visit,
                         ctx);
    break;
  }
  case TYPE_TUPLE:
    visit_object_array(o->value.as_tuple.data, o->value.as_tuple.size, visit,
                       ctx);
    break;
  case TYPE_ERROR:
    VISIT(o->value.err.ptr);
    break;
  case TYPE_RANGE_ITERATOR: {
    struct as_range_iterator *r = o->value.as_range_iterator;
    if (r && !r->is_int64) {
      VISIT(r->current.obj);
      VISIT(r->stop.obj);
      VISIT(r->step.obj);
    }
    break;
  }
  case TYPE_ARRAY_ITERATOR:
    if (o->value.as_array_iterator)
      VISIT(o->value.as_array_iterator->source);
    break;
  case TYPE_TUPLE_ITERATOR:
    if (o->value.as_tuple_iterator)
      VISIT(o->value.as_tuple_iterator->source);
    break;
  case TYPE_STRING_ITERATOR:
    if (o->value.as_string_iterator)
      VISIT(o->value.as_string_iterator->source);
    break;
  case TYPE_DICTIONARY_ITERATOR: {
    struct as_dictionary_iterator *d = o->value.as_dictionary_iterator;
    if (d && d->pairs)
      visit_object_array(d->pairs, 2 * d->size, visit, ctx);
    break;
  }
  // Nothing else to visit:
  //   NULL, BOOL, OBJECT, METHOD, SLICE: everything lives in slots / dict
  //   NUMBER, STRING, BUFFER: plain data
  //   NATIVE_FUNCTION: a C function pointer
  default:
    break;
  }
}

static const char *type_name(ArgonType t) {
  switch (t) {
  case TYPE_NULL: return "null";
  case TYPE_BOOL: return "bool";
  case TYPE_NUMBER: return "number";
  case TYPE_STRING: return "string";
  case TYPE_FUNCTION: return "function";
  case TYPE_NATIVE_FUNCTION: return "native function";
  case TYPE_METHOD: return "method";
  case TYPE_DICTIONARY: return "dictionary";
  case TYPE_ARRAY: return "array";
  case TYPE_ERROR: return "error";
  case TYPE_TUPLE: return "tuple";
  case TYPE_BUFFER: return "buffer";
  case TYPE_OBJECT: return "object";
  case TYPE_RANGE_ITERATOR: return "range iterator";
  case TYPE_ARRAY_ITERATOR: return "array iterator";
  case TYPE_STRING_ITERATOR: return "string iterator";
  case TYPE_TUPLE_ITERATOR: return "tuple iterator";
  case TYPE_DICTIONARY_ITERATOR: return "dictionary iterator";
  case TYPE_SLICE: return "slice";
  }
  return "?";
}

// For verifier reports, e.g. `string "hello"`, `function factorial`.
static void object_describe(GCHeader *h, char *buf, size_t len) {
  ArgonObject *o = (ArgonObject *)h;
  if (o->type == TYPE_STRING && o->value.as_str) {
    snprintf(buf, len, "string \"%.*s\"",
             (int)(o->value.as_str->length > 32 ? 32 : o->value.as_str->length),
             o->value.as_str->data);
    return;
  }
  // Don't use get_builtin_field: it can fall back to the dict, and this runs
  // while reporting a possibly broken heap. Only look at the inline slots.
  for (size_t i = 0; i < o->built_in_slot_length; i++) {
    ArgonObject *name = o->built_in_slot[i].value;
    if (o->built_in_slot[i].field == __name__ && name &&
        name->type == TYPE_STRING && name->value.as_str) {
      snprintf(buf, len, "%s %.*s", type_name(o->type),
               (int)name->value.as_str->length, name->value.as_str->data);
      return;
    }
  }
  snprintf(buf, len, "%s", type_name(o->type));
}

static const GCKind object_kind = {
    .name = "ArgonObject",
    .traverse = object_traverse,
    .finalize = NULL, // owned buffers are freed from stage 5 on
    .describe = object_describe,
};

// ---------------------------------------------------------------------------
// Stack (a scope: its variables, plus the enclosing scope)
// ---------------------------------------------------------------------------

static void stack_traverse(GCHeader *h, gc_visit_fn visit, void *ctx) {
  Stack *s = (Stack *)h;
  hashmap_GC_traverse(s->scope, visit, ctx);
  VISIT(s->prev);
}

static const GCKind stack_kind = {
    .name = "Stack",
    .traverse = stack_traverse,
    .finalize = NULL,
    .describe = NULL,
};

void register_gc_kinds(void) {
  ARGON_OBJECT_GC_KIND = gc_register_kind(&object_kind);
  STACK_GC_KIND = gc_register_kind(&stack_kind);
}
