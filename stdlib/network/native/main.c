// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "socket.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArgonObject *argon_net_init(size_t argc, ArgonObject **argv, ArgonError *err,
                            ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  net_init();
  return api->ARGON_NULL;
}

ArgonObject *argon_net_cleanup(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  net_cleanup();
  return api->ARGON_NULL;
}

ArgonObject *argon_net_listen(size_t argc, ArgonObject **argv, ArgonError *err,
                              ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  int64_t port = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t server_socket = net_listen(port);
  if (server_socket < 0)
    return api->throw_argon_error(
        err, "Socket Error", "failed to open a tcp socket on port %" PRIu64,
        port);

  ArgonObject *server_socket_buffer_object =
      api->create_argon_buffer(sizeof(socket_t));
  struct buffer server_socket_buffer =
      api->argon_buffer_to_buffer(server_socket_buffer_object, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(server_socket_buffer.data, &server_socket, server_socket_buffer.size);
  return server_socket_buffer_object;
}

ArgonObject *argon_net_accept(size_t argc, ArgonObject **argv, ArgonError *err,
                              ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct buffer server_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t server_socket = *(socket_t *)server_socket_buffer.data;

  socket_t connection_socket = net_accept(server_socket);
  if (connection_socket < 0)
    return api->throw_argon_error(err, "Socket Error",
                                  "failed to accept a socket connection");

  ArgonObject *connection_socket_buffer_object =
      api->create_argon_buffer(sizeof(socket_t));
  struct buffer connection_socket_buffer =
      api->argon_buffer_to_buffer(connection_socket_buffer_object, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(connection_socket_buffer.data, &connection_socket,
         connection_socket_buffer.size);

  return connection_socket_buffer_object;
}

ArgonObject *argon_net_send(size_t argc, ArgonObject **argv, ArgonError *err,
                            ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer connection_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t connection_socket = *(socket_t *)connection_socket_buffer.data;

  struct buffer data_buffer = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(
      net_send(connection_socket, data_buffer.data, data_buffer.size));
}

ArgonObject *argon_net_send_string(size_t argc, ArgonObject **argv, ArgonError *err,
                            ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer connection_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t connection_socket = *(socket_t *)connection_socket_buffer.data;

  struct string data_string = api->argon_to_string(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(
      net_send(connection_socket, data_string.data, data_string.length));
}

ArgonObject *argon_net_recv(size_t argc, ArgonObject **argv, ArgonError *err,
                            ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer connection_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t connection_socket = *(socket_t *)connection_socket_buffer.data;

  struct buffer data_buffer = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(
      net_recv(connection_socket, data_buffer.data, data_buffer.size));
}

ArgonObject *argon_net_recv_string(size_t argc, ArgonObject **argv,
                                   ArgonError *err, ArgonState *state,
                                   ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer connection_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t connection_socket = *(socket_t *)connection_socket_buffer.data;

  int64_t size = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  void *data = malloc(size);

  int n = net_recv(connection_socket, data, size);

  return api->string_to_argon((struct string){data, n});
}

ArgonObject *argon_net_close(size_t argc, ArgonObject **argv, ArgonError *err,
                             ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t socket = *(socket_t *)socket_buffer.data;
  net_close(socket);
  return api->ARGON_NULL;
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  api->register_ArgonObject(
      reg, "net_init",
      api->create_argon_native_function("net_init", argon_net_init));
  api->register_ArgonObject(
      reg, "net_cleanup",
      api->create_argon_native_function("net_cleanup", argon_net_cleanup));
  api->register_ArgonObject(
      reg, "net_listen",
      api->create_argon_native_function("net_listen", argon_net_listen));
  api->register_ArgonObject(
      reg, "net_accept",
      api->create_argon_native_function("net_accept", argon_net_accept));
  api->register_ArgonObject(
      reg, "net_send",
      api->create_argon_native_function("net_send", argon_net_send));
  api->register_ArgonObject(
      reg, "net_send_string",
      api->create_argon_native_function("net_send_string", argon_net_send_string));
  api->register_ArgonObject(
      reg, "net_recv",
      api->create_argon_native_function("net_recv", argon_net_recv));
  api->register_ArgonObject(
      reg, "net_recv_string",
      api->create_argon_native_function("net_recv_string", argon_net_recv_string));
  api->register_ArgonObject(
      reg, "net_close",
      api->create_argon_native_function("net_close", argon_net_close));
}