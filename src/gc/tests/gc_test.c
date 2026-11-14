/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Standalone tests for the refcounting core (gc.c, verify.c, heap.c).
// Run with `make gc-test`.
//
// Uses a toy managed type, Node, with four counted child slots and an owned
// malloc'd buffer, and a fake "VM register file" (roots[]) reported through a
// root tracer, exactly how the interpreter will use the API.

#include "../gc.h"
#include "../gc_internal.h"
#include "../heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

// ---------------------------------------------------------------------------
// The toy managed type
// ---------------------------------------------------------------------------

#define NCHILD 4

typedef struct Node {
  GCHeader gc;
  struct Node *child[NCHILD]; // counted
  size_t id;
  char *buf; // owned, freed by finalize
} Node;

static uint8_t NODE;
static size_t finalized;

static void node_traverse(GCHeader *h, gc_visit_fn visit, void *ctx) {
  Node *n = (Node *)h;
  for (int i = 0; i < NCHILD; i++)
    visit(&n->child[i]->gc, ctx); // &NULL->gc is NULL: gc is the first member
}

static void node_finalize(GCHeader *h) {
  free(((Node *)h)->buf);
  finalized++;
}

static void node_describe(GCHeader *h, char *buf, size_t len) {
  snprintf(buf, len, "node %zu", ((Node *)h)->id);
}

static const GCKind node_kind = {"Node", node_traverse, node_finalize,
                                 node_describe};

static size_t next_id;

static Node *node_new(void) {
  Node *n = gc_new(NODE, sizeof(Node));
  n->id = next_id++;
  n->buf = malloc(16);
  snprintf(n->buf, 16, "%zu", n->id);
  return n;
}

// The fake register file: counted holders the verifier learns about through
// a root tracer.
#define NROOTS 64
static Node *roots[NROOTS];

static void trace_roots(gc_visit_fn visit, void *ctx) {
  for (int i = 0; i < NROOTS; i++)
    visit(roots[i] ? &roots[i]->gc : NULL, ctx);
}

static bool is_live(Node *n) { return heap_cell_of(n) == n; }

// Fake VM registers: an UNCOUNTED root (stores into it never incref).
#define NREGS 8
static Node *regs[NREGS];

static void trace_regs(gc_visit_fn visit, void *ctx) {
  for (int i = 0; i < NREGS; i++)
    visit(regs[i] ? &regs[i]->gc : NULL, ctx);
}

// Objects the tests deliberately leave behind for good (an immortal node and
// a NOFREE zombie); every "is the heap empty?" check allows for them.
static size_t permanent;
static Node *immortal_node, *zombie_node; // their buffers are freed by main

static void clear_roots(void) {
  for (int i = 0; i < NROOTS; i++)
    gc_assign(&roots[i], NULL);
}

static void expect_clean_heap(void) {
  clear_roots();
  gc_collect_now();
  CHECK(gc_stats().live == permanent);
}

static void expect_verify_clean(void) {
  GCVerifyResult r = gc_verify();
  CHECK(r.too_low == 0 && r.too_high == 0 && r.dangling == 0 &&
        r.invalid == 0);
}

// ---------------------------------------------------------------------------
// Targeted tests
// ---------------------------------------------------------------------------

static void test_refcount_basics(void) {
  Node *a = node_new(); // refcount 0, only a C local holds it
  gc_assign(&roots[0], a);
  CHECK(a->gc.refcount == 1);
  gc_collect_now();
  CHECK(is_live(a)); // counted, survives

  Node *b = node_new();
  gc_assign(&a->child[0], b);
  gc_assign(&roots[0], NULL); // a -> 0 -> freed, which releases b too
  size_t before = finalized;
  gc_collect_now();
  CHECK(finalized == before + 2);
  CHECK(gc_stats().live == 0);

  // An unstored new object dies at the next safe point.
  node_new();
  gc_collect_now();
  CHECK(gc_stats().live == 0);
  printf("  refcount basics: ok\n");
}

static void test_long_chain(void) {
  // Freeing must not recurse: a 300k-long list would overflow the C stack.
  Node *head = node_new();
  gc_assign(&roots[0], head);
  Node *n = head;
  for (int i = 0; i < 300000; i++) {
    Node *next = node_new();
    gc_assign(&n->child[0], next);
    n = next;
  }
  gc_collect_now();
  CHECK(gc_stats().live == 300001);
  gc_assign(&roots[0], NULL);
  gc_collect_now();
  CHECK(gc_stats().live == 0);
  printf("  long chain: ok\n");
}

