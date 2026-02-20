// rwlock.h
#pragma once

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>

typedef SRWLOCK RWLock;

// Static initializer still works
#define RWLOCK_INIT SRWLOCK_INIT

// Dynamic creation / destruction
static inline void RWLOCK_CREATE(RWLock* lock) {
    InitializeSRWLock(lock);
}

static inline void RWLOCK_DESTROY(RWLock* lock) {
    (void)lock; // no-op
}

// Lock/unlock macros
#define RWLOCK_RDLOCK(l) AcquireSRWLockShared(&(l))
#define RWLOCK_WRLOCK(l) AcquireSRWLockExclusive(&(l))
#define RWLOCK_UNLOCK_RD(l) ReleaseSRWLockShared(&(l))
#define RWLOCK_UNLOCK_WR(l) ReleaseSRWLockExclusive(&(l))
// #elif defined(__APPLE__)
// #include <dispatch/dispatch.h>

// typedef struct {
//     dispatch_queue_t queue;
// } RWLock;

// static inline void RWLOCK_CREATE(RWLock* lock) {
//     lock->queue = dispatch_queue_create("rwlock.queue", DISPATCH_QUEUE_CONCURRENT);
//     if(!lock->queue) abort(); // fail fast if allocation fails
// }

// // On macOS, destroying queues while threads may still use them is unsafe
// static inline void RWLOCK_DESTROY(RWLock* lock) {
//     (void)lock; // no-op
// }

// #define RWLOCK_RDLOCK(l) dispatch_sync((l).queue, ^{})
// #define RWLOCK_WRLOCK(l) dispatch_barrier_sync((l).queue, ^{})
// #define RWLOCK_UNLOCK_RD(l) ((void)0)
// #define RWLOCK_UNLOCK_WR(l) ((void)0)

#else
#include <pthread.h>
#include <stdlib.h>

typedef pthread_rwlock_t RWLock;
#define RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER

static inline void RWLOCK_CREATE(RWLock* lock) {
    if(pthread_rwlock_init(lock, NULL) != 0) {
        abort(); 
    }
}

static inline void RWLOCK_DESTROY(RWLock* lock) {
    pthread_rwlock_destroy(lock);
}

#define RWLOCK_RDLOCK(l) pthread_rwlock_rdlock(&(l))
#define RWLOCK_WRLOCK(l) pthread_rwlock_wrlock(&(l))
#define RWLOCK_UNLOCK_RD(l) pthread_rwlock_unlock(&(l))
#define RWLOCK_UNLOCK_WR(l) pthread_rwlock_unlock(&(l))
#endif