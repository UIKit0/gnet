/* SDR Example
 * Copyright (C) 2000  David Helder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <glib.h>
#include <gnet.h>	/* Or <gnet/gnet.h> when installed. */

#define MAXLINE 4096



int
main(int argc, char** argv)
{
  GUdpPacket* packet;
  gchar buf[MAXLINE + 1];
  gint n;

  struct sap_packet {
    guint32 sap_header;
    guint32 sap_src;
    char sap_data[1];
  } * sapptr;

  GMcastSocket* ms;
  GInetAddr* ia;
  gint rv;

  gnet_init ();

  /* Create a multicast socket */
  ms = gnet_mcast_socket_port_new(9875);
  g_assert(ms != NULL);

  /* Get address of our group */
  ia = gnet_inetaddr_new("sap.mcast.net", 9875);
  g_assert(ia != NULL);

  /* Join the group */
  rv = gnet_mcast_socket_join_group(ms, ia);
  g_assert(rv == 0);

  printf("Joined %s:%d...\n", gnet_inetaddr_get_name(ia), gnet_inetaddr_get_port(ia));

  /* Print some information about our multicast socket */
  g_print ("My addresss: %s\n", gnet_inetaddr_gethostname());
  g_print ("Loopback: %d\n", gnet_mcast_socket_is_loopback(ms));
  g_print ("TTL: %d\n", 
	   gnet_udp_socket_get_ttl(gnet_mcast_socket_to_udp_socket(ms)));
  g_print ("Mcast TTL: %d\n", 
	   gnet_udp_socket_get_mcast_ttl(gnet_mcast_socket_to_udp_socket(ms)));
/*    g_print ("MTU: %d\n",  */
/*  	   gnet_udp_socket_get_MTU(gnet_mcast_socket_to_udp_socket(ms))); */

  /* Change the loopback */
  rv = gnet_mcast_socket_set_loopback(ms, 0);
  g_assert (rv == 0);

  /* Create a packet for receiving */
  packet = gnet_udp_packet_receive_new(buf, MAXLINE);

  for(;;)
    {
      g_print("Waiting for packet...\n");

      /* Receive packet */
      n = gnet_mcast_socket_receive(ms, packet);
      buf[n] = 0;

      sapptr = (struct sap_packet*) buf;

      n -= 2 * sizeof(guint32);
      if (n <= 0)
	{
	  g_warning("SAP packet too small");
	  continue;
	}

      g_print("%s\n", sapptr->sap_data);

    }
}
