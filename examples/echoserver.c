/* Echo server (TCP blocking)
 * Copyright (C) 2000-2003  David Helder
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

#include <signal.h>


static void normal_sig_int (int signum);

static GTcpSocket* normal_server = NULL;


int
main(int argc, char** argv)
{
  gint 	      port;
  GTcpSocket* server;
  GInetAddr*  addr;
  gchar*      name;
  GTcpSocket* client = NULL;
  gchar       buffer[1024];
  guint       n;
  GIOChannel* ioclient = NULL;
  GIOError    error;

  gnet_init ();

  if (argc !=  2)
    {
      fprintf (stderr, "usage: echoserver <port> \n");
      exit(EXIT_FAILURE);
    }

  port = atoi(argv[argc - 1]);

  /* Create the server */
  server = gnet_tcp_socket_server_new_with_port (port);
  if (server == NULL)
    {
      fprintf (stderr, "Could not create server on port %d\n", port);
      exit (EXIT_FAILURE);
    }

  normal_server = server;
  signal (SIGINT, normal_sig_int);

  /* Print the address */
  addr = gnet_tcp_socket_get_inetaddr(server);
  g_assert (addr);
  name = gnet_inetaddr_get_canonical_name (addr);
  g_assert (name);
  port = gnet_inetaddr_get_port (addr);
  g_print ("Normal echoserver running on %s:%d\n", name, port);
  gnet_inetaddr_delete (addr);
  g_free (name);

  while ((client = gnet_tcp_socket_server_accept (server)) != NULL)
    {
      /* Get IOChannel */
      ioclient = gnet_tcp_socket_get_io_channel(client);
      g_assert (ioclient);

      /* Print the address */
      addr = gnet_tcp_socket_get_inetaddr(client);
      g_assert (addr);
      name = gnet_inetaddr_get_canonical_name (addr);
      g_assert (name);
      port = gnet_inetaddr_get_port (addr);
      g_print ("Accepted connection from %s:%d\n", name, port);

      while ((error = gnet_io_channel_readline(ioclient, buffer, 
					       sizeof(buffer), &n)) 
	     == G_IO_ERROR_NONE && (n > 0))
	{
	  error = gnet_io_channel_writen(ioclient, buffer, n, &n);
	  if (error != G_IO_ERROR_NONE) break;
	  fwrite(buffer, n, 1, stdout);
	}

      if (error != G_IO_ERROR_NONE)
      gnet_tcp_socket_delete (client);

      g_print ("Connection from %s:%d closed\n", name, port);

      gnet_inetaddr_delete (addr);
      g_free (name);
    }

  exit (EXIT_SUCCESS);
  return 0;
}


static void 
normal_sig_int (int signum)
{
  gnet_tcp_socket_delete (normal_server);
  exit (EXIT_FAILURE);
}

