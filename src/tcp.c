/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2000  Andrew Lanoix
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



/**
 *  gnet_tcp_socket_connect:
 *  @hostname: Name of host to connect to
 *  @port: Port to connect to
 *
 *  A quick and easy #GTcpSocket constructor.  This connects to the
 *  specified address and port.  This function does block
 *  (gnet_tcp_socket_connect_async() does not).  Use this function
 *  when you're a client connecting to a server and you don't mind
 *  blocking and don't want to mess with #GInetAddr's.  You can get
 *  the InetAddr of the socket by calling
 *  gnet_tcp_socket_get_inetaddr().
 *
 *  Returns: A new #GTcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket*
gnet_tcp_socket_connect (const gchar* hostname, gint port)
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
 *  gnet_tcp_socket_connect_async:
 *  @hostname: Name of host to connect to
 *  @port: Port to connect to
 *  @func: Callback function
 *  @data: User data passed when callback function is called.
 *
 *  A quick and easy non-blocking #GTcpSocket constructor.  This
 *  connects to the specified address and port and then calls the
 *  callback with the data.  Use this function when you're a client
 *  connecting to a server and you don't want to block or mess with
 *  #GInetAddr's.  It may call the callback before the function
 *  returns.  It will call the callback if there is a failure.
 *
 *  Returns: ID of the connection which can be used with
 *  gnet_tcp_socket_connect_async_cancel() to cancel it; NULL on
 *  failure.
 *
 **/
GTcpSocketConnectAsyncID
gnet_tcp_socket_connect_async (const gchar* hostname, gint port, 
			       GTcpSocketConnectAsyncFunc func, 
			       gpointer data)
{
  GTcpSocketConnectState* state;
  gpointer id;

  g_return_val_if_fail(hostname != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  state = g_new0(GTcpSocketConnectState, 1);
  state->func = func;
  state->data = data;

  id = gnet_inetaddr_new_async(hostname, port,  
			       gnet_tcp_socket_connect_inetaddr_cb, 
			       state);

  /* Note that gnet_inetaddr_new_async can fail immediately and call
     our callback which will delete the state.  The users callback
     would be called in the process. */

  if (id == NULL)
    return NULL;

  state->inetaddr_id = id;

  return state;
}



void
gnet_tcp_socket_connect_inetaddr_cb (GInetAddr* inetaddr, 
				     GInetAddrAsyncStatus status, 
				     gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  if (status == GINETADDR_ASYNC_STATUS_OK)
    {
      state->ia = inetaddr;

      state->inetaddr_id = NULL;
      state->tcp_id = gnet_tcp_socket_new_async(inetaddr, 
						gnet_tcp_socket_connect_tcp_cb, 
						state);
      /* Note that this call may delete the state. */
    }
  else
    {
      (*state->func)(NULL, NULL, GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR, 
		     state->data);
      g_free(state);
    }
}


void 
gnet_tcp_socket_connect_tcp_cb(GTcpSocket* socket, 
			       GTcpSocketConnectAsyncStatus status, 
			       gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  if (status == GTCP_SOCKET_NEW_ASYNC_STATUS_OK)
    (*state->func)(socket, state->ia, GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK, state->data);
  else
    (*state->func)(NULL, NULL, GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR, state->data);

  g_free(state);
}


/**
 *  gnet_tcp_socket_connect_async_cancel:
 *  @id: ID of the connection.
 *
 *  Cancel an asynchronous connection that was started with
 *  gnet_tcp_socket_connect_async().
 * 
 */
void
gnet_tcp_socket_connect_async_cancel(GTcpSocketConnectAsyncID id)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) id;

  g_return_if_fail (state != NULL);

  if (state->inetaddr_id)
    {
      gnet_inetaddr_new_async_cancel(state->inetaddr_id);
    }
  else if (state->tcp_id)
    {
      gnet_inetaddr_delete(state->ia);
      gnet_tcp_socket_new_async_cancel(state->tcp_id);
    }

  g_free (state);
}



