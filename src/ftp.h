/* GNet - Networking library
 * Copyright (C) 2000  Xavier Nicolovici
 * <nicolovi@club-internet.fr>
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

/* EXPERIMENTAL - NOT READY FOR PUBLIC CONSUMPTION */

 
#ifndef _GNET_FTP_H
#define _GNET_FTP_H

#if 0

#include <glib.h>
#include "inetaddr.h"
#include "tcp.h"

/*
  All fields in GFtpSocket are private and should be accessed only by
  using the functions below.
*/

typedef struct _GFtpSocket GFtpSocket;

/* ********** */



/**
 *   GFtpSocketConnectStatus:
 * 
 
 *
 **/
typedef enum {
  GFTP_CONNECT_STATUS_DISCONNECTED,
  GFTP_CONNECT_STATUS_CONNECTED,
} GFtpSocketConnectStatus;

/**
 *   GFtpSocketConnectError:
 * 
 *   Status for connecting via gnet_ftp_socket_connect_non_block(), passed by
 *   GFtpSocketConnectFunc.
 *   This error list is not yet complete, and some error codes can be added
 *   (or removed) in a near future. So please check GFTP_CONNECT_STATUS_OK
 *   in callback functions, instead you know what you are doing.
 *
 **/
typedef enum {
  GFTP_CONNECT_STATUS_PASSWORD_ERROR,
  GFTP_CONNECT_STATUS_ADDRESS_ERROR,
  GFTP_CONNECT_STATUS_TIMEOUT_ERROR,
  GFTP_CONNECT_STATUS_OK
} GFtpSocketConnectError;

/**
 *   GFtpSocketProxyType:
 * 
 *   Proxy server type enumeration
 *
 **/
typedef enum {
  GFTP_CONNECT_PROXYTYPE_1,
  GFTP_CONNECT_PROXYTYPE_2,
  GFTP_CONNECT_PROXYTYPE_3
} GFtpSocketProxyType;


/**
 *   GFtpSocketDownloadCoding:
 * 
 *   Define the different codings to download a file
 *   Actually, only to are supported: binary and ASCII
 *   Automatic will be supported in a next release
 *
 **/

typedef enum {
  GFTP_SOCKET_CODINGTYPE_BIN,
  GFTP_SOCKET_CODINGTYPE_ASCII
} GFtpSocketCodingType;


/**
 *   GFtpSocketConnectNonblock:
 *   @ftp: FtpSocket that was connecting
 *   @status: Status of the connection
 *   @data: User data
 *   
 *   Callback for gnet_ftp_socket_connect_non_block()
 *
 **/
typedef void (*GFtpSocketConnectFunc)(GFtpSocket* ftp, 
				      GFtpSocketConnectError status, 
				      gpointer data);




/* ********** */

/* **********Connection functions********** */
/* These function are mainly used to setup  */
/* and make an ftp connection. See the      */
/* navigation functions to see how to       */
/* download a file                          */

/* A GFtpSocket object constructor */
GFtpSocket* gnet_ftp_socket_new(GInetAddr *host,
				gchar *username,
				gchar *password);

/* Sets the remote path */
gint gnet_ftp_socket_set_remote_path(GFtpSocket *ftp, gchar *path);
/* Gets the current remote path */
gchar* gnet_ftp_socket_get_remote_path(GFtpSocket *ftp);

/* Sets the local path */
gint gnet_ftp_socket_set_local_path(GFtpSocket *ftp, gchar *local);
/* Gets the current local path */
gchar* gnet_ftp_socket_get_local_path(GFtpSocket *ftp);

/* Sets the downloading coding type */
void gnet_ftp_socket_set_coding_type(GFtpSocket *ftp, GFtpSocketCodingType coding);

/* Set the proxy */
void gnet_ftp_socket_set_proxy(GFtpSocket *ftp,
				GInetAddr *address,
				GFtpSocketProxyType type);

/* Unset the proxy - Needs to be disconnected */
gint gnet_ftp_socket_unset_proxy(GFtpSocket *ftp);

/* Close the ftp conection */
void gnet_ftp_socket_disconnect(GFtpSocket *ftp);

/* Remote the ftp object from memory - Disconnect if needed */
void gnet_ftp_socket_delete(GFtpSocket *ftp);


/* Establish the ftp connection */
GTcpSocket* gnet_ftp_socket_connect(GFtpSocket *ftp);

/* Establish a non blocking connection */
GTcpSocket* gnet_ftp_socket_connect_non_block(GFtpSocket *ftp,
									GFtpSocketConnectFunc func,
									gpointer data);



/* Quick and easy blocking constructor - Use all the functions above */
/* No proxy - Standard port - Remote path is default - Local dir is current */
GTcpSocket* gnet_ftp_socket_quick_connect(gchar *hostname,
								gchar *username,
								gchar *password);


/* **********End of the connection functions********** */


/* **********Navigation functions********** */
/* These functions are used to navigate in  */
/* the ftp server filesystem, and make the  */
/* file transfers.                          */

/* Gets a list of files in the remote directory */
GList* gnet_ftp_socket_get_filelist(GFtpSocket *ftp);

/* **********End of navigation functions********** */


/* Here are some deprecated functions. */
/* It's possible that, in a next release, this function would be included */
/* again in the GFtpSocket API */

/* Sets the remote hostname */
/* void gnet_ftp_socket_set_remotehost(GFtpSocket *ftp, gchar *remote); */

/* Sets the username */
/* void gnet_ftp_socket_set_username(GFtpSocket *ftp, gchar *name); */

/* Sets the password */
/* void gnet_ftp_socket_set_password(GFtpSocket *ftp, gchar *password); */


#endif  /* _GNET_FTP_H */
#endif
