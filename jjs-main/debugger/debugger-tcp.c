/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jjs-debugger-transport.h"

#include "debugger.h"
#include "jext-common.h"

#if defined(JJS_DEBUGGER) && (JJS_DEBUGGER == 1)

#include <errno.h>

#ifdef _WIN32
#include <BaseTsd.h>

#include <WS2tcpip.h>
#include <winsock2.h>

/* On Windows the WSAEWOULDBLOCK value can be returned for non-blocking operations */
#define JJSX_EWOULDBLOCK WSAEWOULDBLOCK

/* On Windows the invalid socket's value of INVALID_SOCKET */
#define JJSX_SOCKET_INVALID INVALID_SOCKET

/*
 * On Windows, socket functions have the following signatures:
 * int send(SOCKET s, const char *buf, int len, int flags);
 * int recv(SOCKET s, char *buf, int len, int flags);
 * int setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen);
 */
typedef int jjsx_socket_ssize_t;
typedef SOCKET jjsx_socket_t;
typedef char jjsx_socket_void_t;
typedef int jjsx_socket_size_t;
#else /* !_WIN32 */
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

/* On *nix the EWOULDBLOCK errno value can be returned for non-blocking operations */
#define JJSX_EWOULDBLOCK    EWOULDBLOCK

/* On *nix the invalid socket has a value of -1 */
#define JJSX_SOCKET_INVALID (-1)

/*
 * On *nix, socket functions have the following signatures:
 * ssize_t send(int sockfd, const void *buf, size_t len, int flags);
 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 * int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
 */
typedef ssize_t jjsx_socket_ssize_t;
typedef int jjsx_socket_t;
typedef void jjsx_socket_void_t;
typedef size_t jjsx_socket_size_t;
#endif /* _WIN32 */

/**
 * Implementation of transport over tcp/ip.
 */
typedef struct
{
  jjs_debugger_transport_header_t header; /**< transport header */
  jjsx_socket_t tcp_socket; /**< tcp socket */
} jjsx_debugger_transport_tcp_t;

/**
 * Get the network error value.
 *
 * On Windows this returns the result of the `WSAGetLastError ()` call and
 * on any other system the `errno` value.
 *
 *
 * @return last error value.
 */
static inline int
jjsx_debugger_tcp_get_errno (void)
{
#ifdef _WIN32
  return WSAGetLastError ();
#else /* !_WIN32 */
  return errno;
#endif /* _WIN32 */
} /* jjsx_debugger_tcp_get_errno */

/**
 * Correctly close a single socket.
 */
static inline void
jjsx_debugger_tcp_close_socket (jjsx_socket_t socket_id) /**< socket to close */
{
#ifdef _WIN32
  closesocket (socket_id);
#else /* !_WIN32 */
  close (socket_id);
#endif /* _WIN32 */
} /* jjsx_debugger_tcp_close_socket */

/**
 * Log tcp error message.
 */
static void
jjsx_debugger_tcp_log_error (int errno_value) /**< error value to log */
{
  if (errno_value == 0)
  {
    return;
  }

#ifdef _WIN32
  char *error_message = NULL;
  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 (DWORD) errno_value,
                 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPTSTR) &error_message,
                 0,
                 NULL);
  JJSX_ERROR_MSG ("TCP Error: %s\n", error_message);
  LocalFree (error_message);
#else /* !_WIN32 */
  JJSX_ERROR_MSG ("TCP Error: %s\n", strerror (errno_value));
#endif /* _WIN32 */
} /* jjsx_debugger_tcp_log_error */

/**
 * Close a tcp connection.
 */
static void
jjsx_debugger_tcp_close (jjs_debugger_transport_header_t *header_p) /**< tcp implementation */
{
  JJSX_ASSERT (!jjs_debugger_transport_is_connected ());

  jjsx_debugger_transport_tcp_t *tcp_p = (jjsx_debugger_transport_tcp_t *) header_p;

  JJSX_DEBUG_MSG ("TCP connection closed.\n");

  jjsx_debugger_tcp_close_socket (tcp_p->tcp_socket);

  jjs_heap_free ((void *) header_p, sizeof (jjsx_debugger_transport_tcp_t));
} /* jjsx_debugger_tcp_close */