static void test_cycles(void) {
  // a <-> b, plus a self-loop on c, with nothing else pointing at them.
  Node *a = node_new(), *b = node_new(), *c = node_new();
  gc_assign(&a->child[0], b);
  gc_assign(&b->child[0], a);
  gc_assign(&c->child[0], c);
  gc_assign(&roots[0], a);
  gc_assign(&roots[1], c);
  gc_collect_now();
  CHECK(is_live(a) && is_live(b) && is_live(c)); // still rooted

  // Hanging a non-cycle object off the cycle: it must go with the cycle.
  Node *tail = node_new();
  gc_assign(&b->child[1], tail);

  clear_roots(); // counts never reach 0 now, only the cycle collector helps
  gc_collect_now();
  CHECK(gc_stats().live == 0);
  printf("  cycles: ok\n");
}

static void test_epochs(void) {
  // An uncounted C local (like a native's) holds `old` across a nested
  // runtime() invocation. Safe points inside it must not free `old`.
  Node *old = node_new();
  Node *old_cycle = node_new();
  gc_assign(&old_cycle->child[0], old_cycle);

  gc_epoch saved = gc_enter();
  for (int i = 0; i < 50000; i++)
    node_new(); // garbage born inside the nested invocation
  Node *inner_cycle = node_new();
  gc_assign(&inner_cycle->child[0], inner_cycle);
  gc_collect_now();
  CHECK(is_live(old) && is_live(old_cycle));
  CHECK(!is_live(inner_cycle));         // born inside: fair game
  CHECK(gc_stats().live == 2 + permanent); // the 50000 temporaries are gone
  gc_leave(saved);

  gc_collect_now(); // back at the outer level, both can go now
  CHECK(gc_stats().live == 0);
  printf("  epochs: ok\n");
}

static void test_uncounted_roots(void) {
  Node *a = node_new(); // refcount 0, only in a register
  regs[0] = a;          // plain store: registers aren't counted
  Node *b = node_new(), *c = node_new(); // a cycle, only reachable from a reg
  gc_assign(&b->child[0], c);
  gc_assign(&c->child[0], b);
  regs[1] = b;
  gc_collect_now();
  CHECK(is_live(a) && is_live(b) && is_live(c));
  CHECK(a->gc.gc_refs == 0 && !(a->gc.flags & GC_ROOTED)); // scratch cleared
  expect_verify_clean(); // register references aren't counted references

  regs[0] = regs[1] = NULL; // the "frame" returned
  gc_collect_now();
  CHECK(gc_stats().live == permanent);
  printf("  uncounted roots: ok\n");
}

static void test_immortal(void) {
  Node *a = node_new();
  gc_immortalize(a);
  permanent++;
  immortal_node = a;
  gc_assign(&roots[0], a);
  CHECK(a->gc.refcount == 0); // immortal objects aren't counted
  gc_assign(&roots[0], NULL);
  gc_collect_now();
  CHECK(is_live(a));
  // An immortal holder still counts the mortal objects it points at.
  Node *child = node_new();
  gc_assign(&a->child[0], child);
  gc_collect_now();
  CHECK(is_live(child));
  expect_verify_clean();
  gc_assign(&a->child[0], NULL);
  gc_collect_now();
  CHECK(!is_live(child));
  printf("  immortal: ok\n");
}

static void test_verifier(void) {
  fprintf(stderr, "  (the verifier messages below are expected)\n");
  Node *a = node_new(), *b = node_new();
  gc_assign(&roots[0], a);
  a->child[0] = b; // store WITHOUT incref: count too low
  GCVerifyResult r = gc_verify();
  CHECK(r.too_low == 1 && r.too_high == 0);
  gc_incref(b); // fix it

  gc_incref(a); // an extra incref: count too high
  r = gc_verify();
  CHECK(r.too_low == 0 && r.too_high == 1);
  gc_decref(a);
  expect_verify_clean();

  // NOFREE: freed objects become zombies, and live references to them are
  // reported as dangling instead of crashing.
  gc_config.nofree = true;
  Node *victim = node_new();
  gc_assign(&a->child[1], victim);
  Node *holder = node_new();
  gc_assign(&roots[1], holder);
  holder->child[0] = victim; // the uncounted store again
  gc_assign(&a->child[1], NULL);
  gc_collect_now(); // victim's count hit 0 -> "freed" (zombie)
  CHECK(victim->gc.flags & GC_ZOMBIE);
  permanent++; // zombies are never really freed
  zombie_node = victim;
  r = gc_verify();
  CHECK(r.dangling == 1);
  holder->child[0] = NULL;
  gc_config.nofree = false;

  // A traverse that visits something that isn't an object is reported, not
  // dereferenced (a hashmap of flags, say, traversed as if it held objects).
  static long not_an_object[4];
  holder->child[2] = (Node *)not_an_object;
  r = gc_verify();
  CHECK(r.invalid == 1);
  holder->child[2] = NULL;

  expect_clean_heap();
  printf("  verifier: ok\n");
}

// ---------------------------------------------------------------------------
// Randomized test against a shadow model of the graph
// ---------------------------------------------------------------------------

