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
 *  @iface: Interface to bind to (NULL for any interface)
 *  @port: Port to bind to (0 for any port)
 *  @func: Callback to call when a connection is accepted
 *  @user_data: Data to pass to callback.
 *
 *  Create a new #GServer object representing a server.  The interface
 *  is specified as in gnet_tcp_socket_server_new_interface().
 *  Usually, @iface is NULL and the port is set to a specific port.
 *  The callback is called whenever a new connection arrives or if the
 *  socket fails.
 *
 *  Returns: A new #GServer.
 *
 **/
GServer*
gnet_server_new (const GInetAddr* iface, gint port, 
		 GServerFunc func, gpointer user_data)
{
  GTcpSocket* socket;
  GServer* server = NULL;

  g_return_val_if_fail (func, NULL);

  socket = gnet_tcp_socket_server_new_interface (iface, port);
  if (!socket)
    return NULL;

  server = g_new0 (GServer, 1);
  server->func = func;
  server->user_data = user_data;
  server->socket = socket;
  server->iface = gnet_tcp_socket_get_inetaddr (server->socket);
  server->port  = gnet_tcp_socket_get_port (server->socket);

  /* Wait for new connections */
  gnet_tcp_socket_server_accept_async (server->socket, 
				       server_accept_cb, server);

  return server;
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
      iochannel = gnet_tcp_socket_get_io_channel (client);
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

