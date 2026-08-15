// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonFunction.h"

#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

ARGON_FUNCTION(snooze, {
    if (api->fix_to_arg_size(1, argc, err))
        return api->ARGON_NULL;

    double n = api->argon_to_double(argv[0], err);

    if (api->is_error(err))
        return api->ARGON_NULL;

    if (n <= 0.0)
        return api->ARGON_NULL;

#ifdef _WIN32

    while (n > 0.0) {
        double milliseconds = n * 1000.0;

        if (milliseconds >= 4294967295.0) {
            Sleep(4294967295UL);
            n -= 4294967.295;
        } else {
            DWORD ms = (DWORD)milliseconds;

            if ((double)ms < milliseconds)
                ++ms;

            if (ms > 0)
                Sleep(ms);

            break;
        }
    }

#else

    struct timespec remaining;

    remaining.tv_sec = (time_t)n;
    remaining.tv_nsec =
        (long)((n - (double)remaining.tv_sec) * 1000000000.0);

    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR)
            break;
    }

#endif

    return api->ARGON_NULL;
})

INIT_ARGON_MODULE({REGISTER_ARGON_FUNCTION(snooze)})