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
#include <gnet.h>



int
main(int argc, char** argv)
{
  int port = 0;
  GUdpSocket* server;
  gchar buffer[1024];
  gint ttl;
  gint rv;

  gnet_init ();

  if (argc !=  2)
    {
      g_print ("usage: echoserver-udp <port> \n");
      exit(EXIT_FAILURE);
    }
  port = atoi(argv[argc - 1]);

  /* Create the server */
  server = gnet_udp_socket_new_with_port (port);
  g_assert (server);

  /* Get the TTL (for fun) */
  ttl = gnet_udp_socket_get_ttl (server);
  g_assert (ttl >= -1);

  /* Set the TTL to 64 (the default on many systems) */
  rv = gnet_udp_socket_set_ttl (server, 64);
  g_assert (rv == 0);

  ttl = gnet_udp_socket_get_ttl (server);
  g_assert (ttl == 64);


  while (1)
    {
      gint bytes_received;
      gint rv;
      GInetAddr* addr;

      bytes_received = gnet_udp_socket_receive (server, buffer, sizeof(buffer), 
						&addr);
      if (bytes_received == -1)
	continue;

      rv = gnet_udp_socket_send (server, buffer, bytes_received, addr);
      g_assert (rv == 0);

      gnet_inetaddr_delete (addr);
    }

  return 0;

}


