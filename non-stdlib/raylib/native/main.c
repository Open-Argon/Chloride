/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "Argon.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* const char* → char* without a warning, needed because ARGON_STRING_FROM_C_STRING
 * takes char* and raylib returns const char* for many path/name functions. */
#define ARGON_STRING_FROM_CONST(str) \
  api->string_to_argon((struct string){(char *)(uintptr_t)(str), strlen(str)})

/* ─────────────────────────────────────────────────────────────────────────────
 * Helpers
 * ───────────────────────────────────────────────────────────────────────────*/

/* Duplicate an Argon string into a NUL-terminated C string on the heap.
 * Caller is responsible for free(). */
static char *argon_string_to_cstr(struct string s) {
  char *buf = malloc(s.length + 1);
  if (!buf) return NULL;
  memcpy(buf, s.data, s.length);
  buf[s.length] = '\0';
  return buf;
}

/* Pack a Vector2 into a new Argon buffer and return it. */
static ArgonObject *vector2_to_argon(Vector2 v, ArgonNativeAPI *api,
                                     ArgonError *err) {
  ArgonHashmap *vec = api->create_hashmap();
  api->add_to_hashmap_string_key(vec, "x", api->double_to_argon(v.x));
  api->add_to_hashmap_string_key(vec, "y", api->double_to_argon(v.y));
  return api->hashmap_to_dictionary(vec);
}

/* Unpack a Vector2 from an Argon buffer. */
static Vector2 argon_to_vector2(ArgonObject *obj, ArgonNativeAPI *api,
                                ArgonError *err) {
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return (Vector2){0};
  return *(Vector2 *)b.data;
}

/* Pack a Vector3 into a new Argon buffer and return it. */
static ArgonObject *vector3_to_argon(Vector3 v, ArgonNativeAPI *api,
                                     ArgonError *err) {
  ArgonObject *obj = api->create_argon_buffer(sizeof(Vector3));
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Vector3 *)b.data = v;
  return obj;
}

static Vector3 argon_to_vector3(ArgonObject *obj, ArgonNativeAPI *api,
                                ArgonError *err) {
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return (Vector3){0};
  return *(Vector3 *)b.data;
}

/* Pack a Matrix into a new Argon buffer and return it. */
static ArgonObject *matrix_to_argon(Matrix m, ArgonNativeAPI *api,
                                    ArgonError *err) {
  ArgonObject *obj = api->create_argon_buffer(sizeof(Matrix));
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Matrix *)b.data = m;
  return obj;
}

/* Unpack a Color from an Argon buffer. */
static Color argon_to_color(ArgonObject *obj, ArgonNativeAPI *api,
                             ArgonError *err) {
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return (Color){0};
  return *(Color *)b.data;
}

/* Pack a Ray into a new Argon buffer. */
static ArgonObject *ray_to_argon(Ray r, ArgonNativeAPI *api,
                                 ArgonError *err) {
  ArgonObject *obj = api->create_argon_buffer(sizeof(Ray));
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Ray *)b.data = r;
  return obj;
}

/* Unpack a Camera (Camera3D) from an Argon buffer. */
static Camera argon_to_camera(ArgonObject *obj, ArgonNativeAPI *api,
                               ArgonError *err) {
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return (Camera){0};
  return *(Camera *)b.data;
}

/* Unpack a Camera2D from an Argon buffer. */
static Camera2D argon_to_camera2d(ArgonObject *obj, ArgonNativeAPI *api,
                                  ArgonError *err) {
  struct buffer b = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return (Camera2D){0};
  return *(Camera2D *)b.data;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Window
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(InitWindow, {
  if (api->fix_to_arg_size(3, argc, err)) return api->ARGON_NULL;
  int64_t width = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t height = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  struct string title = api->argon_to_string(argv[2], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *t = argon_string_to_cstr(title);
  InitWindow((int)width, (int)height, t);
  free(t);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(CloseWindow, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  CloseWindow();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(WindowShouldClose, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(WindowShouldClose() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowReady, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowReady() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowFullscreen, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowFullscreen() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowHidden, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowHidden() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowMinimized, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowMinimized() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowMaximized, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowMaximized() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowFocused, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowFocused() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowResized, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowResized() ? 1 : 0);
})

ARGON_FUNCTION(IsWindowState, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t flag = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsWindowState((unsigned int)flag) ? 1 : 0);
})

ARGON_FUNCTION(SetWindowState, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t flags = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowState((unsigned int)flags);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(ClearWindowState, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t flags = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  ClearWindowState((unsigned int)flags);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(ToggleFullscreen, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  ToggleFullscreen();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(ToggleBorderlessWindowed, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  ToggleBorderlessWindowed();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(MaximizeWindow, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  MaximizeWindow();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(MinimizeWindow, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  MinimizeWindow();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(RestoreWindow, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  RestoreWindow();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowTitle, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string title = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *t = argon_string_to_cstr(title);
  SetWindowTitle(t);
  free(t);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowPosition, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t x = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t y = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowPosition((int)x, (int)y);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowMonitor, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowMonitor((int)monitor);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowMinSize, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t w = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t h = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowMinSize((int)w, (int)h);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowMaxSize, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t w = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t h = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowMaxSize((int)w, (int)h);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowSize, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t w = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t h = api->argon_to_i64(argv[1], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowSize((int)w, (int)h);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowOpacity, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  double opacity = api->argon_to_double(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetWindowOpacity((float)opacity);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetWindowFocused, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  SetWindowFocused();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(GetScreenWidth, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetScreenWidth());
})

ARGON_FUNCTION(GetScreenHeight, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetScreenHeight());
})

ARGON_FUNCTION(GetRenderWidth, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetRenderWidth());
})

ARGON_FUNCTION(GetRenderHeight, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetRenderHeight());
})

ARGON_FUNCTION(GetMonitorCount, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorCount());
})

ARGON_FUNCTION(GetCurrentMonitor, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetCurrentMonitor());
})