/**
 *  gnet_tcp_socket_new:
 *  @addr: Address to connect to.
 *
 *  Connect to a specified address.  Use this sort of socket when
 *  you're a client connecting to a server.  This function will block
 *  to connect.
 *
 *  Returns a new #GTcpSocket, or NULL if there was a failure.
 *
 */
GTcpSocket* 
gnet_tcp_socket_new(const GInetAddr* addr)
{
  GTcpSocket* s = g_new0(GTcpSocket, 1);
  struct sockaddr_in* sa_in;

  g_return_val_if_fail (addr != NULL, NULL);

  /* Create socket */
  s->ref_count = 1;
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


#ifndef GNET_WIN32  /*********** Unix code ***********/


/**
 *  gnet_tcp_socket_new_async:
 *  @addr: Address to connect to.
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 *
 *  Connect to a specifed address asynchronously.  When the connection
 *  is complete or there is an error, it will call the callback.  It
 *  may call the callback before the function returns.  It will call
 *  the callback if there is a failure.
 *
 *  Returns: ID of the connection which can be used with
 *  gnet_tcp_socket_connect_async_cancel() to cancel it; NULL on
 *  failure.
 *
 **/
GTcpSocketNewAsyncID
gnet_tcp_socket_new_async(const GInetAddr* addr, 
			  GTcpSocketNewAsyncFunc func,
			  gpointer data)
{
  gint sockfd;
  gint flags;
  GTcpSocket* s;
  struct sockaddr_in* sa_in;
  GTcpSocketAsyncState* state;

  g_return_val_if_fail(addr != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
      return NULL;
    }

  /* Get the flags (should all be 0?) */
  flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1)
    {
      (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
      return NULL;
    }

  if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
      return NULL;
    }

  /* Create our structure */
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
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
	  (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
	  return NULL;
	}
    }

  /* Note that if connect returns 0, then we're already connected and
     we could call the call back immediately.  But, it would probably
     make things too complicated for the user if we could call the
     callback before we returned from this function.  */

  /* Wait for the connection */
  state = g_new0(GTcpSocketAsyncState, 1);
  state->socket = s;
  state->func = func;
  state->data = data;
  state->flags = flags;
  state->connect_watch = g_io_add_watch(GNET_SOCKET_IOCHANNEL_NEW(s->sockfd),
					GNET_ANY_IO_CONDITION,
					gnet_tcp_socket_new_async_cb, 
					state);

  return state;
}


gboolean 
gnet_tcp_socket_new_async_cb (GIOChannel* iochannel, 
			      GIOCondition condition, 
			      gpointer data)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) data;

  /* Remove the watch now in case we don't return immediately */
  g_source_remove (state->connect_watch);

  errno = 0;
  if ((condition & G_IO_IN) || (condition & G_IO_OUT))
    {
      gint error, len;
      len = sizeof(error);

      /* Get the error option */
      if (getsockopt(state->socket->sockfd, SOL_SOCKET, SO_ERROR, (void*) &error, &len) >= 0)
	{
	  /* Check if there is an error */
	  if (!error)
	    {
	      /* Reset the flags */
	      if (fcntl(state->socket->sockfd, F_SETFL, state->flags) == 0)
		{
		  (*state->func)(state->socket, GTCP_SOCKET_NEW_ASYNC_STATUS_OK, state->data);
		  g_free(state);
		  return FALSE;
		}
	    }
	}
    }

  /* Otherwise, there was an error */
  (*state->func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, state->data);
  gnet_tcp_socket_delete(state->socket);
  g_free(state);

  return FALSE;
}


/**
 *  gnet_tcp_socket_new_async_cancel:
 *  @id: ID of the connection.
 *
 *  Cancel an asynchronous connection that was started with
 *  gnet_tcp_socket_new_async().
 *
 **/
