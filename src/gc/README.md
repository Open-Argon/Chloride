<!--
SPDX-FileCopyrightText: 2026 William Bell

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Chloride's garbage collector

This directory holds the memory management backends. The rest of the
interpreter only talks to them through [`../memory.h`](../memory.h).

| build | backend | file |
|---|---|---|
| `make` | Boehm (the default, and what ships) | `gc_boehm.c` |
| `make GC=custom` | our own collector (work in progress), built as `bin/argon-customgc` | `gc_custom.c`, `heap.c`, … |
| `make gc-test` | standalone tests for the collector's pieces (with ASan) | `tests/` |

The custom collector is **non-moving reference counting**, with a
**stop-the-world trial-deletion cycle collector** (the approach CPython uses)
to reclaim the cycles that refcounting can't. This document explains how it
works, the rules interpreter code has to follow, and how far along it is.

---

## 1. The ideas, from the ground up

### Reference counting

Every managed object carries a count of how many references to it are
*stored* somewhere. Storing a pointer increments the count (`incref`), and
overwriting or dropping one decrements it (`decref`). At zero, nothing can
reach the object any more, so it's freed and its children are decref'd in
turn.

Good: memory comes back promptly, and there are no big pauses.
Bad: every store has to be counted exactly, and **cycles never reach zero**.

### Cycles, and why Argon has so many

```
function ──(.stack)──▶ Stack ──(.scope)──▶ hashmap ──("f")──▶ function
```

Any function defined in a scope that it captures forms a cycle. So do classes
and their methods, objects that refer to themselves, and so on. Refcounting
alone would leak every one of them.

### Trial deletion: finding cycles without looking at the C stack

Take a set of candidate objects (normally all of them).

1. Copy each candidate's refcount into a scratch field, `gc_refs`.
2. For every reference *from* one candidate *to* another, subtract 1 from the
   target's `gc_refs`.
3. Whatever still has `gc_refs > 0` is referenced from **outside** the
   candidate set (a VM register, a frame, a global…). Those are alive, and so
   is everything they can reach.
4. Everything else is referenced only from other garbage: it's an unreachable
   cycle. Free it.

This never needs to know where the roots are, which is why we don't need to
scan C stacks. The price is that it **trusts the refcounts completely**:

* a count that is **too low** (missing incref) makes a live object look like
  garbage, so it gets freed while still in use → use-after-free.
* a count that is **too high** (missing decref) makes garbage look externally
  referenced, so it is **never** freed, not even by the cycle collector.

So both mistakes are bugs. The debug verifier (section 4) exists to catch them.

### Deferred refcounting: not counting C locals

Textbook refcounting (CPython's) also counts every pointer held in a C local
variable. In Chloride that would mean editing almost every function in the
interpreter and the stdlib. Instead we use **deferred reference counting**
(Deutsch & Bobrow, 1976):

* Pointers in **C locals are not counted**. Everything stored in the heap,
  VM registers, frames and call argument lists **is**.
* When a count hits zero the object isn't freed immediately. It goes into the
  **zero count table (ZCT)**, and so does every newly allocated object (which
  starts at zero).
* The ZCT is processed at **safe points**: places where we *know* no C local
  holds an uncounted pointer that still matters. Anything still at zero then
  is truly dead.

Our safe point is the **top of the bytecode loop in `runtime()`, between two
instructions**. At that moment the only live values belong to the VM, and the
VM's holders are all counted.

### Birth numbers: safe points inside nested calls

There's a catch. Natives call back into Argon (`array.map` calls your lambda,
`sort` calls the comparator…). That starts a **nested** `runtime()`, and while
it runs, the native that started it is suspended with live C locals. A safe
point in the nested loop must not free anything those locals point at.

Every object gets a **birth number** (a global allocation counter), and every
`runtime()` invocation records the counter when it starts (its *epoch*).

> At a safe point, only objects born **after the current invocation began**
> may be freed: ZCT entries and cycle-collection candidates alike.

A suspended native only ever got its pointers *before* it called back into
Argon, so it can't hold anything born afterwards. Older objects that die
during a nested call simply stay in the ZCT until control returns to a
shallower safe point where they become eligible. At the outermost loop the
epoch is the start of the program, so everything is eligible.

### Immortal objects

Built-in types, `null`/`true`/`false`, cached small ints and chars, and
everything created while bootstrapping is **immortal**: incref and decref
ignore it and it's never freed. Static objects (`small_ints`, `empty_str`, …)
aren't on the heap at all, so they *must* be immortal. Immortality is the
default: the allocator sets a `MORTAL` flag, and a zero-initialised static
object therefore never has it.

### Non-moving

Objects never change address once allocated. C code can keep raw
`ArgonObject *` pointers freely, with no handles and no pointer fix-ups.

---

## 2. The rules for interpreter code

These are what the conversion work is about.

1. **Counted holders.** Any store of an object pointer into one of these must
   incref the new value and decref the old one:
   object fields and built-in slots, hashmaps, arrays, tuples, function
   structs (closure stack, default values), `Stack` scopes, VM registers,
   `StackFrame`s, `call_instance` argument lists and kwargs, `ErrorCatch`
   entries, iterator state.
2. **C locals are free.** A pointer in a local variable or argument needs no
   bookkeeping, *as long as it doesn't survive past a safe point that could
   free it*. The birth rule makes that automatic for natives that call back
   into Argon.
