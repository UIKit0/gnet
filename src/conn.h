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


#ifndef _GNET_CONN_H
#define _GNET_CONN_H

/* This module is experimental, buggy, and unstable.  Use at your own risk. */
#ifdef GNET_EXPERIMENTAL 

#include <glib.h>
#include "tcp.h"
#include "iochannel.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef struct _GConn GConn;

typedef enum {
  GNET_CONN_STATUS_CONNECT,
  GNET_CONN_STATUS_CLOSE,
  GNET_CONN_STATUS_READ,
  GNET_CONN_STATUS_WRITE,
  GNET_CONN_STATUS_TIMEOUT,
  GNET_CONN_STATUS_ERROR
} GConnStatus;

/* rv only affects reads... */
typedef gboolean (*GConnFunc)(GConn* conn, GConnStatus status, 
			      gchar* buffer, gint length,
			      gpointer user_data);


struct _GConn
{
  gchar*			hostname;
  gint				port;

  guint				ref_count;

  GTcpSocketConnectAsyncID 	connect_id;
  GTcpSocketNewAsyncID 		new_id;

  GTcpSocket* 			socket;
  GInetAddr*			inetaddr;
  GIOChannel* 			iochannel;

  guint				read_watch;
  guint				write_watch;
  guint				err_watch;

  GNetIOChannelWriteAsyncID 	write_id;
  GList*			queued_writes;

  GNetIOChannelReadAsyncID  	read_id;

  guint				timer;

  GConnFunc			func;
  gpointer			user_data;

};



/* ********** */

GConn*     gnet_conn_new (gchar* hostname, gint port, 
			  GConnFunc func, gpointer user_data);
GConn*     gnet_conn_new_inetaddr (GInetAddr* inetaddr, 
				   GConnFunc func, gpointer user_data);
void 	   gnet_conn_delete (GConn* conn, gboolean delete_buffers);
	      
void	   gnet_conn_ref (GConn* conn);
void	   gnet_conn_unref (GConn* conn, gboolean delete_buffers);
	      

/* ********** */

void	   gnet_conn_connect (GConn* conn, guint timeout);
void	   gnet_conn_disconnect (GConn* conn, gboolean delete_buffers);
	      
gboolean   gnet_conn_is_connected (GConn* conn);
	      

/* ********** */

void	   gnet_conn_read (GConn* conn, gchar* buffer, 
			   guint length, guint timeout,
			   gboolean read_one_byte_at_a_time,
			   GNetIOChannelReadAsyncCheckFunc check_func, 
			   gpointer check_user_data);
void	   gnet_conn_readany (GConn* conn, gchar* buffer, 
			      guint length, guint timeout);
void	   gnet_conn_readline (GConn* conn, gchar* buffer, 
			       guint length, guint timeout);
void	   gnet_conn_write (GConn* conn, gchar* buffer, 
			    gint length, guint timeout);
void	   gnet_conn_timeout (GConn* conn, guint timeout);


/* ********** */

void	   gnet_conn_watch_add_read (GConn* conn, GIOFunc func, 
				     gpointer user_data);
void	   gnet_conn_watch_add_write (GConn* conn, GIOFunc func, 
				      gpointer user_data);
void	   gnet_conn_watch_add_error (GConn* conn, GIOFunc func, 
				      gpointer user_data);

void	   gnet_conn_watch_remove_read (GConn* conn);
void	   gnet_conn_watch_remove_write (GConn* conn);
void	   gnet_conn_watch_remove_error (GConn* conn);



#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_CONN_H */
