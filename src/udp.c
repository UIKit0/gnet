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

#include "gnet-private.h"
#include "udp.h"


/**
 *  gnet_udp_socket_new:
 *  
 *  Create and open a new UDP socket with any port.  
 *
 *  Returns: a new #GUdpSocket, or NULL if there was a failure.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new(void)
{
  return gnet_udp_socket_port_new(0);
}


/**
 *  gnet_udp_socket_port_new:
 *  @port: port number for the socket.
 * 
 *  Create and open a new UDP socket with a specific port.  
 *
 *  Returns: a new #GUdpSocket, or NULL if there was a failure.
 *
 **/
GUdpSocket* 
gnet_udp_socket_port_new(const gint port)
{
  GUdpSocket* s = g_new0(GUdpSocket, 1);
  struct sockaddr_in* sa_in;

  /* Create socket */
  s->ref_count = 1;
  s->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (s->sockfd < 0)
    return NULL;

  /* Set up address and port (any address, any port) */
  sa_in = (struct sockaddr_in*) &s->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = g_htons(port);

  /* Bind to the socket to some local address and port */
  if (bind(s->sockfd, &s->sa, sizeof(s->sa)) != 0)
    return NULL;

  return s;
}



/**
 *  gnet_udp_socket_delete:
 *  @s: #GUdpSocket to delete.
 *
 *  Close and delete a UDP socket.
 *
 **/
void
gnet_udp_socket_delete(GUdpSocket* s)
{
  if (s != NULL)
    gnet_udp_socket_unref(s);
}



/**
 *  gnet_udp_socket_ref
 *  @s: #GUdpSocket to reference
 *
 *  Increment the reference counter of the #GUdpSocket.
 *
 **/
void
gnet_udp_socket_ref(GUdpSocket* s)
{
  g_return_if_fail(s != NULL);

  ++s->ref_count;
}


/**
 *  gnet_udp_socket_unref
 *  @s: #GUdpSocket to unreference
 *
 *  Remove a reference from the #GUdpSocket.  When reference count
 *  reaches 0, the socket is deleted.
 *
 **/
void
gnet_udp_socket_unref(GUdpSocket* s)
{
  g_return_if_fail(s != NULL);

  --s->ref_count;

  if (s->ref_count == 0)
    {
      GNET_CLOSE_SOCKET(s->sockfd);	/* Don't care if this fails... */

      if (s->iochannel)
	g_io_channel_unref(s->iochannel);

      g_free(s);
    }
}



/**
 *  gnet_upd_socket_send:
 *  @s: #GUdpSocket to use to send.
 *  @packet: Packet to send.
 *
 *  Send the packet using the #GUdpSocket.
 *
 *  Returns: 0 if successful.
 *
 **/
gint 
gnet_udp_socket_send(const GUdpSocket* s, const GUdpPacket* packet)
{
  gint bytes_sent;
  struct sockaddr to_sa;

  to_sa = gnet_private_inetaddr_get_sockaddr(packet->addr);

  bytes_sent = sendto(s->sockfd, (void*) packet->data, packet->length, 
		      0, &to_sa, sizeof(to_sa));

  return (bytes_sent != (signed) packet->length);  /* Return 0 if ok, return 1 otherwise */
}



/**
 *  gnet_udp_socket_receive:
 *  @s: #GUdpSocket to receive from.
 *  @packet: Packet to receive.
 *
 *  Receive a packet using the UDP socket.  
 *
 *  Returns the number of bytes received, -1 if unsuccessful.
 *
 **/
gint 
gnet_udp_socket_receive(const GUdpSocket* s, GUdpPacket* packet)
{
  gint bytes_received;
  struct sockaddr from_sa;
  gint length = sizeof(struct sockaddr);

  bytes_received = recvfrom(s->sockfd, (void*) packet->data, packet->length, 
			    0, &from_sa, &length);

  /* Set the address from where this is from */
  if (packet->addr != NULL)	
    gnet_inetaddr_delete(packet->addr);

  packet->addr = gnet_private_inetaddr_sockaddr_new(from_sa);

  return bytes_received;
}


#ifndef GNET_WIN32  /*********** Unix code ***********/


