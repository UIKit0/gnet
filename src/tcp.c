/* GNet - Networking library
 * Copyright (C) 2000  David Helder
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
#include "tcp.h"


static gboolean gnet_tcp_socket_new_nonblock_cb (GIOChannel* iochannel, 
						 GIOCondition condition, 
						 gpointer data);

typedef struct _GTcpSocketNonblockState 
{
  GTcpSocket* socket;
  GTcpSocketNonblockFunc func;
  gpointer data;
  gint flags;

} GTcpSocketNonblockState;


static void gnet_tcp_socket_connect_inetaddr_cb(GInetAddr* inetaddr, 
						GInetAddrNonblockStatus status, 
						gpointer data);

static void gnet_tcp_socket_connect_tcp_cb(GTcpSocket* socket, 
					   GTcpSocketNonblockStatus status, 
					   gpointer data);

typedef struct _GTcpSocketConnectState 
{
  GInetAddr* ia;
  GTcpSocketConnectFunc func;
  gpointer data;

} GTcpSocketConnectState;



/**
 *  gnet_tcp_socket_connect:
 *  @hostname: Name of host to connect to
 *  @port: Port to connect to
 *
 *  A quick and easy GTcpSocket constructor.  This connects to the
 *  specified address and port.  This function does block -
 *  gnet_tcp_socket_connect_nonblock() does not.  Use this function
 *  when you're a client connecting to a server and you don't mind
 *  blocking and don't want to mess with GInetAddr's.  You can get the
 *  InetAddr of the socket by calling gnet_tcp_socket_get_inetaddr().
 *
 *  Returns a new TcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket*
gnet_tcp_socket_connect(gchar* hostname, gint port)
{
  GInetAddr* ia;
  GTcpSocket* socket;

  ia = gnet_inetaddr_new(hostname, port);
  if (ia == NULL)
    return NULL;

  socket = gnet_tcp_socket_new(ia);
  gnet_inetaddr_delete(ia);

  return socket;
}


/**
 *  gnet_tcp_socket_connect_nonblock:
 *  @hostname: Name of host to connect to
 *  @port: Port to connect to
 *  @func: Callback function
 *  @data: User data passed when callback function is called.
 *
 *  A quick and easy non-blocking GTcpSocket constructor.  This
 *  connects to the specified address and port and then calls the
 *  callback with the data.  Use this function when you're a client
 *  connecting to a server and you don't want to block and don't want
 *  to mess with GInetAddr's.
 *
 **/
void
gnet_tcp_socket_connect_nonblock(gchar* hostname, gint port, 
				 GTcpSocketConnectFunc func, gpointer data)
{
  GTcpSocketConnectState* state;

  g_return_if_fail(hostname != NULL);
  g_return_if_fail(func != NULL);

  state = g_new0(GTcpSocketConnectState, 1);
  state->func = func;
  state->data = data;

  gnet_inetaddr_new_nonblock(hostname, port,  
			     gnet_tcp_socket_connect_inetaddr_cb, state);
}



static void
gnet_tcp_socket_connect_inetaddr_cb (GInetAddr* inetaddr, 
				     GInetAddrNonblockStatus status, 
				     gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  if (status == GINETADDR_NONBLOCK_STATUS_OK)
    {
      state->ia = inetaddr;

      gnet_tcp_socket_new_nonblock(inetaddr, gnet_tcp_socket_connect_tcp_cb, 
				   state);
    }
  else
    {
      (*state->func)(NULL, NULL, GTCP_CONNECT_STATUS_INETADDR_ERROR, state->data);
      g_free(state);
    }
}


static void 
gnet_tcp_socket_connect_tcp_cb(GTcpSocket* socket, 
			       GTcpSocketNonblockStatus status, 
			       gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  if (status == GTCP_SOCKET_NONBLOCK_STATUS_OK)
    (*state->func)(socket, state->ia, GTCP_CONNECT_STATUS_OK, state->data);
  else
    (*state->func)(NULL, NULL, GTCP_CONNECT_STATUS_TCP_ERROR, state->data);

  g_free(state);
}