void
gnet_tcp_socket_new_async_cancel(GTcpSocketNewAsyncID id)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) id;

  g_source_remove(state->connect_watch);
  gnet_tcp_socket_delete(state->socket);
  g_free (state);
}

#else	/*********** Windows code ***********/


GTcpSocketNewAsyncID
gnet_tcp_socket_new_async(const GInetAddr* addr,
			  GTcpSocketNewAsyncFunc func,
			  gpointer data)
{
  gint sockfd;
  gint status;
  GTcpSocket* s;
  struct sockaddr_in* sa_in;
  GTcpSocketAsyncState* state;
	u_long arg;

  g_return_val_if_fail(addr != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == INVALID_SOCKET)
    {
      (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
      return NULL;
    }
	
  /* Create our structure */
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;

  /* Set up address and port for connection */
  memcpy(&s->sa, &addr->sa, sizeof(s->sa));
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;

	/* Force the socket into non-blocking mode */
	arg = 1;
	ioctlsocket(sockfd, FIONBIO, &arg);

  status = connect(s->sockfd, &s->sa, sizeof(s->sa));
  if (status == SOCKET_ERROR) /* Returning an error is ok, unless.. */
    {
      status = WSAGetLastError();
      if (status != WSAEWOULDBLOCK)
	{
	  (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
	  g_free(s);
	  return NULL;
	}
    }
  /* Wait for the connection */
  state = g_new0(GTcpSocketAsyncState, 1);
  state->socket = s;
  state->func = func;
  state->data = data;
  state->socket->sockfd = sockfd;

	state->connect_watch = g_io_add_watch(GNET_SOCKET_IOCHANNEL_NEW(s->sockfd),
					G_IO_IN | G_IO_ERR,
					gnet_tcp_socket_new_async_cb, 
					state);


	if (state->connect_watch <= 0)
    {
      (func)(NULL, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, data);
      return NULL;
    }

  return state;
}



gboolean
gnet_tcp_socket_new_async_cb (GIOChannel* iochannel,
			      GIOCondition condition,
			      gpointer data)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) data;

  if (condition & G_IO_ERR)
    {
      (*state->func)(state->socket, GTCP_SOCKET_NEW_ASYNC_STATUS_ERROR, 
		     state->data);
      g_free(state);
      return FALSE;
    }

  (*state->func)(state->socket, GTCP_SOCKET_NEW_ASYNC_STATUS_OK, state->data);
	g_free(state);

  return FALSE;
}


void
gnet_tcp_socket_new_async_cancel(GTcpSocketNewAsyncID id)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) id;
	
  g_source_remove(state->connect_watch);
  gnet_tcp_socket_delete(state->socket);
  g_free (state);
}

#endif		/*********** End Windows code ***********/



/**
 *  gnet_tcp_socket_delete:
 *  @s: TcpSocket to delete.
 *
 *  Close and delete a #GTcpSocket.
 *
 **/
void
gnet_tcp_socket_delete(GTcpSocket* s)
{
  if (s != NULL)
    gnet_tcp_socket_unref(s);
}



/**
 *  gnet_tcp_socket_ref
 *  @s: GTcpSocket to reference
 *
 *  Increment the reference counter of the GTcpSocket.
 *
 **/
void
gnet_tcp_socket_ref(GTcpSocket* s)
{
  g_return_if_fail(s != NULL);

  ++s->ref_count;
}


/**
 *  gnet_tcp_socket_unref
 *  @s: #GTcpSocket to unreference
 *
 *  Remove a reference from the #GTcpSocket.  When reference count
 *  reaches 0, the socket is deleted.
 *
 **/
void
gnet_tcp_socket_unref(GTcpSocket* s)
{
  g_return_if_fail(s != NULL);

  --s->ref_count;

  if (s->ref_count == 0)
    {
      GNET_CLOSE_SOCKET(s->sockfd);	/* Don't care if this fails... */

      if (s->iochannel)
	g_io_channel_unref(s->iochannel);

      g_free(s);
    }
}



