// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonFunction.h"
#include <errno.h>
#include <time.h>

ARGON_FUNCTION(snooze, {
    if (api->fix_to_arg_size(1, argc, err))
        return api->ARGON_NULL;

    double n = api->argon_to_double(argv[0], err);

    if (api->is_error(err))
        return api->ARGON_NULL;

    struct timespec remaining;

    remaining.tv_sec = (time_t)n;
    remaining.tv_nsec =
        (long)((n - (double)remaining.tv_sec) * 1000000000.0);

    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR)
            break;
    }

    return api->ARGON_NULL;
})

INIT_ARGON_MODULE({REGISTER_ARGON_FUNCTION(snooze)})