ARGON_FUNCTION(GetMonitorPosition, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon(GetMonitorPosition((int)monitor), api, err);
})

ARGON_FUNCTION(GetMonitorWidth, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorWidth((int)monitor));
})

ARGON_FUNCTION(GetMonitorHeight, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorHeight((int)monitor));
})

ARGON_FUNCTION(GetMonitorPhysicalWidth, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorPhysicalWidth((int)monitor));
})

ARGON_FUNCTION(GetMonitorPhysicalHeight, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorPhysicalHeight((int)monitor));
})

ARGON_FUNCTION(GetMonitorRefreshRate, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMonitorRefreshRate((int)monitor));
})

ARGON_FUNCTION(GetWindowPosition, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetWindowPosition(), api, err);
})

ARGON_FUNCTION(GetWindowScaleDPI, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetWindowScaleDPI(), api, err);
})

ARGON_FUNCTION(GetMonitorName, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t monitor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  const char *name = GetMonitorName((int)monitor);
  return ARGON_STRING_FROM_CONST(name ? name : "");
})

ARGON_FUNCTION(SetClipboardText, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  SetClipboardText(c);
  free(c);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(GetClipboardText, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  const char *text = GetClipboardText();
  if (!text) return ARGON_STRING_FROM_C_STRING("");
  return ARGON_STRING_FROM_CONST(text);
})