/**
 *  gnet_udp_socket_has_packet:
 *  @s: #GUdpSocket to check
 *
 *  Test if the socket has a receive packet.  It's strongly
 *  recommended that you use a #GIOChannel with a read watch instead
 *  of this function.
 *
 *  Returns: TRUE if there is packet waiting, FALSE otherwise.
 *
 **/
gboolean
gnet_udp_socket_has_packet(const GUdpSocket* s)
{
  struct pollfd pfd;

  pfd.fd = s->sockfd;
  pfd.events = POLLIN | POLLPRI;
  pfd.revents = 0;

  if (poll(&pfd, 1, 0) >= 0)
    {
      if ((pfd.events & pfd.revents) != 0)
	return TRUE;
    }

  return FALSE;
}


#else	/*********** Windows code ***********/


gboolean
gnet_udp_socket_has_packet(const GUdpSocket* s)
{
  gint bytes_received;
  gchar data[1];
  guint packetlength;
  gint arg;
  gint error;

  arg = 1;
  ioctlsocket(s->sockfd, FIONBIO, &arg); /* set to non-blocking mode */

  packetlength = 1;
  bytes_received = recvfrom(s->sockfd, (void*) data, packetlength, 
			    MSG_PEEK, NULL, NULL);

  error = WSAGetLastError();

  arg = 0;
  ioctlsocket(s->sockfd, FIONBIO, &arg); /* set blocking mode */

  if (bytes_received == SOCKET_ERROR)
    {
      if (WSAEMSGSIZE != error)
	{
	  return FALSE;
	}
      /* else, the buffer was not big enough, which is fine since we
	 just want to see if a packet is there..*/
    }

  if (bytes_received)
    return TRUE;

  return FALSE;
}
	
#endif		/*********** End Windows code ***********/



/**
 *  gnet_udp_socket_get_iochannel:
 *  @socket: #GUdpSocket to get #GIOChannel from.
 *
 *  Get a #GIOChannel from the #GUdpSocket.  
 *
 *  THIS IS NOT A NORMAL GIOCHANNEL - DO NOT READ OR WRITE WITH IT.
 *
 *  Use the channel with g_io_add_watch() to do non-blocking IO (so if
 *  you do not want to do non-blocking IO, you do not need the
 *  channel).  If you can read from the channel, use
 *  gnet_udp_socket_receive() to read a packet.  If you can write to
 *  the channel, use gnet_udp_socket_send() to write a packet.
 *
 *  There is one channel for every socket.  This function refs the
 *  channel before returning it.  You should unref the channel when
 *  you are done with it.  However, you should not close the channel -
 *  this is done when you delete the socket.
 *
 *  Returns: A #GIOChannel; NULL on failure.
 *
 **/
GIOChannel* 
gnet_udp_socket_get_iochannel(GUdpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = GNET_SOCKET_IOCHANNEL_NEW(socket->sockfd);
  
  g_io_channel_ref (socket->iochannel);

  return socket->iochannel;
}



/**
 *  gnet_udp_socket_get_ttl:
 *  @us: #GUdpSocket to get TTL from.
 *
 *  Get the TTL of the UDP socket.  TTL is the Time To Live - the
 *  number of hops outgoing packets will travel.  This is useful for
 *  resource discovery; for most programs, you don't need to use it.
 *
 *  Returns: the TTL; -1 on failure.
 *
 **/
gint
gnet_udp_socket_get_ttl(GUdpSocket* us)
{
  gint32 ttl;	/* Warning: on Linux this is 32 bits, but it should be 8 bits */
  socklen_t ttlSize;

  ttlSize = sizeof(ttl);

  if (getsockopt(us->sockfd, IPPROTO_IP, IP_TTL, (void*) &ttl, &ttlSize) < 0)
    return(-1);

  g_assert(ttlSize <= sizeof(ttl));

  return(ttl);
}


/**
 *  gnet_udp_socket_set_ttl:
 *  @us: GUdpSocket to set TTL.
 *  @val: Value to set TTL to.
 *
 *  Set the TTL of the UDP socket.
 *
 *  Returns: 0 if successful.
 *
 **/
gint
gnet_udp_socket_set_ttl(GUdpSocket* us, int val)
{
  int ttl;

  ttl = (guchar) val;

  return setsockopt(us->sockfd, IPPROTO_IP, IP_TTL, (void*) &ttl, sizeof(ttl));
}


