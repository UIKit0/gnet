/* GNet - Networking library
 * Copyright (C) 2000, 2002-3  David Helder
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
 *  Create a new multicast socket bound to all interfaces using an
 *  arbitrary port.
 *
 *  Returns: a new #GMcastSocket, or NULL if there was a failure. 
 *
 **/
GMcastSocket*
gnet_mcast_socket_new (void)
{
  return gnet_mcast_socket_new_full (NULL, 0);
}



/**
 *  gnet_mcast_socket_port_new:
 *  @port: Port for the #GMcastSocket.
 *
 *  Create a #GMcastSocket bound to all interfaces and port @port.  If
 *  @port is 0, an arbitrary port will be used.  If you know the port
 *  of the group you will join, use this constructor.
 *
 *  Returns: a new #GMcastSocket, or NULL if there was a failure.  
 *
 **/
GMcastSocket* 
gnet_mcast_socket_new_with_port (gint port)
{
  return gnet_mcast_socket_new_full (NULL, port);
}



/**
 *  gnet_mcast_socket_inetaddr_new:
 *  @iface: Interface to bind to (NULL for all interfaces)
 *  @port: Port to bind to (0 for an arbitrary port)
 *
 *  Create and open a new #GMcastSocket bound to interface @iface and
 *  port @port.  If @iface is NULL, all interfaces will be used.  If
 *  @port is 0, an arbitrary port will be used.  To receive packets
 *  from this group, call gnet_mcast_socket_join_group() next.
 *
 *  Returns: a new #GMcastSocket, or NULL if there was a failure.
 *
 **/
GMcastSocket* 
gnet_mcast_socket_new_full (const GInetAddr* iface, gint port)
{
  int 			  sockfd;
  struct sockaddr_storage sa;
  GMcastSocket* 	  ms;
  const int               on = 1;

  /* Create sockfd and address */
  sockfd = gnet_private_create_listen_socket (SOCK_DGRAM, iface, port, &sa);
  if (sockfd < 0)
    return NULL;

  /* Set socket option to share the UDP port */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
		 (void*) &on, sizeof(on)) != 0)
    g_warning("Can't reuse mcast socket\n");

  /* Bind to the socket to some local address and port */
  if (bind(sockfd, &GNET_SOCKADDR_SA(sa), GNET_SOCKADDR_LEN(sa)) != 0)
    {
      GNET_CLOSE_SOCKET (sockfd);
      return NULL;
    }

  /* Create socket */
  ms = g_new0(GMcastSocket, 1);
  ms->sockfd = sockfd;
  memcpy (&ms->sa, &sa, sizeof(ms->sa));
  ms->ref_count = 1;

  return ms;
}



/**
 *  gnet_mcast_socket_delete:
 *  @ms: #GMcastSocket to delete.
 *
 *  Close and delete a multicast socket.
 *
 **/
void
gnet_mcast_socket_delete (GMcastSocket* ms)
{
  if (ms != NULL)
    gnet_mcast_socket_unref(ms);
}



/**
 *  gnet_mcast_socket_ref
 *  @s: #GMcastSocket to reference
 *
 *  Increment the reference counter of the #GMcastSocket.
 *
 **/
void
gnet_mcast_socket_ref (GMcastSocket* s)
{
  g_return_if_fail (s != NULL);

  ++s->ref_count;
}


/**
 *  gnet_mcast_socket_get_io_channel:
 *  @socket: #GUdpSocket to get #GIOChannel from.
 *
 *  Get a #GIOChannel from the #GUdpSocket.  
 *
 *  THIS IS NOT A NORMAL GIOCHANNEL - DO NOT READ FROM OR WRITE TO IT.
 *
 *  Use the IO channel with g_io_add_watch() to do asynchronous IO (so
 *  if you do not want to do asynchronous IO, you do not need the
 *  channel).  If you can read from the channel, use
 *  gnet_udp_socket_receive() to read a packet.  If you can write to
 *  the channel, use gnet_mcast_socket_send() to send a packet.
 *
 *  There is one channel for every socket.  If the channel is refed
 *  then it must be unrefed eventually.  Do not close the channel --
 *  this is done when the socket is deleted.
 *
 *  Returns: A #GIOChannel; NULL on failure.
 *
 **/
GIOChannel*   
gnet_mcast_socket_get_io_channel (GMcastSocket* socket)
{
  return gnet_udp_socket_get_io_channel((GUdpSocket*) socket);
}



