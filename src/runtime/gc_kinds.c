/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// What every managed type points at, and what it owns, for the garbage
// collector.
//
// traverse must visit exactly the references the type *counts* (see the rules
// in src/gc/README.md). When you add a field that holds an ArgonObject or a
// Stack to any of these structs, add it there too, or the collector won't
// know the object keeps it alive.
//
// finalize frees the memory the object owns *by itself*: buffers allocated
// separately from the object's own cell (value structs placed right after the
// object, `(char *)obj + sizeof(ArgonObject)`, are part of the cell and go
// with it). It never decrefs: the collector has already released the
// object's references through traverse by the time finalize runs.

#include "gc_kinds.h"
#include "../arobject.h"
#include "../gc/gc.h"
#include "../memory.h"
#include "objects/object.h"
#include <gmp.h>
#include "../gc/platform.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t ARGON_OBJECT_GC_KIND;
uint8_t STACK_GC_KIND;

// ---------------------------------------------------------------------------
// Error objects (TYPE_ERROR, from the native API's create_err_object)
//
// Natives get a raw ArErr * into one of these (err_object_to_err) and the
// error gets written through it, often by another thread (the threading
// module stores a thread's error there). Writes like that can't be counted,
// so err.ptr isn't a counted reference: instead every live error object is
// listed here, and what they hold is reported as an uncounted root.
// ---------------------------------------------------------------------------

static ArgonObject **error_objects;
static size_t error_object_count, error_object_cap;
static gc_mutex error_objects_lock = GC_MUTEX_INIT;

void gc_track_error_object(ArgonObject *object) {
  gc_mutex_lock(&error_objects_lock);
  if (error_object_count == error_object_cap) {
    error_object_cap = error_object_cap ? error_object_cap * 2 : 16;
    error_objects =
        realloc(error_objects, error_object_cap * sizeof *error_objects);
    if (!error_objects) {
      fprintf(stderr, "panic: unable to allocate memory\n");
      exit(EXIT_FAILURE);
    }
  }
  error_objects[error_object_count++] = object;
  gc_mutex_unlock(&error_objects_lock);
}

static void untrack_error_object(ArgonObject *object) {
  gc_mutex_lock(&error_objects_lock);
  for (size_t i = 0; i < error_object_count; i++)
    if (error_objects[i] == object) {
      error_objects[i] = error_objects[--error_object_count];
      break;
    }
  gc_mutex_unlock(&error_objects_lock);
}

static void trace_error_objects(gc_visit_fn visit, void *ctx) {
  for (size_t i = 0; i < error_object_count; i++)
    visit((GCHeader *)error_objects[i]->value.err.ptr, ctx);
}

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
  hashmap_GC_traverse_from(o->dict, o, visit, ctx);

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
    hashmap_GC_traverse_from(o->value.as_hashmap, o, visit, ctx);
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
    // Not counted: see "Error objects" below.
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

// Free `p` unless it lives inside the object's own cell.
static void free_unless_inline(ArgonObject *o, void *p) {
  char *cell = (char *)o;
  if (p && !((char *)p >= cell && (char *)p < cell + gc_object_size(o)))
    ar_free_owned(p);
}

static void object_finalize(GCHeader *h) {
  ArgonObject *o = (ArgonObject *)h;

  // An attribute dict always belongs to its object (it's never shared).
  hashmap_GC_free(o->dict);

  switch (o->type) {
  case TYPE_STRING:
    if (o->value.as_str)
      free_unless_inline(o, o->value.as_str->data); // the characters
    break;
  case TYPE_NUMBER: {
    struct as_number *n = o->value.as_number;
    if (n && !n->is_int64 && n->n.mpq) { // a big or fractional number
      mpq_clear(*n->n.mpq);              // its limbs
      free_unless_inline(o, n->n.mpq);
    }
    break;
  }
  case TYPE_ARRAY:
    if (o->value.as_array) {
      darray_armem_free_data(o->value.as_array);
      free_unless_inline(o, o->value.as_array);
    }
    break;
  case TYPE_DICTIONARY:
    // Only if it owns the map: a view (see create_dictionary) doesn't.
    if (h->flags & GC_KIND_BIT)
      hashmap_GC_free(o->value.as_hashmap);
    break;
  case TYPE_BUFFER:
    if (o->value.as_buffer)
      free_unless_inline(o, o->value.as_buffer->data);
    break;
  case TYPE_DICTIONARY_ITERATOR:
    if (o->value.as_dictionary_iterator)
      free_unless_inline(o, o->value.as_dictionary_iterator->pairs);
    break;
  case TYPE_ERROR:
    untrack_error_object(o);
    break;
  // Everything else keeps its state inline (functions, tuples, the other
  // iterators) or in slots, or owns nothing (native functions, errors).
  default:
    break;
  }
}

static const GCKind object_kind = {
    .name = "ArgonObject",
    .traverse = object_traverse,
    .finalize = object_finalize,
    .describe = object_describe,
};

// ---------------------------------------------------------------------------
// Stack (a scope: its variables, plus the enclosing scope)
// ---------------------------------------------------------------------------

static void stack_traverse(GCHeader *h, gc_visit_fn visit, void *ctx) {
  Stack *s = (Stack *)h;
  hashmap_GC_traverse_from(s->scope, s, visit, ctx);
  VISIT(s->prev);
}

static void stack_finalize(GCHeader *h) {
  // A scope's variables map is created by (and belongs to) its Stack. Views
  // of it (the dictionary `import` returns) keep the Stack alive instead.
  hashmap_GC_free(((Stack *)h)->scope);
}

static const GCKind stack_kind = {
    .name = "Stack",
    .traverse = stack_traverse,
    .finalize = stack_finalize,
    .describe = NULL,
};

void register_gc_kinds(void) {
  ARGON_OBJECT_GC_KIND = gc_register_kind(&object_kind);
  STACK_GC_KIND = gc_register_kind(&stack_kind);
  gc_add_uncounted_roots(trace_error_objects);
}
