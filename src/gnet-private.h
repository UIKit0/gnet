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

#ifndef _GNET_PRIVATE_H
#define _GNET_PRIVATE_H

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <glib.h>
#include "gnet.h"
#include "gnetconfig.h"


#ifndef GNET_WIN32  /*********** Unix specific ***********/

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <netinet/in_systm.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>		/* Need for TOS */
#include <arpa/inet.h>

#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#ifndef socklen_t
#  ifdef GNET_APPLE_DARWIN
#    define socklen_t int	/* socklen_t is int in Darwin */
#  else
#    define socklen_t size_t	/* it's size_t on all other Unixes */
#  endif
#endif

#define GNET_CLOSE_SOCKET(SOCKFD) close(SOCKFD)

/* Use gnet_private_io_channel_new() to create iochannels */
#define GNET_SOCKET_IO_CHANNEL_NEW(SOCKFD) g_io_channel_unix_new(SOCKFD)

#else	/*********** Windows specific ***********/

#include <windows.h>
#include <winbase.h>
#include <winuser.h>
#include <ws2tcpip.h>

#define socklen_t gint32

#define GNET_CLOSE_SOCKET(SOCKFD) closesocket(SOCKFD)

/* Use gnet_private_io_channel_new() to create iochannels */
#define GNET_SOCKET_IO_CHANNEL_NEW(SOCKFD) g_io_channel_win32_new_socket(SOCKFD)

#endif	/*********** End Windows specific ***********/

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#define GNET_SOCKADDR_IN(s)    	(*((struct sockaddr_in*) &s))

#define GNET_SOCKADDR_SA(s)	(*((struct sockaddr*) &s))
#define GNET_SOCKADDR_SA4(s)	(*((struct sockaddr_in*) &s))
#define GNET_SOCKADDR_SA6(s)	(*((struct sockaddr_in6*) &s))
#define GNET_SOCKADDR_FAMILY(s) ((s).ss_family)
#define GNET_SOCKADDR_ADDRP(s)	(((s).ss_family == AF_INET)?\
                                  (void*)&((struct sockaddr_in*)&s)->sin_addr:\
                                  (void*)&((struct sockaddr_in6*)&s)->sin6_addr)
#define GNET_SOCKADDR_ADDR32(s,n)(((s).ss_family == AF_INET)?\
                                  ((struct sockaddr_in*)&s)->sin_addr.s_addr:\
                                  *(guint32*)&((struct sockaddr_in6*)&s)->sin6_addr.s6_addr[(n)*4])
#define GNET_SOCKADDR_ADDRLEN(s) (((s).ss_family == AF_INET)?\
				 sizeof(struct in_addr):\
				 sizeof(struct in6_addr))
#define GNET_SOCKADDR_PORT(s)	(((s).ss_family == AF_INET)?\
                                  ((struct sockaddr_in*)&s)->sin_port:\
                                  ((struct sockaddr_in6*)&s)->sin6_port)
#define GNET_SOCKADDR_LEN(s)	(((s).ss_family == AF_INET)?\
                                  sizeof(struct sockaddr_in):\
                                  sizeof(struct sockaddr_in6))
#ifdef HAVE_SOCKADDR_SA_LEN
#define GNET_SOCKADDR_SET_SS_LEN(s) do{(s).ss_len = GNET_SOCKADDR_LEN(s);}while(0)
#else
#define GNET_SOCKADDR_SET_SS_LEN(s) while(0){} /* do nothing */
#endif


#define GNET_INETADDR_SA(i)         GNET_SOCKADDR_SA((i)->sa)
#define GNET_INETADDR_SA4(i)        GNET_SOCKADDR_SA4((i)->sa)
#define GNET_INETADDR_SA6(i)        GNET_SOCKADDR_SA6((i)->sa) 
#define GNET_INETADDR_FAMILY(i)     GNET_SOCKADDR_FAMILY((i)->sa)
#define GNET_INETADDR_ADDRP(i)      GNET_SOCKADDR_ADDRP((i)->sa)
#define GNET_INETADDR_ADDR32(i,n)   GNET_SOCKADDR_ADDR32((i)->sa,(n))
#define GNET_INETADDR_ADDRLEN(i)    GNET_SOCKADDR_ADDRLEN((i)->sa)
#define GNET_INETADDR_PORT(i)       GNET_SOCKADDR_PORT((i)->sa)
#define GNET_INETADDR_LEN(i)        GNET_SOCKADDR_LEN((i)->sa)
#define GNET_INETADDR_SET_SS_LEN(i) GNET_SOCKADDR_SET_SS_LEN((i)->sa)


#define GNET_ANY_IO_CONDITION   (G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*

   This is the private declaration and definition file.  The things in
   here are to be used by GNet ONLY.  Use at your own risk.  If this
   file includes something that you absolutely must have, write the
   GNet developers.

*/



struct _GUdpSocket
{
  gint sockfd;
  guint ref_count;
  GIOChannel* iochannel;
  struct sockaddr_storage sa;
};

struct _GMcastSocket
{
  gint sockfd;
  guint ref_count;
  GIOChannel* iochannel;
  struct sockaddr_storage sa;
};

