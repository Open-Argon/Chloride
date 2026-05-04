// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "socket.h"
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>   /* TCP_NODELAY */
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

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

socket_t net_connect(const char *host, int port) {
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;    /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
#ifdef _WIN32
        return INVALID_SOCKET;
#else
        return -1;
#endif

    socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifdef _WIN32
    if (s == INVALID_SOCKET) { freeaddrinfo(res); return INVALID_SOCKET; }
#else
    if (s < 0)               { freeaddrinfo(res); return -1; }
#endif

    if (connect(s, res->ai_addr, (int)res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        net_close(s);
#ifdef _WIN32
        return INVALID_SOCKET;
#else
        return -1;
#endif
    }

    freeaddrinfo(res);
    return s;
}

int net_set_nonblocking(socket_t s, int enable) {
#ifdef _WIN32
    u_long mode = enable ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode) == 0 ? 0 : -1;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return -1;
    if (enable)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return fcntl(s, F_SETFL, flags) == 0 ? 0 : -1;
#endif
}

int net_poll(socket_t s, int want_read, int want_write, int timeout_ms) {
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    if (want_read)  FD_SET(s, &rfds);
    if (want_write) FD_SET(s, &wfds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int r = select((int)s + 1,
                   want_read  ? &rfds : NULL,
                   want_write ? &wfds : NULL,
                   NULL,
                   timeout_ms >= 0 ? &tv : NULL);
    if (r < 0)  return -1;
    if (r == 0) return 0;

    int result = 0;
    if (want_read  && FD_ISSET(s, &rfds)) result |= NET_POLL_READ;
    if (want_write && FD_ISSET(s, &wfds)) result |= NET_POLL_WRITE;
    return result;
}

int net_peek(socket_t s, void *buf, int len) {
    return (int)recv(s, buf, len, MSG_PEEK);
}

int net_set_opt(socket_t s, int opt, int value) {
#ifdef _WIN32
    DWORD ms;
#endif
    switch (opt) {
    case NET_OPT_KEEPALIVE: {
        int v = value ? 1 : 0;
        return setsockopt(s, SOL_SOCKET, SO_KEEPALIVE,
                          (const char *)&v, sizeof(v));
    }
    case NET_OPT_NODELAY: {
        int v = value ? 1 : 0;
        return setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                          (const char *)&v, sizeof(v));
    }
    case NET_OPT_REUSEADDR: {
        int v = value ? 1 : 0;
        return setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                          (const char *)&v, sizeof(v));
    }
    case NET_OPT_RCVTIMEO:
#ifdef _WIN32
        ms = (DWORD)value;
        return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
                          (const char *)&ms, sizeof(ms));
#else
    {
        struct timeval tv = { value / 1000, (value % 1000) * 1000 };
        return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
                          (const void *)&tv, sizeof(tv));
    }
#endif
    case NET_OPT_SNDTIMEO:
#ifdef _WIN32
        ms = (DWORD)value;
        return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO,
                          (const char *)&ms, sizeof(ms));
#else
    {
        struct timeval tv = { value / 1000, (value % 1000) * 1000 };
        return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO,
                          (const void *)&tv, sizeof(tv));
    }
#endif
    default:
        return -1;
    }
}