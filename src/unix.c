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
#include "unix.h"

#define PATH(S) (((struct sockaddr_un *) (&(S)->sa))->sun_path)
gboolean gnet_unix_socket_unlink (const gchar *path);

/**
 *  gnet_unix_socket_connect:
 *  @path: Path to socket to connect to
 *
 *  A quick and easy #GUnixSocket constructor.  This connects to the
 *  specified path.  This function does block.  Use this function when
 *  you are a client connecting to a server and you don't mind
 *  blocking.
 *
 *  Returns: A new #GUnixSocket, or NULL if there was a failure.  
 *
 **/
GUnixSocket *
gnet_unix_socket_connect(const gchar *path)
{
  return gnet_unix_socket_new(path);
}
	
/**
 *  gnet_unix_socket_new:
 *  @path: Path to connect to.
 *
 *  Connect to a specified address.  Use this sort of socket when
 *  you're a client connecting to a server.  This function will block
 *  to connect.
 *
 *  Returns: A new #GUnixSocket, or NULL if there was a failure.
 *
 **/
GUnixSocket *
gnet_unix_socket_new(const gchar *path)
{
  GUnixSocket *s = g_new0(GUnixSocket, 1);
  struct sockaddr_un *sa_un;
	
  g_return_val_if_fail(path != NULL, NULL);
  sa_un = (struct sockaddr_un *) &s->sa;
  /* Create socket */
  s->ref_count = 1;
  s->server = FALSE;
  s->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (s->sockfd < 0) {
    g_free(s);
    return NULL;
  }
  memcpy(sa_un->sun_path, path, strlen(path));
  sa_un->sun_family = AF_UNIX;
  if (connect(s->sockfd, &s->sa, sizeof(s->sa)) != 0) {
    g_free(s);
    return NULL;
  }
  return s;
}


/**
 *  gnet_unix_socket_delete:
 *  @s: UnixSocket to delete.
 *
 *  Close and delete a #GUnixSocket, regardless of reference count.
 *
 **/
void
gnet_unix_socket_delete(GUnixSocket *s)
{
  g_return_if_fail(s != NULL);
  
  GNET_CLOSE_SOCKET(s->sockfd); /* Don't care if this fails. */
  if (s->iochannel)
    g_io_channel_unref(s->iochannel);
  if (s->server)
    gnet_unix_socket_unlink(PATH(s));
  g_free(s);
}

/**
 *  gnet_unix_socket_ref
 *  @s: GUnixSocket to reference
 *
 *  Increment the reference counter of a #GUnixSocket.
 *
 **/
void
gnet_unix_socket_ref(GUnixSocket *s)
{
  g_return_if_fail(s != NULL);
  ++s->ref_count;
}

/**
 *  gnet_unix_socket_unref
 *  @s: GUnixSocket to unreference
 *
 *  Remove a reference from the #GUnixSocket.  When the reference count
 *  reaches 0, the socket is deleted.  
 *
 **/
void
gnet_unix_socket_unref(GUnixSocket *s)
{
  g_return_if_fail(s != NULL);

  --s->ref_count;
  if (s->ref_count == 0) 
    gnet_unix_socket_delete(s);
}
    
/**
 *  gnet_unix_socket_get_io_channel
 *  @socket: GUnixSocket to get GIOChannel from.
 *
 *  Get the #GIOChannel for the #GUnixSocket.
 *
 *  For a client socket, the #GIOChannel represents the data stream.
 *  Use it like you would any other #GIOChannel.
 *
 *  For a server socket, however, the #GIOChannel represents incoming
 *  connections.  If you can read from it, there's a connection
 *  waiting to be accepted.
 *
 *  There is one channel for every socket.  If the channel is refed
 *  then it must be unrefed eventually.  Do not close the channel --
 *  this is done when the socket is deleted.
 *
 *  Returns: A #GIOChannel; NULL on failure.  
 *
 **/
GIOChannel*
gnet_unix_socket_get_io_channel(GUnixSocket *socket)
{
  g_return_val_if_fail(socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = gnet_private_io_channel_new(socket->sockfd);

  return socket->iochannel;
}


/* GNet 1.1 compatibility function (DEPRICATED) */
GIOChannel* 
gnet_unix_socket_get_iochannel (GUnixSocket* socket)
{
  GIOChannel* iochannel;

  g_return_val_if_fail (socket != NULL, NULL);

  iochannel = gnet_unix_socket_get_io_channel (socket);
  if (!iochannel)
    return NULL;

  g_io_channel_ref (iochannel);

  return iochannel;
}


/**
 *  gnet_unix_socket_get_path:
 *  @socket: GUnixSocket to get the path of.
 *
 *  Get the path of the socket.
 *
 *  Returns: Caller-owned path; NULL on failure.
 *
 **/
gchar*
gnet_unix_socket_get_path(const GUnixSocket* socket)
{
  g_return_val_if_fail(socket != NULL, NULL);
  return g_strdup (PATH(socket));
}


/**
 *  gnet_unix_socket_server_new:
 *  @path: Path for the socket.
 *
 *  Create and open a new #GUnixSocket with the specified path.  Use
 *  this sort of socket when you are a server.
 *
 *  Returns: a new #GUnixSocket, or NULL if there was a failure.  
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
  memcpy(sa_un->sun_path, path, strlen(path));
  s->ref_count = 1;
  s->server = TRUE;
  
  if (! gnet_unix_socket_unlink(PATH(s)))
    goto error;
  
  s->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    goto error;

  flags = fcntl(s->sockfd, F_GETFL, 0);
  if (flags == -1)
    goto error;
  /* Make the socket non-blocking */
  if (fcntl(s->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    goto error;

  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    goto error;

  n = sizeof(s->sa);
  /* Get the socket name FIXME: why? */
  if (getsockname(s->sockfd, &s->sa, &n) != 0)
    goto error;

  if (listen(s->sockfd, 10) != 0)
    goto error;

  return s;
	
 error:
  if (s)
    gnet_unix_socket_delete(s);
  return NULL;
}


/**
 *  gnet_unix_socket_server_accept:
 *  @socket: GUnixSocket to accept connections from
 *
 *  Accept connections from the socket.  The socket must have been
 *  created using gnet_unix_socket_server_new(). This function will
 *  block (use gnet_unix_socket_server_accept_nonblock() if you don't
 *  want to block).  If the socket's #GIOChannel is readable, it DOES
 *  NOT mean that this function will block.
 *
 *  Returns: a new #GUnixSocket if there is another connect, or NULL if
 *  there's an error.  
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
 *  gnet_unix_socket_server_accept_nonblock:
 *  @socket: GUnixSocket to accept connections from.
 *
 *  Accept a connection from the socket without blocking.  The socket
 *  must have been created using gnet_unix_socket_server_new().  This
 *  function is best used with the socket's #GIOChannel.  If the channel
 *  is readable, then you PROBABLY have a connection.  It is possible
 *  for the connection to close by the time you call this, so it may
 *  return NULL even if the channel was readable.
 *
 *  Returns: a new #GUnixSocket if there was another connect, or NULL
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
 *  gnet_unix_socket_unlink:
 *  @path: Path to socket to unlink.
 *
 *  Verifies that the file at the end of the path is a socket, and then
 *  unlinks it.
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
