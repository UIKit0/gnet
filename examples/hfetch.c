/* Hfetch - Download a file from a web server.
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


#define DELIM ":/"

void usage(void);
void hfetch(gchar* server, gint port, gchar* filename);



int
main(int argc, char** argv)
{
  gchar* server = NULL;
  gchar* port_str = NULL;
  gint port = 80;
  gchar* filename = NULL;
  gchar* p = NULL;


  if (argc != 2)
    {
      usage();
      exit(EXIT_FAILURE);
    }

  p = argv[1];

  /* Strip off the prefix */
  if (strncmp("http://", p, sizeof("http://") - 1) == 0)
    {
      p = &p[sizeof("http://") - 1];
    }

  /* Read in the server */
  server = p;
  while (*p != 0 && *p != ':' && *p != '/') ++p;
  if ((p - server) == 0) { usage(); exit(EXIT_FAILURE); }
  server = g_strndup(server, p - server);

  /* Read in the port */
  if (*p == ':')
    {
      port_str = ++p;
      while (*p != 0 && *p != '/') ++p;
      if ((p - port_str) == 0) { usage(); exit(EXIT_FAILURE); }
      port_str = g_strndup(port_str, p - port_str);
      port = atoi(port_str);
    }

  /* Read in the file */
  if (*p == 0)
    {
      filename = g_strdup("/");
    }
  else
    {
      filename = g_strdup(p);
    }

/*    g_print ("url = %s\n", argv[1]); */
/*    g_print ("server = %s\n", server); */
/*    g_print ("port = %d\n", port); */
/*    g_print ("file = %s\n", filename); */


  hfetch(server, port, filename);

  g_free(server);
  g_free(port_str);
  g_free(filename);

  return 0;
}


void
usage(void)
{
  g_print ("usage: hfetch <url>\n");
}


void
hfetch(gchar* server, gint port, gchar* filename)
{
  GInetAddr* addr;
  GTcpSocket* socket;
  GIOChannel* iochannel;
  gchar* command;
  gchar buffer[1024];
  GIOError error;
  guint n;


  /* Create the address */
  addr = gnet_inetaddr_new(server, port);
  g_assert (addr != NULL);

  /* Create the socket */
  socket = gnet_tcp_socket_new(addr);
  gnet_inetaddr_delete (addr);
  g_assert (socket != NULL);

  /* Get the IOChannel */
  iochannel = gnet_tcp_socket_get_iochannel(socket);
  g_assert (iochannel != NULL);

  /* Send the command */
  command = g_strconcat("GET ", filename, "\n", NULL);
  error = gnet_io_channel_writen(iochannel, command, strlen(command), &n);
  g_free(command);

  if (error != G_IO_ERROR_NONE)
    {
      g_warning("Write error: %d\n", error);
    }

  /* Read the output */
  while (1)
    {
      error = g_io_channel_read(iochannel, buffer, sizeof(buffer), &n);
      if (error != G_IO_ERROR_NONE)
	{
	  g_warning("Read error: %d\n", error);
	  break;
	}

      if (n == 0)
	break;

      fwrite(buffer, n, 1, stdout);
    }
  
  g_io_channel_unref(iochannel);
  gnet_tcp_socket_delete(socket);
}

