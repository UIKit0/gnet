/* Unix Socket Echo client
 * Copyright (C) 2001  Mark Ferlatte
 * Adapted from echoclient.c, Copyright (C) 2000  David Helder
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
#include <glib.h>
#include <gnet.h>

typedef enum { NORMAL, ASYNC} ClientType;


static void usage (int status);

static void normal_echoclient (gchar* path);
static void async_echoclient (gchar* path);

int
main(int argc, char** argv)
{
  ClientType client_type = NORMAL;

  gnet_init ();

  if (argc != 2 && argc != 3)
    usage(EXIT_FAILURE);
  if (argc == 3) {
    if (strcmp(argv[1], "--async") == 0)
      client_type = ASYNC;
    else
      usage(EXIT_FAILURE);
  }

  switch (client_type) {
  case NORMAL:
    g_print ("Normal echo client running\n");
    normal_echoclient(argv[argc - 1]);
    break;
  case ASYNC:
    g_print ("Asynchronous echo client running (UNFINISHED)\n");
    async_echoclient(argv[argc - 1]);
    break;
  default:
    g_assert_not_reached();
  }

  return 0;
}

static void
usage (int status)
{
	g_print ("usage: echoclient [(nothing)|--async] <path>\n");
	exit(status);
}


static void
normal_echoclient(gchar* path)
{
  GUnixSocket *socket = NULL;
  GIOChannel* iochannel = NULL;
  gchar buffer[1024];
  gsize n;
  GIOError e = G_IO_ERROR_NONE;

  g_assert(path != NULL);

  /* Create the socket */
  socket = gnet_unix_socket_new(path);
  g_assert (socket != NULL);

  /* Get the IOChannel */
  iochannel = gnet_unix_socket_get_io_channel(socket);
  g_assert (iochannel != NULL);

  while (fgets(buffer, sizeof(buffer), stdin) != 0) {
    n = strlen(buffer);
    e = gnet_io_channel_writen(iochannel, buffer, n, &n);
    if (e != G_IO_ERROR_NONE)
      break;
    e = gnet_io_channel_readn(iochannel, buffer, n, &n);
    if (e != G_IO_ERROR_NONE)
      break;
    fwrite(buffer, n, 1, stdout);
  }

  if (e != G_IO_ERROR_NONE) 
    g_print ("IO error (%d)\n", e);
  return;
}

static void
async_echoclient(gchar *path)
{
  g_print("Sorry, there is no async echoclient yet.\n");
  return;
}