struct _GTcpSocket
{
  gint sockfd;
  guint ref_count;
  GIOChannel* iochannel;
  struct sockaddr_storage sa;
  /* sa is remote host for clients, local host for servers */

  GTcpSocketAcceptFunc accept_func;
  gpointer accept_data;
  guint	accept_watch;
};

struct _GUnixSocket
{
  gint sockfd;
  guint ref_count;
  GIOChannel *iochannel;
  struct sockaddr_un sa;

  gboolean server;
};

struct _GInetAddr
{
  gchar* name;
  guint ref_count;
  struct sockaddr_storage sa;
};


/* **************************************** */
/* Async functions			*/

gboolean gnet_inetaddr_new_list_async_cb (GIOChannel* iochannel, 
					  GIOCondition condition, 
					  gpointer data);


typedef struct _GInetAddrNewListState 
{
  GList*	ias;
  gint		port;
  GInetAddrNewListAsyncFunc func;
  gpointer 	data;

#ifndef GNET_WIN32		/* UNIX */
  gboolean 	in_callback;
#ifdef HAVE_LIBPTHREAD		/* UNIX pthread	*/
  pthread_mutex_t mutex;
  gboolean	is_cancelled;
  gboolean	lookup_failed;
  guint 	source;

#else			       	/* UNIX process	*/
  int 		fd;
  pid_t 	pid;
  GIOChannel* 	iochannel;
  guint 	watch;
  int 		len;
  guchar 	buffer[256];

#endif
#else				/* Windows */
  int 		WSAhandle;
  char 		hostentBuffer[MAXGETHOSTSTRUCT];
  int 		errorcode;
  gboolean 	in_callback;
#endif

} GInetAddrNewListState;



typedef struct _GInetAddrNewState 
{
  GInetAddrNewListAsyncID	list_id;
  GInetAddrNewAsyncFunc 	func;
  gpointer 			data;
  gboolean			in_callback;

} GInetAddrNewState;



gboolean gnet_inetaddr_get_name_async_cb (GIOChannel* iochannel, 
					  GIOCondition condition, 
					  gpointer data);

typedef struct _GInetAddrReverseAsyncState 
{
  GInetAddr* ia;
  GInetAddrGetNameAsyncFunc func;
  gpointer data;
  gboolean in_callback;
#ifndef GNET_WIN32		/* UNIX 	*/
#ifdef HAVE_LIBPTHREAD		/* UNIX pthread	*/
  pthread_mutex_t mutex;
  gboolean	is_cancelled;
  gchar*	name;
  guint 	source;
#else				/* UNIX process	*/
  int 		fd;
  pid_t 	pid;
  guint 	watch;
  GIOChannel* 	iochannel;
#endif				/* WINDOWS */
  guchar	buffer[256 + 1];/* Names can only be 256 characters? */
  int 		len;
#else
  int WSAhandle;
  char hostentBuffer[MAXGETHOSTSTRUCT];
  int errorcode;
#endif

} GInetAddrReverseAsyncState;


gboolean gnet_tcp_socket_new_async_cb (GIOChannel* iochannel, 
				       GIOCondition condition, 
				       gpointer data);

typedef struct _GTcpSocketAsyncState 
{
  GTcpSocket* 		 socket;
  GTcpSocketNewAsyncFunc func;
  gpointer 		 data;
  gint 			 flags;
  GIOChannel* 		 iochannel;
  guint 		 connect_watch;
#ifdef GNET_WIN32
  gint 			 errorcode;
#endif
} GTcpSocketAsyncState;

#ifdef GNET_WIN32
/*
Used for:
-gnet_inetaddr_new_async
-gnet_inetaddr_get_name_asymc
*/
typedef struct _SocketWatchAsyncState 
{
	GIOChannel *channel;
	gint fd;
	long winevent;
	gint eventcode;
  gint errorcode;
	GSList* callbacklist;
} SocketWatchAsyncState;
#endif

void gnet_tcp_socket_connect_inetaddr_cb (GList* ia_list, gpointer data);

void gnet_tcp_socket_connect_tcp_cb(GTcpSocket* socket, gpointer data);

typedef struct _GTcpSocketConnectState 
{
  GList* ia_list;
  GList* ia_next;

  gpointer inetaddr_id;
  gpointer tcp_id;

  gboolean in_callback;

  GTcpSocketConnectAsyncFunc func;
  gpointer data;

} GTcpSocketConnectState;


/* **************************************** 	*/
/* More Windows specific stuff 			*/

#ifdef GNET_WIN32

extern WNDCLASSEX gnetWndClass;
extern HWND  gnet_hWnd; 
extern guint gnet_io_watch_ID;
extern GIOChannel *gnet_iochannel;
	
extern GHashTable *gnet_hash;
extern HANDLE gnet_Mutex; 
extern HANDLE gnet_hostent_Mutex;
	
#define IA_NEW_MSG 100		/* gnet_inetaddr_new_async */
#define GET_NAME_MSG 101	/* gnet_inetaddr_get_name_asymc */

#endif



/* ************************************************************ */

/* Private/Experimental functions */

GIOChannel* gnet_private_io_channel_new (int sockfd);

int gnet_private_create_listen_socket (int type, const GInetAddr* iface, int port, struct sockaddr_storage* sa);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_PRIVATE_H */
