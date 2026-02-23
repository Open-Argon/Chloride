#pragma once
#include <stdatomic.h>

#ifdef _WIN32
#include <stdlib.h>
#include <windows.h>

typedef SRWLOCK RWLock;

#define RWLOCK_INIT SRWLOCK_INIT

static inline void RWLOCK_CREATE(RWLock *lock) { InitializeSRWLock(lock); }

static inline void RWLOCK_DESTROY(RWLock *lock) { (void)lock; }

// Assume thread_count is defined externally
extern atomic_int thread_count;

// Execute a block while holding a read lock
#define RWLOCK_RDLOCK(l, block)                                                \
  do {                                                                         \
    if (atomic_load(&thread_count) == 0) {                                     \
      block;                                                                   \
    } else {                                                                   \
      AcquireSRWLockShared(&(l));                                              \
      block;                                                                   \
      ReleaseSRWLockShared(&(l));                                              \
    }                                                                          \
  } while (0)

#define RWLOCK_WRLOCK(l, block)                                                \
  do {                                                                         \
    if (atomic_load(&thread_count) == 0) {                                     \
      block;                                                                   \
    } else {                                                                   \
      AcquireSRWLockExclusive(&(l));                                           \
      block;                                                                   \
      ReleaseSRWLockExclusive(&(l));                                           \
    }                                                                          \
  } while (0)

#define RWBLOCK

// #elif defined(__APPLE__)
// #include <dispatch/dispatch.h>

// typedef struct {
//   dispatch_queue_t queue;
// } RWLock;

// static inline void RWLOCK_CREATE(RWLock *lock) {
//   lock->queue =
//       dispatch_queue_create("rwlock.queue", DISPATCH_QUEUE_CONCURRENT);
//   if (!lock->queue)
//     abort();
// }

// static inline void RWLOCK_DESTROY(RWLock *lock) {
//   (void)lock; // safe no-op
// }

// // Assume thread_count is defined externally
// extern atomic_int thread_count;

// // Execute a block inside a read lock (dispatch_sync allows parallel readers)
// #define RWLOCK_RDLOCK(l, block)                                                \
//   do {                                                                         \
//     if (atomic_load(&thread_count) == 0) {                                     \
//       block;                                                                   \
//     } else {                                                                   \
//       dispatch_sync((l).queue, ^{                                              \
//         block;                                                                 \
//       });                                                                      \
//     }                                                                          \
//   } while (0)

// // Execute a block inside a write lock (barrier ensures exclusivity)
// #define RWLOCK_WRLOCK(l, block)                                                \
//   do {                                                                         \
//     if (atomic_load(&thread_count) == 0) {                                     \
//       block;                                                                   \
//     } else {                                                                   \
//       dispatch_barrier_sync((l).queue, ^{                                      \
//         block;                                                                 \
//       });                                                                      \
//     }                                                                          \
//   } while (0)

// #define RWBLOCK __block

#else
#include <pthread.h>
#include <stdlib.h>

typedef pthread_rwlock_t RWLock;
#define RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER

static inline void RWLOCK_CREATE(RWLock *lock) {
  if (pthread_rwlock_init(lock, NULL) != 0)
    abort();
}

static inline void RWLOCK_DESTROY(RWLock *lock) {
  pthread_rwlock_destroy(lock);
}

// Assume thread_count is defined externally
extern atomic_int thread_count;

// Wrap block inside pthread rwlock with fast-path
#define RWLOCK_RDLOCK(l, block)                                                \
  do {                                                                         \
    if (atomic_load(&thread_count) == 0) {                                     \
      block;                                                                   \
    } else {                                                                   \
      pthread_rwlock_rdlock(&(l));                                             \
      block;                                                                   \
      pthread_rwlock_unlock(&(l));                                             \
    }                                                                          \
  } while (0)

#define RWLOCK_WRLOCK(l, block)                                                \
  do {                                                                         \
    if (atomic_load(&thread_count) == 0) {                                     \
      block;                                                                   \
    } else {                                                                   \
      pthread_rwlock_wrlock(&(l));                                             \
      block;                                                                   \
      pthread_rwlock_unlock(&(l));                                             \
    }                                                                          \
  } while (0)

#define RWBLOCK

#endif