/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2001  Mark Ferlatte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include "gnet-private.h"

#ifndef GNET_WIN32

#include "unix.h"
#include <errno.h>
#include <string.h>

#ifndef SUN_LEN
#define SUN_LEN(sa_un) \
    G_STRUCT_OFFSET (struct sockaddr_un, sun_path) + strlen (sa_un->sun_path)
#endif

struct _GUnixSocket
{
  gint sockfd;
  guint ref_count;
  GIOChannel *iochannel;
  struct sockaddr_un sa;

  gboolean server;
};

#define PATH(S) (((struct sockaddr_un *) (&(S)->sa))->sun_path)
gboolean gnet_unix_socket_unlink (const gchar *path);


/**
 *  gnet_unix_socket_new
 *  @path: path
 *
 *  Creates a #GUnixSocket and connects to @path.  This function will
 *  block to connect.  Use this constructor to create a #GUnixSocket
 *  for a client.
 *
 *  Returns: a new #GUnixSocket; NULL on failure.
 *
 **/
GUnixSocket*
gnet_unix_socket_new (const gchar* path)
{
  struct sockaddr_un *sa_un;
  GUnixSocket *s;
	
  g_return_val_if_fail (path != NULL, NULL);

  /* Create socket */
  s = g_new0 (GUnixSocket, 1);
  sa_un = (struct sockaddr_un *) &s->sa;
  s->ref_count = 1;
  s->server = FALSE;
  s->sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (!GNET_IS_SOCKET_VALID (s->sockfd)) {
    g_warning ("socket(%s) failed: %s", path, g_strerror (errno));
    g_free(s);
    return NULL;
  }
  strncpy (sa_un->sun_path, path, sizeof (sa_un->sun_path) - 1);
  sa_un->sun_family = AF_UNIX;
  if (connect (s->sockfd, (struct sockaddr*) sa_un, SUN_LEN (sa_un)) != 0) {
    g_warning ("connect(%s) failed: %s", path, g_strerror (errno));
    GNET_CLOSE_SOCKET (s->sockfd);
    g_free(s);
    return NULL;
  }
  return s;
}



/**
 *  gnet_unix_socket_delete
 *  @socket: a #GUnixSocket
 *
 *  Deletes a #GUnixSocket.
 *
 **/
void
gnet_unix_socket_delete (GUnixSocket* socket)
{
  if (socket != NULL)
    gnet_unix_socket_unref (socket);
}


/**
 *  gnet_unix_socket_ref
 *  @socket: a #GUnixSocket
 *
 *  Adds a reference to a #GUnixSocket.
 *
 **/
void
gnet_unix_socket_ref (GUnixSocket* socket)
{
  g_return_if_fail (socket != NULL);

  socket->ref_count++;
}


/**
 *  gnet_unix_socket_unref
 *  @socket: a #GUnixSocket
 *
 *  Removes a reference from a #GUnixSocket.  A #GUnixSocket is
 *  deleted when the reference count reaches 0.
 *
 **/
void
gnet_unix_socket_unref (GUnixSocket* socket)
{
  g_return_if_fail (socket != NULL);

  socket->ref_count--;
  if (socket->ref_count == 0) 
    {
      GNET_CLOSE_SOCKET(socket->sockfd); /* Don't care if this fails. */
      if (socket->iochannel)
	g_io_channel_unref(socket->iochannel);
      if (socket->server)
	gnet_unix_socket_unlink(PATH(socket));
      g_free(socket);
    }
}
 
   
/**
 *  gnet_unix_socket_get_io_channel
 *  @socket: a #GUnixSocket
 *
 *  Gets the #GIOChannel of a #GUnixSocket.
 *
 *  For a client socket, the #GIOChannel represents the data stream.
 *  Use it like you would any other #GIOChannel.
 *
 *  For a server socket, however, the #GIOChannel represents the
 *  listening socket.  When it's readable, there's a connection
 *  waiting to be accepted.
 *
 *  Every #GUnixSocket has one and only one #GIOChannel.  If you ref
 *  the channel, then you must unref it eventually.  Do not close the
 *  channel.  The channel is closed by GNet when the socket is
 *  deleted.
 *
 *  Returns: a #GIOChannel.
 *
 **/
