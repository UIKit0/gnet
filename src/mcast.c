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

#include "gnet-private.h"
#include "mcast.h"


/**
 *  gnet_mcast_socket_new:
 *  
 *  Create a new multicast socket with any available port.  Use this
 *  constructor when you are creating a new group and the port number
 *  doesn't matter.  If you want to receive packets from the group,
 *  you will have to join it next.
 *
 *  Returns: a new GMcastSocket, or NULL if there was a failure. 
 *
 **/
GMcastSocket*
gnet_mcast_socket_new(void)
{
  GMcastSocket* ms;

  ms = gnet_mcast_socket_port_new(0);

  return ms;
}



/**
 *  gnet_mcast_socket_port_new:
 *  @port: Port for the GMcastSocket.
 *
 *  Create a new multicast socket with the given port.  If you know
 *  the port of the group you will join, use this constructor.  If you
 *  want to receive packets from the group, you will have to join it,
 *  using the full address, next.
 *
 *  Returns: a new GMcastSocket, or NULL if there was a failure.  
 *
 **/
GMcastSocket* 
gnet_mcast_socket_port_new(gint port)
{
  struct sockaddr sa;
  struct sockaddr_in* saip;
  GInetAddr* ia;
  GMcastSocket* ms;

  saip = (struct sockaddr_in*) &sa;

  saip->sin_family = AF_INET;
  saip->sin_addr.s_addr = INADDR_ANY;
  saip->sin_port = g_htons(port);

  ia = gnet_private_inetaddr_sockaddr_new(sa);
  ms = gnet_mcast_socket_inetaddr_new(ia);
  gnet_inetaddr_delete(ia);
  
  return ms;
}



/**
 *  gnet_mcast_socket_inetaddr_new:
 *  @ia: GInetAddr of the multicast group.
 *
 *  Create a new multicast socket with the GInetAddr.  If you know the
 *  GInetAddr of the group you will join, use this constructor.  If
 *  you want to receive packets from the group, you will have to join
 *  it next.
 *
 *  Returns: a new GMcastSocket, or NULL if there was a failure.
 *
 **/
GMcastSocket* 
gnet_mcast_socket_inetaddr_new(GInetAddr* ia)
{
  GMcastSocket* ms;
  const int on = 1;

  ms = g_new0(GMcastSocket, 1);

  /* Create socket */
  ms->ref_count = 1;
  ms->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ms->sockfd < 0)
    return NULL;

  /* Copy address */
  ms->sa = ia->sa;

  /* Set socket option to share the UDP port */
  if (setsockopt(ms->sockfd, SOL_SOCKET, SO_REUSEADDR, 
		     (void*) &on, sizeof(on)) != 0)
    g_warning("Can't reuse mcast socket\n");

  /* Bind to the socket to some local address and port */
  if (bind(ms->sockfd, &ms->sa, sizeof(ms->sa)) != 0)
    return NULL;

  return ms;
}



/**
 *  gnet_mcast_socket_delete:
 *  @ms: GMcastSocket to delete.
 *
 *  Close and delete a multicast socket.
 *
 **/
void
gnet_mcast_socket_delete(GMcastSocket* ms)
{
  if (ms != NULL)
    gnet_mcast_socket_unref(ms);
}



/**
 *  gnet_mcast_socket_ref
 *  @ia: GMcastSocket to reference
 *
 *  Increment the reference counter of the GMcastSocket.
 *
 **/
void
gnet_mcast_socket_ref(GMcastSocket* s)
{
  g_return_if_fail(s != NULL);

  ++s->ref_count;
}


/**
 *  gnet_mcast_socket_unref
 *  @ia: GMcastSocket to unreference
 *
 *  Remove a reference from the GMcastSocket.  When reference count
 *  reaches 0, the socket is deleted.
 *
 **/
void
gnet_mcast_socket_unref(GMcastSocket* s)
{
  g_return_if_fail(s != NULL);

  --s->ref_count;

  if (s->ref_count == 0)
    {
      close(s->sockfd);	/* Don't care if this fails... */

      if (s->iochannel)
	g_io_channel_unref(s->iochannel);

      g_free(s);
    }
}



