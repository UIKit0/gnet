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


#ifndef _GNET_UDP_H
#define _GNET_UDP_H

#include "inetaddr.h"

#include <glib.h>



/*

  All fields in GUdpSocket are private and should be accessed only by
  using the functions below.

 */
typedef struct _GUdpSocket GUdpSocket;


/*

  All fields in the GUdpPacket are public.

 */
typedef struct _GUdpPacket  
{
  gint8* data;
  guint length;

  GInetAddr* addr;

} GUdpPacket;




/* ******************************************** */
/* UDP socket functions				*/

GUdpSocket* gnet_udp_socket_new(void);

GUdpSocket* gnet_udp_socket_port_new(const gint port);

void gnet_udp_socket_delete(GUdpSocket* s);

/* ********** */

gint gnet_udp_socket_send(const GUdpSocket* s, const GUdpPacket* packet);

gint gnet_udp_socket_receive(const GUdpSocket* s, GUdpPacket* packet);

gboolean gnet_udp_socket_has_packet(const GUdpSocket* s);

/* ********** */

/* Do not read or write from the iochannel - it's for watches only */
GIOChannel* gnet_udp_socket_get_iochannel(GUdpSocket* socket);

gint gnet_udp_socket_get_ttl(GUdpSocket* us);

gint gnet_udp_socket_set_ttl(GUdpSocket* us, int val);

gint gnet_udp_socket_get_mcast_ttl(GUdpSocket* us);

gint gnet_udp_socket_set_mcast_ttl(GUdpSocket* us, gint val);



/* ******************************************** */
/* UDP packet functions 			*/

GUdpPacket* gnet_udp_packet_receive_new(gint8* data, gint length);

GUdpPacket* gnet_udp_packet_send_new(gint8* data, gint length, GInetAddr* addr);

void gnet_udp_packet_delete(GUdpPacket* packet);



/* **************************************** */
/* TO IMPLEMENT?  Write if you need it...    */

/**

   Get port the socket is bound to.  UNIMPLEMENTED

*/
/* gint udp_socket_get_local_port(const UdpSocket* socket); */


/**

   Get the inet address the socket is bound to.  UNIMPLEMENTED

*/
/* InetAddr* udp_socket_get_local_inetaddr(const UdpSocket* socket); */


/**
 
   Get the socket's timeout (0 if there isn't one).  Timeout is the
   number of seconds to wait on a send or receive.  UNIMPLEMENTED 

*/
/* gint udp_socket_get_timeout(const UdpSocket* socket); */


/**

   Set the socket's timeout.  Returns 0 if successful.  UNIMPLEMENTED

*/
/* gint udp_socket_set_timeout(UdpSocket* socket, const gint timeout); */



#endif /* _GNET_UDP_H */
