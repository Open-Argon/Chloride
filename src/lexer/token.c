/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "token.h"
#include <stdlib.h>

void free_token(void *ptr) {
  Token *token = ptr;
  free(token->value);
}