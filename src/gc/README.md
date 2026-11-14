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
instructions**. At that moment the only live values belong to the VM.

### Uncounted VM roots

The VM's own holders are deferred too. Registers, each frame's current
scope, half-built calls (callee, arguments, kwargs) and saved exception
handlers are rewritten constantly, so counting them would make every
instruction pay for two refcount updates. They aren't counted at all.
Instead, a safe point first asks `trace_vm_roots` (in `runtime.c`) for every
object they point at, and treats each one as alive:

* ZCT entries that a root points at aren't freed (they stay in the ZCT);
* for the cycle collector, a root reference counts as an outside reference.

So when a frame returns, nothing needs to be decref'd: its registers simply
stop being roots, and anything that only they held dies at the next safe
point. This is the classic form of deferred reference counting.

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
   incref the new value and decref the old one (use `gc_assign`):
   object fields and built-in slots, hashmaps, object arrays, tuples, function
   structs (closure stack, default values), `Stack` scopes, iterator state.
   Hashmaps and object darrays do this themselves on insert/remove/pop/etc.;
   code that writes an array's elements directly must use
   `darray_armem_set_object` (or `gc_incref_each` after a `memcpy`).
2. **Uncounted VM roots.** Registers, frames, `call_instance`s and
   `ErrorCatch` entries are *not* counted; `trace_vm_roots` reports them at
   every safe point instead. A new VM structure that holds objects across
   instructions must be added there. Error objects (`TYPE_ERROR`) work the
   same way, because natives write into them through raw pointers: their
   `err.ptr` is reported by `trace_error_objects` in `gc_kinds.c`.
3. **C locals are free.** A pointer in a local variable or argument needs no
   bookkeeping, *as long as it doesn't survive past a safe point that could
   free it*. The birth rule makes that automatic for natives that call back
   into Argon.
4. **Globals.** A C global (or `static`) that points at an object must hold
   an immortal object. Call `gc_immortalize(obj)` when storing one after
   bootstrap. Caches that live for the whole program (variable-name strings,
   number constants, imported modules) do exactly this.
5. **Memory outside the GC.** Never keep the only reference to an object in
   `malloc`'d memory without counting it (the same rule Boehm already has).
6. **Holders must be traversable.** Every managed type has a `traverse`
   function listing its outgoing references. A store into a structure the
   collector can't traverse is invisible to it.