#define MAX_NODES 400000
static long model_child[MAX_NODES][NCHILD]; // ids, or -1
static Node *by_id[MAX_NODES];
static long model_root[NROOTS];
static unsigned char seen[MAX_NODES];
static long reachable[MAX_NODES];
static size_t nreachable;

static void model_reachability(void) {
  memset(seen, 0, next_id);
  nreachable = 0;
  for (int r = 0; r < NROOTS; r++)
    if (model_root[r] >= 0 && !seen[model_root[r]]) {
      seen[model_root[r]] = 1;
      reachable[nreachable++] = model_root[r];
    }
  for (size_t i = 0; i < nreachable; i++)
    for (int c = 0; c < NCHILD; c++) {
      long k = model_child[reachable[i]][c];
      if (k >= 0 && !seen[k]) {
        seen[k] = 1;
        reachable[nreachable++] = k;
      }
    }
}

static void check_against_model(bool full) {
  model_reachability();
  for (size_t i = 0; i < nreachable; i++) {
    long id = reachable[i];
    Node *n = by_id[id];
    CHECK(is_live(n));
    CHECK(n->id == (size_t)id); // not freed and reused for something else
    CHECK(atol(n->buf) == id);
    for (int c = 0; c < NCHILD; c++)
      CHECK(model_child[id][c] < 0 ? n->child[c] == NULL
                                   : n->child[c] == by_id[model_child[id][c]]);
  }
  if (full) // after a full collection there is no garbage left at all
    CHECK(gc_stats().live == nreachable + permanent);
}

static long pick_reachable(void) {
  return nreachable ? reachable[rand() % nreachable] : -1;
}

static void test_random(void) {
  next_id = 0; // ids index the model; earlier tests' nodes are all gone
  for (size_t i = 0; i < MAX_NODES; i++)
    for (int c = 0; c < NCHILD; c++)
      model_child[i][c] = -1;
  for (int r = 0; r < NROOTS; r++)
    model_root[r] = -1;
  srand(4242);

  int nested = 0;
  gc_epoch saved = 0;
  for (int round = 0; next_id < MAX_NODES - 2000; round++) {
    model_reachability();
    // Mutate without any safe point in between, so every node we pick from
    // `reachable` stays allocated even if a mutation orphans it.
    for (int m = 0; m < 1000; m++) {
      int op = rand() % 100;
      if (op < 35) { // new node, stored as a child of something reachable
        Node *n = node_new();
        by_id[n->id] = n;
        long parent = pick_reachable();
        int c = rand() % NCHILD;
        if (parent >= 0 && rand() % 4) {
          gc_assign(&by_id[parent]->child[c], n);
          model_child[parent][c] = n->id;
        } else {
          int r = rand() % NROOTS;
          gc_assign(&roots[r], n);
          model_root[r] = n->id;
        }
        reachable[nreachable++] = n->id; // pickable this round
      } else if (op < 80) { // rewire an edge (this is what makes cycles)
        long from = pick_reachable(), to = pick_reachable();
        if (from < 0)
          continue;
        int c = rand() % NCHILD;
        if (rand() % 5 == 0)
          to = -1;
        gc_assign(&by_id[from]->child[c], to < 0 ? NULL : by_id[to]);
        model_child[from][c] = to;
      } else if (op < 90) { // repoint or clear a root
        int r = rand() % NROOTS;
        long to = rand() % 3 ? pick_reachable() : -1;
        gc_assign(&roots[r], to < 0 ? NULL : by_id[to]);
        model_root[r] = to;
      }
      // else: no-op
    }

    // Sometimes enter or leave a nested "runtime()" invocation.
    if (!nested && rand() % 8 == 0) {
      saved = gc_enter();
      nested = 1;
    } else if (nested && rand() % 3 == 0) {
      gc_leave(saved);
      nested = 0;
    }

    bool full = rand() % 4 == 0;
    if (full)
      gc_collect_now();
    else
      gc_safe_point();
    check_against_model(full && !nested);
    if (round % 50 == 0)
      expect_verify_clean();
  }
  if (nested)
    gc_leave(saved);
  gc_collect_now();
  check_against_model(true);
  GCStats s = gc_stats();
  printf("  random graph: ok (%zu allocations, %zu freed by refcount, %zu by "
         "cycle collection, %zu cycle collections)\n",
         s.allocated, s.freed_by_rc, s.freed_by_cycle, s.cycle_collections);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  gc_init();
  NODE = gc_register_kind(&node_kind);
  gc_add_counted_roots(trace_roots);
  gc_add_uncounted_roots(trace_regs);

  printf("gc_test:\n");
  test_refcount_basics();
  test_long_chain();
  test_cycles();
  test_epochs();
  test_uncounted_roots();
  test_immortal();
  expect_clean_heap();
  test_verifier();
  test_random();
  expect_clean_heap(); // and every owned buffer is finalized
  free(immortal_node->buf);
  free(zombie_node->buf);
  printf("gc_test: ok\n");
  return 0;
}
