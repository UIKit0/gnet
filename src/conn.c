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


#include "conn.h"



typedef struct _QueuedWrite
{
  gchar* buffer;
  gint length;
  gint timeout;

} QueuedWrite;

static void conn_check_queued_writes (GConn* conn);

static void conn_connect_cb (GTcpSocket* socket, GInetAddr* ia,
			     GTcpSocketConnectAsyncStatus status, 
			     gpointer user_data);

static void conn_new_cb (GTcpSocket* socket, 
			 GTcpSocketNewAsyncStatus status, 
			 gpointer user_data);

static void conn_write_cb (GIOChannel* iochannel, 
			   gchar* buffer, guint length, guint bytes_writen,
			   GNetIOChannelWriteAsyncStatus status, 
			   gpointer user_data);

static gboolean conn_read_cb (GIOChannel* iochannel, 
			      GNetIOChannelReadAsyncStatus status, 
			      gchar* buffer, guint length, gpointer user_data);

static gboolean conn_timeout_cb (gpointer data);


GConn*
gnet_conn_new (gchar* hostname, gint port, GConnFunc func, gpointer user_data)
{
  GConn* conn;

  g_return_val_if_fail (hostname, NULL);

  conn = g_new0 (GConn, 1);
  conn->ref_count = 1;
  conn->hostname = g_strdup(hostname);
  conn->port = port;
  conn->inetaddr = gnet_inetaddr_new_nonblock (hostname, port);
  conn->func = func;
  conn->user_data = user_data;

  return conn;
}


GConn*   
gnet_conn_new_inetaddr (GInetAddr* inetaddr, 
			GConnFunc func, gpointer user_data)
{
  GConn* conn;

  g_return_val_if_fail (inetaddr, NULL);

  conn = g_new0 (GConn, 1);
  conn->ref_count = 1;
  conn->hostname = gnet_inetaddr_get_canonical_name (inetaddr);
  conn->port = gnet_inetaddr_get_port (inetaddr);
  conn->inetaddr = gnet_inetaddr_clone (inetaddr);
  conn->func = func;
  conn->user_data = user_data;

  return conn;
}



void
gnet_conn_delete (GConn* conn, gboolean delete_buffers)
{
  if (conn)
    {
      gnet_conn_disconnect (conn, delete_buffers);

      if (conn->inetaddr)
	gnet_inetaddr_delete (conn->inetaddr);

      g_free (conn->hostname);

      if (conn->timer)
	g_source_remove (conn->timer);

      memset (conn, 0, sizeof(*conn));
      g_free (conn);
    }
}


void
gnet_conn_ref (GConn* conn)
{
  g_return_if_fail (conn);

  ++conn->ref_count;
}


void
gnet_conn_unref (GConn* conn, gboolean delete_buffers)
{
  g_return_if_fail (conn);

  --conn->ref_count;

  if (conn->ref_count == 0)
    gnet_conn_delete (conn, delete_buffers);
}


void
gnet_conn_connect (GConn* conn, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  if (conn->connect_id || conn->new_id || conn->socket)
    return;

  if (conn->inetaddr)
    conn->new_id = 
      gnet_tcp_socket_new_async (conn->inetaddr, conn_new_cb, conn);
  
  else if (conn->hostname)
    conn->connect_id = 
      gnet_tcp_socket_connect_async (conn->hostname, conn->port, 
				     conn_connect_cb, conn);
  else
    g_return_if_fail (FALSE);
}


static void
conn_connect_cb (GTcpSocket* socket, GInetAddr* ia,
		 GTcpSocketConnectAsyncStatus status, 
		 gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  GConnStatus st = GNET_CONN_STATUS_ERROR;

  g_return_if_fail (conn);

  conn->connect_id = NULL;

  if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      conn->socket = socket;
      conn->inetaddr = ia;
      conn->iochannel = gnet_tcp_socket_get_iochannel (socket);

      st = GNET_CONN_STATUS_CONNECT;

      conn_check_queued_writes (conn);
    }

  (conn->func)(conn, st, NULL, 0, conn->user_data);
}