/**
 *  gnet_tcp_socket_new:
 *  @addr: Address to connect to.
 *
 *  Connect to a specified address.  Use this sort of socket when
 *  you're a client connecting to a server.  This function will block
 *  to connect.
 *
 *  Returns a new TcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_new(const GInetAddr* addr)
{
  GTcpSocket* s = g_new0(GTcpSocket, 1);
  struct sockaddr_in* sa_in;

  g_return_val_if_fail (addr != NULL, NULL);

  /* Create socket */
  s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    {
      g_free(s);
      return NULL;
    }

  /* Set up address and port for connection */
  memcpy(&s->sa, &addr->sa, sizeof(s->sa));
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;

  if (connect(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    {
      g_free(s);
      return NULL;
    }

  return s;
}


/**
 *  gnet_tcp_socket_new_nonblock:
 *  @addr: Address to connect to.
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 *
 *  Connect to a specifed address and don't blcok.  Use this when you
 *  want to connect to something, but need to do it asynchronously or
 *  with a timeout.  When the connection is complete or there is an
 *  error, it will call the callback.
 *
 *  WARNING: Future versions will probably return something else.  It
 *  doesn't make sense to return the socket since it's not valid yet.
 *  (Or maybe it does if we can ask it its status.)
 *
 *  Returns: A non-valid TcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_new_nonblock(const GInetAddr* addr, 
			     GTcpSocketNonblockFunc func,
			     gpointer data)
{
  gint sockfd;
  gint flags;
  GTcpSocket* s;
  struct sockaddr_in* sa_in;
  GTcpSocketNonblockState* state;

  g_return_val_if_fail(addr != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return NULL;

  /* Get the flags (should all be 0?) */
  flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1)
    return NULL;

  if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    return NULL;

  /* Create our structure */
  s = g_new0(GTcpSocket, 1);
  s->sockfd = sockfd;

  /* Set up address and port for connection */
  memcpy(&s->sa, &addr->sa, sizeof(s->sa));
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;

  /* Connect (but non-blocking!) */
  if (connect(s->sockfd, &s->sa, sizeof(s->sa)) < 0)
    {
      if (errno != EINPROGRESS)
	{
	  g_free(s);
	  return NULL;
	}
    }

  /* Note that if connect returns 0, then we're already connected and
     we could call the call back immediately.  But, it would probably
     make things too complicated for the user if we could call the
     callback before we returned from this function.  */

  /* Wait for the connection */
  state = g_new0(GTcpSocketNonblockState, 1);
  state->socket = s;
  state->func = func;
  state->data = data;
  state->flags = flags;
  g_io_add_watch(g_io_channel_unix_new(s->sockfd), 		/* FIX for WIN! */
		 GNET_ANY_IO_CONDITION,
		 gnet_tcp_socket_new_nonblock_cb, 
		 state);

  return s;
}


static gboolean 
gnet_tcp_socket_new_nonblock_cb (GIOChannel* iochannel, 
				 GIOCondition condition, 
				 gpointer data)
{
  GTcpSocketNonblockState* state = (GTcpSocketNonblockState*) data;
  GTcpSocketNonblockStatus status = GTCP_SOCKET_NONBLOCK_STATUS_ERROR;


  errno = 0;
  if ((condition | G_IO_IN) || (condition | G_IO_OUT))
    {
      gint error, len;
      len = sizeof(error);

      /* Get the error option */
      if (getsockopt(state->socket->sockfd, SOL_SOCKET, SO_ERROR, (void*) &error, &len) >= 0)
	{
	  /* TODO: Solaris pending error in Stevens? */
/*  	  errno = error; */

	  /* Check if there is an error */
	  if (!error)
	    {
	      /* Reset the flags */
	      if (fcntl(state->socket->sockfd, F_SETFL, state->flags) == 0)
		{
		  status = GTCP_SOCKET_NONBLOCK_STATUS_OK;
		}
	    }
	}
    }

/*    if (errno) */
/*        g_warning("tcp_socket_new_nonblock_cb: %s\n", g_strerror(errno)); */


  /* Call back */
  (*state->func)(state->socket, status, state->data);

  g_free(state);

  return FALSE;
}



/**
 *  gnet_tcp_socket_delete:
 *  @s: TcpSocket to delete.
 *
 *  Close and delete a TCP socket.
 *
 **/
void
gnet_tcp_socket_delete(GTcpSocket* s)
{
  if (s != NULL)
    {
      close(s->sockfd);	/* Don't care if this fails... */

      if (s->iochannel)
	g_io_channel_unref(s->iochannel);

      g_free(s);
    }
}