/**
 *  gnet_mcast_socket_join_group:
 *  @ms: GMcastSocket to use.
 *  @ia: Address of the group.
 *
 *  Join the multicast group using the multicast socket.  You should
 *  only join one group per socket.  
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_join_group(GMcastSocket* ms, GInetAddr* ia)
{
  struct ip_mreq mreq;

  /* Create the multicast request structure */
  memcpy(&mreq.imr_multiaddr, 
	 &((struct sockaddr_in*) &ia->sa)->sin_addr,
	 sizeof(struct in_addr));
  mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

  /* Join the group */
  return setsockopt(ms->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		    (void*) &mreq, sizeof(mreq));
}


/**
 *  gnet_mcast_socket_leave_group:
 *  @ms: GMcastSocket to use.
 *  @ia: Address of the group to leave.
 *
 *  Leave the mulitcast group using the multicast socket.  
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_leave_group(GMcastSocket* ms, GInetAddr* ia)
{
  struct ip_mreq mreq;

  /* Create the multicast request structure */
  memcpy(&mreq.imr_multiaddr,
	 &((struct sockaddr_in*) &ia->sa)->sin_addr,
	 sizeof(struct in_addr));
  mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

  /* Leave the group */
  return setsockopt(ms->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		    (void*) &mreq, sizeof(mreq));
}


/**
 *  gnet_mcast_socket_send:
 *  @ms: GMcastSocket to use to send.
 *  @packet: Packet to send.
 *
 *  Send the packet using the mcast socket.  
 *
 *  Returns: 0 if successful.
 *
 **/
gint 
gnet_mcast_socket_send(const GMcastSocket* ms, const GUdpPacket* packet)
{
  return gnet_udp_socket_send((const GUdpSocket*) ms, packet);
}


/**
 *  gnet_mcast_socket_receive:
 *  @ms: GMcastSocket to receive from.
 *  @packet: Packet to receive.
 *
 *  Receive a packet using the mcast socket.  
 *
 *  Returns: the number of bytes received, -1 if unsuccessful.
 *
 **/
gint 
gnet_mcast_socket_receive(const GMcastSocket* ms, GUdpPacket* packet)
{
  return gnet_udp_socket_receive((const GUdpSocket*) ms, packet);
}


/**
 *  gnet_mcast_socket_has_packet:
 *  @s: GMcastSocket to check
 *
 *  Test if the socket has a receive packet.  
 *
 *  Returns: TRUE if there is packet waiting, FALSE otherwise.
 *
 **/
gboolean
gnet_mcast_socket_has_packet(const GMcastSocket* s)
{
  return gnet_udp_socket_has_packet((const GUdpSocket*) s);
}


/**
 *  gnet_mcast_socket_is_loopback:
 *  @ms: GMcastSocket to check.
 *
 *  Check if the multicast socket has loopback enabled.  If loopback
 *  is enabled, you receive all the packets you send.
 *
 *  Returns: 0 if loopback is disabled.
 *
 **/
gint
gnet_mcast_socket_is_loopback(GMcastSocket* ms)
{
  guchar flag;
  socklen_t flagSize;

  flagSize = sizeof(flag);

  if (getsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, &flagSize) < 0)
    return(-1);

  g_assert(flagSize <= sizeof(flag));

  return (gint) flag;
}



/**
 *  gnet_mcast_socket_set_loopback:
 *  @ms: GMcastSocket to use.
 *  @b: Value to set it to (0 or 1)
 *
 *  Turn the loopback on or off.  If loopback is on, when the process
 *  sends a packet, it will automatically be looped back to the host.
 *  If it is off, not only will this the process not receive datagrams
 *  it sends, other processes on the host will not receive its
 *  packets.
 *
 *  Returns: 0 if successful.
 *
 **/
gint
gnet_mcast_socket_set_loopback(GMcastSocket* ms, int b)
{
  guchar flag;

  flag = (guchar) b;

  return setsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
		    &flag, sizeof(flag));
}
