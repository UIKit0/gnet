/* GNet - Networking library
 * Copyright (C) 2000, 2002-3  David Helder
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
 *  Create and open a new UDP socket bound to all interfaces and an
 *  arbitrary port.
 *
 *  Returns: a new #GUdpSocket, or NULL if there was a failure.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new (void)
{
  return gnet_udp_socket_new_full (NULL, 0);
}


/**
 *  gnet_udp_socket_new_with_port:
 *  @port: Port to use (0 for an arbitrary port)
 * 
 *  Create a #GUdpSocket bound to all interfaces and port @port.  If
 *  @port is 0, an arbitrary port will be used.
 *
 *  Returns: a new #GUdpSocket, or NULL if there was a failure.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new_with_port (gint port)
{
  return gnet_udp_socket_new_full (NULL, port);
}


/**
 *  gnet_udp_socket_new_full:
 *  @iface: Interface to bind to (NULL for all interfaces)
 *  @port: Port to bind to (0 for an arbitrary port)
 * 
 *  Create and open a new #GUdpSocket bound to interface @iface and
 *  port @port.  If @iface is NULL, all interfaces will be used.  If
 *  @port is 0, an arbitrary port will be used.
 *
 *  Returns: a new #GUdpSocket, or NULL if there was a failure.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new_full (const GInetAddr* iface, gint port)
{
  int 			  sockfd;
  struct sockaddr_storage sa;
  GUdpSocket* 		  s;
  const int 		  on = 1;

  /* Create sockfd and address */
  sockfd = gnet_private_create_listen_socket (SOCK_DGRAM, iface, port, &sa);
  if (sockfd < 0)
    return NULL;

  /* Set broadcast option.  This allows the user to broadcast packets.
     It has no effect otherwise. */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
		 (void*) &on, sizeof(on)) != 0)
    {
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  /* Bind to the socket to some local address and port */
  if (bind(sockfd, &GNET_SOCKADDR_SA(sa), GNET_SOCKADDR_LEN(sa)) != 0)
    {
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  /* Create UDP socket */
  s = g_new0 (GUdpSocket, 1);
  s->sockfd = sockfd;
  memcpy (&s->sa, &sa, sizeof(s->sa));
  s->ref_count = 1;

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
gnet_udp_socket_delete (GUdpSocket* s)
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
gnet_udp_socket_ref (GUdpSocket* s)
{
  g_return_if_fail(s != NULL);

  ++s->ref_count;
}


/**
 *  gnet_udp_socket_get_io_channel:
 *  @socket: #GUdpSocket to get #GIOChannel from.
 *
 *  Get a #GIOChannel from the #GUdpSocket.  
 *
 *  THIS IS NOT A NORMAL GIOCHANNEL - DO NOT READ FROM OR WRITE TO IT.
 *
 *  Use the channel with g_io_add_watch() to do asynchronous IO (so if
 *  you do not want to do asynchronous IO, you do not need the
 *  channel).  If you can read from the channel, use
 *  gnet_udp_socket_receive() to read a packet.  If you can write to
 *  the channel, use gnet_udp_socket_send() to send a packet.
 *
 *  There is one channel for every socket.  If the channel is refed
 *  then it must be unrefed eventually.  Do not close the channel --
 *  this is done when the socket is deleted.
 *
 *  Returns: A #GIOChannel; NULL on failure.
 *
 **/
GIOChannel* 
gnet_udp_socket_get_io_channel (GUdpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = gnet_private_io_channel_new(socket->sockfd);
  
  return socket->iochannel;
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
gnet_udp_socket_unref (GUdpSocket* s)
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
 *  gnet_udp_socket_send:
 *  @s: #GUdpSocket to use to send.
 *  @packet: Packet to send.
 *  @data: Data to send
 *  @length: Length of data
 *  @dst: Destination address
 *
 *  Send data to a host using the #GUdpSocket.
 *
 *  Returns: 0 if successful.
 *
 **/
gint 
gnet_udp_socket_send (GUdpSocket* s, const gint8* data, guint length, const GInetAddr* dst)
{
  gint bytes_sent;

  g_return_val_if_fail (s,   -1);
  g_return_val_if_fail (dst, -1);

  bytes_sent = sendto(s->sockfd, 
		      (void*) data, length, 0, 
		      &GNET_SOCKADDR_SA(dst->sa), GNET_SOCKADDR_LEN(dst->sa));
/*    if (bytes_sent == -1) */
/*      g_warning ("sendto failed: %s\n", g_strerror(errno)); */

  /* Return 0 if ok */
  return (bytes_sent != (signed) length);  
}



/**
 *  gnet_udp_socket_receive:
 *  @s: #GUdpSocket to use to receive.
 *  @data: the buffer to write to
 *  @length: length of @data
 *  @src: pointer source address
 *
 *  Receive data using the UDP socket.  The source address is created
 *  and the pointer is stored in @src.  @src is caller-owned.  @src
 *  may be NULL if the source address isn't needed.
 *
 *  Returns: the number of bytes received, -1 if unsuccessful.
 *
 **/
gint 
gnet_udp_socket_receive (GUdpSocket* s, gint8* data, guint length,
			 GInetAddr** src)
{
  gint bytes_received;
  struct sockaddr_storage sa;
  gint sa_len = sizeof(struct sockaddr_storage);

  g_return_val_if_fail (s,    -1);
  g_return_val_if_fail (data, -1);

  bytes_received = recvfrom (s->sockfd, 
			     (void*) data, length, 
			     0, (struct sockaddr*) &sa, &sa_len);

  if (bytes_received == -1)
    return -1;

  if (src)
    {
      (*src) = g_new0 (GInetAddr, 1);
      memcpy (&GNET_INETADDR_SA(*src), &sa, sa_len);
      (*src)->ref_count = 1;
    }

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
gnet_udp_socket_has_packet (const GUdpSocket* s)
{
  fd_set readfds;
  struct timeval timeout = {0, 0};

  FD_ZERO (&readfds);
  FD_SET (s->sockfd, &readfds);
  if ((select(s->sockfd + 1, &readfds, NULL, NULL, &timeout)) == 1)
    {
      return TRUE;
    }

  return FALSE;
}


#else	/*********** Windows code ***********/


gboolean
gnet_udp_socket_has_packet (const GUdpSocket* s)
{
  gint bytes_received;
  gchar data[1];
  guint packetlength;
  u_long arg;
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
 *  gnet_udp_socket_get_ttl:
 *  @us: #GUdpSocket to get TTL from.
 *
 *  Get the TTL of the UDP socket.  TTL is the Time To Live - the
 *  number of hops outgoing packets will travel.  This is useful for
 *  resource discovery; for most programs, you don't need to use it.
 *
 *  Returns: the TTL (an integer between 0 and 255), -1 if the kernel
 *  default is being used, or an integer less than -1 on error.
 *
 **/
gint
gnet_udp_socket_get_ttl (const GUdpSocket* us)
{
  int ttl;
  socklen_t ttl_size;
  int rv;


  ttl_size = sizeof(ttl);

  /* Get the IPv4 TTL if it's bound to an IPv4 address */
  if (GNET_SOCKADDR_FAMILY(us->sa) == AF_INET)
    {
      rv = getsockopt(us->sockfd, IPPROTO_IP, IP_TTL, 
		      (void*) &ttl, &ttl_size);
    }

  /* Or, get the IPv6 TTL if it's bound to an IPv6 address */
  else if (GNET_SOCKADDR_FAMILY(us->sa) == AF_INET6)
    {
      rv = getsockopt(us->sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, 
		      (void*) &ttl, &ttl_size);
    }
  else
    g_assert_not_reached();

  
  if (rv == -1)
    return -2;

  return ttl;
}


/**
 *  gnet_udp_socket_set_ttl:
 *  @us: GUdpSocket to set TTL.
 *  @val: Value to set TTL to.
 *
 *  Set the TTL of the UDP socket.  Set the TTL to -1 to use the
 *  kernel default.
 *
 *  Returns: 0 if successful.
 *
 **/
gint
gnet_udp_socket_set_ttl (GUdpSocket* us, gint val)
{
  int ttl;
  int rv1, rv2;
  GIPv6Policy policy;

  rv1 = -1;
  rv2 = -1;

  /* If the bind address is anything IPv4 *or* the bind address is
     0::0 IPv6 and IPv6 policy allows IPv4, set IP_TTL.  In the latter case,
     if we bind to 0::0 and the host is dual-stacked, then all IPv4
     interfaces will be bound to also. */
  if (GNET_SOCKADDR_FAMILY(us->sa) == AF_INET ||
      (GNET_SOCKADDR_FAMILY(us->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6(us->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4)))
    {
      ttl = val;
      rv1 = setsockopt(us->sockfd, IPPROTO_IP, IP_TTL, 
		       (void*) &ttl, sizeof(ttl));
    }


  /* If the bind address is IPv6, set IPV6_UNICAST_HOPS */
  if (GNET_SOCKADDR_FAMILY(us->sa) == AF_INET6)
    {
      ttl = val;
      rv2 = setsockopt(us->sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, 
		       (void*) &ttl, sizeof(ttl));
    }

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}