/**
 *  gnet_mcast_socket_unref
 *  @s: #GMcastSocket to unreference
 *
 *  Remove a reference from the #GMcastSocket.  The socket is deleted
 *  when reference count reaches 0.
 *
 **/
void
gnet_mcast_socket_unref (GMcastSocket* s)
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
 *  gnet_mcast_socket_join_group:
 *  @ms: #GMcastSocket to use.
 *  @ia: Address of the group.
 *
 *  Join a multicast group using the multicast socket.  You should
 *  only join one group per socket.
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_join_group (GMcastSocket* ms, const GInetAddr* ia)
{
  gint rv;

  if (GNET_INETADDR_FAMILY(ia) == AF_INET)
    {
      struct ip_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.imr_multiaddr, GNET_INETADDR_ADDRP(ia), 
	     sizeof(mreq.imr_multiaddr));
      mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

      /* Join the group */
      rv = setsockopt(ms->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		      (void*) &mreq, sizeof(mreq));
    }
  else if (GNET_INETADDR_FAMILY(ia) == AF_INET6)
    {
      struct ipv6_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.ipv6mr_multiaddr, GNET_INETADDR_ADDRP(ia), 
	     sizeof(mreq.ipv6mr_multiaddr));
      mreq.ipv6mr_interface = 0;

      /* Join the group */
      rv = setsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
		      (void*) &mreq, sizeof(mreq));
    }
  else
    g_assert_not_reached ();

  return rv;
}


/**
 *  gnet_mcast_socket_leave_group:
 *  @ms: #GMcastSocket to use.
 *  @ia: Address of the group to leave.
 *
 *  Leave the mulitcast group.
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_leave_group (GMcastSocket* ms, const GInetAddr* ia)
{
  gint rv;

  if (GNET_INETADDR_FAMILY(ia) == AF_INET)
    {
      struct ip_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.imr_multiaddr, GNET_INETADDR_ADDRP(ia), 
	     sizeof(mreq.imr_multiaddr));
      mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

      /* Join the group */
      rv = setsockopt(ms->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		      (void*) &mreq, sizeof(mreq));
    }
  else if (GNET_INETADDR_FAMILY(ia) == AF_INET6)
    {
      struct ipv6_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.ipv6mr_multiaddr, GNET_INETADDR_ADDRP(ia), 
	     sizeof(mreq.ipv6mr_multiaddr));
      mreq.ipv6mr_interface = 0;

      /* Join the group */
      rv = setsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
		      (void*) &mreq, sizeof(mreq));
    }
  else
    g_assert_not_reached ();

  return rv;
}




/**
 *  gnet_mcast_socket_get_ttl:
 *  @us: GMcastSocket to get TTL from.
 *
 *  Get the TTL for outgoing multicast packets.  TTL is the Time To
 *  Live - the maximum number of hops outgoing multicast packets can
 *  travel.  The default TTL is usually 1, which mean outgoing packets
 *  will only travel as far as the local subnet.
 *
 *  Here's a handy table.  Note that the "meaning" really doesn't mean
 *  anything.  The multicast people basically just gave them these
 *  names because it seemed like a good idea at the time.
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
 *  Returns: the TTL (an integer between 0 and 255), -1 if the kernel
 *  default is being used, or an integer less than -1 on error.
 *
 **/
gint
gnet_mcast_socket_get_ttl (const GMcastSocket* ms)
{
  guchar ttl;
  socklen_t ttl_size;
  int rv;

  ttl_size = sizeof(ttl);

  /* Get the IPv4 TTL if the socket is bound to an IPv4 address */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET)
    {
      rv = getsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
		      (void*) &ttl, &ttl_size);
    }

  /* Get the IPv6 TTL if the socket is bound to an IPv6 address */
  else if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6)
    {
      rv = getsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
		      (void*) &ttl, &ttl_size);
    }
  else
    g_assert_not_reached ();

  if (rv == -1)
    return -2;

  return ttl;
}


/**
 *  gnet_mcast_socket_set_ttl:
 *  @ms: GMcatSocket to set multicast TTL.
 *  @val: Value to set multicast TTL to.
 *
 *  Set the TTL for outgoing multicast packets.
 *
 *  Returns 0 if successful.  
 *
 **/