3. **Globals.** A C global (or `static`) that points at an object must hold
   an immortal object. Call `gc_immortalize(obj)` when storing one after
   bootstrap.
4. **Memory outside the GC.** Never keep the only reference to an object in
   `malloc`'d memory without counting it (the same rule Boehm already has).
5. **Holders must be traversable.** Every managed type has a `traverse`
   function listing its outgoing references. A store into a structure the
   collector can't traverse is invisible to it.
6. **Internal buffers are owned, not shared.** An array's element buffer, a
   hashmap's buckets, a string's bytes and so on belong to exactly one managed
   object and are freed with it (plain `malloc`/`free`). They are not
   refcounted themselves.

---

## 3. How the pieces fit

```
memory.h        public API used by the interpreter (both backends)
gc/gc.h         the API: GCHeader, kinds, incref/decref/gc_assign, epochs,
                safe points, verifier (works in both builds)
gc/heap.c       page allocator: size classes, free lists, page table
gc/gc.c         refcounts, ZCT, safe points, epochs, trial deletion
gc/verify.c     debug verifier: recounts every reference and compares
gc/gc_custom.c  memory.h for GC=custom; for now untyped ar_alloc is
                plain calloc and is never freed (shrinks as stage 4 goes on)
gc/gc_boehm.c   Boehm backend (incref/decref compile to nothing)
gc/tests/       standalone tests, run by `make gc-test`: heap_test.c, and
                gc_test.c, which drives the core with a toy managed type and
                checks a randomized object graph against a shadow model
```

### The heap (`heap.c`)

Managed objects come from 64 KiB pages, each holding cells of a single size
class (16 B up to 16 KiB, with larger objects getting their own run of pages).
Every page has an *allocated* bitmap, which lets the collector walk every live
object without an extra list link per object. A page table maps any address
to its page, which the debug checks use to ask "is this a live object?".

### An object's header

Every managed object starts with a `GCHeader`:

| field | purpose |
|---|---|
| `refcount` | counted references |
| `gc_refs` | scratch space for trial deletion |
| `kind` | which managed type this is (selects `traverse` / `dealloc`) |
| `flags` | `MORTAL`, `IN_ZCT`, `DYING`, … |
| `birth` | allocation number, for the epoch rule |

### Freeing an object

`dealloc(kind)` decrefs each child the object holds (children reaching zero
join the ZCT), frees the object's owned buffers, then returns the cell to its
page's free list.

### A safe point

```
if (zct is big enough || cycle collection is due)
    gc_safe_point()
        1. process ZCT: free entries with refcount 0 and birth >= epoch
        2. maybe run trial deletion over objects with birth >= epoch
```

---

## 4. Debugging tools

| env var | effect |
|---|---|
| `ARGON_GC_STATS=1` | print every collection and a summary at exit |
| `ARGON_GC_STRESS=N` | run a full safe point every N allocations |
| `ARGON_GC_VERIFY=1` | at each safe point, recount every reference by traversing everything and report objects whose refcount is wrong (too low → would crash, too high → would leak) |
| `ARGON_GC_NOFREE=1` | do all the bookkeeping but never free anything; pair it with `VERIFY` while code is still being converted |

`NOFREE` + `VERIFY` is how the conversion gets done: run a program, read the
list of miscounted objects (the verifier names the kind and the holder), fix
those stores, and repeat until it's clean. Then try it with freeing on.

---

## 5. Roadmap

- [x] **0. One interface.** All Boehm calls behind `memory.h`; `GC=custom`
      build switch for Make and CMake.
- [x] **1. Heap.** Page allocator (`heap.c`) with a randomized test.
      `GC=custom` runs the interpreter but leaks everything (the baseline).
- [x] **2. Core.** `GCHeader`, kinds, incref/decref, ZCT, epochs, safe points,
      trial deletion, verifier, with a standalone C test (`make gc-test`).
      Not used by the interpreter yet.
- [ ] **3. Managed types.** Headers plus `traverse`/`dealloc` for
      `ArgonObject` (per `ArgonType`) and `Stack`; owned buffers moved to
      `malloc`; statics made immortal; hashmaps told whether their keys/values
      are objects. Boehm never calls `finalize`, so owned buffers need a small
      allocator wrapper (`malloc`/`free` in the custom build, Boehm in the
      Boehm build).
- [ ] **4. Counted holders.** Convert stores subsystem by subsystem, driven by
      `NOFREE` + `VERIFY`: registers and frames → scopes → objects and slots
      → arrays/tuples/dicts → calls and errors → natives and the stdlib API.
- [ ] **5. Freeing on.** Turn freeing on and stress-test with `ARGON_GC_STRESS`.
- [ ] **6. Threads.** Atomic refcounts (or biased refcounting), per-thread
      epochs and ZCTs, cooperative stop-the-world at safe points.
- [ ] **7. Later.** Generations for the cycle collector, finalizers, freeing
      module bytecode.

## 6. Open questions / known limits

* **ZCT growth inside long native calls.** Temporaries created by a lambda
  that `map` calls die at the next safe point inside the nested loop, which is
  fine. But objects *older* than the nested call that die during it wait until
  the call returns.
* **Bytecode and constants** are treated as immortal for now.
* **Threads** are not supported by the custom backend until stage 6.