static void
conn_new_cb (GTcpSocket* socket, 
	     GTcpSocketNewAsyncStatus status, 
	     gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  GConnStatus st = GNET_CONN_STATUS_ERROR;

  g_return_if_fail (conn);

  conn->new_id = NULL;

  if (status == GTCP_SOCKET_NEW_ASYNC_STATUS_OK)
    {
      conn->socket = socket;
      conn->iochannel = gnet_tcp_socket_get_iochannel (socket);

      st = GNET_CONN_STATUS_CONNECT;

      conn_check_queued_writes (conn);
    }

  (conn->func)(conn, st, NULL, 0, conn->user_data);
}



void
gnet_conn_disconnect (GConn* conn, gboolean delete_buffers)
{
  GList* i;

  g_return_if_fail (conn);

  if (conn->connect_id)
    {
      gnet_tcp_socket_connect_async_cancel (conn->connect_id);
      conn->connect_id = NULL;
    }

  if (conn->new_id)
    {
      gnet_tcp_socket_new_async_cancel (conn->new_id);
      conn->new_id = NULL;
    }

  gnet_conn_watch_remove_read  (conn);
  gnet_conn_watch_remove_write (conn);
  gnet_conn_watch_remove_error (conn);

  for (i = conn->queued_writes; i != NULL; i = i->next)
    {
      QueuedWrite* queued_write = i->data;

      if (delete_buffers)
	g_free (queued_write->buffer);

      g_free (queued_write);
    }
  g_list_free (conn->queued_writes);
  conn->queued_writes = NULL;

  if (conn->write_id)
    {
      gnet_io_channel_write_async_cancel (conn->write_id, delete_buffers);
      conn->write_id = NULL;
    }
      
  if (conn->read_id)
    {
      gnet_io_channel_read_async_cancel (conn->read_id);
      conn->read_id = NULL;
    }

  if (conn->iochannel)
    {
      g_io_channel_unref (conn->iochannel);
      conn->iochannel = NULL;
    }

  if (conn->socket)
    {
      gnet_tcp_socket_delete (conn->socket);
      conn->socket = NULL;
    }
}


gboolean
gnet_conn_is_connected (GConn* conn)
{
  g_return_val_if_fail (conn, FALSE);

  return (conn->socket != NULL);
}


void
gnet_conn_watch_add_read (GConn* conn, GIOFunc func, gpointer user_data)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->iochannel);
  g_return_if_fail (!conn->read_id);

  if (!conn->read_watch)
    conn->read_watch = g_io_add_watch (conn->iochannel, G_IO_IN, func, user_data);
}


void
gnet_conn_watch_add_write (GConn* conn, GIOFunc func, gpointer user_data)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->iochannel);
  g_return_if_fail (!conn->write_id);

  if (!conn->write_watch)
    conn->write_watch = g_io_add_watch (conn->iochannel, G_IO_OUT, func, user_data);
}


void
gnet_conn_watch_add_error (GConn* conn, GIOFunc func, gpointer user_data)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->iochannel);

  if (!conn->err_watch)
    conn->err_watch  = 
      g_io_add_watch (conn->iochannel, 
		      G_IO_ERR | G_IO_HUP | G_IO_NVAL, func, user_data);
}


void
gnet_conn_watch_remove_read (GConn* conn)
{
  g_return_if_fail (conn);

  if (conn->read_watch)
    {
      g_assert (g_source_remove(conn->read_watch));
      conn->read_watch = 0;
    }
}


void
gnet_conn_watch_remove_write (GConn* conn)
{
  g_return_if_fail (conn);

  if (conn->write_watch)
    {
      g_assert (g_source_remove(conn->write_watch));
      conn->write_watch = 0;
    }
}


void
gnet_conn_watch_remove_error (GConn* conn)
{
  g_return_if_fail (conn);

  if (conn->err_watch)
    {
      g_assert (g_source_remove(conn->err_watch));
      conn->err_watch = 0;
    }
}


void
gnet_conn_read (GConn* conn, gchar* buffer, guint length, guint timeout,
		gboolean read_one_byte_at_a_time,
		GNetIOChannelReadAsyncCheckFunc check_func, 
		gpointer check_user_data)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->iochannel);
  g_return_if_fail (conn->func);
  g_return_if_fail (!conn->read_id);

  conn->read_id = 
    gnet_io_channel_read_async (conn->iochannel, buffer, length, timeout,
				read_one_byte_at_a_time,
				check_func, check_user_data,
				conn_read_cb, conn);
}