/**
 * Send data over a tcp connection.
 *
 * @return true - if the data has been sent successfully
 *         false - otherwise
 */
static bool
jjsx_debugger_tcp_send (jjs_debugger_transport_header_t *header_p, /**< tcp implementation */
                          uint8_t *message_p, /**< message to be sent */
                          size_t message_length) /**< message length in bytes */
{
  JJSX_ASSERT (jjs_debugger_transport_is_connected ());

  jjsx_debugger_transport_tcp_t *tcp_p = (jjsx_debugger_transport_tcp_t *) header_p;
  jjsx_socket_size_t remaining_bytes = (jjsx_socket_size_t) message_length;

  do
  {
#ifdef __linux__
    ssize_t is_err = recv (tcp_p->tcp_socket, NULL, 0, MSG_PEEK);

    if (is_err == 0 && errno != JJSX_EWOULDBLOCK)
    {
      int err_val = errno;
      jjs_debugger_transport_close ();
      jjsx_debugger_tcp_log_error (err_val);
      return false;
    }
#endif /* __linux__ */

    jjsx_socket_ssize_t sent_bytes = send (tcp_p->tcp_socket, (jjsx_socket_void_t *) message_p, remaining_bytes, 0);

    if (sent_bytes < 0)
    {
      int err_val = jjsx_debugger_tcp_get_errno ();

      if (err_val == JJSX_EWOULDBLOCK)
      {
        continue;
      }

      jjs_debugger_transport_close ();
      jjsx_debugger_tcp_log_error (err_val);
      return false;
    }

    message_p += sent_bytes;
    remaining_bytes -= (jjsx_socket_size_t) sent_bytes;
  } while (remaining_bytes > 0);

  return true;
} /* jjsx_debugger_tcp_send */

/**
 * Receive data from a tcp connection.
 */
static bool
jjsx_debugger_tcp_receive (jjs_debugger_transport_header_t *header_p, /**< tcp implementation */
                             jjs_debugger_transport_receive_context_t *receive_context_p) /**< receive context */
{
  jjsx_debugger_transport_tcp_t *tcp_p = (jjsx_debugger_transport_tcp_t *) header_p;

  jjsx_socket_void_t *buffer_p =
    (jjsx_socket_void_t *) (receive_context_p->buffer_p + receive_context_p->received_length);
  jjsx_socket_size_t buffer_size =
    (jjsx_socket_size_t) (JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE - receive_context_p->received_length);

  jjsx_socket_ssize_t length = recv (tcp_p->tcp_socket, buffer_p, buffer_size, 0);

  if (length <= 0)
  {
    int err_val = jjsx_debugger_tcp_get_errno ();

    if (err_val != JJSX_EWOULDBLOCK || length == 0)
    {
      jjs_debugger_transport_close ();
      jjsx_debugger_tcp_log_error (err_val);
      return false;
    }
    length = 0;
  }

  receive_context_p->received_length += (size_t) length;

  if (receive_context_p->received_length > 0)
  {
    receive_context_p->message_p = receive_context_p->buffer_p;
    receive_context_p->message_length = receive_context_p->received_length;
  }

  return true;
} /* jjsx_debugger_tcp_receive */

/**
 * Utility method to prepare the server socket to accept connections.
 *
 * The following steps are performed:
 *  * Configure address re-use.
 *  * Bind the socket to the given port
 *  * Start listening on the socket.
 *
 * @return true if everything is ok
 *         false if there was an error
 */