GIOChannel*
gnet_unix_socket_get_io_channel (GUnixSocket* socket)
{
  g_return_val_if_fail(socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = gnet_private_io_channel_new(socket->sockfd);

  return socket->iochannel;
}


/**
 *  gnet_unix_socket_get_path
 *  @socket: a #GUnixSocket
 *
 *  Gets the path of a #GUnixSocket.
 *
 *  Returns: the path.
 *
 **/
gchar*
gnet_unix_socket_get_path(const GUnixSocket* socket)
{
  g_return_val_if_fail(socket != NULL, NULL);

  return g_strdup (PATH(socket));
}


/**
 *  gnet_unix_socket_server_new
 *  @path: path
 *
 *  Creates a #GUnixSocket bound to @path.  Use this constructor to
 *  create a #GUnixSocket for a server.
 *
 *  Returns: a new #GUnixSocket; NULL on error.
 *
 **/
GUnixSocket*
gnet_unix_socket_server_new (const gchar *path)
{
  struct sockaddr_un *sa_un;
  GUnixSocket *s;
  gint flags;
  socklen_t n;

  g_return_val_if_fail(path != NULL, NULL);
	
  s = g_new0(GUnixSocket, 1);
  sa_un = (struct sockaddr_un *) &s->sa;
  sa_un->sun_family = AF_UNIX;
  strncpy (sa_un->sun_path, path, sizeof (sa_un->sun_path) - 1);
  s->ref_count = 1;
  s->server = TRUE;
  
  if (!gnet_unix_socket_unlink(PATH(s)))
    goto error;
  
  s->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (!GNET_IS_SOCKET_VALID(s->sockfd))
    {
      g_warning ("socket(%s) failed: %s", path, g_strerror (errno));
      goto error;
    }

  flags = fcntl(s->sockfd, F_GETFL, 0);
  if (flags == -1)
    {
      g_warning ("fcntl(%s) failed: %s", path, g_strerror (errno));
      goto error;
    }

  /* Make the socket non-blocking */
  if (fcntl(s->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      g_warning ("fcntl(%s) failed: %s", path, g_strerror (errno));
      goto error;
    }

  if (bind (s->sockfd, (struct sockaddr*) sa_un, SUN_LEN (sa_un)) != 0)
    goto error;

  n = sizeof(s->sa);
  /* Get the socket name FIXME (why? -DAH) */
  if (getsockname (s->sockfd, (struct sockaddr*) &s->sa, &n) != 0)
    goto error;

  if (listen(s->sockfd, 10) != 0)
    goto error;

  return s;
	
error:
  gnet_unix_socket_delete (s);
  return NULL;
}


/**
 *  gnet_unix_socket_server_accept
 *  @socket: a #GUnixSocket
 *
 *  Accepts a connection from a #GUnixSocket.  The socket must have
 *  been created using gnet_unix_socket_server_new(). This function
 *  will block.  Even if the socket's #GIOChannel is readable, the
 *  function may still block.
 *
 *  Returns: a new #GUnixSocket representing a new connection; NULL on
 *  error.
 *
 **/
GUnixSocket*
gnet_unix_socket_server_accept (const GUnixSocket *socket)
{
  gint sockfd;
  struct sockaddr sa;
  fd_set fdset;
  socklen_t n;
  GUnixSocket *s;

  g_return_val_if_fail(socket != NULL, NULL);

 try_again:
  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1) {
    if (errno == EINTR)
      goto try_again;
    return NULL;
  }
  n = sizeof(s->sa);
  if ((sockfd = accept(socket->sockfd, &sa, &n)) == -1) {
    if (errno == EWOULDBLOCK ||
	errno == ECONNABORTED ||
#ifdef EPROTO		/* OpenBSD does not have EPROTO */
	errno == EPROTO ||
#endif /* EPROTO */
	errno == EINTR)
      goto try_again;
    return NULL;
  }
  s = g_new0(GUnixSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));
  return s;
}


/**
 *  gnet_unix_socket_server_accept_nonblock
 *  @socket: a #GUnixSocket
 *
 *  Accepts a connection from a #GUnixSocket without blocking.  The
 *  socket must have been created using gnet_unix_socket_server_new().
 *
 *  Note that if the socket's #GIOChannel is readable, then there is
 *  PROBABLY a new connection.  It is possible for the connection
 *  to close by the time this function is called, so it may return
 *  NULL.
 *
 *  Returns: a new #GUnixSocket representing a new connection; NULL
 *  otherwise.
 *
 **/
GUnixSocket*
gnet_unix_socket_server_accept_nonblock (const GUnixSocket *socket)
{
  gint sockfd;
  struct sockaddr sa;
  fd_set fdset;
  socklen_t n;
  GUnixSocket *s;
  struct timeval tv = {0, 0};

  g_return_val_if_fail (socket != NULL, NULL);

 try_again:
  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if(select(socket->sockfd + 1, &fdset, NULL, NULL, &tv) == -1) {
    if (errno == EINTR)
      goto try_again;
    return NULL;
  }

  n = sizeof(sa);
	
  if ((sockfd = accept(socket->sockfd, &sa, &n)) == -1) {
    /* If we get an error, return.  We don't want to try again
       as we do in gnet_unix_socket_server_accept() - it might
       cause a block. */
    return NULL;
  }

  s = g_new0(GUnixSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}


/**
 *  gnet_unix_socket_unlink
 *  @path: path
 *
 *  Unlink a Unix socket named @path.
 *
 *  Returns: TRUE on success, FALSE on failure.
 **/
gboolean
gnet_unix_socket_unlink(const gchar *path)
{
  struct stat stbuf;
  int r;

  g_return_val_if_fail(path != NULL, FALSE);
  r = stat(path, &stbuf);
  if (r == 0) {
    if (S_ISSOCK(stbuf.st_mode)) {
      if (unlink(path) == 0) {
	return TRUE;
      } else {
				/* Can't unlink */
	return FALSE;
      }
    } else {
      /* path is not a socket */
      return FALSE;
    }
  } else if (errno == ENOENT) {
    /* File doesn't exist, so we're okay */
    return TRUE;
  }
  return FALSE;
}

#endif /* !GNET_WIN32 */