void
gnet_conn_readany (GConn* conn, gchar* buffer, guint length, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (buffer);
  g_return_if_fail (conn->func);
  g_return_if_fail (conn->iochannel);
  g_return_if_fail (!conn->read_id);

  conn->read_id = 
    gnet_io_channel_readany_async(conn->iochannel, buffer, length, 
				  timeout, conn_read_cb, conn);
}


void
gnet_conn_readline (GConn* conn, gchar* buffer, guint length, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);
  g_return_if_fail (conn->iochannel);
  g_return_if_fail (!conn->read_id);

  conn->read_id = 
    gnet_io_channel_readline_async(conn->iochannel, buffer, length, timeout,
				   conn_read_cb, conn);
}


static gboolean
conn_read_cb (GIOChannel* iochannel, GNetIOChannelReadAsyncStatus status, 
	      gchar* buffer, guint length, gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  gpointer read_id;

  g_return_val_if_fail (conn, FALSE);

  /* If the upper level calls disconnect, we don't want it to delete
     the read_id. */
  read_id = conn->read_id;
  conn->read_id = NULL;

  if (status == GNET_IOCHANNEL_READ_ASYNC_STATUS_OK)
    {
      if (length)
	{
	  gboolean rv;

	  rv = (conn->func)(conn, GNET_CONN_STATUS_READ, buffer, length, conn->user_data);
	  if (rv)
	    conn->read_id = read_id;
	  return rv;
	}
      else
	{
	  (conn->func)(conn, GNET_CONN_STATUS_CLOSE, NULL, 0, conn->user_data);
	}
    }
  else
    {
      (conn->func)(conn, GNET_CONN_STATUS_ERROR, NULL, 0, conn->user_data);
    }

  return FALSE;
}


void
gnet_conn_write (GConn* conn, gchar* buffer, gint length, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  if (conn->iochannel && !conn->write_id)
    {
      conn->write_id = 
	gnet_io_channel_write_async (conn->iochannel, buffer, length,
				     timeout, conn_write_cb, conn);
    }
  else
    {
      QueuedWrite* queued_write = g_new0 (QueuedWrite, 1);
      queued_write->buffer = buffer;
      queued_write->length = length;
      queued_write->timeout = timeout;

      conn->queued_writes = g_list_append (conn->queued_writes, queued_write);
    }
}


static void
conn_write_cb (GIOChannel* iochannel, gchar* buffer, guint length, 
	       guint bytes_writen,
	       GNetIOChannelWriteAsyncStatus status, gpointer user_data)
{
  GConn* conn = (GConn*) user_data;

  g_return_if_fail (conn);

  conn->write_id = NULL;

  if (status == GNET_IOCHANNEL_WRITE_ASYNC_STATUS_OK)
    {
      conn_check_queued_writes (conn);

      (conn->func)(conn, GNET_CONN_STATUS_WRITE, buffer, length, conn->user_data);
    }
  else
    {
      (conn->func)(conn, GNET_CONN_STATUS_ERROR, NULL, 0, conn->user_data);
    }
}


void
gnet_conn_timeout (GConn* conn, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  if (conn->timer)
    {
      g_source_remove (conn->timer);
      conn->timer = 0;
    }

  if (timeout)
    {
      conn->timer = g_timeout_add (timeout, conn_timeout_cb, conn);
    }
}


static gboolean 
conn_timeout_cb (gpointer data)
{
  GConn* conn = (GConn*) data;

  g_return_val_if_fail (conn, FALSE);

  (conn->func)(conn, GNET_CONN_STATUS_TIMEOUT, NULL, 0, conn->user_data);

  return FALSE;
}


static void
conn_check_queued_writes (GConn* conn)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->iochannel);

  if (conn->write_id)
    g_return_if_fail (FALSE);

  if (conn->queued_writes)
    {
      QueuedWrite* queued_write = conn->queued_writes->data;
      conn->queued_writes = g_list_remove_link (conn->queued_writes, 
						conn->queued_writes);

      conn->write_id = 
	gnet_io_channel_write_async (conn->iochannel, 
				     queued_write->buffer, 
				     queued_write->length,
				     queued_write->timeout, 
				     conn_write_cb, conn);
      g_free (queued_write);
    }
}
