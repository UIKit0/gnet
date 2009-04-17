/* Echo client (UDP)
 * Copyright (C) 2000  David Helder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <gnet.h>


static void usage (int status);


int
main(int argc, char** argv)
{
  gchar* hostname;
  gint port;
  GInetAddr* addr = NULL;
  GUdpSocket* socket = NULL;
  gint ttl;
  gint rv;
  gchar buffer[1024];
  guint n;

  gnet_init ();

  if (argc != 3)
    usage (EXIT_FAILURE);
  hostname = argv[argc-2];
  port = atoi(argv[argc-1]);

  /* Create the address */
  addr = gnet_inetaddr_new (hostname, port);
  g_assert (addr != NULL);

  /* Create the socket */
  socket = gnet_udp_socket_new ();
  g_assert (socket != NULL);

  /* Get the TTL */
  ttl = gnet_udp_socket_get_ttl (socket);
  g_assert (ttl >= -1);

  /* Set the TTL to 64 (the default on many systems) */
  rv = gnet_udp_socket_set_ttl (socket, 64);
  g_assert (rv == 0);

  /* Make sure that worked */
  ttl = gnet_udp_socket_get_ttl (socket);
  g_assert (ttl == 64);

  while (fgets(buffer, sizeof(buffer), stdin) != 0)
    {
      gint rv;

      /* Send packet */
      n = strlen(buffer);
      rv = gnet_udp_socket_send (socket, buffer, n, addr);
      g_assert (rv == 0);

      /* Receive packet */
      n = gnet_udp_socket_receive (socket, buffer, sizeof(buffer), NULL);
      if (n == -1) break;

      /* Write out */
      if (fwrite(buffer, n, 1, stdout) != 1) {
       fprintf (stderr, "Error: fwrite to stdout failed: %s\n", g_strerror (errno));
      }
    }

  gnet_inetaddr_delete (addr);
  gnet_udp_socket_delete (socket);

  return 0;
}


static void
usage (int status)
{
  g_print ("usage: echoclient-udp <server> <port>\n");
  exit(status);
}
