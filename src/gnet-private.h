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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include <glib.h>
#include "gnet.h"


#ifndef socklen_t
#define socklen_t size_t
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
#endif

#define GNET_SOCKADDR_IN(s) (*((struct sockaddr_in*) &s))
#define GNET_ANY_IO_CONDITION  (G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL)





/**

   This is the private declaration and definition file.  The things in
   here are to be used by GNet ONLY.  Use at your own risk.  If this
   file includes something that you absolutely must have, write the
   GNet developers.

*/


struct _GUdpSocket
{
  gint sockfd;			/* private */
  struct sockaddr sa;		/* private (Why not an InetAddr?) */

};

struct _GTcpSocket
{
  gint sockfd;
  struct sockaddr sa;		/* Why not an InetAddr? */

};

struct _GMcastSocket
{
  gint sockfd;
  struct sockaddr sa;

};

struct _GInetAddr
{
  gchar* name;
  struct sockaddr sa;

};


GInetAddr* gnet_private_inetaddr_sockaddr_new(const struct sockaddr sa);

struct sockaddr gnet_private_inetaddr_get_sockaddr(const GInetAddr* ia);
/* gtk-doc doesn't like this function...  (but it will be fixed.) */

GList* gnet_private_inetaddr_list_interfaces(void);


/* TODO: Need to port this to Solaris.  This also assumes eth0 (we
   need to get the name of the interface of the socket) */

/*gint gnet_udp_socket_get_MTU(GUdpSocket* us);*/