/**
 *  gnet_tcp_socket_get_iochannel:
 *  @socket: GTcpSocket to get GIOChannel from.
 *
 *  Get the #GIOChannel for the #GTcpSocket.
 *
 *  For a client socket, the #GIOChannel represents the data stream.
 *  Use it like you would any other #GIOChannel.
 *
 *  For a server socket however, the #GIOChannel represents incoming
 *  connections.  If you can read from it, there's a connection
 *  waiting.
 *
 *  There is one channel for every socket.  This function refs the
 *  channel before returning it.  You should unref the channel when
 *  you are done with it.  However, you should not close the channel -
 *  this is done when you delete the socket.
 *
 *  Returns: A #GIOChannel; NULL on failure.
 *
 **/
GIOChannel* 
gnet_tcp_socket_get_iochannel(GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = GNET_SOCKET_IOCHANNEL_NEW(socket->sockfd);
  
  g_io_channel_ref (socket->iochannel);

  return socket->iochannel;
}



/**
 *  gnet_tcp_socket_get_inetaddr:
 *  @socket: #GTcpSocket to get address of.
 *
 *  Get the address of the socket.  If the socket is client socket,
 *  the address is the address of the remote host it is connected to.
 *  If the socket is a server socket, the address is the address of
 *  the local host.  (Though you should use
 *  gnet_inetaddr_gethostaddr() to get the #GInetAddr of the local
 *  host.)
 *
 *  Returns: #GInetAddr of socket; NULL on failure.
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
 *  Get the port number the socket is bound to.
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

/**
 *  gnet_tcp_socket_set_tos:
 *  @socket: GTcpSocket to set the type-of-service of
 *  @tos: Type of service (in tcp.h)
 *
 *  Set the type-of-service of the socket.  Usually, routers don't
 *  honor this.  Some systems don't support this function.  If the
 *  operating system does not support it, the function does nothing.
 *
 **/
void
gnet_tcp_socket_set_tos (GTcpSocket* socket, GNetTOS tos)
{
  int sotos;

  g_return_if_fail (socket != NULL);

  /* Some systems (e.g. OpenBSD) do not have IPTOS_*.  Other systems
     have some of them, but not others.  And some systems have them,
     but with different names (e.g. FreeBSD has IPTOS_MINCOST).  If a
     system does not have a IPTOS, or any of them, then this function
     does nothing.  */
  switch (tos)
    {
#ifdef IPTOS_LOWDELAY
    case GNET_TOS_LOWDELAY:	sotos = IPTOS_LOWDELAY;		break;
#endif
#ifdef IPTOS_THROUGHPUT
    case GNET_TOS_THROUGHPUT:	sotos = IPTOS_THROUGHPUT;	break;
#endif
#ifdef IPTOS_RELIABILITY
    case GNET_TOS_RELIABILITY:	sotos = IPTOS_RELIABILITY;	break;
#endif
#ifdef IPTOS_LOWCOST
    case GNET_TOS_LOWCOST:	sotos = IPTOS_LOWCOST;		break;
#else
#ifdef IPTOS_MINCOST	/* Called MINCOST in FreeBSD 4.0 */
    case GNET_TOS_LOWCOST:	sotos = IPTOS_MINCOST;		break;
#endif
#endif
    default: return;
    }

#ifdef IP_TOS
  if (setsockopt(socket->sockfd, IPPROTO_IP, IP_TOS, (void*) &sotos, sizeof(sotos)) != 0)
    g_warning ("Can't set TOS on TCP socket\n");
#endif

}


/* **************************************** */
/* Server stuff */


/**
 *  gnet_tcp_socket_server_new:
 *  @port: Port number for the socket (0 if you don't care).
 *
 *  Create and open a new #GTcpSocket with the specified port number.
 *  Use this sort of socket when your are a server and you know what
 *  the port number should be (or pass 0 if you don't care what the
 *  port is).
 *
 *  Returns: a new #GTcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new (gint port)
{
  GInetAddr iface;
  struct sockaddr_in* sa_in;

  /* Set up address and port (any address, any port) */
  memset (&iface, 0, sizeof(iface));
  sa_in = (struct sockaddr_in*) &iface.sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = g_htons(port);

  return gnet_tcp_socket_server_new_interface (&iface);
}