7. **Internal buffers are owned, not shared.** An array's element buffer, a
   hashmap's buckets, a string's bytes and so on belong to exactly one managed
   object and are freed with it (plain `malloc`/`free`). They are not
   refcounted themselves. A hashmap can still be *viewed* by a second object
   (`import` returns a dictionary that is a live view of the module's scope):
   `hashmap_GC_claim` makes the first holder the owner, and any later holder
   counts a reference to the owner instead of to the entries.
8. **Don't borrow another object's memory.** A key or string that points into
   some other object's characters dangles once that object is freed; copy it
   (attribute dict keys and unpacked kwargs keys are copied for this reason).

---

## 3. How the pieces fit

```
memory.h        public API used by the interpreter (both backends)
gc/gc.h         the API: GCHeader, kinds, incref/decref/gc_assign, epochs,
                safe points, verifier (works in both builds)
gc/heap.c       page allocator: size classes, free lists, page table
gc/platform.h   mutex, condition variable, clock: Win32 or POSIX
gc/gc.c         refcounts, ZCT, safe points, epochs, trial deletion
gc/verify.c     debug verifier: recounts every reference and compares
gc/gc_custom.c  memory.h for GC=custom; for now untyped ar_alloc is
                plain calloc and is never freed (shrinks as stage 4 goes on)
gc/gc_boehm.c   Boehm backend (incref/decref compile to nothing)
runtime/gc_kinds.c   the interpreter's managed types (ArgonObject, Stack):
                what each one points at, per ArgonType
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

Three steps (`free_object` in `gc.c`):

1. `traverse` with a visitor that decrefs each child (children reaching zero
   join the ZCT, so even a million-long list is freed without recursion);
2. the kind's `finalize` frees the buffers the object owns (in
   `runtime/gc_kinds.c`, next to `traverse`);
3. the cell goes back on its page's free list.

A garbage cycle is freed the same way, except that step 1 runs for *every*
object in the cycle before any step 2, so no finalizer can free something
another member's traverse still needs.

### A safe point

```
if (zct is big enough || cycle collection is due)
    gc_safe_point()
        0. mark everything the uncounted VM roots point at as rooted
        1. if the ZCT is big enough (or for a full collection): process it,
           freeing entries with refcount 0, birth >= epoch and not rooted
        2. if due: trial deletion over the young generation, or every
           object (a full collection), born >= epoch
        3. clear the root marks
```

### Generations

Each cycle collection used to examine the whole heap. Most garbage cycles
are young, though, so now there are two kinds (as in CPython):

* a **young** collection every 10,000 allocations looks only at objects
  allocated since the last cycle collection. `gc_new` appends every new
  object to a list (`young` in `gc.c`) that the collection empties; entries
  for objects freed meanwhile are skipped with `heap_cell_of`. References to
  young objects from older ones count as coming from outside, so anything
  an old object points at is kept - safe, just not exhaustive;
* a **full** collection examines everything, once the objects that survived
  young collections since the last full one add up to a quarter of what was
  live after it (and at least 50,000). That keeps its cost per allocation
  constant however big the heap is.

Young objects the epoch rule protects at that moment aren't candidates; they
count as survivors and wait for a full collection. (Keeping them on the young
list instead made every young collection rescan all of them, which inside a
long native call is most of the heap.)

A young collection doesn't make the safe point process the ZCT first (each
pass re-examines every entry the epoch rule or a root keeps, which got
expensive when young collections came every 10,000 allocations). So the
cycle collector may find garbage that is still listed in the ZCT. It
releases such an object's references as usual but leaves it `DYING`, and
the next ZCT pass frees it.

On the stress script this took cycle collection from 312 ms to about 120 ms
(longest pause 43 ms -> 22 ms) and the whole run from 1.2 s to 1.06 s
(Boehm: 1.11 s).

### Making it fast

`for (i in 1 to 10) array(1 to 1e6)` took 5.6 s against Boehm's 1.5 s. The
fixes, roughly in order of how much they mattered:

* **Acyclic objects aren't cycle candidates.** A kind's `acyclic` callback
  says when an object holds no references to mortal objects (nearly every
  number and string: just its built-in class); trial deletion skips those.
  Before, every young and full collection examined millions of numbers.
* **Numbers and strings skip the young list** (`gc_new_leaf`): they're almost
  never part of a cycle, so young collections don't even have to look at
  them (full collections still do).
* **New objects leave the ZCT on their first reference.** Every new object
  starts in the ZCT, and is usually stored straight away while it's still
  the last entry (`gc_first_reference` pops it). The ZCT then holds only
  things that actually died.
* **Leaves are freed where they die.** When freeing an object drops an
  acyclic child to zero (and nothing else protects it), `release_child` frees
  it on the spot, while it's still in cache, instead of listing it in the ZCT
  and visiting it again later from cold memory.
* **The ZCT is processed by allocation count too** (after the larger of 1,024
  and the number of entries the last pass kept), since one entry can be an
  array holding a million objects; that keeps rescans of kept entries
  amortised.
* **Young collections look candidates up by address** (a small hash set), so
  traversing a big array doesn't read the header of every element just to
  see it isn't a candidate.
* **Empty pages are kept** for one trim interval before going back to the OS
  (and on POSIX with `MADV_FREE`), so a program that frees and reallocates
  the same amount stops paying for page faults (214k -> 63k on the example).
* **Cheaper allocation:** no page-table lookup per allocation, only the
  requested bytes are zeroed, and cell indices use a multiply by a
  precomputed reciprocal instead of a division.
* **Interned attribute names**: one shared copy per distinct name instead of
  a `malloc`'d copy per attribute per object.

Result on that example: about 2.0 s vs Boehm's 1.7 s wall time (equal in CPU
time: Boehm marks with several threads). The stress script is faster than
Boehm; the cycle-heavy one about 1.2x slower.

---

## 4. Debugging tools

| env var | effect |
|---|---|
| `ARGON_GC_STATS=1` | print every collection and a summary at exit |
| `ARGON_GC_STRESS=N` | run a full safe point every N allocations |
| `ARGON_GC_VERIFY=1` | at each full safe point and at exit, recount every reference by traversing everything and report objects whose refcount is wrong (too low → would crash, too high → would leak), plus any pointer a `traverse` visits that isn't an object at all (*invalid*: a traverse or hashmap-flag bug) |
| `ARGON_GC_NOFREE=1` | do all the bookkeeping but never free anything; pair it with `VERIFY` while code is still being converted |

`NOFREE` + `VERIFY` is how the conversion gets done: run a program, read the
list of miscounted objects (the verifier names the kind and the holder), fix
those stores, and repeat until it's clean. Then try it with freeing on.

**AddressSanitizer** catches mistakes in what gets freed (double frees,
freeing something not owned, using a freed buffer), because owned buffers
come from `malloc` in the custom build. Object cells come from our own pages,
which ASan can't see inside; `NOFREE` + `VERIFY` covers those instead.

```
make GC=custom full-debug     # bin/argon-customgc with ASan
ARGON_GC_STRESS=50 ASAN_OPTIONS=hard_rss_limit_mb=1500:detect_leaks=0 \
    ./bin/argon-customgc program.ar
```

ASan needs a huge virtual address range, so cap it with `hard_rss_limit_mb`
rather than `ulimit -v`. With `detect_leaks=1` *and* `ARGON_GC_STATS=1` (which
collects once more at exit) the leak report lists what's really left: only
program-lifetime data (bytecode, bootstrap objects), which LSan reports
because the pointers to it live in our pages where it can't look.

---

## 5. Roadmap

- [x] **0. One interface.** All Boehm calls behind `memory.h`; `GC=custom`
      build switch for Make and CMake.
- [x] **1. Heap.** Page allocator (`heap.c`) with a randomized test.
      `GC=custom` runs the interpreter but leaks everything (the baseline).
- [x] **2. Core.** `GCHeader`, kinds, incref/decref, ZCT, epochs, safe points,
      trial deletion, verifier, with a standalone C test (`make gc-test`).
      Not used by the interpreter yet.
- [x] **3. Managed types.** `GCHeader` in `ArgonObject` and `Stack`, which
      now come from `gc_new`; `traverse` per `ArgonType`
      (`runtime/gc_kinds.c`); iterators hold the object they iterate over
      (dictionary iterators take a snapshot of the entries); hashmaps record
      whether keys/values are objects; statics registered; everything created
      during bootstrap is immortal. Checked with `ARGON_GC_VERIFY`: 0 invalid
      pointers on every test script.
- [x] **4. Counted holders.** Object slots and attribute dicts, hashmaps,
      object arrays, tuples, closures and default values, scopes, iterators
      are counted; VM registers/frames/calls are uncounted roots traced at
      safe points; the safe point and epochs are in `runtime()`. With
      `NOFREE` + `VERIFY` every test script is clean (no miscounted object),
      and with freeing switched on the output is unchanged, also when
      collecting every 50 allocations. Code paths that no script exercised
      (most stdlib natives: file, network, regex, subprocess, ...) haven't
      been verified yet: run your programs with
      `ARGON_GC_NOFREE=1 ARGON_GC_VERIFY=1 ARGON_GC_STRESS=10000` to check.
- [x] **5. Freeing on.** `finalize` for `ArgonObject` (per type) and `Stack`
      frees what they own: string characters, bignum limbs, array buffers,
      dictionaries' and attribute dicts' hashmaps (with their nodes and owned
      keys), buffer data, dictionary-iterator snapshots. Value structs inside
      the object's own cell are left alone. The VM frees its frames, pending
      calls and temporary argument arrays, hashmaps free removed nodes, and
      hashmap snapshots are copies that their users free. `ar_free_owned` is
      `free` in the custom build and does nothing under Boehm, so the Boehm
      build behaves exactly as before. Checked with ASan under
      `ARGON_GC_STRESS`: no errors, and output identical to Boehm. The stress
      script now peaks at 269 MB (Boehm: 243 MB; before this stage: 728 MB).
- [x] **6a. Threads, stepping stone.** While any extra thread exists,
      refcounts are updated atomically and `gc_assign` swaps atomically, so
      counts stay exact even when threads share objects; allocation and the
      ZCT take a lock; epochs are per thread; safe points don't free. Live
      threads are counted (now through `ar_prepare_thread`, see "Threads"),
      and when the last one finishes, freeing resumes and catches up. Error objects (written
      through raw pointers by other threads) are uncounted roots rather than
      counted holders. Checked: verifier clean after six threads hammered a
      shared dict; ASan clean; 40/40 runs of the start-up race test.
- [x] **6b. Threads, parallel collection.** Memory is reclaimed *while*
      threads run (see "Threads" below): a thread registry with per-thread
      VM roots, cooperative stop-the-world with a timeout, blocking regions
      (`join`, `sleep` and the shell's input are marked), and the epoch rule
      across threads. Results: the 8-thread test peaks at 121 MB (Boehm:
      246 MB; 6a: 925 MB); a 6-thread script at 20 MB (Boehm: 59 MB).
      ASan clean; 25/25 and 15/15 repeated stress runs correct; verifier
      clean after the threads finish.
- [ ] **7. Later.**
  - [x] Generations for the cycle collector (see "Generations").
  - [x] Speed: see "Making it fast".
  - [ ] Per-thread allocation, so threads don't contend on one lock for
        every allocation (threaded programs are still about twice as slow
        as with Boehm).
  - [ ] Finalizers (`ar_finalizer`; nothing uses them yet).
  - [ ] Freeing module bytecode (modules are immortal for now).

## 6. Threads

Every thread that may touch objects has a record (`GCThread` in `gc.c`) in
one of three states:

| state | meaning |
|---|---|
| `RUNNING` | executing; may touch objects at any moment |
| `PARKED` | stopped at a safe point because another thread wants to collect |
| `BLOCKING` | has promised not to touch objects: inside `gc_blocking_begin`/`end`, or not registered with the runtime (yet, or any more) |

**Collecting.** A thread whose safe point finds work to do sets
`stop_requested`; every other running thread notices at its next safe point
and parks. Once all of them are parked or blocking, the collector traces
*every* thread's VM roots, collects, and wakes them. If someone doesn't stop
within 2 ms (a long native call, say), it gives up and tries again later, so
a native that forgets to mark a wait can delay collection but never deadlock
it.

**The epoch rule across threads.** Each thread's suspended natives can hold
objects born before that thread's current epoch. So with several threads an
object may only be freed if it was born after the current epoch of *every*
thread. That's conservative: while one thread is deep in a native call that
started recently, older garbage waits for a later collection.

**Starting and stopping threads.** Natives hand new threads objects nothing
counts (the threading module's `thread_arg` holds the function to run), so
the collector must know about a thread *before* it runs. Whoever starts a
thread passes its start routine and argument through `api->prepare_thread`
(`ar_prepare_thread` inside the interpreter) and starts the thread - with
`pthread_create`, `CreateThread`, whatever - using what comes back:

```c
void *(*start)(void *) = thread_fn;
void *arg = my_arg;
api->prepare_thread(&start, &arg);    // makes the thread's GC record now
if (create_the_thread(start, arg) != 0)
  api->cancel_thread(arg);            // no thread after all
```

The record is made *in the parent*, as `BLOCKING`, with an epoch that
protects every object existing at that moment. What comes back is a
trampoline that adopts the record, runs the real start routine, and
finishes the record afterwards (on POSIX also on `pthread_exit` or
cancellation). The thread becomes `RUNNING` when it calls the native API's
`register_thread` and goes back to `BLOCKING` at `unregister_thread`.

A thread started *without* `prepare_thread` gets a record late, when it
calls `register_thread`. That's a fallback, not a safe alternative: objects
handed to the thread can be freed before it registers (forcing this in a
test gave wrong results in 3 runs out of 15). Any native that starts
threads must use `prepare_thread`.

**Blocking regions.** Natives that wait should wrap the wait in
`api->blocking_begin()` / `api->blocking_end()`, and not touch any object in
between. `Join` (threading), `sleep` (time) and the shell's input already do.

**Counting.** While more than one thread exists, refcount updates are atomic
and `gc_assign` swaps atomically; allocation and the ZCT take a lock. With a
single thread none of that costs anything.

## 7. Platforms

The custom backend is meant to build and run on Windows, macOS and Linux.
What depends on the OS is kept in two places:

| | Windows | macOS, Linux (POSIX) |
|---|---|---|
| pages (`heap.c`, "The OS layer") | `VirtualAlloc`, `VirtualFree`, `MEM_RESET`; pages mapped one at a time (`VirtualFree` releases whole allocations only) | `mmap`, `munmap`, `madvise`; 16 pages mapped at a time, over-mapped and trimmed for 64 KiB alignment |
| locks, condition variables, clock (`platform.h`) | `SRWLOCK`, `CONDITION_VARIABLE`, `QueryPerformanceCounter` | pthreads, `clock_gettime` |

Everything else is plain C11 plus GCC/Clang builtins (`__atomic_*`,
`__typeof__`, `__builtin_ctzll`) and `__thread`, which MinGW-w64 GCC and
Apple Clang both support. The page table assumes user addresses fit in 48
bits, true of current 64-bit Windows, macOS and Linux; it aborts with a
message if one doesn't. `make gc-test` uses AddressSanitizer, so it's a
Linux/macOS developer target.

## 8. Open questions / known limits

* **Natives that start threads without `prepare_thread`** are unsafe (see
  "Starting and stopping threads"). The threading module uses it.
* **Unmarked waits** in natives (network reads, subprocess waits, ...) stop
  collection from happening while they wait (after a 2 ms attempt, with a
  backoff). Marking them with `blocking_begin/end` fixes that.
* **Stress mode with threads is slow:** `ARGON_GC_STRESS=N` counts
  allocations across all threads and stops every thread each time; use a
  few thousand rather than 50.
* **Natives holding objects in their own memory** (like `thread_arg`) rely on
  the epoch protection above. Incref/decref in the native API would let them
  hold counted references instead, which is less conservative.
* **Not yet run under ThreadSanitizer** (`libtsan` isn't installed on the
  development machine); that's the obvious next check for this stage.
* **Unsynchronised writes from several threads** to the same array element
  through the raw-write paths in `array.c` (slice assignment, delete) can
  still lose a count, because those paths move elements without the
  darray's lock. Programs that share an array between threads without
  synchronisation already race under Boehm; here it could also leak or
  corrupt that array's elements.
* **Natives keeping objects in `malloc`'d memory** (like the threading
  module's `thread_arg`) aren't counted. Stage 6 needs incref/decref in the
  native API so they can be.
* **ZCT growth inside long native calls.** Temporaries created by a lambda
  that `map` calls die at the next safe point inside the nested loop, which is
  fine. But objects *older* than the nested call that die during it wait until
  the call returns.
* **Bytecode and constants** are treated as immortal for now.
