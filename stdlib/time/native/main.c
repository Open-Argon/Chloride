// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonFunction.h"
#include <unistd.h>

ARGON_FUNCTION(snooze, {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  double n = api->argon_to_double(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  usleep(n * 1e6);

  return api->ARGON_NULL;
})

INIT_ARGON_MODULE({
 REGISTER_ARGON_FUNCTION(snooze)
})