/* GNet - Networking library
 * Copyright (C) 2000-2002  David Helder
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

#include <memory.h>
#include "gnet-private.h"
#include "server.h"



static void server_accept_cb (GTcpSocket* server_socket, GTcpSocket* client, gpointer data);


/**
 *  gnet_server_new:
 *  @iface: Interface to bind to (NULL if any)
 *  @force_port: Fail if can't get requested port
 *  @func: Callback to call when a connection is accepted
 *  @user_data: Data to pass to callback.
 *
 *  Create a new #GServer object representing a server.  The interface
 *  is specified as in gnet_tcp_socket_server_new_interface().
 *  Usually, iface is NULL or the iface is created by
 *  gnet_inetaddr_new_any() and the port is set to a specific port.
 *  The callback is called whenever a new connection arrives or if the
 *  socket fails.
 *
 *  FIX: Remove force_port.  It's easier for someone to call
 *  gnet_server_new again than for me to explain how force port works.
 *  (If force_port is TRUE, and the socket with the specified port
 *  cannot be created, this function fails.  If force_port is FALSE,
 *  the function reattempts to create a socket but lets the OS choose
 *  the port.
 *
 *  Returns: A new #GServer.
 *
 **/
GServer*
gnet_server_new (const GInetAddr* iface, gboolean force_port, 
		 GServerFunc func, gpointer user_data)
{
  GServer* server = NULL;

  g_return_val_if_fail (func, NULL);

  server = g_new0 (GServer, 1);
  server->func = func;
  server->user_data = user_data;

  /* Create a listening socket */
  server->socket = gnet_tcp_socket_server_new_interface (iface);
  if (!server->socket && force_port)
    goto error;

  if (!server->socket && iface)
    {
      GInetAddr iface_cpy;

      iface_cpy = *iface;
      GNET_SOCKADDR_IN(iface_cpy.sa).sin_port = 0;
      server->socket = gnet_tcp_socket_server_new_interface(&iface_cpy);
    }

  if (!server->socket)
    goto error;

  /* Get the port number */
  server->port = gnet_tcp_socket_get_port (server->socket);
  if (server->port == 0)
    goto error;

  /* Get the address */
  server->iface = gnet_tcp_socket_get_inetaddr (server->socket);

  /* Wait for new connections */
  gnet_tcp_socket_server_accept_async (server->socket, 
				       server_accept_cb, server);

  return server;

 error:
  gnet_server_delete (server);
  return NULL;
}



/**
 *  gnet_server_delete:
 *  @server: Server to delete.
 *
 *  Close and delete a #GServer.
 *
 **/
void
gnet_server_delete (GServer* server)
{
  if (server)
    {
      if (server->socket)	 gnet_tcp_socket_delete (server->socket);
      if (server->iface)     	 gnet_inetaddr_delete (server->iface);

      memset (server, 0, sizeof(*server));
      g_free (server);
    }
}



static void
server_accept_cb (GTcpSocket* server_socket, GTcpSocket* client, gpointer data)
{
  GServer* server = (GServer*) data;

  g_return_if_fail (server);

  if (client)
    {
      GIOChannel* iochannel = NULL;
      GConn* conn; 

      /* Get the iochannel */
      iochannel = gnet_tcp_socket_get_iochannel (client);
      g_return_if_fail (iochannel);

      /* Create a Connection */
      conn = g_new0 (GConn, 1);
      conn->socket = client;
      conn->iochannel = iochannel;
      conn->inetaddr = gnet_tcp_socket_get_inetaddr (client);
      conn->hostname = gnet_inetaddr_get_canonical_name (conn->inetaddr);
      conn->port = gnet_inetaddr_get_port (conn->inetaddr);

      (server->func)(server, GNET_SERVER_STATUS_CONNECT, 
		     conn, server->user_data);
    }
  else
    {
      gnet_tcp_socket_server_accept_async_cancel (server_socket);
      (server->func)(server, GNET_SERVER_STATUS_ERROR, 
		     NULL, server->user_data);
    }
}