/**
 *  gnet_tcp_socket_server_new2:
 *  @iface: Interface to bind to (NULL if all interfaces)
 *  @port: Port number for the socket (0 if you don't care).
 *
 *  Create and open a new #GTcpSocket with the specified port number.
 *  If the interface is specified, it will bind to that interface.
 *  Otherwise, it will bind to all interfaces.  Use this sort of
 *  socket when your are a server and you know what the port number
 *  should be (or pass 0 if you don't care what the port is).
 *
 *  WARNING: gnet_tcp_socket_server_new2() will eliminated in GNet
 *  1.2.  Use gnet_tcp_socket_server_interface_new instead.
 *
 *  Returns: a new #GTcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new2 (const GInetAddr* iface, gint port)
{
  GTcpSocket* s;
  struct sockaddr_in* sa_in;
  const int on = 1;
  socklen_t socklen;

  /* Create socket */
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    goto error;

  /* Set up address and port for connection */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  if (iface)
    sa_in->sin_addr.s_addr = GNET_SOCKADDR_IN(iface->sa).sin_addr.s_addr;
  else
    sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = g_htons(port);

  /* The socket is set to non-blocking mode later in the Windows
     version.*/
#ifndef GNET_WIN32
  {
    gint flags;

    /* Set REUSEADDR so we can reuse the port */
    if (setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, 
		   (void*) &on, sizeof(on)) != 0)
      g_warning("Can't set reuse on tcp socket\n");

    /* Get the flags (should all be 0?) */
    flags = fcntl(s->sockfd, F_GETFL, 0);
    if (flags == -1)
      goto error;

    /* Make the socket non-blocking */
    if (fcntl(s->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
      goto error;
  }
#endif

  /* Bind */
  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    goto error;

  /* Get the socket name - don't care if it fails */
  socklen = sizeof(s->sa);
  if (getsockname(s->sockfd, &s->sa, &socklen) != 0)
    goto error;

  /* Listen */
  if (listen(s->sockfd, 10) != 0)
    goto error;

  return s;

 error:
  if (s)    		g_free(s);
  return NULL;
}


/**
 *  gnet_tcp_socket_server_interface_new:
 *  @iface: Interface to bind to
 *
 *  Create and open a new #GTcpSocket bound to the specified
 *  interface.  Use this sort of socket when your are a server and
 *  have a specific address the server must be bound to.  If the
 *  interface address's port number is 0, the OS will choose the port.
 *
 *  Returns: a new #GTcpSocket, or NULL if there was a failure.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new_interface (const GInetAddr* iface)
{
  GTcpSocket* s;
  struct sockaddr_in* sa_in;
  const int on = 1;
  socklen_t socklen;

  g_return_val_if_fail (iface, NULL);

  /* Create socket */
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sockfd < 0)
    goto error;

  /* Set up address and port for connection */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_addr.s_addr = GNET_SOCKADDR_IN(iface->sa).sin_addr.s_addr;
  sa_in->sin_port = GNET_SOCKADDR_IN(iface->sa).sin_port;

  /* The socket is set to non-blocking mode later in the Windows
     version.*/
#ifndef GNET_WIN32
  {
    gint flags;

    /* Set REUSEADDR so we can reuse the port */
    if (setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR, 
		   (void*) &on, sizeof(on)) != 0)
      g_warning("Can't set reuse on tcp socket\n");

    /* Get the flags (should all be 0?) */
    flags = fcntl(s->sockfd, F_GETFL, 0);
    if (flags == -1)
      goto error;

    /* Make the socket non-blocking */
    if (fcntl(s->sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
      goto error;
  }
#endif

  /* Bind */
  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    goto error;

  /* Get the socket name - don't care if it fails */
  socklen = sizeof(s->sa);
  if (getsockname(s->sockfd, &s->sa, &socklen) != 0)
    goto error;

  /* Listen */
  if (listen(s->sockfd, 10) != 0)
    goto error;

  return s;

 error:
  if (s)    		g_free(s);
  return NULL;
}