ARGON_FUNCTION(EnableEventWaiting, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EnableEventWaiting();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(DisableEventWaiting, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  DisableEventWaiting();
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Cursor
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(ShowCursor, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  ShowCursor();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(HideCursor, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  HideCursor();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(IsCursorHidden, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsCursorHidden() ? 1 : 0);
})

ARGON_FUNCTION(EnableCursor, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EnableCursor();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(DisableCursor, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  DisableCursor();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(IsCursorOnScreen, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsCursorOnScreen() ? 1 : 0);
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Drawing
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(ClearBackground, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Color c = argon_to_color(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  ClearBackground(c);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(BeginDrawing, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  BeginDrawing();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(EndDrawing, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EndDrawing();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(BeginMode2D, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Camera2D cam = argon_to_camera2d(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  BeginMode2D(cam);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(EndMode2D, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EndMode2D();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(BeginMode3D, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct buffer b = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  BeginMode3D(*(Camera3D *)b.data);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(EndMode3D, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EndMode3D();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(BeginScissorMode, {
  if (api->fix_to_arg_size(4, argc, err)) return api->ARGON_NULL;
  int64_t x = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t y = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t w = api->argon_to_i64(argv[2], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t h = api->argon_to_i64(argv[3], err); if (api->is_error(err)) return api->ARGON_NULL;
  BeginScissorMode((int)x, (int)y, (int)w, (int)h);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(EndScissorMode, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  EndScissorMode();
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Timing
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(SetTargetFPS, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t fps = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetTargetFPS((int)fps);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(GetFrameTime, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetFrameTime());
})

ARGON_FUNCTION(GetTime, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon(GetTime());
})

ARGON_FUNCTION(GetFPS, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetFPS());
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Frame control
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(SwapScreenBuffer, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  SwapScreenBuffer();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(PollInputEvents, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  PollInputEvents();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(WaitTime, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  double seconds = api->argon_to_double(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  WaitTime(seconds);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Random
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(SetRandomSeed, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t seed = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetRandomSeed((unsigned int)seed);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(GetRandomValue, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t mn = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t mx = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetRandomValue((int)mn, (int)mx));
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Misc
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(TakeScreenshot, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  TakeScreenshot(c);
  free(c);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetConfigFlags, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t flags = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetConfigFlags((unsigned int)flags);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(OpenURL, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  OpenURL(c);
  free(c);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetTraceLogLevel, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t level = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetTraceLogLevel((int)level);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * File system
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(FileExists, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = FileExists(c);
  free(c);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(DirectoryExists, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = DirectoryExists(c);
  free(c);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(IsFileExtension, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct string fname = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  struct string ext   = api->argon_to_string(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  char *cf = argon_string_to_cstr(fname);
  char *ce = argon_string_to_cstr(ext);
  int result = IsFileExtension(cf, ce);
  free(cf); free(ce);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(GetFileLength, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = GetFileLength(c);
  free(c);
  return api->i64_to_argon(result);
})

ARGON_FUNCTION(GetFileModTime, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  long result = GetFileModTime(c);
  free(c);
  return api->i64_to_argon((int64_t)result);
})

ARGON_FUNCTION(GetFileExtension, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  const char *ext = GetFileExtension(c);
  ArgonObject *result = ARGON_STRING_FROM_CONST(ext ? ext : "");
  free(c);
  return result;
})

ARGON_FUNCTION(GetFileName, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  const char *name = GetFileName(c);
  ArgonObject *result = ARGON_STRING_FROM_CONST(name ? name : "");
  free(c);
  return result;
})

ARGON_FUNCTION(GetFileNameWithoutExt, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  const char *name = GetFileNameWithoutExt(c);
  ArgonObject *result = ARGON_STRING_FROM_CONST(name ? name : "");
  free(c);
  return result;
})

ARGON_FUNCTION(GetDirectoryPath, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  const char *path = GetDirectoryPath(c);
  ArgonObject *result = ARGON_STRING_FROM_CONST(path ? path : "");
  free(c);
  return result;
})

ARGON_FUNCTION(GetPrevDirectoryPath, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  const char *path = GetPrevDirectoryPath(c);
  ArgonObject *result = ARGON_STRING_FROM_CONST(path ? path : "");
  free(c);
  return result;
})

ARGON_FUNCTION(GetWorkingDirectory, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  const char *wd = GetWorkingDirectory();
  return ARGON_STRING_FROM_CONST(wd ? wd : "");
})

ARGON_FUNCTION(GetApplicationDirectory, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  const char *ad = GetApplicationDirectory();
  return ARGON_STRING_FROM_CONST(ad ? ad : "");
})

ARGON_FUNCTION(MakeDirectory, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = MakeDirectory(c);
  free(c);
  return api->i64_to_argon(result);
})

ARGON_FUNCTION(ChangeDirectory, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = ChangeDirectory(c);
  free(c);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(IsPathFile, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = IsPathFile(c);
  free(c);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(IsFileNameValid, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  int result = IsFileNameValid(c);
  free(c);
  return api->i64_to_argon(result ? 1 : 0);
})

/* LoadDirectoryFiles / LoadDroppedFiles: returns an Argon array of strings. */
static ArgonObject *file_path_list_to_argon_array(FilePathList list,
                                                   ArgonNativeAPI *api,
                                                   ArgonError *err) {
  ArgonHashmap *map = api->create_hashmap();
  api->add_to_hashmap_string_key(map, "count",
                                 api->i64_to_argon((int64_t)list.count));
  for (unsigned int i = 0; i < list.count; i++) {
    char key[32];
    snprintf(key, sizeof(key), "%u", i);
    api->add_to_hashmap_string_key(map, key,
                                   ARGON_STRING_FROM_CONST(list.paths[i]));
  }
  return api->hashmap_to_dictionary(map);
}

ARGON_FUNCTION(LoadDirectoryFiles, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  FilePathList list = LoadDirectoryFiles(c);
  free(c);
  ArgonObject *result = file_path_list_to_argon_array(list, api, err);
  UnloadDirectoryFiles(list);
  return result;
})

ARGON_FUNCTION(IsFileDropped, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsFileDropped() ? 1 : 0);
})

ARGON_FUNCTION(LoadDroppedFiles, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  FilePathList list = LoadDroppedFiles();
  ArgonObject *result = file_path_list_to_argon_array(list, api, err);
  UnloadDroppedFiles(list);
  return result;
})

ARGON_FUNCTION(GetDirectoryFileCount, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
  unsigned int count = GetDirectoryFileCount(c);
  free(c);
  return api->i64_to_argon((int64_t)count);
#else
  free(c);
  api->throw_argon_error(err, api->RuntimeError,
                         "GetDirectoryFileCount requires raylib >= 5.1");
  return api->ARGON_NULL;
#endif
})

ARGON_FUNCTION(SaveFileText, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct string path = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  struct string text = api->argon_to_string(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  char *cp = argon_string_to_cstr(path);
  char *ct = argon_string_to_cstr(text);
  bool result = SaveFileText(cp, ct);
  free(cp); free(ct);
  return api->i64_to_argon(result ? 1 : 0);
})

ARGON_FUNCTION(LoadFileText, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
  char *text = LoadFileText(c);
  free(c);
  if (!text) return ARGON_STRING_FROM_C_STRING("");
  ArgonObject *result = ARGON_STRING_FROM_C_STRING(text);
  UnloadFileText(text);
  return result;
})

ARGON_FUNCTION(FileExists2, { /* alias — already done above */ return api->ARGON_NULL; })

ARGON_FUNCTION(FileRename, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct string a = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  struct string b = api->argon_to_string(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  char *ca = argon_string_to_cstr(a);
  char *cb = argon_string_to_cstr(b);
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
  int result = FileRename(ca, cb);
  free(ca); free(cb);
  return api->i64_to_argon(result);
#else
  free(ca); free(cb);
  api->throw_argon_error(err, api->RuntimeError, "FileRename requires raylib >= 5.1");
  return api->ARGON_NULL;
#endif
})

ARGON_FUNCTION(FileRemove, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *c = argon_string_to_cstr(s);
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
  int result = FileRemove(c);
  free(c);
  return api->i64_to_argon(result);
#else
  free(c);
  api->throw_argon_error(err, api->RuntimeError, "FileRemove requires raylib >= 5.1");
  return api->ARGON_NULL;
#endif
})

ARGON_FUNCTION(FileCopy, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct string a = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  struct string b = api->argon_to_string(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  char *ca = argon_string_to_cstr(a);
  char *cb = argon_string_to_cstr(b);
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
  int result = FileCopy(ca, cb);
  free(ca); free(cb);
  return api->i64_to_argon(result);
#else
  free(ca); free(cb);
  api->throw_argon_error(err, api->RuntimeError, "FileCopy requires raylib >= 5.1");
  return api->ARGON_NULL;
#endif
})

ARGON_FUNCTION(FileMove, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct string a = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  struct string b = api->argon_to_string(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  char *ca = argon_string_to_cstr(a);
  char *cb = argon_string_to_cstr(b);
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
  int result = FileMove(ca, cb);
  free(ca); free(cb);
  return api->i64_to_argon(result);
#else
  free(ca); free(cb);
  api->throw_argon_error(err, api->RuntimeError, "FileMove requires raylib >= 5.1");
  return api->ARGON_NULL;
#endif
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Compression / Encoding
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(CompressData, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct buffer input = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int compSize = 0;
  unsigned char *comp = CompressData((const unsigned char *)input.data,
                                     (int)input.size, &compSize);
  ArgonObject *out = api->create_argon_buffer((size_t)compSize);
  struct buffer ob = api->argon_buffer_to_buffer(out, err);
  if (api->is_error(err)) { MemFree(comp); return api->ARGON_NULL; }
  memcpy(ob.data, comp, (size_t)compSize);
  MemFree(comp);
  return out;
})

ARGON_FUNCTION(DecompressData, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct buffer input = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int outSize = 0;
  unsigned char *data = DecompressData((const unsigned char *)input.data,
                                       (int)input.size, &outSize);
  ArgonObject *out = api->create_argon_buffer((size_t)outSize);
  struct buffer ob = api->argon_buffer_to_buffer(out, err);
  if (api->is_error(err)) { MemFree(data); return api->ARGON_NULL; }
  memcpy(ob.data, data, (size_t)outSize);
  MemFree(data);
  return out;
})

ARGON_FUNCTION(EncodeDataBase64, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct buffer input = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  int outSize = 0;
  char *encoded = EncodeDataBase64((const unsigned char *)input.data,
                                   (int)input.size, &outSize);
  /* outSize includes the NUL terminator. */
  struct string s = {encoded, (size_t)(outSize > 0 ? outSize - 1 : 0)};
  ArgonObject *result = api->string_to_argon(s);
  MemFree(encoded);
  return result;
})

ARGON_FUNCTION(DecodeDataBase64, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct string s = api->argon_to_string(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  char *cs = argon_string_to_cstr(s);
  int outSize = 0;
  unsigned char *data = DecodeDataBase64(cs, &outSize);
  free(cs);
  ArgonObject *out = api->create_argon_buffer((size_t)outSize);
  struct buffer ob = api->argon_buffer_to_buffer(out, err);
  if (api->is_error(err)) { MemFree(data); return api->ARGON_NULL; }
  memcpy(ob.data, data, (size_t)outSize);
  MemFree(data);
  return out;
})

ARGON_FUNCTION(ComputeCRC32, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  struct buffer b = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  unsigned int crc = ComputeCRC32((unsigned char *)b.data, (int)b.size);
  return api->i64_to_argon((int64_t)crc);
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Screen-space
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(GetScreenToWorldRay, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  Vector2 pos = argon_to_vector2(argv[0], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  Camera cam = argon_to_camera(argv[1], api, err);  if (api->is_error(err)) return api->ARGON_NULL;
  return ray_to_argon(GetScreenToWorldRay(pos, cam), api, err);
})

ARGON_FUNCTION(GetWorldToScreen, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  Vector3 pos = argon_to_vector3(argv[0], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  Camera cam = argon_to_camera(argv[1], api, err);   if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon(GetWorldToScreen(pos, cam), api, err);
})

ARGON_FUNCTION(GetWorldToScreen2D, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  Vector2 pos  = argon_to_vector2(argv[0], api, err);  if (api->is_error(err)) return api->ARGON_NULL;
  Camera2D cam = argon_to_camera2d(argv[1], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon(GetWorldToScreen2D(pos, cam), api, err);
})

ARGON_FUNCTION(GetScreenToWorld2D, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  Vector2 pos  = argon_to_vector2(argv[0], api, err);  if (api->is_error(err)) return api->ARGON_NULL;
  Camera2D cam = argon_to_camera2d(argv[1], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon(GetScreenToWorld2D(pos, cam), api, err);
})

ARGON_FUNCTION(GetCameraMatrix, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Camera cam = argon_to_camera(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return matrix_to_argon(GetCameraMatrix(cam), api, err);
})

ARGON_FUNCTION(GetCameraMatrix2D, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Camera2D cam = argon_to_camera2d(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return matrix_to_argon(GetCameraMatrix2D(cam), api, err);
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Camera update
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(UpdateCamera, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  struct buffer b = api->argon_buffer_to_buffer(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t mode = api->argon_to_i64(argv[1], err);              if (api->is_error(err)) return api->ARGON_NULL;
  UpdateCamera((Camera *)b.data, (int)mode);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(UpdateCameraPro, {
  if (api->fix_to_arg_size(4, argc, err)) return api->ARGON_NULL;
  struct buffer cb = api->argon_buffer_to_buffer(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  Vector3 movement = argon_to_vector3(argv[1], api, err);       if (api->is_error(err)) return api->ARGON_NULL;
  Vector3 rotation = argon_to_vector3(argv[2], api, err);       if (api->is_error(err)) return api->ARGON_NULL;
  double zoom      = api->argon_to_double(argv[3], err);        if (api->is_error(err)) return api->ARGON_NULL;
  UpdateCameraPro((Camera *)cb.data, movement, rotation, (float)zoom);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Keyboard input
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(IsKeyPressed, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsKeyPressed((int)key) ? 1 : 0);
})

ARGON_FUNCTION(IsKeyPressedRepeat, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsKeyPressedRepeat((int)key) ? 1 : 0);
})

ARGON_FUNCTION(IsKeyDown, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsKeyDown((int)key) ? 1 : 0);
})

ARGON_FUNCTION(IsKeyReleased, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsKeyReleased((int)key) ? 1 : 0);
})

ARGON_FUNCTION(IsKeyUp, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsKeyUp((int)key) ? 1 : 0);
})

ARGON_FUNCTION(GetKeyPressed, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetKeyPressed());
})

ARGON_FUNCTION(GetCharPressed, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetCharPressed());
})

ARGON_FUNCTION(GetKeyName, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  const char *name = GetKeyName((int)key);
  return ARGON_STRING_FROM_C_STRING(name ? name : "");
})

ARGON_FUNCTION(SetExitKey, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t key = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetExitKey((int)key);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Gamepad input
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(IsGamepadAvailable, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGamepadAvailable((int)gp) ? 1 : 0);
})

ARGON_FUNCTION(GetGamepadName, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  const char *name = GetGamepadName((int)gp);
  return ARGON_STRING_FROM_C_STRING(name ? name : "");
})