/**
 *  gnet_tcp_socket_get_iochannel:
 *  @socket: GTcpSocket to get GIOChannel from.
 *
 *  Get the IO Channel for the GTcpSocket.
 *
 *  For a client socket, the GIOChannel represents the data stream.
 *  Use it like you would any other GIOChannel.
 *
 *  For a server socket however, the GIOChannel represents incoming
 *  connections.  If you can read from it, there's a connection
 *  waiting.
 *
 *  There is one channel for every socket.  This function refs the
 *  channel before returning it.  You should unref the channel when
 *  you are done with it.  However, you should not close the channel -
 *  this is done when you delete the socket.
 *
 *  Returns: A GIOChannel; NULL on failure.
 *
 **/
GIOChannel* 
gnet_tcp_socket_get_iochannel(GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = g_io_channel_unix_new(socket->sockfd);
  
  g_io_channel_ref (socket->iochannel);

  return socket->iochannel;
}



/**
 *  gnet_tcp_socket_get_inetaddr:
 *  @socket: GTcpSocket to get address of.
 *
 *  Get the address of the socket.  If the socket is client socket,
 *  the address is the address of the remote host it is connected to.
 *  If the socket is a server socket, the address is the address of
 *  the local host.  (Though you should use
 *  gnet_inetaddr_gethostaddr() to get the GInetAddr of the local
 *  host.)
 *
 *  Returns: GInetAddr of socket; NULL on failure.
 *
 **/
GInetAddr* 
gnet_tcp_socket_get_inetaddr(const GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  return gnet_private_inetaddr_sockaddr_new(socket->sa);
}


/**
 *  gnet_tcp_socket_get_port:
 *  @socket: GTcpSocket to get the port number of.
 *
 *  Get the number of the port the socket is bound to.
 *
 *  Returns: Port number of the socket.
 *
 **/
gint
gnet_tcp_socket_get_port(const GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, 0);

  return g_ntohs(GNET_SOCKADDR_IN(socket->sa).sin_port);
}



/* **************************************** */
/* Server stuff */

/*
   [TODO: udp_socket has udp_socket_port if you know the port, make
   this consistent].
*/

/**
 *  gnet_tcp_socket_server_new:
 *  @port: Port number for the socket (0 if you don't care).
 *
 *  Create and open a new TCP socket with the specific port number.
 *  Use this sort of socket when your are a server and you know what
 *  the port number should be (or pass 0 if you don't care what the
 *  port is).
 *
 *  Returns: a new GTcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new(const gint port)
{
  GTcpSocket* s = g_new0(GTcpSocket, 1);
  struct sockaddr_in* sa_in;
  const int on = 1;
  socklen_t socklen;
  gint flags;


  /* Create socket */
  s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    {
      g_free(s);
      return NULL;
    }

  /* Set up address and port for connection */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = g_htons(port);

  /* Set REUSEADDR so we can reuse the port */
  if (setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, 
		     (void*) &on, sizeof(on)) != 0)
    g_warning("Can't set reuse on tcp socket\n");

  /* Get the flags (should all be 0?) */
  flags = fcntl(s->sockfd, F_GETFL, 0);
  if (flags == -1)
    return NULL;

  /* Make the socket non-blocking */
  if (fcntl(s->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    return NULL;

  /* Bind */
  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    {
      g_free(s);
      return NULL;
    }

  /* Get the socket name - don't care if it fails */
  socklen = sizeof(s->sa);
  getsockname(s->sockfd, &s->sa, &socklen);

  /* Listen */
  if (listen(s->sockfd, 10) != 0)
    {
      g_free(s);
      return NULL;
    }

  return s;
}


/**
 *  gnet_tcp_socket_server_accept:
 *  @socket: GTcpSocket to accept connections from.
 *
 *  Accept connections from the socket.  The socket must have been
 *  created using gnet_tcp_socket_server_new().  If you're worried
 *  about this function blocking, get the GIOChannel and wait until
 *  it's readable before calling this.
 *
 *  Returns a new GTcpSocket if there is another connect, or NULL
 *  otherwise.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_accept(GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr sa;
  socklen_t n;
  fd_set fdset;
  GTcpSocket* s;

  g_return_val_if_fail (socket != NULL, NULL);

 try_again:

  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
      if (errno == EINTR)
	goto try_again;

      return NULL;
    }

  n = sizeof(s->sa);

  if ((sockfd = accept(socket->sockfd, &sa, &n)) == -1)
    {
      if (errno == EWOULDBLOCK || errno == ECONNABORTED ||
	  errno == EPROTO || errno == EINTR)
	goto try_again;

      return NULL;
    }

  s = g_new0(GTcpSocket, 1);
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}
