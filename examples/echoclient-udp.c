/* Echo client (UDP)
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
#include <gnet.h>	/* Or <gnet/gnet.h> when installed. */

typedef enum { NORMAL} ClientType;


static void usage (int status);

static void normal_echoclient (gchar* hostname, gint port);


int
main(int argc, char** argv)
{
  ClientType client_type = NORMAL;

  gnet_init ();

  if (argc != 3)
    usage(EXIT_FAILURE);

  switch (client_type)
    {
    case NORMAL:
      g_print ("Normal echo client running\n");
      normal_echoclient(argv[argc-2], atoi(argv[argc-1]));
      break;
    default:
      g_assert_not_reached();
    }

  return 0;
}


static void
usage (int status)
{
  g_print ("usage: echoclient-udp <server> <port>\n");
  exit(status);
}



/* ************************************************************ */


static void
normal_echoclient(gchar* hostname, gint port)
{
  GInetAddr* addr = NULL;
  GUdpSocket* socket = NULL;
  gchar buffer[1024];
  guint n;

  /* Create the address */
  addr = gnet_inetaddr_new (hostname, port);
  g_assert (addr != NULL);

  /* Create the socket */
  socket = gnet_udp_socket_new();
  g_assert (socket != NULL);

  while (fgets(buffer, sizeof(buffer), stdin) != 0)
    {
      GUdpPacket* packet;
      gint rv;

      /* Create packet */
      n = strlen(buffer);
      packet = gnet_udp_packet_send_new (buffer, n, addr);

      /* Send packet */
      rv = gnet_udp_socket_send(socket, packet);
      g_assert (rv == 0);
      gnet_udp_packet_delete (packet);

      /* Receive packet */
      packet = gnet_udp_packet_receive_new (buffer, sizeof(buffer));
      n = gnet_udp_socket_receive (socket, packet);
      if (n == 0) break;
      gnet_inetaddr_delete (packet->addr);
      gnet_udp_packet_delete (packet);

      /* Write out */
      fwrite(buffer, n, 1, stdout);
    }

  gnet_inetaddr_delete (addr);
  gnet_udp_socket_delete (socket);
}

