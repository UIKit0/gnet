/* Echo client
 * Copyright (C) 2000-2001  David Helder
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
  gchar* hostname;
  gint port;
  GInetAddr* addr;
  GTcpSocket* socket;
  GIOChannel* iochannel;
  GIOError error = G_IO_ERROR_NONE;
  gchar buffer[1024];
  gsize n;

  gnet_init ();

  /* Parse args */
  if (argc != 3)
    {  
      g_print ("usage: %s <server> <port>\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  hostname = argv[1];
  port = atoi(argv[2]);

  /* Create the address */
  addr = gnet_inetaddr_new (hostname, port);
  if (!addr)
    {
      fprintf (stderr, "Error: Name lookup for %s failed\n", hostname);
      exit (EXIT_FAILURE);
    }

  /* Create the socket */
  socket = gnet_tcp_socket_new (addr);
  gnet_inetaddr_delete (addr);
  if (!socket)
    {
      fprintf (stderr, "Error: Could not connect to %s:%d\n", hostname, port);
      exit (EXIT_FAILURE);
    }

#if 0
  {
    gchar* cname;

    /* Print local address */
    addr = gnet_tcp_socket_get_local_inetaddr (socket);
    g_assert (addr);
    cname = gnet_inetaddr_get_canonical_name (addr);
    g_assert (cname);
    g_print ("Local address: %s:%d\n", cname, gnet_inetaddr_get_port(addr));
    g_free (cname);
    gnet_inetaddr_delete (addr);

    /* Print remote address */
    addr = gnet_tcp_socket_get_remote_inetaddr (socket);
    g_assert (addr);
    cname = gnet_inetaddr_get_canonical_name (addr);
    g_assert (cname);
    g_print ("Remote address: %s:%d\n", cname, gnet_inetaddr_get_port(addr));
    g_free (cname);
    gnet_inetaddr_delete (addr);
  }
#endif

  /* Get the IOChannel */
  iochannel = gnet_tcp_socket_get_io_channel (socket);
  g_assert (iochannel != NULL);

  while (fgets(buffer, sizeof(buffer), stdin) != 0)
    {
      n = strlen(buffer);
      error = gnet_io_channel_writen (iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      error = gnet_io_channel_readn (iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      fwrite(buffer, n, 1, stdout);
    }

  if (error != G_IO_ERROR_NONE) 
    fprintf (stderr, "Error: IO error (%d)\n", error);

  gnet_tcp_socket_delete (socket);

  return 0;
}

