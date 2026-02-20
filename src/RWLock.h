#pragma once

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>

typedef SRWLOCK RWLock;

#define RWLOCK_INIT SRWLOCK_INIT

static inline void RWLOCK_CREATE(RWLock* lock) {
    InitializeSRWLock(lock);
}

static inline void RWLOCK_DESTROY(RWLock* lock) {
    (void)lock;
}

// Execute a block while holding a read lock
#define RWLOCK_RDLOCK(l, block)          \
    do {                                 \
        AcquireSRWLockShared(&(l));      \
        block;                            \
        ReleaseSRWLockShared(&(l));      \
    } while(0)

#define RWLOCK_WRLOCK(l, block)          \
    do {                                 \
        AcquireSRWLockExclusive(&(l));   \
        block;                            \
        ReleaseSRWLockExclusive(&(l));   \
    } while(0)

#elif defined(__APPLE__)
#include <dispatch/dispatch.h>

typedef struct {
    dispatch_queue_t queue;
} RWLock;

static inline void RWLOCK_CREATE(RWLock* lock) {
    lock->queue = dispatch_queue_create("rwlock.queue", DISPATCH_QUEUE_CONCURRENT);
    if(!lock->queue) abort();
}

static inline void RWLOCK_DESTROY(RWLock* lock) {
    (void)lock; // safe no-op
}

// Execute a block inside a read lock (dispatch_sync allows parallel readers)
#define RWLOCK_RDLOCK(l, block) dispatch_sync((l).queue, ^{ block; })

// Execute a block inside a write lock (barrier ensures exclusivity)
#define RWLOCK_WRLOCK(l, block) dispatch_barrier_sync((l).queue, ^{ block; })

#else
#include <pthread.h>
#include <stdlib.h>

typedef pthread_rwlock_t RWLock;
#define RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER

static inline void RWLOCK_CREATE(RWLock* lock) {
    if(pthread_rwlock_init(lock, NULL) != 0) abort();
}

static inline void RWLOCK_DESTROY(RWLock* lock) {
    pthread_rwlock_destroy(lock);
}

// Wrap block inside pthread rwlock
#define RWLOCK_RDLOCK(l, block)          \
    do {                                 \
        pthread_rwlock_rdlock(&(l));     \
        block;                            \
        pthread_rwlock_unlock(&(l));     \
    } while(0)

#define RWLOCK_WRLOCK(l, block)          \
    do {                                 \
        pthread_rwlock_wrlock(&(l));     \
        block;                            \
        pthread_rwlock_unlock(&(l));     \
    } while(0)

#endif