ARGON_FUNCTION(IsGamepadButtonPressed, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t bt = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGamepadButtonPressed((int)gp, (int)bt) ? 1 : 0);
})

ARGON_FUNCTION(IsGamepadButtonDown, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t bt = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGamepadButtonDown((int)gp, (int)bt) ? 1 : 0);
})

ARGON_FUNCTION(IsGamepadButtonReleased, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t bt = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGamepadButtonReleased((int)gp, (int)bt) ? 1 : 0);
})

ARGON_FUNCTION(IsGamepadButtonUp, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t bt = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGamepadButtonUp((int)gp, (int)bt) ? 1 : 0);
})

ARGON_FUNCTION(GetGamepadButtonPressed, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetGamepadButtonPressed());
})

ARGON_FUNCTION(GetGamepadAxisCount, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t gp = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetGamepadAxisCount((int)gp));
})

ARGON_FUNCTION(GetGamepadAxisMovement, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t gp   = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t axis = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetGamepadAxisMovement((int)gp, (int)axis));
})

ARGON_FUNCTION(SetGamepadVibration, {
  if (api->fix_to_arg_size(4, argc, err)) return api->ARGON_NULL;
  int64_t gp   = api->argon_to_i64(argv[0], err);    if (api->is_error(err)) return api->ARGON_NULL;
  double left  = api->argon_to_double(argv[1], err);  if (api->is_error(err)) return api->ARGON_NULL;
  double right = api->argon_to_double(argv[2], err);  if (api->is_error(err)) return api->ARGON_NULL;
  double dur   = api->argon_to_double(argv[3], err);  if (api->is_error(err)) return api->ARGON_NULL;
  SetGamepadVibration((int)gp, (float)left, (float)right, (float)dur);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Mouse input
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(IsMouseButtonPressed, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t btn = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsMouseButtonPressed((int)btn) ? 1 : 0);
})

