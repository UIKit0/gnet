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

#include <memory.h>
#include "gnet-private.h"
#include "server.h"



static gboolean server_accept_cb (GIOChannel* listen_iochannel, 
				  GIOCondition condition, gpointer user_data);


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

  /* Get the iochannel */
  server->iochannel = gnet_tcp_socket_get_iochannel (server->socket);
  if (server->iochannel == NULL)
    goto error;

  /* Wait for new connections */
  server->watch_id = g_io_add_watch (server->iochannel, 
				     G_IO_IN | G_IO_ERR | G_IO_NVAL, 
				     server_accept_cb, server);

  return server;

 error:
  gnet_server_delete (server);
  return NULL;
}



void
gnet_server_delete (GServer* server)
{
  if (server)
    {
      if (server->watch_id)      g_source_remove (server->watch_id);
      if (server->iochannel)     g_io_channel_unref (server->iochannel);
      if (server->socket)	 gnet_tcp_socket_delete (server->socket);
      if (server->iface)     	 gnet_inetaddr_delete (server->iface);

      memset (server, 0, sizeof(*server));
      g_free (server);
    }
}



static gboolean
server_accept_cb (GIOChannel* listen_iochannel, 
		  GIOCondition condition, gpointer user_data)
{
  GServer* server = (GServer*) user_data;

  g_return_val_if_fail (server, FALSE);

  if (condition == G_IO_IN)
    {
      GTcpSocket* socket = NULL;
      GIOChannel* iochannel = NULL;
      GConn* conn; 

      /* Accept the connection */
      socket = gnet_tcp_socket_server_accept_nonblock (server->socket);

      /* Ignore if the connection is now gone */
      if (socket == NULL) 
	return TRUE;

      /* Get the iochannel */
      iochannel = gnet_tcp_socket_get_iochannel (socket);
      g_return_val_if_fail (iochannel, FALSE);

      /* Create a Connection */
      conn = g_new0 (GConn, 1);
      conn->socket = socket;
      conn->iochannel = iochannel;
      conn->inetaddr = gnet_tcp_socket_get_inetaddr (socket);
      conn->hostname = gnet_inetaddr_get_canonical_name (conn->inetaddr);
      conn->port = gnet_inetaddr_get_port (conn->inetaddr);

      (server->func)(server, GNET_SERVER_STATUS_CONNECT, 
		     conn, server->user_data);
    }
  else
    {
      (server->func)(server, GNET_SERVER_STATUS_ERROR, 
		     NULL, server->user_data);
      return FALSE;
    }

  return TRUE;
}

