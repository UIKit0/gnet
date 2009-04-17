/* Unix socket Echo server
 * Copyright (C) 2001  Mark Ferlatte
 * Adapted from echoserver.c, Copyright (C) 2000  David Helder
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
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include <gnet.h>

typedef enum { NORMAL, ASYNC} ServerType;

void cleanup_on_sig(int signum);

static void usage(int status);
static void normal_echoserver(gchar *path, gboolean abstract);
static void async_echoserver(gchar *path, gboolean abstract);

GUnixSocket *server;

int
main(int argc, char** argv)
{
  gchar *path = NULL;
  ServerType server_type = NORMAL;
  gboolean abstract = FALSE;
	
  gnet_init ();

  if (argc < 2) {
    usage(EXIT_FAILURE);
  }
  if (argc > 2) {
    if (strcmp(argv[1], "--async") == 0 || strcmp(argv[2], "--async") == 0)
      server_type = ASYNC;
    if (strcmp(argv[1], "--abstract") == 0 ||
            strcmp(argv[2], "--abstract") == 0)
      abstract = TRUE;
  }

  path = g_strdup (argv[argc - 1]);

  signal(SIGINT, cleanup_on_sig);
  signal(SIGTERM, cleanup_on_sig);
		
  switch (server_type) {
  case NORMAL:
    g_print("Normal echo server running\n");
    normal_echoserver(path, abstract);
    break;
  case ASYNC:
    g_print("Async echo server running\n");
    async_echoserver(path, abstract);
    break;
  default:
    g_assert_not_reached();
  }
  return 0;
}

static void
usage(int status)
{
  g_print("usage: echoserver-unix [(nothing)|--async| --abstract] <path>\n");
  exit(status);
}

void
cleanup_on_sig(int signum)
{
  gnet_unix_socket_delete(server);
  exit(EXIT_SUCCESS);
}

static void
normal_echoserver(gchar *path, gboolean abstract)
{
  GUnixSocket *client = NULL;
  gchar buffer[1024];
  gsize n;
  GIOChannel *ioclient = NULL;
  GIOError e;

  g_assert(path != NULL);
  /* Create the server */
  if (abstract)
    server = gnet_unix_socket_server_new_abstract(path);
  else
    server = gnet_unix_socket_server_new(path);
  g_assert(server != NULL);

  while ((client = gnet_unix_socket_server_accept(server)) != NULL) {
    ioclient = gnet_unix_socket_get_io_channel(client);
    g_assert(ioclient != NULL);

    while ((e = gnet_io_channel_readline(ioclient, buffer,
					 sizeof(buffer), &n))
	   == G_IO_ERROR_NONE && (n > 0)) {
      e = gnet_io_channel_writen(ioclient, buffer, n, &n);
      if (e != G_IO_ERROR_NONE)
	break;
      if (fwrite(buffer, n, 1, stdout) != 1) {
       fprintf (stderr, "Error: fwrite to stdout failed: %s\n", g_strerror (errno));
      }
    }
    if (e != G_IO_ERROR_NONE)
      fprintf(stderr,
	      "\nRecieved error %d (closing socket).\n",
	      e);
    gnet_unix_socket_delete (client);
  }
}

typedef struct
{
  GUnixSocket *socket;
  guint out_watch;
  gchar buffer[1024];
  guint n;
} client_state;

static void clientstate_delete(client_state *state);

static gboolean async_server_iofunc(GIOChannel *server,
				    GIOCondition c, gpointer data);
static gboolean async_client_iofunc(GIOChannel *client,
				    GIOCondition c, gpointer data);

static void
clientstate_delete(client_state *state)
{
  if (state->out_watch)
    g_source_remove(state->out_watch);
  gnet_unix_socket_delete(state->socket);
  g_free(state);
}

