/* Echo server (UDP)
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
#include <string.h>
#include <glib.h>
#include <gnet/gnet.h>



typedef enum { NORMAL} ServerType;

static void normal_echoserver (gint port);


int
main(int argc, char** argv)
{
  ServerType server_type = NORMAL;
  int port = 0;

  if (argc !=  2)
    {
      g_print ("usage: echoserver-udp <port> \n");
      exit(EXIT_FAILURE);
    }


  port = atoi(argv[argc - 1]);

  switch (server_type)
    {
    case NORMAL:
      g_print ("Normal echo server running\n");
      normal_echoserver(port);
      break;
    default:
      g_assert_not_reached();
    }

  return 0;

}


/* ************************************************************ */

void
normal_echoserver (gint port)
{
  GUdpSocket* server;
  GUdpPacket* packet;
  gchar buffer[1024];

  /* Create the server */
  server = gnet_udp_socket_port_new (port);
  g_assert (server);

  /* Create a packet */
  packet = gnet_udp_packet_receive_new(buffer, sizeof(buffer));
  g_assert (packet);

  while (1)
    {
      guint n;
      GUdpPacket* packet_out;

      n = gnet_udp_socket_receive(server, packet);
      if (n == 0)
	continue;

      packet_out = gnet_udp_packet_send_new (buffer, n, packet->addr);
      
      g_assert (gnet_udp_socket_send(server, packet_out) == 0);

      gnet_udp_packet_delete (packet_out);
    }
}
