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

#include <stdio.h>
#include <memory.h> /* required for windows */


typedef struct _QueuedWrite
{
  gchar* buffer;
  gint length;
  gint timeout;

} QueuedWrite;

static void conn_check_queued_writes (GConn* conn);

static void conn_new_cb (GTcpSocket* socket, 
			 GTcpSocketNewAsyncStatus status, 
			 gpointer user_data);

static void conn_connect_cb (GTcpSocket* socket,
			     GTcpSocketConnectAsyncStatus status, 
			     gpointer user_data);

static gboolean conn_connect_timeout_cb (gpointer data);

static void conn_write_cb (GIOChannel* iochannel, 
			   gchar* buffer, guint length, guint bytes_writen,
			   GNetIOChannelWriteAsyncStatus status, 
			   gpointer user_data);

static gboolean conn_read_cb (GIOChannel* iochannel, 
			      GNetIOChannelReadAsyncStatus status, 
			      gchar* buffer, guint length, gpointer user_data);

static gboolean conn_timeout_cb (gpointer data);


/**
 *  gnet_conn_new
 *  @hostname: Hostname of host
 *  @port: Port of host
 *  @func: Function to call on connection, I/O, or error
 *  @user_data: Data to pass to func
 *
 *  Create a connection object representing a connection to a host.
 *  The actual connection is not made until gnet_conn_connect() is
 *  called.  The callback is called when events occur.  The events are
 *  connect, read, write, error, and timeout.  These only occur if the
 *  appropriate function is called first.  For example, use
 *  gnet_conn_read() to have the callback called when data is read.
 *
 *  Returns: A #GConn.
 *
 **/
GConn*
gnet_conn_new (const gchar* hostname, gint port, GConnFunc func, gpointer user_data)
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


/**
 *  gnet_conn_new_inetaddr
 *  @inetaddr: address of host
 *  @func: Function to call on connection, I/O, or error
 *  @user_data: Data to pass to func
 *
 *  Create a connection object representing a connection to a host.
 *  This function is similar to gnet_conn_new() but has different
 *  arguments.
 *
 *  Returns: A #GConn.
 *
 **/
GConn*   
gnet_conn_new_inetaddr (const GInetAddr* inetaddr, 
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



/**
 *  gnet_conn_delete
 *  @conn: Connection to delete
 *  @delete_buffers: True if write buffers should be deleted.
 *
 *  Delete the connection.  If delete_buffers is set, any write
 *  buffers are deleted.
 *
 **/
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


/**
 *  gnet_conn_ref
 *  @conn: #GConn to reference
 *
 *  Increment the reference counter of the GConn.
 *
 **/
void
gnet_conn_ref (GConn* conn)
{
  g_return_if_fail (conn);

  ++conn->ref_count;
}


/**
 *  gnet_conn_unref
 *  @conn: #GConn to unreference
 *
 *  Remove a reference from the #GConn.  When reference count reaches
 *  0, the connection is deleted.
 *
 **/
void
gnet_conn_unref (GConn* conn, gboolean delete_buffers)
{
  g_return_if_fail (conn);

  --conn->ref_count;

  if (conn->ref_count == 0)
    gnet_conn_delete (conn, delete_buffers);
}


/**
 *  gnet_conn_connect
 *  @conn: Conn to connect to
 *  @timeout: Timeout for connection (in milliseconds)
 *
 *  Establish the connection.  If the connection is pending or already
 *  established, this function does nothing.  The callback is called
 *  when the connection is established or an error occurs.  If
 *  @timeout is 0, no timeout is set.
 *
 **/
void
gnet_conn_connect (GConn* conn, guint timeout)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  if (conn->connect_id || conn->new_id || conn->socket)
    return;

  if (conn->inetaddr)
    {
      conn->new_id = 
	gnet_tcp_socket_new_async (conn->inetaddr, conn_new_cb, conn);

      if (timeout)
	conn->connect_timeout = 
	  g_timeout_add (timeout, conn_connect_timeout_cb, conn);
    }
  
  else if (conn->hostname)
    {
      conn->connect_id = 
	gnet_tcp_socket_connect_async (conn->hostname, conn->port, 
				       conn_connect_cb, conn);
      if (timeout)
	conn->connect_timeout = 
	  g_timeout_add (timeout, conn_connect_timeout_cb, conn);
    }
  else
    g_return_if_fail (FALSE);
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

  if (conn->connect_timeout)
    {
      g_source_remove (conn->connect_timeout);
      conn->connect_timeout = 0;
    }

  if (status == GTCP_SOCKET_NEW_ASYNC_STATUS_OK)
    {
      conn->socket = socket;
      conn->iochannel = gnet_tcp_socket_get_io_channel (socket);

      st = GNET_CONN_STATUS_CONNECT;

      conn_check_queued_writes (conn);
    }

  (conn->func)(conn, st, NULL, 0, conn->user_data);
}


static void
conn_connect_cb (GTcpSocket* socket, 
		 GTcpSocketConnectAsyncStatus status, 
		 gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  GConnStatus st = GNET_CONN_STATUS_ERROR;

  g_return_if_fail (conn);

  conn->connect_id = NULL;

  if (conn->connect_timeout)
    {
      g_source_remove (conn->connect_timeout);
      conn->connect_timeout = 0;
    }

  if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      conn->socket = socket;
      conn->inetaddr = gnet_tcp_socket_get_inetaddr (socket);
      conn->iochannel = gnet_tcp_socket_get_io_channel (socket);

      st = GNET_CONN_STATUS_CONNECT;

      conn_check_queued_writes (conn);
    }

  (conn->func)(conn, st, NULL, 0, conn->user_data);
}


