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
 
 
#include "ftp.h"
#include "gnet-private.h"
 
#if 0
 
/* **********Structure declarations********** */
struct _GFtpSocket /* This definition must be included in gnet-private.h */
{
 	GInetAddr *host;
 	gchar *username;
 	gchar *password;
 	gchar *remotepath;
 	gchar *localpath;
 	GFtpSocketCodingType coding;
 	
 	GFtpSocketConnectStatus status;
 	
 	GInetAddr *proxy;
 	GFtpSocketProxyType proxy_type;
 	
	GTcpSocket *ftpsocket;
  GTcpSocket *ftp-datasocket;
}
/* **********End of structure declarations********** */
 
 
 
/* **********Static functions declarations********** */

/* **********End of static functions declarations********** */

 
/**
 *  gnet_ftp_socket_new:
 *  @host: An initialized GInetAddr structure
 *  @username: The login name to use for the ftp connection
 *  @password: The password to use
 *  
 *  This function is non-blocking since the hostname resolving
 *  has been done in the GInetAddr structure.
 *  This functions initialize a new GFtpSocket, but do not
 *  connect to the ftp server. All the other values are set
 *  to default:
 *  - FTP port: 21
 *  - Remote path: default login one
 *  - Local path: current directory process
 *  - Downloading coding type: binary
 *  - Proxy: no
 *
 *  Returns a new GFtpSocket, or NULL if there was a failure.
 *
 **/
GFtpSocket*
gnet_ftp_socket_new(GInetAddr *host,
			gchar *username,
			gchar *password)
{
	GFtpSocket *retval_gftpsocket;
	
	retval_gftpsocket=NULL;
	
	/* do the stuff */
	
	return(retval_gftpsocket);
};
 
/**
 *  gnet_ftp_socket_set_remote_path:
 *  @ftp: The ftp object to modify
 *  @path: The remote path to set
 *  
 *  This is a blocking function when the socket is connected
 *  This functions sets the remote path on an ftp structure.
 *  If the ftp socket is connected, commands are passed to the
 *  ftp server to change directory immediately.
 *
 *  Returns 1 if okay, -1 if not.
 *
 **/
gint
gnet_ftp_socket_set_remote_path(GFtpSocket *ftp, gchar *path)
{
	gint retval_gint;

	retval_gint=0;
	
	/* do the stuff */
	
	return(retval_gint);
};
 
 /**
 *  gnet_ftp_socket_get_remote_path:
 *  @ftp: The ftp object to read
 *
 *  This is a non blocking function.
 *  This function read the current remote path. It could be called
 *  while the socket is connected or not. If the socket is connected,
 *  nothing is sended over the network since the remote path is stored
 *  internaly in the ftp object
 *
 *  Returns a string, or NULL if an error occured
 *
 **/
gchar*
gnet_ftp_socket_get_remote_path(GFtpSocket *ftp)
{
	gchar *retval_gchar;

	retval_gchar=NULL;
	
	/* do the stuff */
	
	return(retval_gchar);
};
 
 /**
 *  gnet_ftp_socket_set_local_path:
 *  @ftp: The ftp object to modify
 *  @local: The local path to set
 *
 *  This is a non-blocking function
 *  This function sets the local directory to use for
 *  file transfer.
 *
 *  Returns 1 if okay, -1 if not.
 *
 **/
gint
gnet_ftp_socket_set_local_path(GFtpSocket *ftp, gchar *local)
{
	gint retval_gint;

	retval_gint=0;
	
	/* do the stuff */
	
	return(retval_gint);
};
 
 
 /**
 *  gnet_ftp_socket_get_local_path:
 *  @ftp: The ftp object to read
 *
 *  This function is non-blocking
 *  It is used to get the current local path.
 *
 *  Returns a string, or NULL if an error occured
 *
 **/
gchar*
gnet_ftp_socket_get_local_path(GFtpSocket *ftp)
{
	gchar *retval_gchar;

	retval_gchar=NULL;
	
	/* do the stuff */
	
	return(retval_gchar);
};
 
 /**
 *  gnet_ftp_socket_set_coding_type:
 *  @ftp: The ftp objet to modify
 *  @coding: The coding type to use for file transfer
 *
 *  This is a non blocking function
 *  Sets the coding type to use for downloading files.
 *  
 *  Returns nothing.
 *  
 **/
