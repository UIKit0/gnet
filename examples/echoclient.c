/* Echo client
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



int
main(int argc, char** argv)
{
  GInetAddr* addr = NULL;
  GTcpSocket* socket = NULL;
  GIOChannel* iochannel = NULL;
  gchar buffer[1024];
  guint n;
  GIOError error = G_IO_ERROR_NONE;


  if (argc != 3)
    {
      g_print ("usage: echoclient <server> <port>\n");
      exit(EXIT_FAILURE);
    }

  /* Create the address */
  addr = gnet_inetaddr_new(argv[1], atoi(argv[2]));
  g_assert (addr != NULL);

  /* Create the socket */
  socket = gnet_tcp_socket_new(addr);
  g_assert (socket != NULL);

  /* Get the IOChannel */
  iochannel = gnet_tcp_socket_get_iochannel(socket);
  g_assert (iochannel != NULL);


  while (fgets(buffer, sizeof(buffer), stdin) != 0)
    {
      n = strlen(buffer) + 1;
      error = gnet_io_channel_writen(iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      error = gnet_io_channel_readn(iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      fwrite(buffer, n, 1, stdout);
    }

  if (error != G_IO_ERROR_NONE) g_print ("IO error = %d\n", error);

  return 0;
}
