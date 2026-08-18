// SPDX-FileCopyrightText: 2025, 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "socket.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef WSAPOLLFD poll_fd_t;
#define poll_sockets(fds, n, ms) WSAPoll((fds), (n), (ms))
#define POLLIN 0x0100
#define POLLOUT 0x0010
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h> /* TCP_NODELAY */
#include <poll.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
typedef struct pollfd poll_fd_t;
#define poll_sockets(fds, n, ms) poll((fds), (n), (ms))
#endif

int net_init(void) {
#ifdef _WIN32
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2, 2), &wsa);
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
  if (s < 0)
    return -1;
  int opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
  if (s == INVALID_SOCKET)
    return -1;
#endif

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    return -1;

  if (listen(s, 16) < 0)
    return -1;

  return s;
}

socket_t net_accept(socket_t server) { return accept(server, NULL, NULL); }

int net_send(socket_t s, const void *buf, int len) {
#ifdef _WIN32
  return send(s, buf, len, 0);
#else
  return (int)send(s, buf, len, MSG_NOSIGNAL);
#endif
}

int net_recv(socket_t s, void *buf, int len) { return recv(s, buf, len, 0); }

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
  hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port_str, &hints, &res) != 0)
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    return -1;
#endif

  socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifdef _WIN32
  if (s == INVALID_SOCKET) {
    freeaddrinfo(res);
    return INVALID_SOCKET;
  }
#else
  if (s < 0) {
    freeaddrinfo(res);
    return -1;
  }
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
  if (flags < 0)
    return -1;
  if (enable)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;
  return fcntl(s, F_SETFL, flags) == 0 ? 0 : -1;
#endif
}

int net_poll(socket_t s, int want_read, int want_write, int timeout_ms) {
  poll_fd_t pfd;
  pfd.fd = s;
  pfd.events = (want_read ? POLLIN : 0) | (want_write ? POLLOUT : 0);
  pfd.revents = 0;

  int r = poll_sockets(&pfd, 1, timeout_ms);
  if (r < 0)
    return -1;
  if (r == 0)
    return 0;

  int result = 0;
  if (want_read && (pfd.revents & POLLIN))
    result |= NET_POLL_READ;
  if (want_write && (pfd.revents & POLLOUT))
    result |= NET_POLL_WRITE;
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
    return setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char *)&v, sizeof(v));
  }
  case NET_OPT_NODELAY: {
    int v = value ? 1 : 0;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&v, sizeof(v));
  }
  case NET_OPT_REUSEADDR: {
    int v = value ? 1 : 0;
    return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&v, sizeof(v));
  }
  case NET_OPT_RCVTIMEO:
#ifdef _WIN32
    ms = (DWORD)value;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms,
                      sizeof(ms));
#else
  {
    struct timeval tv = {value / 1000, (value % 1000) * 1000};
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv,
                      sizeof(tv));
  }
#endif
  case NET_OPT_SNDTIMEO:
#ifdef _WIN32
    ms = (DWORD)value;
    return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&ms,
                      sizeof(ms));
#else
  {
    struct timeval tv = {value / 1000, (value % 1000) * 1000};
    return setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv,
                      sizeof(tv));
  }
#endif
  default:
    return -1;
  }
}

#ifndef NET_WITHOUT_TLS

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

static int tls_globally_initialised = 0;

static void tls_global_init_once(void) {
  if (tls_globally_initialised)
    return;
  /* OpenSSL >= 1.1.0 self-initialises lazily, but calling this explicitly
   * keeps behaviour consistent across the versions we might link against. */
  SSL_library_init();
  SSL_load_error_strings();
  tls_globally_initialised = 1;
}

const char *tls_last_error(void) {
  static char buf[256];
  unsigned long code = ERR_get_error();
  if (!code) {
    return "no error";
  }
  ERR_error_string_n(code, buf, sizeof(buf));
  return buf;
}

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#elif defined(__APPLE__)
#include <Security/Security.h>
#endif

