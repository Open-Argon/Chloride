/*
 * SPDX-FileCopyrightText: 2026 William Bell
 */

// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int socket_t;
#endif

int  net_init(void);
void net_cleanup(void);
socket_t net_listen(int port);
socket_t net_accept(socket_t server);
int  net_send(socket_t s, const void *buf, int len);
int  net_recv(socket_t s, void *buf, int len);
void net_close(socket_t s);

// net_connect: client-side connect to host:port. Returns socket or -1/INVALID_SOCKET.
socket_t net_connect(const char *host, int port);

// net_set_nonblocking: 1 = non-blocking, 0 = blocking. Returns 0 on success, -1 on error.
int net_set_nonblocking(socket_t s, int enable);

// net_poll: wait up to timeout_ms for readability (want_read) and/or writability (want_write).
// Returns a bitmask: bit 0 = readable, bit 1 = writable, -1 on error, 0 on timeout.
#define NET_POLL_READ  (1 << 0)
#define NET_POLL_WRITE (1 << 1)
int net_poll(socket_t s, int want_read, int want_write, int timeout_ms);

// net_peek: like net_recv but leaves data in the kernel buffer. Returns bytes peeked.
int net_peek(socket_t s, void *buf, int len);

// net_set_opt: set common socket options. Returns 0 on success, -1 on error.
#define NET_OPT_KEEPALIVE  1   // SO_KEEPALIVE
#define NET_OPT_NODELAY    2   // TCP_NODELAY  (disable Nagle)
#define NET_OPT_REUSEADDR  3   // SO_REUSEADDR
#define NET_OPT_RCVTIMEO   4   // SO_RCVTIMEO  (value = ms)
#define NET_OPT_SNDTIMEO   5   // SO_SNDTIMEO  (value = ms)
int net_set_opt(socket_t s, int opt, int value);