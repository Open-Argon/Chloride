#include "random.h"
#include <stdint.h>

/*
 * PCG32 implementation
 * Public domain reference: https://www.pcg-random.org/
 *
 * This implementation is:
 *  - fully deterministic
 *  - endian-safe
 *  - identical across OSes and architectures
 */

#include <stdint.h>

#if defined(_WIN32)

#include <windows.h>
#include <bcrypt.h>

uint64_t random_os_seed(void) {
    uint64_t seed = 0;

    /* Tier 1: BCryptGenRandom */
    if (BCryptGenRandom(
            NULL,
            (PUCHAR)&seed,
            sizeof(seed),
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        ) == 0) {
        return seed;
    }

    /* Tier 3 fallback */
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    seed ^= (uint64_t)counter.QuadPart;
    seed ^= (uint64_t)GetCurrentProcessId() << 32;
    seed ^= (uint64_t)(uintptr_t)&seed;

    return seed;
}

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

uint64_t random_os_seed(void) {
    uint64_t seed = 0;

    /* Tier 1: arc4random */
    seed = ((uint64_t)arc4random() << 32) | arc4random();
    if (seed != 0)
        return seed;

    /* Tier 3 fallback */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    seed ^= (uint64_t)tv.tv_sec;
    seed ^= (uint64_t)tv.tv_usec << 32;
    seed ^= (uint64_t)getpid();
    seed ^= (uint64_t)(uintptr_t)&seed;

    return seed;
}

#else /* Linux / POSIX */

#include <unistd.h>
#include <sys/random.h>
#include <sys/time.h>
#include <fcntl.h>

uint64_t random_os_seed(void) {
    uint64_t seed = 0;

    /* Tier 1: getrandom */
    if (getrandom(&seed, sizeof(seed), 0) == sizeof(seed)) {
        return seed;
    }

    /* Tier 2: /dev/urandom */
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, &seed, sizeof(seed));
        close(fd);
        if (n == sizeof(seed))
            return seed;
    }

    /* Tier 3 fallback */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    seed ^= (uint64_t)tv.tv_sec;
    seed ^= (uint64_t)tv.tv_usec << 32;
    seed ^= (uint64_t)getpid();
    seed ^= (uint64_t)(uintptr_t)&seed;

    return seed;
}
#endif

static uint32_t pcg32_next(Random *r) {
    uint64_t oldstate = r->state;

    /* Advance internal state */
    r->state = oldstate * 6364136223846793005ULL + (r->inc | 1ULL);

    /* Output function (XSH RR) */
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void random_seed(Random *r, uint64_t seed) {
    /* Recommended PCG seeding sequence */
    r->state = 0ULL;
    r->inc   = (seed << 1u) | 1u;
    pcg32_next(r);
    r->state += seed;
    pcg32_next(r);
}

uint32_t random_next_u32(Random *r) {
    return pcg32_next(r);
}

double random_next_double(Random *r) {
    uint32_t a = random_next_u32(r) >> 5;  // top 27 bits
    uint32_t b = random_next_u32(r) >> 6;  // top 26 bits

    uint64_t combined = ((uint64_t)a << 26) | b; // 53-bit integer

    return combined / 9007199254740992.0; // 2^53
}