static bool
jjsx_debugger_tcp_configure_socket (jjsx_socket_t server_socket, /** < socket to configure */
                                      uint16_t port) /** < port number to be used for the socket */
{
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = INADDR_ANY;

  const int opt_value = 1;

  if (setsockopt (server_socket, SOL_SOCKET, SO_REUSEADDR, (const jjsx_socket_void_t *) &opt_value, sizeof (int))
      != 0)
  {
    return false;
  }

  if (bind (server_socket, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) != 0)
  {
    return false;
  }

  if (listen (server_socket, 1) != 0)
  {
    return false;
  }

  return true;
} /* jjsx_debugger_tcp_configure_socket */

/**
 * Create a tcp connection.
 *
 * @return true if successful,
 *         false otherwise
 */
bool
jjsx_debugger_tcp_create (uint16_t port) /**< listening port */
{
#ifdef _WIN32
  WSADATA wsaData;
  int wsa_init_status = WSAStartup (MAKEWORD (2, 2), &wsaData);
  if (wsa_init_status != NO_ERROR)
  {
    JJSX_ERROR_MSG ("WSA Error: %d\n", wsa_init_status);
    return false;
  }
#endif /* _WIN32*/

  jjsx_socket_t server_socket = socket (AF_INET, SOCK_STREAM, 0);
  if (server_socket == JJSX_SOCKET_INVALID)
  {
    jjsx_debugger_tcp_log_error (jjsx_debugger_tcp_get_errno ());
    return false;
  }

  if (!jjsx_debugger_tcp_configure_socket (server_socket, port))
  {
    int error = jjsx_debugger_tcp_get_errno ();
    jjsx_debugger_tcp_close_socket (server_socket);
    jjsx_debugger_tcp_log_error (error);
    return false;
  }

  JJSX_DEBUG_MSG ("Waiting for client connection\n");

  struct sockaddr_in addr;
  socklen_t sin_size = sizeof (struct sockaddr_in);

  jjsx_socket_t tcp_socket = accept (server_socket, (struct sockaddr *) &addr, &sin_size);

  jjsx_debugger_tcp_close_socket (server_socket);

  if (tcp_socket == JJSX_SOCKET_INVALID)
  {
    jjsx_debugger_tcp_log_error (jjsx_debugger_tcp_get_errno ());
    return false;
  }

  /* Set non-blocking mode. */
#ifdef _WIN32
  u_long nonblocking_enabled = 1;
  if (ioctlsocket (tcp_socket, (long) FIONBIO, &nonblocking_enabled) != NO_ERROR)
  {
    jjsx_debugger_tcp_close_socket (tcp_socket);
    return false;
  }
#else /* !_WIN32 */
  int socket_flags = fcntl (tcp_socket, F_GETFL, 0);

  if (socket_flags < 0)
  {
    close (tcp_socket);
    return false;
  }

  if (fcntl (tcp_socket, F_SETFL, socket_flags | O_NONBLOCK) == -1)
  {
    close (tcp_socket);
    return false;
  }
#endif /* _WIN32 */

  JJSX_DEBUG_MSG ("Connected from: %s\n", inet_ntoa (addr.sin_addr));

  jjs_size_t size = sizeof (jjsx_debugger_transport_tcp_t);

  jjs_debugger_transport_header_t *header_p;
  header_p = (jjs_debugger_transport_header_t *) jjs_heap_alloc (size);

  if (!header_p)
  {
    jjsx_debugger_tcp_close_socket (tcp_socket);
    return false;
  }

  header_p->close = jjsx_debugger_tcp_close;
  header_p->send = jjsx_debugger_tcp_send;
  header_p->receive = jjsx_debugger_tcp_receive;

  ((jjsx_debugger_transport_tcp_t *) header_p)->tcp_socket = tcp_socket;

  jjs_debugger_transport_add (header_p,
                                0,
                                JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE,
                                0,
                                JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE);

  return true;
} /* jjsx_debugger_tcp_create */

#else /* !(defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1)) */

/**
 * Dummy function when debugger is disabled.
 *
 * @return false
 */
bool
jjsx_debugger_tcp_create (uint16_t port)
{
  JJSX_UNUSED (port);
  return false;
} /* jjsx_debugger_tcp_create */

#endif /* defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1) */
