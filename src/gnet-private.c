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
#include "gnet.h"

#include <config.h>



/**
 *  gnet_private_inetaddr_sockaddr_new:
 *  @sa: sockaddr struct to create InetAddr from.
 *
 *  Create an internet address from a sockaddr struct.  WARNING: This
 *  may go away or be hidden in a future version.
 *
 *  Returns: a new InetAddr, or NULL if there was a failure.  
 *
 **/
GInetAddr* 
gnet_private_inetaddr_sockaddr_new(const struct sockaddr sa)
{
  GInetAddr* ia = g_new0(GInetAddr, 1);

  ia->sa = sa;

  return ia;
}



/**
 *  gnet_private_inetaddr_get_sockaddr:
 *  @ia: Address to get the sockaddr of.
 *
 *  Get the sockaddr struct.  WARNING: This may go away or be hidden
 *  in a future version.
 *
 *  Returns: the sockaddr struct
 **/
struct sockaddr 
gnet_private_inetaddr_get_sockaddr(const GInetAddr* ia)
{
  g_assert(ia != NULL);

  return ia->sa;
}



/**
 *  gnet_private_inetaddr_list_interfaces:
 *
 *  Get a list of InetAddr's on this host (usually there's only one,
 *  but there can be more).  This isn't complete - we only look at
 *  running, non-loopback addresses.
 *
 *  (TODO: Make a NetInterface struct which includes flags so you know
 *  if its mcast or bcast and can have the bcast address, etc.).
 *
 *  THIS IS HERE BECAUSE IT'S INCOMPLETE.
 *
 *  Returns: A list of InetAddrs representing available interfaces.
 *
 **/
GList* 
gnet_private_inetaddr_list_interfaces(void)
{
  GList* list = NULL;
  gint len, lastlen;
  gchar* buf;
  gchar* ptr;
  gint sockfd;
  struct ifconf ifc;


  /* Create a dummy socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) return NULL;

  len = 8 * sizeof(struct ifreq);
  lastlen = 0;

  /* Get the list of interfaces.  We might have to try multiple times
     if there are a lot of interfaces. */
  while(1)
    {
      buf = g_new0(gchar, len);
      
      ifc.ifc_len = len;
      ifc.ifc_buf = buf;
      if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
	{
	  /* Might have failed because our buffer was too small */
	  if (errno != EINVAL || lastlen != 0)
	    {
	      g_free(buf);
	      return NULL;
	    }
	}
      else
	{
	  /* Break if we got all the interfaces */
	  if (ifc.ifc_len == lastlen)
	    break;

	  lastlen = ifc.ifc_len;
	}

      /* Did not allocate big enough buffer - try again */
      len += 8 * sizeof(struct ifreq);
      g_free(buf);
    }


  /* Create the list.  Stevens has a much more complex way of doing
     this, but his is probably much more correct portable.  */
  for (ptr = buf; ptr < (buf + ifc.ifc_len); ptr += sizeof(struct ifreq))
    {
      struct ifreq* ifr = (struct ifreq*) ptr;
      struct sockaddr addr;
      GInetAddr* ia;

      /* Ignore non-AF_INET */
      if (ifr->ifr_addr.sa_family != AF_INET)
	continue;

      /* FIX: Skip colons in name?  Can happen if aliases, maybe. */

      /* Save the address - the next call will clobber it */
      memcpy(&addr, &ifr->ifr_ifru.ifru_addr, sizeof(addr));
      
      /* Get the flags */
      ioctl(sockfd, SIOCGIFFLAGS, ifr);

      /* Ignore entries that aren't running or are loopback.  Someday
	 we'll write an interface structure and include this stuff. */
      if (!(ifr->ifr_ifru.ifru_flags & IFF_RUNNING) ||
	  (ifr->ifr_ifru.ifru_flags & IFF_LOOPBACK))
	continue;

      /* Create an InetAddr for this one and add it to our list */
      ia = gnet_private_inetaddr_sockaddr_new(addr);
      if (ia != NULL)
	list = g_list_prepend(list, ia);
    }

  return list;
}


/* TODO: Need to port this to Solaris */

#if 0

/**
 *  gnet_udp_socket_get_MTU:
 *  @us: GUdpSocket to get MTU from.
 *
 *  Get the MTU for outgoing packets.  
 *
 *  Returns: MTU; -1 if unknown.
 *
 **/
gint
gnet_udp_socket_get_MTU(GUdpSocket* us)
{
  struct ifreq ifr;

  /* FIX: Not everyone has ethernet, right? */
  strncpy (ifr.ifr_name, "eth0", sizeof (ifr.ifr_name));
  if (!ioctl(us->sockfd, SIOCGIFMTU, &ifr)) 
    return ifr.ifr_mtu;

  return -1;
}

#endif
