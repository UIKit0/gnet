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


#ifndef _GNET_TCP_H
#define _GNET_TCP_H

#include "inetaddr.h"

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*

  All fields in GTcpSocket are private and should be accessed only by
  using the functions below.

 */
typedef struct _GTcpSocket GTcpSocket;


/* ********** */



/**
 *   GTcpSocketConnectStatus:
 * 
 *   Status for connecting via gnet_tcp_socket_connect(), passed by
 *   GTcpSocketConnectFunc.
 *
 **/
typedef enum {
  GTCP_CONNECT_STATUS_OK,
  GTCP_CONNECT_STATUS_INETADDR_ERROR,
  GTCP_CONNECT_STATUS_TCP_ERROR
} GTcpSocketConnectStatus;



/**
 *   GTcpSocketConnectFunc:
 *   @socket: TcpSocket that was connecting
 *   @ia: InetAddr of the TcpSocket
 *   @status: Status of the connection
 *   @data: User data
 *   
 *   Callback for gnet_tcp_socket_connect_async().
 *
 **/
typedef void (*GTcpSocketConnectFunc)(GTcpSocket* socket, 
				      GInetAddr* ia,
				      GTcpSocketConnectStatus status, 
				      gpointer data);


/**
 *   GTcpSocketAsyncStatus:
 * 
 *   Status for connecting via gnet_tcp_socket_new(), passed by
 *   GTcpSocketAsyncFunc.  More errors may be added in the future, so
 *   it's best to compare against %GTCP_SOCKET_ASYNC_STATUS_OK.
 *
 **/
typedef enum {
  GTCP_SOCKET_ASYNC_STATUS_OK,
  GTCP_SOCKET_ASYNC_STATUS_ERROR
} GTcpSocketAsyncStatus;



/**
 *   GTcpSocketAsyncFunc:
 *   @socket: Socket that was connecting
 *   @status: Status of the connection
 *   @data: User data
 *   
 *   Callback for gnet_tcp_socket_new_async().
 *
 **/
typedef void (*GTcpSocketAsyncFunc)(GTcpSocket* socket, 
				    GTcpSocketAsyncStatus status, 
				    gpointer data);




/* ********** */


/* Quick and easy blocking constructor */
GTcpSocket* gnet_tcp_socket_connect(gchar* hostname, gint port);

/* Quick and easy asynchronous constructor */
gpointer gnet_tcp_socket_connect_async(gchar* hostname, gint port, 
				       GTcpSocketConnectFunc func, gpointer data);

/* Cancel quick and easy asynchronous constructor */
void gnet_tcp_socket_connect_async_cancel(gpointer id);

/* Blocking constructor */
GTcpSocket* gnet_tcp_socket_new(const GInetAddr* addr);

/* Asynchronous constructor */
gpointer gnet_tcp_socket_new_async(const GInetAddr* addr, 
				   GTcpSocketAsyncFunc func,
				   gpointer data);

/* Cancel asynchronous constructor */
void gnet_tcp_socket_new_async_cancel(gpointer id);

void gnet_tcp_socket_delete(GTcpSocket* s);

void gnet_tcp_socket_ref(GTcpSocket* s);

void gnet_tcp_socket_unref(GTcpSocket* s);


/* ********** */

GIOChannel* gnet_tcp_socket_get_iochannel(GTcpSocket* socket);

GInetAddr* gnet_tcp_socket_get_inetaddr(const GTcpSocket* socket);

gint gnet_tcp_socket_get_port(const GTcpSocket* socket);

/* ********** */

GTcpSocket* gnet_tcp_socket_server_new(const gint port);

GTcpSocket* gnet_tcp_socket_server_accept(GTcpSocket* socket);




/* **************************************** */
/* TO IMPLEMENT?:			    */


/**

   Get the inet address the socket is connected to.  UNIMPLEMENTED
p
*/
/* InetAddr* tcp_socket_get_local_inetaddr(const TcpSocket* socket); */


/**

   Test if the socket has data to read.  Returns TRUE if there is
   data, FALSE otherwise.  UNIMPLEMENTED

*/
/*  gboolean tcp_socket_has_data(const tcp_socket* s); */

/**

   Create and open a new TCP socket with any port.  Use this sort of
   socket when you are a server and the port number doesn't matter.
   Returns a new TcpSocket, or NULL if there was a failure.  Use this

   UNIMPLEMENTED

*/
/*  TcpSocket* tcp_socket_server_any_new(void); */

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_TCP_H */