ARGON_FUNCTION(IsMouseButtonDown, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t btn = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsMouseButtonDown((int)btn) ? 1 : 0);
})

ARGON_FUNCTION(IsMouseButtonReleased, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t btn = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsMouseButtonReleased((int)btn) ? 1 : 0);
})

ARGON_FUNCTION(IsMouseButtonUp, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t btn = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsMouseButtonUp((int)btn) ? 1 : 0);
})

ARGON_FUNCTION(GetMouseX, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMouseX());
})

ARGON_FUNCTION(GetMouseY, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetMouseY());
})

ARGON_FUNCTION(GetMousePosition, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetMousePosition(), api, err);
})

ARGON_FUNCTION(GetMouseDelta, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetMouseDelta(), api, err);
})

ARGON_FUNCTION(SetMousePosition, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t x = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t y = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  SetMousePosition((int)x, (int)y);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetMouseOffset, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  int64_t ox = api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t oy = api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  SetMouseOffset((int)ox, (int)oy);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(SetMouseScale, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  double sx = api->argon_to_double(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  double sy = api->argon_to_double(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  SetMouseScale((float)sx, (float)sy);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(GetMouseWheelMove, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetMouseWheelMove());
})

ARGON_FUNCTION(GetMouseWheelMoveV, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetMouseWheelMoveV(), api, err);
})

ARGON_FUNCTION(SetMouseCursor, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t cursor = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetMouseCursor((int)cursor);
  return api->ARGON_NULL;
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Touch input
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(GetTouchX, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetTouchX());
})

ARGON_FUNCTION(GetTouchY, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetTouchY());
})

ARGON_FUNCTION(GetTouchPosition, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t index = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon(GetTouchPosition((int)index), api, err);
})

ARGON_FUNCTION(GetTouchPointId, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t index = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetTouchPointId((int)index));
})

ARGON_FUNCTION(GetTouchPointCount, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetTouchPointCount());
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Gestures
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(SetGesturesEnabled, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t flags = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  SetGesturesEnabled((unsigned int)flags);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(IsGestureDetected, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  int64_t gesture = api->argon_to_i64(argv[0], err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->i64_to_argon(IsGestureDetected((unsigned int)gesture) ? 1 : 0);
})

ARGON_FUNCTION(GetGestureDetected, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->i64_to_argon(GetGestureDetected());
})

ARGON_FUNCTION(GetGestureHoldDuration, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetGestureHoldDuration());
})

ARGON_FUNCTION(GetGestureDragVector, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetGestureDragVector(), api, err);
})

ARGON_FUNCTION(GetGestureDragAngle, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetGestureDragAngle());
})

ARGON_FUNCTION(GetGesturePinchVector, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return vector2_to_argon(GetGesturePinchVector(), api, err);
})