#ifndef GNET_WIN32  /*********** Unix code ***********/


/**
 *  gnet_tcp_socket_server_accept:
 *  @socket: #GTcpSocket to accept connections from.
 *
 *  Accept a connection from the socket.  The socket must have been
 *  created using gnet_tcp_socket_server_new().  This function will
 *  block (use gnet_tcp_socket_server_accept_nonblock() if you don't
 *  want to block).  If the socket's #GIOChannel is readable, it DOES
 *  NOT mean that this function will not block.
 *
 *  Returns: a new #GTcpSocket if there is another connect, or NULL if
 *  there's an error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_accept (const GTcpSocket* socket)
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
      if (errno == EWOULDBLOCK || 
	  errno == ECONNABORTED ||
#ifdef EPROTO		/* OpenBSD does not have EPROTO */
	  errno == EPROTO || 
#endif
	  errno == EINTR)
	goto try_again;

      return NULL;
    }

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}





/**
 *  gnet_tcp_socket_server_accept_nonblock:
 *  @socket: GTcpSocket to accept connections from.
 *
 *  Accept a connection from the socket without blocking.  The socket
 *  must have been created using gnet_tcp_socket_server_new().  This
 *  function is best used with the sockets #GIOChannel.  If the
 *  channel is readable, then you PROBABLY have a connection.  It is
 *  possible for the connection to close by the time you call this, so
 *  it may return NULL even if the channel was readable.
 *
 *  Returns a new GTcpSocket if there is another connect, or NULL
 *  otherwise.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_accept_nonblock (const GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr sa;
  socklen_t n;
  fd_set fdset;
  GTcpSocket* s;
  struct timeval tv = {0, 0};

  g_return_val_if_fail (socket != NULL, NULL);

 try_again:

  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, &tv) == -1)
    {
      if (errno == EINTR)
	goto try_again;

      return NULL;
    }

  n = sizeof(sa);
  if ((sockfd = accept(socket->sockfd, &sa, &n)) == -1)
    {
      /* If we get an error, return.  We don't want to try again as we
         do in gnet_tcp_socket_server_accept() - it might cause a
         block. */

      return NULL;
    }
  
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}


#else	/*********** Windows code ***********/


GTcpSocket*
gnet_tcp_socket_server_accept (const GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr sa;
  gint n;
  fd_set fdset;
  GTcpSocket* s;

  g_return_val_if_fail (socket != NULL, NULL);

	
  FD_ZERO(&fdset);
  FD_SET((unsigned)socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
      return NULL;
    }

  /* Don't force the socket into blocking mode */

  sockfd = accept(socket->sockfd, &sa, NULL);
  /* if it fails, looping isn't going to help */

  if (sockfd == INVALID_SOCKET)
    {
      return NULL;
    }

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}

GTcpSocket*
gnet_tcp_socket_server_accept_nonblock (const GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr sa;

  fd_set fdset;
  GTcpSocket* s;
  u_long arg;

  g_return_val_if_fail (socket != NULL, NULL);
  FD_ZERO(&fdset);
  FD_SET((unsigned)socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
      return NULL;
    }
  /* make sure the socket is in non-blocking mode */

  arg = 1;
  if(ioctlsocket(socket->sockfd, FIONBIO, &arg))
    return NULL;

  sockfd = accept(socket->sockfd, &sa, NULL);
  /* if it fails, looping isn't going to help */

  if (sockfd == INVALID_SOCKET)
    {
      return NULL;
    }

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  memcpy(&s->sa, &sa, sizeof(s->sa));

  return s;
}


#endif		/*********** End Windows code ***********/