static gboolean 
conn_connect_timeout_cb (gpointer data)
{
  GConn* conn = (GConn*) data;

  conn->connect_timeout = 0;

  (conn->func)(conn, GNET_CONN_STATUS_TIMEOUT, NULL, 0, conn->user_data);

  return FALSE;
}


/**
 *  gnet_conn_disconnect
 *  @conn: Conn to disconnect
 *  @delete_buffers: True if write buffers should be deleted.
 *
 *  End the connection.  The connection can later be reestablished by
 *  calling gnet_conn_connect() again.  If there the connection was
 *  not establish, this function does nothing.  If delete_buffers is
 *  set, any write buffers are deleted.
 *
 **/
void
gnet_conn_disconnect (GConn* conn, gboolean delete_buffers)
{
  GList* i;

  g_return_if_fail (conn);

  if (conn->connect_timeout)
    {
      g_source_remove (conn->connect_timeout);
      conn->connect_timeout = 0;
    }

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
      conn->iochannel = NULL;
    }

  if (conn->socket)
    {
      gnet_tcp_socket_delete (conn->socket);
      conn->socket = NULL;
    }
}


/**
 *  gnet_conn_is_connected
 *  @conn: Connection to check
 *
 *  Check if the connection is established.
 *
 *  Returns: TRUE if the connection is established, FALSE otherwise.
 *
 **/
gboolean
gnet_conn_is_connected (const GConn* conn)
{
  g_return_val_if_fail (conn, FALSE);

  return (conn->socket != NULL);
}


/**
 *  gnet_conn_read:
 *  @conn: Connection to read from
 *  @buffer: Buffer to read to (NULL if to be allocated)
 *  @length: Length of the buffer (maximum size if buffer is to be allocated)
 *  @read_one_byte_at_a_time: TRUE if bytes should be read one-at-a-time
 *  @timeout: Timeout for read (in milliseconds)
 *  @check_func: Function to check if read is complete
 *  @check_user_data: User data for check_func
 *
 *  Set up an asynchronous read from the connection to the buffer.
 *  This is a wrapper around gnet_io_channel_read_async(), which reads
 *  data until the check function stops it.  The callback for this
 *  #GConn is called when the read is complete or there is an error.
 *  If @timeout is 0, no timeout is set.
 *
 **/
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


/**
 *  gnet_conn_readany:
 *  @conn: Connection to read from
 *  @buffer: Buffer to read to (NULL if to be allocated)
 *  @length: Length of the buffer (maximum size if buffer is to be allocated)
 *  @timeout: Timeout for read (in milliseconds)
 *
 *  Set up an asynchronous read from the connection to the buffer.
 *  This is a wrapper around gnet_io_channel_readany(), which will
 *  read any amount of data.  If @timeout is 0, no timeout is set.
 *
 **/
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


/**
 *  gnet_conn_readline:
 *  @conn: Connection to read from
 *  @buffer: Buffer to read to (NULL if to be allocated)
 *  @length: Length of the buffer (maximum size if buffer is to be allocated)
 *  @timeout: Timeout for read (in milliseconds)
 *
 *  Set up an asynchronous read from the connection to the buffer.
 *  This is a wrapper around gnet_io_channel_readline(), which will
 *  read data until a newline.  If @timeout is 0, no timeout is set.
 *
 **/
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
  g_return_val_if_fail (conn->func, FALSE);

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
	  /* read_id is invalid */
	  (conn->func)(conn, GNET_CONN_STATUS_CLOSE, NULL, 0, conn->user_data);
	}
    }
  else
    {
      /* read_id is invalid */
      (conn->func)(conn, GNET_CONN_STATUS_ERROR, NULL, 0, conn->user_data);
    }

  return FALSE;
}


/**
 *  gnet_conn_write
 *  @conn: Connection to write to
 *  @buffer: Buffer to write
 *  @length: Length of buffer
 *  @timeout: Timeout for write (in milliseconds)
 *
 *  Set up an asynchronous write to the connection from the buffer.
 *  This is a wrapper around gnet_io_channel_write_async().  This
 *  function may be called again before another asynchronous write
 *  completes.  If @timeout is 0, no timeout is set.
 *
 **/
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
      QueuedWrite* queued_write;

      queued_write = g_new0 (QueuedWrite, 1);
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
  /* write id is invalid */

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


/**
 *  gnet_conn_timeout
 *  @conn: Connection to set timeout on
 *  @timeout: Timeout (in milliseconds)
 * 
 *  Set a timeout on the connection.  When the time expires, the
 *  #GConn's callback is called.  If there already is a timeout, the
 *  old timeout is canceled.
 *
 **/
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

  conn->timer = 0;
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
      conn->queued_writes = g_list_remove (conn->queued_writes, queued_write);

      conn->write_id = 
	gnet_io_channel_write_async (conn->iochannel, 
				     queued_write->buffer, 
				     queued_write->length,
				     queued_write->timeout, 
				     conn_write_cb, conn);
      g_free (queued_write);
    }
}