ARGON_FUNCTION(GetGesturePinchAngle, {
  if (api->fix_to_arg_size(0, argc, err)) return api->ARGON_NULL;
  return api->double_to_argon((double)GetGesturePinchAngle());
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Color constructor & DrawText (kept from original)
 * ───────────────────────────────────────────────────────────────────────────*/

ARGON_FUNCTION(Color, {
  if (api->fix_to_arg_size(4, argc, err)) return api->ARGON_NULL;
  Color c;
  c.r = (unsigned char)api->argon_to_i64(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  c.g = (unsigned char)api->argon_to_i64(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  c.b = (unsigned char)api->argon_to_i64(argv[2], err); if (api->is_error(err)) return api->ARGON_NULL;
  c.a = (unsigned char)api->argon_to_i64(argv[3], err); if (api->is_error(err)) return api->ARGON_NULL;
  ArgonObject *obj = api->create_argon_buffer(sizeof(Color));
  struct buffer b  = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Color *)b.data = c;
  return obj;
})

ARGON_FUNCTION(DrawText, {
  if (api->fix_to_arg_size(5, argc, err)) return api->ARGON_NULL;
  struct string text = api->argon_to_string(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  int64_t posX     = api->argon_to_i64(argv[1], err);     if (api->is_error(err)) return api->ARGON_NULL;
  int64_t posY     = api->argon_to_i64(argv[2], err);     if (api->is_error(err)) return api->ARGON_NULL;
  int64_t fontSize = api->argon_to_i64(argv[3], err);     if (api->is_error(err)) return api->ARGON_NULL;
  Color color      = argon_to_color(argv[4], api, err);   if (api->is_error(err)) return api->ARGON_NULL;
  char *ct = argon_string_to_cstr(text);
  DrawText(ct, (int)posX, (int)posY, (int)fontSize, color);
  free(ct);
  return api->ARGON_NULL;
})

/* Vector2 / Vector3 / Camera constructors — convenience for Argon users */

ARGON_FUNCTION(Vector2, {
  if (api->fix_to_arg_size(2, argc, err)) return api->ARGON_NULL;
  double x = api->argon_to_double(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  double y = api->argon_to_double(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  return vector2_to_argon((Vector2){(float)x, (float)y}, api, err);
})

ARGON_FUNCTION(Vector3, {
  if (api->fix_to_arg_size(3, argc, err)) return api->ARGON_NULL;
  double x = api->argon_to_double(argv[0], err); if (api->is_error(err)) return api->ARGON_NULL;
  double y = api->argon_to_double(argv[1], err); if (api->is_error(err)) return api->ARGON_NULL;
  double z = api->argon_to_double(argv[2], err); if (api->is_error(err)) return api->ARGON_NULL;
  return vector3_to_argon((Vector3){(float)x, (float)y, (float)z}, api, err);
})

/* Camera3D(position, target, up, fovy, projection) */
ARGON_FUNCTION(Camera3D, {
  if (api->fix_to_arg_size(5, argc, err)) return api->ARGON_NULL;
  Vector3 position   = argon_to_vector3(argv[0], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  Vector3 target     = argon_to_vector3(argv[1], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  Vector3 up         = argon_to_vector3(argv[2], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  double fovy        = api->argon_to_double(argv[3], err);  if (api->is_error(err)) return api->ARGON_NULL;
  int64_t projection = api->argon_to_i64(argv[4], err);    if (api->is_error(err)) return api->ARGON_NULL;
  Camera3D cam = {position, target, up, (float)fovy, (int)projection};
  ArgonObject *obj = api->create_argon_buffer(sizeof(Camera3D));
  struct buffer b  = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Camera3D *)b.data = cam;
  return obj;
})

/* Camera2D(offset, target, rotation, zoom) */
ARGON_FUNCTION(Camera2D, {
  if (api->fix_to_arg_size(4, argc, err)) return api->ARGON_NULL;
  Vector2 offset = argon_to_vector2(argv[0], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  Vector2 target = argon_to_vector2(argv[1], api, err); if (api->is_error(err)) return api->ARGON_NULL;
  double rotation = api->argon_to_double(argv[2], err); if (api->is_error(err)) return api->ARGON_NULL;
  double zoom     = api->argon_to_double(argv[3], err); if (api->is_error(err)) return api->ARGON_NULL;
  Camera2D cam = {offset, target, (float)rotation, (float)zoom};
  ArgonObject *obj = api->create_argon_buffer(sizeof(Camera2D));
  struct buffer b  = api->argon_buffer_to_buffer(obj, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  *(Camera2D *)b.data = cam;
  return obj;
})

/* Vector2 field accessors */
ARGON_FUNCTION(Vector2GetX, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Vector2 v = argon_to_vector2(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)v.x);
})

ARGON_FUNCTION(Vector2GetY, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Vector2 v = argon_to_vector2(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)v.y);
})

/* Vector3 field accessors */
ARGON_FUNCTION(Vector3GetX, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Vector3 v = argon_to_vector3(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)v.x);
})

ARGON_FUNCTION(Vector3GetY, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Vector3 v = argon_to_vector3(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)v.y);
})

ARGON_FUNCTION(Vector3GetZ, {
  if (api->fix_to_arg_size(1, argc, err)) return api->ARGON_NULL;
  Vector3 v = argon_to_vector3(argv[0], api, err);
  if (api->is_error(err)) return api->ARGON_NULL;
  return api->double_to_argon((double)v.z);
})

/* ─────────────────────────────────────────────────────────────────────────────
 * Module init
 * ───────────────────────────────────────────────────────────────────────────*/

INIT_ARGON_MODULE({
  /* ── Predefined colors ── */
  typedef struct { char *name; Color color; } NamedColor;
  NamedColor colors[] = {
    {"LIGHTGRAY", LIGHTGRAY}, {"GRAY", GRAY},       {"DARKGRAY", DARKGRAY},
    {"YELLOW", YELLOW},       {"GOLD", GOLD},         {"ORANGE", ORANGE},
    {"PINK", PINK},           {"RED", RED},            {"MAROON", MAROON},
    {"GREEN", GREEN},         {"LIME", LIME},          {"DARKGREEN", DARKGREEN},
    {"SKYBLUE", SKYBLUE},     {"BLUE", BLUE},          {"DARKBLUE", DARKBLUE},
    {"PURPLE", PURPLE},       {"VIOLET", VIOLET},      {"DARKPURPLE", DARKPURPLE},
    {"BEIGE", BEIGE},         {"BROWN", BROWN},        {"DARKBROWN", DARKBROWN},
    {"WHITE", WHITE},         {"BLACK", BLACK},        {"BLANK", BLANK},
    {"MAGENTA", MAGENTA},     {"RAYWHITE", RAYWHITE},
  };
  int numColors = (int)(sizeof(colors) / sizeof(colors[0]));
  for (int i = 0; i < numColors; i++) {
    ArgonObject *obj = api->create_argon_buffer(sizeof(Color));
    struct buffer b  = api->argon_buffer_to_buffer(obj, err);
    if (api->is_error(err)) return;
    memcpy(b.data, &colors[i].color, sizeof(Color));
    api->register_ArgonObject(reg, colors[i].name, obj);
  }

  /* ── Window ── */
  REGISTER_ARGON_FUNCTION(InitWindow)
  REGISTER_ARGON_FUNCTION(CloseWindow)
  REGISTER_ARGON_FUNCTION(WindowShouldClose)
  REGISTER_ARGON_FUNCTION(IsWindowReady)
  REGISTER_ARGON_FUNCTION(IsWindowFullscreen)
  REGISTER_ARGON_FUNCTION(IsWindowHidden)
  REGISTER_ARGON_FUNCTION(IsWindowMinimized)
  REGISTER_ARGON_FUNCTION(IsWindowMaximized)
  REGISTER_ARGON_FUNCTION(IsWindowFocused)
  REGISTER_ARGON_FUNCTION(IsWindowResized)
  REGISTER_ARGON_FUNCTION(IsWindowState)
  REGISTER_ARGON_FUNCTION(SetWindowState)
  REGISTER_ARGON_FUNCTION(ClearWindowState)
  REGISTER_ARGON_FUNCTION(ToggleFullscreen)
  REGISTER_ARGON_FUNCTION(ToggleBorderlessWindowed)
  REGISTER_ARGON_FUNCTION(MaximizeWindow)
  REGISTER_ARGON_FUNCTION(MinimizeWindow)
  REGISTER_ARGON_FUNCTION(RestoreWindow)
  REGISTER_ARGON_FUNCTION(SetWindowTitle)
  REGISTER_ARGON_FUNCTION(SetWindowPosition)
  REGISTER_ARGON_FUNCTION(SetWindowMonitor)
  REGISTER_ARGON_FUNCTION(SetWindowMinSize)
  REGISTER_ARGON_FUNCTION(SetWindowMaxSize)
  REGISTER_ARGON_FUNCTION(SetWindowSize)
  REGISTER_ARGON_FUNCTION(SetWindowOpacity)
  REGISTER_ARGON_FUNCTION(SetWindowFocused)
  REGISTER_ARGON_FUNCTION(GetScreenWidth)
  REGISTER_ARGON_FUNCTION(GetScreenHeight)
  REGISTER_ARGON_FUNCTION(GetRenderWidth)
  REGISTER_ARGON_FUNCTION(GetRenderHeight)
  REGISTER_ARGON_FUNCTION(GetMonitorCount)
  REGISTER_ARGON_FUNCTION(GetCurrentMonitor)
  REGISTER_ARGON_FUNCTION(GetMonitorPosition)
  REGISTER_ARGON_FUNCTION(GetMonitorWidth)
  REGISTER_ARGON_FUNCTION(GetMonitorHeight)
  REGISTER_ARGON_FUNCTION(GetMonitorPhysicalWidth)
  REGISTER_ARGON_FUNCTION(GetMonitorPhysicalHeight)
  REGISTER_ARGON_FUNCTION(GetMonitorRefreshRate)
  REGISTER_ARGON_FUNCTION(GetWindowPosition)
  REGISTER_ARGON_FUNCTION(GetWindowScaleDPI)
  REGISTER_ARGON_FUNCTION(GetMonitorName)
  REGISTER_ARGON_FUNCTION(SetClipboardText)
  REGISTER_ARGON_FUNCTION(GetClipboardText)
  REGISTER_ARGON_FUNCTION(EnableEventWaiting)
  REGISTER_ARGON_FUNCTION(DisableEventWaiting)

  /* ── Cursor ── */
  REGISTER_ARGON_FUNCTION(ShowCursor)
  REGISTER_ARGON_FUNCTION(HideCursor)
  REGISTER_ARGON_FUNCTION(IsCursorHidden)
  REGISTER_ARGON_FUNCTION(EnableCursor)
  REGISTER_ARGON_FUNCTION(DisableCursor)
  REGISTER_ARGON_FUNCTION(IsCursorOnScreen)

  /* ── Drawing ── */
  REGISTER_ARGON_FUNCTION(ClearBackground)
  REGISTER_ARGON_FUNCTION(BeginDrawing)
  REGISTER_ARGON_FUNCTION(EndDrawing)
  REGISTER_ARGON_FUNCTION(BeginMode2D)
  REGISTER_ARGON_FUNCTION(EndMode2D)
  REGISTER_ARGON_FUNCTION(BeginMode3D)
  REGISTER_ARGON_FUNCTION(EndMode3D)
  REGISTER_ARGON_FUNCTION(BeginScissorMode)
  REGISTER_ARGON_FUNCTION(EndScissorMode)

  /* ── Timing ── */
  REGISTER_ARGON_FUNCTION(SetTargetFPS)
  REGISTER_ARGON_FUNCTION(GetFrameTime)
  REGISTER_ARGON_FUNCTION(GetTime)
  REGISTER_ARGON_FUNCTION(GetFPS)

  /* ── Frame control ── */
  REGISTER_ARGON_FUNCTION(SwapScreenBuffer)
  REGISTER_ARGON_FUNCTION(PollInputEvents)
  REGISTER_ARGON_FUNCTION(WaitTime)

  /* ── Random ── */
  REGISTER_ARGON_FUNCTION(SetRandomSeed)
  REGISTER_ARGON_FUNCTION(GetRandomValue)

  /* ── Misc ── */
  REGISTER_ARGON_FUNCTION(TakeScreenshot)
  REGISTER_ARGON_FUNCTION(SetConfigFlags)
  REGISTER_ARGON_FUNCTION(OpenURL)
  REGISTER_ARGON_FUNCTION(SetTraceLogLevel)

  /* ── File system ── */
  REGISTER_ARGON_FUNCTION(FileExists)
  REGISTER_ARGON_FUNCTION(DirectoryExists)
  REGISTER_ARGON_FUNCTION(IsFileExtension)
  REGISTER_ARGON_FUNCTION(GetFileLength)
  REGISTER_ARGON_FUNCTION(GetFileModTime)
  REGISTER_ARGON_FUNCTION(GetFileExtension)
  REGISTER_ARGON_FUNCTION(GetFileName)
  REGISTER_ARGON_FUNCTION(GetFileNameWithoutExt)
  REGISTER_ARGON_FUNCTION(GetDirectoryPath)
  REGISTER_ARGON_FUNCTION(GetPrevDirectoryPath)
  REGISTER_ARGON_FUNCTION(GetWorkingDirectory)
  REGISTER_ARGON_FUNCTION(GetApplicationDirectory)
  REGISTER_ARGON_FUNCTION(MakeDirectory)
  REGISTER_ARGON_FUNCTION(ChangeDirectory)
  REGISTER_ARGON_FUNCTION(IsPathFile)
  REGISTER_ARGON_FUNCTION(IsFileNameValid)
  REGISTER_ARGON_FUNCTION(LoadDirectoryFiles)
  REGISTER_ARGON_FUNCTION(IsFileDropped)
  REGISTER_ARGON_FUNCTION(LoadDroppedFiles)
  REGISTER_ARGON_FUNCTION(GetDirectoryFileCount)
  REGISTER_ARGON_FUNCTION(SaveFileText)
  REGISTER_ARGON_FUNCTION(LoadFileText)
  REGISTER_ARGON_FUNCTION(FileRename)
  REGISTER_ARGON_FUNCTION(FileRemove)
  REGISTER_ARGON_FUNCTION(FileCopy)
  REGISTER_ARGON_FUNCTION(FileMove)

  /* ── Compression / Encoding ── */
  REGISTER_ARGON_FUNCTION(CompressData)
  REGISTER_ARGON_FUNCTION(DecompressData)
  REGISTER_ARGON_FUNCTION(EncodeDataBase64)
  REGISTER_ARGON_FUNCTION(DecodeDataBase64)
  REGISTER_ARGON_FUNCTION(ComputeCRC32)

  /* ── Screen-space ── */
  REGISTER_ARGON_FUNCTION(GetScreenToWorldRay)
  REGISTER_ARGON_FUNCTION(GetWorldToScreen)
  REGISTER_ARGON_FUNCTION(GetWorldToScreen2D)
  REGISTER_ARGON_FUNCTION(GetScreenToWorld2D)
  REGISTER_ARGON_FUNCTION(GetCameraMatrix)
  REGISTER_ARGON_FUNCTION(GetCameraMatrix2D)

  /* ── Camera update ── */
  REGISTER_ARGON_FUNCTION(UpdateCamera)
  REGISTER_ARGON_FUNCTION(UpdateCameraPro)

  /* ── Keyboard ── */
  REGISTER_ARGON_FUNCTION(IsKeyPressed)
  REGISTER_ARGON_FUNCTION(IsKeyPressedRepeat)
  REGISTER_ARGON_FUNCTION(IsKeyDown)
  REGISTER_ARGON_FUNCTION(IsKeyReleased)
  REGISTER_ARGON_FUNCTION(IsKeyUp)
  REGISTER_ARGON_FUNCTION(GetKeyPressed)
  REGISTER_ARGON_FUNCTION(GetCharPressed)
  REGISTER_ARGON_FUNCTION(GetKeyName)
  REGISTER_ARGON_FUNCTION(SetExitKey)

  /* ── Gamepad ── */
  REGISTER_ARGON_FUNCTION(IsGamepadAvailable)
  REGISTER_ARGON_FUNCTION(GetGamepadName)
  REGISTER_ARGON_FUNCTION(IsGamepadButtonPressed)
  REGISTER_ARGON_FUNCTION(IsGamepadButtonDown)
  REGISTER_ARGON_FUNCTION(IsGamepadButtonReleased)
  REGISTER_ARGON_FUNCTION(IsGamepadButtonUp)
  REGISTER_ARGON_FUNCTION(GetGamepadButtonPressed)
  REGISTER_ARGON_FUNCTION(GetGamepadAxisCount)
  REGISTER_ARGON_FUNCTION(GetGamepadAxisMovement)
  REGISTER_ARGON_FUNCTION(SetGamepadVibration)

  /* ── Mouse ── */
  REGISTER_ARGON_FUNCTION(IsMouseButtonPressed)
  REGISTER_ARGON_FUNCTION(IsMouseButtonDown)
  REGISTER_ARGON_FUNCTION(IsMouseButtonReleased)
  REGISTER_ARGON_FUNCTION(IsMouseButtonUp)
  REGISTER_ARGON_FUNCTION(GetMouseX)
  REGISTER_ARGON_FUNCTION(GetMouseY)
  REGISTER_ARGON_FUNCTION(GetMousePosition)
  REGISTER_ARGON_FUNCTION(GetMouseDelta)
  REGISTER_ARGON_FUNCTION(SetMousePosition)
  REGISTER_ARGON_FUNCTION(SetMouseOffset)
  REGISTER_ARGON_FUNCTION(SetMouseScale)
  REGISTER_ARGON_FUNCTION(GetMouseWheelMove)
  REGISTER_ARGON_FUNCTION(GetMouseWheelMoveV)
  REGISTER_ARGON_FUNCTION(SetMouseCursor)

  /* ── Touch ── */
  REGISTER_ARGON_FUNCTION(GetTouchX)
  REGISTER_ARGON_FUNCTION(GetTouchY)
  REGISTER_ARGON_FUNCTION(GetTouchPosition)
  REGISTER_ARGON_FUNCTION(GetTouchPointId)
  REGISTER_ARGON_FUNCTION(GetTouchPointCount)

  /* ── Gestures ── */
  REGISTER_ARGON_FUNCTION(SetGesturesEnabled)
  REGISTER_ARGON_FUNCTION(IsGestureDetected)
  REGISTER_ARGON_FUNCTION(GetGestureDetected)
  REGISTER_ARGON_FUNCTION(GetGestureHoldDuration)
  REGISTER_ARGON_FUNCTION(GetGestureDragVector)
  REGISTER_ARGON_FUNCTION(GetGestureDragAngle)
  REGISTER_ARGON_FUNCTION(GetGesturePinchVector)
  REGISTER_ARGON_FUNCTION(GetGesturePinchAngle)

  /* ── Convenience constructors & accessors ── */
  REGISTER_ARGON_FUNCTION(Color)
  REGISTER_ARGON_FUNCTION(DrawText)
  REGISTER_ARGON_FUNCTION(Vector2)
  REGISTER_ARGON_FUNCTION(Vector3)
  REGISTER_ARGON_FUNCTION(Camera3D)
  REGISTER_ARGON_FUNCTION(Camera2D)
  REGISTER_ARGON_FUNCTION(Vector2GetX)
  REGISTER_ARGON_FUNCTION(Vector2GetY)
  REGISTER_ARGON_FUNCTION(Vector3GetX)
  REGISTER_ARGON_FUNCTION(Vector3GetY)
  REGISTER_ARGON_FUNCTION(Vector3GetZ)
})