static int tls_load_default_ca(SSL_CTX *ctx) {
#ifdef _WIN32
    X509_STORE *store = SSL_CTX_get_cert_store(ctx);

    HCERTSTORE cert_store = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_A,
        0,
        0,
        CERT_SYSTEM_STORE_CURRENT_USER,
        "ROOT"
    );

    if (!cert_store)
        return 0;

    int loaded = 0;
    PCCERT_CONTEXT cert = NULL;

    while ((cert = CertEnumCertificatesInStore(cert_store, cert)) != NULL) {
        const unsigned char *data = cert->pbCertEncoded;

        X509 *x509 = d2i_X509(NULL, &data, cert->cbCertEncoded);

        if (x509) {
            if (X509_STORE_add_cert(store, x509) == 1)
                loaded = 1;

            X509_free(x509);
        }
    }

    CertCloseStore(cert_store, 0);

    return loaded;
#elif defined(__APPLE__)
// loads only Apple's built-in root CAs, not user/admin-trusted certs
    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    int loaded = 0;

    CFArrayRef anchors = NULL;
    OSStatus status = SecTrustCopyAnchorCertificates(&anchors);
    if (status != errSecSuccess || !anchors)
        return 0;

    CFIndex count = CFArrayGetCount(anchors);
    for (CFIndex i = 0; i < count; i++) {
        SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex(anchors, i);
        CFDataRef der = SecCertificateCopyData(cert);
        if (!der) continue;

        const unsigned char *data = CFDataGetBytePtr(der);
        long len = CFDataGetLength(der);

        X509 *x509 = d2i_X509(NULL, &data, len);
        if (x509) {
            if (X509_STORE_add_cert(store, x509) == 1)
                loaded = 1;
            X509_free(x509);
        }
        CFRelease(der);
    }

    CFRelease(anchors);
    return loaded;
#else
    return SSL_CTX_set_default_verify_paths(ctx) == 1;
#endif
}

bool tls_connect(const char *host, int port, int verify_peer,
                       const char *ca_path, tls_conn_t *conn) {
  tls_global_init_once();

  socket_t sock = net_connect(host, port);
#ifdef _WIN32
  if (sock == INVALID_SOCKET)
#else
  if (sock < 0)
#endif
    return false;

  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx) {
    net_close(sock);
    return false;
  }

  /* Modern TLS only: 1.2 minimum. */
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

  if (verify_peer) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    int loaded = 0;
    if (ca_path && ca_path[0]) {
      loaded = SSL_CTX_load_verify_locations(ctx, ca_path, NULL) == 1;
    } else {
      loaded = tls_load_default_ca(ctx);
    }
    if (!loaded) {
      SSL_CTX_free(ctx);
      net_close(sock);
      return false;
    }
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    SSL_CTX_free(ctx);
    net_close(sock);
    return false;
  }

  /* SNI */
  SSL_set_tlsext_host_name(ssl, host);

  if (verify_peer) {
    /* Hostname verification against the leaf certificate. */
    SSL_set1_host(ssl, host);
    SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);
  }

  if (SSL_set_fd(ssl, (int)sock) != 1) {
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    net_close(sock);
    return false;
  }

  if (SSL_connect(ssl) != 1) {
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    net_close(sock);
    return false;
  }

  conn->sock = sock;
  conn->ctx = ctx;
  conn->ssl = ssl;
  return true;
}

int tls_send(tls_conn_t *conn, const void *buf, int len) {
  if (!conn)
    return -1;
  int n = SSL_write(conn->ssl, buf, len);
  if (n <= 0)
    return -1;
  return n;
}

int tls_recv(tls_conn_t *conn, void *buf, int len) {
  if (!conn)
    return -1;
  int n = SSL_read(conn->ssl, buf, len);
  if (n <= 0) {
    int reason = SSL_get_error(conn->ssl, n);
    if (reason == SSL_ERROR_ZERO_RETURN)
      return 0; /* clean shutdown */
    return -1;
  }
  return n;
}

int tls_poll(tls_conn_t *conn, int want_read, int want_write, int timeout_ms) {
  if (!conn)
    return -1;
  return net_poll(conn->sock, want_read, want_write, timeout_ms);
}

void tls_close(tls_conn_t *conn) {
  if (!conn)
    return;
  if (conn->ssl) {
    SSL_shutdown(conn->ssl);
    SSL_free(conn->ssl);
  }
  if (conn->ctx)
    SSL_CTX_free(conn->ctx);
  #ifdef _WIN32
      if (conn->sock != INVALID_SOCKET)
  #else
      if (conn->sock >= 0)
  #endif
    net_close(conn->sock);
}

#endif // NET_WITH_TLS