void
gnet_ftp_socket_set_coding_type(GFtpSocket *ftp, GFtpSocketCodingType coding)
{
};
 
 /**
 *  gnet_ftp_socket_set_proxy:
 *  @ftp: The ftp object where to set the proxy
 *  @address: A GInetAddr* pointing to the proxy
 *  @type: Type of the proxy according to the GFtpSocketProxyType structure
 *
 *  This function is non-blocking since the resolving of the proxy hostname
 *  has been done in the GInetAddr structure.
 *  This function is not functionnal for now, but if you call it, nothing
 *  will change int the ftp connection.
 *
 *  Returns nothing.
 *  
 **/
void
gnet_ftp_socket_set_proxy(GFtpSocket *ftp,
					GInetAddr *address,
					GFtpSocketProxyType type)
{
};

 /**
 *  gnet_ftp_socket_unset_proxy:
 *  @ftp: The ftp object where to unset the proxy
 *
 *  This function is non-blocking and returns an error if called while
 *  the ftp socket is connected.
 *  It is used to unset the proxy. In real application, this function
 *  will not be used.
 *
 *  Returns 1 if okay, -1 if the call was done while the ftp socket
 *  was connected
 *
 **/
gint
gnet_ftp_socket_unset_proxy(GFtpSocket *ftp)
{
	gint retval_gint;

	retval_gint=0;
	
	/* do the stuff */
	
	return(retval_gint);
};

 /**
 *  gnet_ftp_socket_disconnect:
 *  @ftp: The ftp socket to close
 *
 *  This function is a non-blocking one
 *  This function will close the ftp socket connection.
 *
 *  Returns nothing.
 *
 **/
void
gnet_ftp_socket_disconnect(GFtpSocket *ftp)
{
};

 /**
 *  gnet_ftp_socket_delete:
 *  @ftp: The ftp object to destroy.
 *
 *  This function is a non-blocking one.
 *  It close the GFtpSocket if it was not, and then destroy it
 *  (meaning that it's removed from memory.
 *
 *  Returns nothing.
 *
 **/
void
gnet_ftp_socket_delete(GFtpSocket *ftp)
{
};

 /**
 *  gnet_ftp_socket_connect:
 *  @ftp: The ftp object to conect
 *
 *  This function is blocking.
 *  It establishes the ftp connection as it is set in the ftp object.
 *  The GTcpSocket returned is stored internally in the GFtpSocket structure,
 *  so it's no need to store it for future use with the GFtpSocket API.
 *
 *  Returns the GTcpSocket created or NULL if an error occured
 *
 **/
GTcpSocket*
gnet_ftp_socket_connect(GFtpSocket *ftp)
{
	GTcpSocket *retval_gtcpsocket;

	retval_gtcpsocket=NULL;
	
	/* do the stuff */
	
	return(retval_gtcpsocket);
};
 
 /**
 *  gnet_ftp_socket_connect_non_block:
 *  @ftp: The GFtpSocket object to use
 *  @func: The callback function
 *  @data: User data to pass to the callback function
 *
 *  This function is non-blocking.
 *  It returns immediately, and then connect the GFtpSocket. When the connection
 *  is up, or when an error occured, the callback is called passing the
 *  user data as parameter. See the callback GFtpSocketConnectFunc definition
 *  for more information on how it works
 *
 *  Returns a non-valid GTcpSocket, or NULL if an error occured before it
 *  started to connect. Be warned to not use this returned pointer since
 *  it is not a valid one.
 *
 **/
GTcpSocket*
gnet_ftp_socket_connect_non_block(GFtpSocket *ftp,
							GFtpSocketConnectFunc func,
							gpointer data)
{
	GTcpSocket *retval_gtcpsocket;

	retval_gtcpsocket=NULL;
	
	/* do the stuff */
	
	return(retval_gtcpsocket);
};
 
 /**
 *  gnet_ftp_socket_quick_connect:
 *  @hostname: The hostname of the remote ftp site
 *  @username: The login name to use
 *  @password: The login password
 *
 *  This function is a blocking one.
 *  This function is a quick and easy GFtpSocket constructor since it is
 *  waiting a few parameters, and establish immediatly the connection.
 *  Default parameters are uses:
 *  - FTP port: 21
 *  - Remote path: default login one
 *  - Local path: current directory process
 *  - Downloading coding type: binary
 *  - Proxy: no
 *  The GTcpSocket returned is stored internally in the GFtpSocket structure,
 *  so it's no need to store it for future use with the GFtpSocket API.
 *
 *  Returns the GTcpSocket created or NULL if an error occured
 *
 **/
GTcpSocket*
gnet_ftp_socket_quick_connect(gchar *hostname,
						gchar *username,
						gchar *password)
{
	GTcpSocket *retval_gtcpsocket;
	
	retval_gtcpsocket=NULL;

	/* do the stuff */
	
	return(retval_gtcpsocket);
};

#endif
