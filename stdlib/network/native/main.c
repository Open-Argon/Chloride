/*
 * SPDX-FileCopyrightText: 2026 William Bell
 */

// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "ArgonFunction.h"
#include "socket.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ARGON_FUNCTION(argon_net_init, {
  (void)argv;
  (void)state;
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  net_init();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(argon_net_cleanup, {
  (void)argv;
  (void)state;
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  net_cleanup();
  return api->ARGON_NULL;
})

ARGON_FUNCTION(argon_net_listen, {
  (void)state;
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  int64_t port = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t server_socket = net_listen(port);
#ifdef _WIN32
  if (server_socket == INVALID_SOCKET)
#else
  if (server_socket < 0)
#endif
    return api->throw_argon_error(
        err, argv[1], "failed to open a tcp socket on port %" PRIu64, port);

  ArgonObject *server_socket_buffer_object =
      api->create_argon_buffer(sizeof(socket_t));
  struct buffer server_socket_buffer =
      api->argon_buffer_to_buffer(server_socket_buffer_object, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(server_socket_buffer.data, &server_socket, server_socket_buffer.size);
  return server_socket_buffer_object;
})

ARGON_FUNCTION(argon_net_accept, {
  (void)state;
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer server_socket_buffer =
      api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t server_socket = *(socket_t *)server_socket_buffer.data;

  socket_t connection_socket = net_accept(server_socket);
#ifdef _WIN32
  if (connection_socket == INVALID_SOCKET)
#else
  if (connection_socket < 0)
#endif
    return api->throw_argon_error(err, argv[1],
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
})

ARGON_FUNCTION(argon_net_send, {
  (void)state;
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
})

ARGON_FUNCTION(argon_net_send_string, {
  (void)state;
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
})

ARGON_FUNCTION(argon_net_recv, {
  (void)state;
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
})

ARGON_FUNCTION(argon_net_recv_string, {
  (void)state;
  (void)argv;
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
})

ARGON_FUNCTION(argon_net_close, {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t socket = *(socket_t *)socket_buffer.data;
  net_close(socket);
  return api->ARGON_NULL;
})

ARGON_FUNCTION(argon_net_connect, {
  (void)state;
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  struct string host = api->argon_to_string(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  int64_t port = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  socket_t sock = net_connect(host.data, (int)port);
#ifdef _WIN32
  if (sock == INVALID_SOCKET)
#else
  if (sock < 0)
#endif
    return api->throw_argon_error(
        err, argv[2], "failed to connect to %s:%" PRId64, host.data, port);

  ArgonObject *buf_obj = api->create_argon_buffer(sizeof(socket_t));
  struct buffer buf = api->argon_buffer_to_buffer(buf_obj, err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  memcpy(buf.data, &sock, buf.size);
  return buf_obj;
})

ARGON_FUNCTION(argon_net_set_nonblocking, {
  (void)state;
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buf = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  socket_t sock = *(socket_t *)socket_buf.data;

  int64_t enable = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(net_set_nonblocking(sock, (int)enable));
})

ARGON_FUNCTION(argon_net_poll, {
  (void)state;
  if (api->fix_to_arg_size(4, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buf = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  socket_t sock = *(socket_t *)socket_buf.data;

  int64_t want_read = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  int64_t want_write = api->argon_to_i64(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  int64_t timeout_ms = api->argon_to_i64(argv[3], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(
      net_poll(sock, (int)want_read, (int)want_write, (int)timeout_ms));
})

ARGON_FUNCTION(argon_net_peek, {
  (void)state;
  if (api->fix_to_arg_size(2, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buf = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  socket_t sock = *(socket_t *)socket_buf.data;

  struct buffer data_buf = api->argon_buffer_to_buffer(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(net_peek(sock, data_buf.data, (int)data_buf.size));
})

ARGON_FUNCTION(argon_net_set_opt, {
  (void)state;
  if (api->fix_to_arg_size(3, argc, err))
    return api->ARGON_NULL;

  struct buffer socket_buf = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  socket_t sock = *(socket_t *)socket_buf.data;

  int64_t opt = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  int64_t value = api->argon_to_i64(argv[2], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->i64_to_argon(net_set_opt(sock, (int)opt, (int)value));
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
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
  api->register_ArgonObject(reg, "net_send_string",
                            api->create_argon_native_function(
                                "net_send_string", argon_net_send_string));
  api->register_ArgonObject(
      reg, "net_recv",
      api->create_argon_native_function("net_recv", argon_net_recv));
  api->register_ArgonObject(reg, "net_recv_string",
                            api->create_argon_native_function(
                                "net_recv_string", argon_net_recv_string));
  api->register_ArgonObject(
      reg, "net_close",
      api->create_argon_native_function("net_close", argon_net_close));
  api->register_ArgonObject(
      reg, "net_connect",
      api->create_argon_native_function("net_connect", argon_net_connect));
  api->register_ArgonObject(
      reg, "net_set_nonblocking",
      api->create_argon_native_function("net_set_nonblocking",
                                        argon_net_set_nonblocking));
  api->register_ArgonObject(
      reg, "net_poll",
      api->create_argon_native_function("net_poll", argon_net_poll));
  api->register_ArgonObject(
      reg, "net_peek",
      api->create_argon_native_function("net_peek", argon_net_peek));
  api->register_ArgonObject(
      reg, "net_set_opt",
      api->create_argon_native_function("net_set_opt", argon_net_set_opt));
}