gint
gnet_mcast_socket_set_ttl (GMcastSocket* ms, int val)
{
  guchar ttl;
  int rv1, rv2;
  GIPv6Policy policy;

  rv1 = -1;
  rv2 = -1;

  /* If the bind address is anything IPv4 *or* the bind address is
     0::0 IPv6 and IPv6 policy allows IPv4, set IP_TTL.  In the latter case,
     if we bind to 0::0 and the host is dual-stacked, then all IPv4
     interfaces will be bound to also. */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET ||
      (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6(ms->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4)))
    {
      ttl = val;
      rv1 = setsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
		       (void*) &ttl, sizeof(ttl));
    }


  /* If the bind address is IPv6, set IPV6_UNICAST_HOPS */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6)
    {
      ttl = val;
      rv2 = setsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
		       (void*) &ttl, sizeof(ttl));
    }

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}



/**
 *  gnet_mcast_socket_send:
 *  @ms: #GMcastSocket to use to send.
 *  @packet: Packet to send.
 *  @data: Data to send
 *  @length: Length of data
 *  @dst: Destination address
 *
 *  Send data to a host using the #GMcastSocket.
 *
 *  Returns: 0 if successful.
 *
 **/
gint 
gnet_mcast_socket_send (GMcastSocket* ms, const gint8* data, guint length, const GInetAddr* dst)
{
  return gnet_udp_socket_send((GUdpSocket*) ms, data, length, dst);
}


/**
 *  gnet_mcast_socket_receive:
 *  @ms: #GMcastSocket to use to receive.
 *  @data: the buffer to write to
 *  @length: length of @data
 *  @src: pointer source address
 *
 *  Receive data using the multicast socket.  The source address is
 *  created and the pointer is stored in @src.  @src is caller-owned.
 *  @src may be NULL if the source address isn't needed.
 *
 *  Returns: the number of bytes received, -1 if unsuccessful.
 *
 **/
gint 
gnet_mcast_socket_receive (GMcastSocket* ms, gint8* data, guint length,
			   GInetAddr** src)
{
  return gnet_udp_socket_receive((GUdpSocket*) ms, data, length, src);
}


/**
 *  gnet_mcast_socket_has_packet:
 *  @s: #GMcastSocket to check
 *
 *  Test if the socket has a receive packet.  
 *
 *  Returns: TRUE if there is packet waiting, FALSE otherwise.
 *
 **/
gboolean
gnet_mcast_socket_has_packet (const GMcastSocket* s)
{
  return gnet_udp_socket_has_packet((const GUdpSocket*) s);
}


/**
 *  gnet_mcast_socket_is_loopback:
 *  @ms: #GMcastSocket to check.
 *
 *  Check if the multicast socket has loopback enabled.  If loopback
 *  is enabled, you receive all the packets you send.  Most people
 *  don't want this.
 *
 *  Returns: 0 if loopback is disabled, 1 if enabled, and -1 on error.
 *
 **/
gint
gnet_mcast_socket_is_loopback (const GMcastSocket* ms)
{
  socklen_t flag_size;
  int rv;
  gint is_loopback = 0;

  /* Get IPv4 loopback if it's bound to a IPv4 address */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET)
    {
      guchar flag;

      flag_size = sizeof (flag);
      rv = getsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, 
		      &flag, &flag_size);
      if (flag)
	is_loopback = 1;
    }

  /* Otherwise, get IPv6 loopback */
  else if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6)
    {
      guint flag;

      flag_size = sizeof (flag);
      rv = getsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
		      &flag, &flag_size);
      if (flag)
	is_loopback = 1;
    }
  else
    g_assert_not_reached();

  if (rv == -1)
    return -1;

  return is_loopback;
}



/**
 *  gnet_mcast_socket_set_loopback:
 *  @ms: #GMcastSocket to use.
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
gnet_mcast_socket_set_loopback (GMcastSocket* ms, int b)
{
  int rv1, rv2;
  GIPv6Policy policy;

  rv1 = -1;
  rv2 = -1;

  /* Set IPv4 loopback.  (As in set_ttl().) */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET ||
      (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6(ms->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4)))
    {
      guchar flag;

      flag = (guchar) b;

      rv1 = setsockopt(ms->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
		       &flag, sizeof(flag));
    }

  /* Set IPv6 loopback */
  if (GNET_SOCKADDR_FAMILY(ms->sa) == AF_INET6)
    {
      guint flag;

      flag = (guint) b;

      rv2 = setsockopt(ms->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       &flag, sizeof(flag));
    }

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}