/**
 *  gnet_udp_socket_get_mcast_ttl
 *  @us: GUdpSocket to get TTL from.
 *
 *  Get the TTL for outgoing multicast packests.  TTL is the Time To
 *  Live - the number of hops outgoing packets will travel.  The
 *  default TTL is usually 1, which mean outgoing packets will only
 *  travel as far as the local subnet.
 *
 *  This reason this function is in the UDP module is that UdpSocket's
 *  (as well as McastSocket's) can be used to sent to multicast
 *  groups.
 *
 *  Here's a handy table.  Note that the "meaning" really doesn't mean
 *  anything.  The mcast people basically just gave them these names
 *  because they sounded cool.
 *
 *  <table>
 *    <title>TTL and "meaning"</title>
 *    <tgroup cols=2 align=left>
 *    <thead>
 *      <row>
 *        <entry>TTL</entry>
 *        <entry>meaning</entry>
 *      </row>
 *    </thead>
 *    <tbody>
 *      <row>
 *        <entry>0</entry>
 *        <entry>node local</entry>
 *      </row>
 *      <row>
 *        <entry>1</entry>
 *        <entry>link local</entry>
 *      </row>
 *      <row>
 *        <entry>2-32</entry>
 *        <entry>site local</entry>
 *      </row>
 *      <row>
 *        <entry>33-64</entry>
 *        <entry>region local</entry>
 *      </row>
 *      <row>
 *        <entry>65-128</entry>
 *        <entry>continent local</entry>
 *      </row>
 *      <row>
 *        <entry>129-255</entry>
 *        <entry>unrestricted (global)</entry>
 *      </row>
 *    </tbody>
 *  </table>
 *
 *  Returns: the TTL; -1 on failure.
 *
 **/
gint
gnet_udp_socket_get_mcast_ttl(GUdpSocket* us)
{
  guchar ttl;
  socklen_t ttlSize;

  ttlSize = sizeof(ttl);

  if (getsockopt(us->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
		 (void*) &ttl, &ttlSize) < 0)
    return(-1);

  g_assert(ttlSize <= sizeof(ttl));

  return(ttl);
}


/**
 *  gnet_udp_socket_set_mcast_ttl:
 *  @us: GUdpSocket to set mcast TTL.
 *  @val: Value to set mcast TTL to.
 *
 *  Set the TTL for outgoing multicast packets.
 *
 *  This reason this function is in the UDP module is that UdpSocket's
 *  (as well as McastSocket's) can be used to sent to multicast
 *  groups.
 *
 *  Returns 0 if successful.  
 *
 **/
gint
gnet_udp_socket_set_mcast_ttl(GUdpSocket* us, int val)
{
  guchar ttl;

  ttl = (guchar) val;

  return(setsockopt(us->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
		    (void*) &ttl, sizeof(ttl)));
}




/* **************************************** */



/**
 *  gnet_udp_packet_receive_new:
 *  @data: A pointer to the buffer to use for the received data.  
 *  @length: The length of this buffer.
 *
 *  Create a packet for receiving.  
 *
 *  Returns: a new GUdpPacket.
 *
 **/
GUdpPacket* 
gnet_udp_packet_receive_new(gint8* data, gint length)
{
  /* A receive packet is the same as a send packet without an address */
  return gnet_udp_packet_send_new(data, length, NULL);
}


/**
 *  gnet_udp_packet_send_new:
 *  @data: A pointer to the buffer which contains the data to send. 
 *  @length: The length of this buffer.
 *  @addr: The address to which the packet should be sent.
 *
 *  Create a packet for sending.
 *
 *  Returns: a new GUdpPacket.
 *
 **/
GUdpPacket* 
gnet_udp_packet_send_new(gint8* data, gint length, GInetAddr* addr)
{
  GUdpPacket* packet = g_new(GUdpPacket, 1);

  packet->data = data;
  packet->length = length;
  packet->addr = addr;

  return packet;
}


/**
 *  gnet_udp_packet_delete:
 *  @packet: GUdpPacket to delete.
 *
 *  Delete a UDP packet.  The buffer is not deleted.
 *
 **/
void 
gnet_udp_packet_delete(GUdpPacket* packet)
{
  g_free(packet);
}
