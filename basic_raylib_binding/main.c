/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../include/Argon.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *name;
  Color color;
} NamedColor;

ArgonObject *Argon_InitWindow(size_t argc, ArgonObject **argv, ArgonError *err,
                              ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(3, argc, err)) {
    return api->ARGON_NULL;
  }
  int64_t width = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  int64_t height = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  struct string title = api->argon_to_string(argv[2], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  char *title_c = malloc(title.length + 1);
  title_c[title.length] = '\0';
  memcpy(title_c, title.data, title.length);
  InitWindow(width, height, title_c);
  free(title_c);
  return api->ARGON_NULL;
}

ArgonObject *Argon_SetTargetFPS(size_t argc, ArgonObject **argv,
                                ArgonError *err, ArgonState *state,
                                ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }
  int64_t fps = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  SetTargetFPS(fps);
  return api->ARGON_NULL;
}

ArgonObject *Argon_WindowShouldClose(size_t argc, ArgonObject **argv,
                                     ArgonError *err, ArgonState *state,
                                     ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err)) {
    return api->ARGON_NULL;
  }
  return api->i64_to_argon(WindowShouldClose());
}

ArgonObject *Argon_BeginDrawing(size_t argc, ArgonObject **argv,
                                ArgonError *err, ArgonState *state,
                                ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err)) {
    return api->ARGON_NULL;
  }
  BeginDrawing();
  return api->ARGON_NULL;
}

ArgonObject *Argon_ClearBackground(size_t argc, ArgonObject **argv,
                                   ArgonError *err, ArgonState *state,
                                   ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }
  struct buffer color = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  Color color_ray = *(Color *)color.data;
  ClearBackground(color_ray);
  return api->ARGON_NULL;
}

ArgonObject *Argon_DrawText(size_t argc, ArgonObject **argv, ArgonError *err,
                            ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(5, argc, err)) {
    return api->ARGON_NULL;
  }
  struct string text = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  int64_t posX = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  int64_t posY = api->argon_to_i64(argv[2], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  int64_t fontSize = api->argon_to_i64(argv[3], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }

  struct buffer color = api->argon_buffer_to_buffer(argv[4], err);
  if (api->is_error(err)) {
    return api->ARGON_NULL;
  }
  Color color_ray = *(Color *)color.data;

  char *text_c = malloc(text.length + 1);
  text_c[text.length] = '\0';
  memcpy(text_c, text.data, text.length);

  DrawText(text_c, posX, posY, fontSize, color_ray);
  free(text_c);
  return api->ARGON_NULL;
}

ArgonObject *Argon_EndDrawing(size_t argc, ArgonObject **argv, ArgonError *err,
                              ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err)) {
    return api->ARGON_NULL;
  }
  EndDrawing();
  return api->ARGON_NULL;
}

ArgonObject *Argon_CloseWindow(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err)) {
    return api->ARGON_NULL;
  }
  CloseWindow();
  return api->ARGON_NULL;
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  NamedColor colors[] = {
      {"WHITE", WHITE},     {"BLACK", BLACK},         {"RED", RED},
      {"GREEN", GREEN},     {"BLUE", BLUE},           {"YELLOW", YELLOW},
      {"ORANGE", ORANGE},   {"PINK", PINK},           {"PURPLE", PURPLE},
      {"BEIGE", BEIGE},     {"BROWN", BROWN},         {"MAROON", MAROON},
      {"GRAY", GRAY},       {"LIGHTGRAY", LIGHTGRAY}, {"DARKGRAY", DARKGRAY},
      {"SKYBLUE", SKYBLUE}, {"VIOLET", VIOLET},       {"GOLD", GOLD},
      {"LIME", LIME}};
  int numColors = sizeof(colors) / sizeof(colors[0]);

  for (int i = 0; i < numColors; i++) {
    ArgonObject *color_object =
        api->create_argon_buffer(sizeof(colors[i].color));
    struct buffer b = api->argon_buffer_to_buffer(color_object, err);
    if (api->is_error(err))
      return;
    memcpy(b.data, &colors[i].color, b.size);
    api->register_ArgonObject(reg, colors[i].name, color_object);
  }

  api->register_ArgonObject(
      reg, "InitWindow",
      api->create_argon_native_function("InitWindow", Argon_InitWindow));

  api->register_ArgonObject(
      reg, "SetTargetFPS",
      api->create_argon_native_function("SetTargetFPS", Argon_SetTargetFPS));

  api->register_ArgonObject(reg, "WindowShouldClose",
                            api->create_argon_native_function(
                                "WindowShouldClose", Argon_WindowShouldClose));

  api->register_ArgonObject(
      reg, "BeginDrawing",
      api->create_argon_native_function("BeginDrawing", Argon_BeginDrawing));

  api->register_ArgonObject(reg, "ClearBackground",
                            api->create_argon_native_function(
                                "ClearBackground", Argon_ClearBackground));

  api->register_ArgonObject(
      reg, "DrawText",
      api->create_argon_native_function("DrawText", Argon_DrawText));

  api->register_ArgonObject(
      reg, "EndDrawing",
      api->create_argon_native_function("EndDrawing", Argon_EndDrawing));

  api->register_ArgonObject(
      reg, "CloseWindow",
      api->create_argon_native_function("CloseWindow", Argon_CloseWindow));
}