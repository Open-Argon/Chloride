// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "socket.h"
#include <string.h>
#include <stdio.h>

int net_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,2), &wsa);
#else
    return 0;
#endif
}

void net_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

socket_t net_listen(int port) {
    socket_t s = socket(AF_INET, SOCK_STREAM, 0);

#ifndef _WIN32
    if (s < 0) return -1;
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
    if (s == INVALID_SOCKET) return -1;
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    if (listen(s, 16) < 0)
        return -1;

    return s;
}

socket_t net_accept(socket_t server) {
    return accept(server, NULL, NULL);
}

int net_send(socket_t s, const void *buf, int len) {
#ifdef _WIN32
    return send(s, buf, len, 0);
#else
    return (int)send(s, buf, len, MSG_NOSIGNAL);
#endif
}

int net_recv(socket_t s, void *buf, int len) {
    return recv(s, buf, len, 0);
}

void net_close(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}