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