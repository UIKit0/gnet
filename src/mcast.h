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

#ifndef _GNET_MCAST_H
#define _GNET_MCAST_H

#include "inetaddr.h"
#include "udp.h"

#include <glib.h>



/*

  All fields in GMcastSocket are private and should be accessed only
  by using the functions below.

 */
typedef struct _GMcastSocket GMcastSocket;



/* ********** */

GMcastSocket* gnet_mcast_socket_new(void);

GMcastSocket* gnet_mcast_socket_port_new(gint port);

GMcastSocket* gnet_mcast_socket_inetaddr_new(GInetAddr* ia);

void gnet_mcast_socket_delete(GMcastSocket* ms);

gint gnet_mcast_socket_join_group(GMcastSocket* ms, GInetAddr* ia);

gint gnet_mcast_socket_leave_group (GMcastSocket* ms, GInetAddr* ia);

/* ********** */

gint gnet_mcast_socket_send(const GMcastSocket* ms, const GUdpPacket* packet);

gint gnet_mcast_socket_receive(const GMcastSocket* ms, GUdpPacket* packet);

gboolean gnet_mcast_socket_has_packet(const GMcastSocket* s);

/* ********** */

gint gnet_mcast_socket_is_loopback(GMcastSocket* ms);

gint gnet_mcast_socket_set_loopback(GMcastSocket* ms, gint b);

/* ********** */


/**
 *  gnet_mcast_socket_to_udp_socket:
 *  @MS: A GMcastSocket
 *
 *  Convert a multicast socket to a UDP socket (since a multicast
 *  socket is just a UDP socket with some special features).
 *
 **/
#define gnet_mcast_socket_to_udp_socket(MS) ((GUdpSocket*) (MS))


#endif /* _GNET_MCAST_H */