static void
async_echoserver(gchar *path, gboolean abstract)
{
  GIOChannel *iochannel = NULL;
  GMainLoop *main_loop = NULL;

  g_assert(path != NULL);
  if (abstract)
    server = gnet_unix_socket_server_new_abstract(path);
  else
    server = gnet_unix_socket_server_new(path);
  g_assert(server != NULL);

  main_loop = g_main_new(FALSE);

  /* Add a watch for incoming clients */
  iochannel = gnet_unix_socket_get_io_channel(server);
  g_io_add_watch(iochannel,
		 G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		 async_server_iofunc, server);

  /* Start the main loop */
  g_main_run(main_loop);
}

static gboolean
async_server_iofunc(GIOChannel *iochannel, GIOCondition c, gpointer data)
{
  GUnixSocket *server = (GUnixSocket *) data;
  g_assert(server != NULL);

  if (c & G_IO_IN) {
    GUnixSocket *client = NULL;
    GIOChannel *client_iochannel = NULL;
    client_state *cs = NULL;

    client = gnet_unix_socket_server_accept(server);
    g_assert(client != NULL);

    client_iochannel = gnet_unix_socket_get_io_channel(client);
    g_assert(client_iochannel != NULL);

    cs = g_new0(client_state, 1);
    cs->socket = client;

    g_io_add_watch(client_iochannel,
		   G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		   async_client_iofunc, cs);
  } else {
    fprintf(stderr, "Server error %d\n", c);
    return FALSE;
  }
  return TRUE;
}

/*

  This callback is called for (IN|ERR) or OUT.  

  We add the watch for (IN|ERR) when the client connects in
  async_server_iofunc().  We remove it if the connection is closed
  (i.e. we read 0 bytes) or if there's an error.

  We add the watch for OUT when we have something to write and remove
  it when we're done writing or when the connection is closed.
 */
static gboolean
async_client_iofunc(GIOChannel *iochannel, GIOCondition c,
		    gpointer data)
{
  client_state *cs = (client_state *) data;

  g_assert(cs != NULL);

  /* Check for socket error */
  if (c & G_IO_ERR) {
    fprintf(stderr, "Client socket error!\n");
    goto error;
  }
  /* Check for data to be read (or if the socket was closed) */
  if (c & G_IO_IN) {
    GIOError e;
    gsize bytes_read;

    /* Read the data into our buffer */
    e = g_io_channel_read(iochannel,
			  &cs->buffer[cs->n],
			  sizeof(cs->buffer) - cs->n,
			  &bytes_read);
    /* Check for socket error */
    if (e != G_IO_ERROR_NONE) {
      fprintf(stderr, "Client read error: %d\n", e);
      goto error;
    } else if (bytes_read == 0) {
      /* Check if we read 0 bytes, which means the connection
	 is closed */
      goto error;
    } else {
      g_assert(bytes_read > 0);
      /* If there isn't an out_watch, add one */
      if (cs->out_watch == 0) {
	cs->out_watch =
	  g_io_add_watch(iochannel,
			 G_IO_OUT,
			 async_client_iofunc,
			 cs);
      }
      if (fwrite(&cs->buffer[cs->n], bytes_read, 1, stdout) != 1) {
       fprintf (stderr, "Error: fwrite to stdout failed: %s\n", g_strerror (errno));
      }
      cs->n += bytes_read;
    }
  }
  if (c & G_IO_OUT) {
    GIOError e;
    gsize bytes_written;
    /* Write the data out */
    e = g_io_channel_write(iochannel, cs->buffer, cs->n,
			   &bytes_written);
    if (e != G_IO_ERROR_NONE) {
      fprintf(stderr, "Client write error: %d\n", e);
      clientstate_delete(cs);
      return FALSE;
    } else if (bytes_written > 0) {
      /* Move the memory down some (you wouldn't do this
	 in a performance server because it's slow) */
      g_memmove(cs->buffer,
		&cs->buffer[bytes_written],
		bytes_written);
      cs->n -= bytes_written;
    }
    /* Check if done */
    if (cs->n == 0) {
      cs->out_watch = 0;
      return FALSE;
    }
  }
  return TRUE;

 error:
  clientstate_delete(cs);
  return FALSE;
}
