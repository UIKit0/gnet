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


typedef struct
{
  GTcpSocket* socket;
  gchar* name;
  guint watch_flags;
  gint port;
  guint watch;
  gchar buffer[1024];
  guint n;

} ClientState;


static void clientstate_delete (ClientState* state);

static void async_accept (GTcpSocket* server_socket, 
			  GTcpSocket* client_socket,
			  gpointer data);
static gboolean async_client_iofunc (GIOChannel* client, 
				     GIOCondition condition, gpointer data);

static void async_sig_int (int signum);

static GTcpSocket* async_server = NULL;


int
main(int argc, char** argv)
{
  GTcpSocket* server;
  GInetAddr*  addr;
  gchar*      name;
  gint	      port;
  GMainLoop*  main_loop;

  gnet_init ();

  if (argc != 2)
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

  async_server = server;
  signal (SIGINT, async_sig_int);

  /* Print the address */
  addr = gnet_tcp_socket_get_local_inetaddr(server);
  g_assert (addr);
  name = gnet_inetaddr_get_canonical_name (addr);
  g_assert (name);
  port = gnet_inetaddr_get_port (addr);
  g_print ("Async echoserver running on %s:%d\n", name, port);
  gnet_inetaddr_delete (addr);
  g_free (name);

  /* Create the main loop */
  main_loop = g_main_new (FALSE);

  /* Wait asyncy for incoming clients */
  gnet_tcp_socket_server_accept_async (server, async_accept, NULL);

  /* Start the main loop */
  g_main_run (main_loop);

  exit (EXIT_SUCCESS);
  return 0;
}



static void
clientstate_delete (ClientState* state)
{
  g_source_remove (state->watch);
  gnet_tcp_socket_delete (state->socket);
  g_free (state->name);
  g_free (state);
}

static void
async_accept (GTcpSocket* server, GTcpSocket* client, gpointer data)
{
  if (client)
    {
      GInetAddr*  addr;
      gchar*      name;
      gint	  port;
      GIOChannel* client_iochannel;
      ClientState* client_state;

      /* Print the address */
      addr = gnet_tcp_socket_get_local_inetaddr(client);
      g_assert (addr);
      name = gnet_inetaddr_get_canonical_name (addr);
      g_assert (name);
      port = gnet_inetaddr_get_port (addr);
      gnet_inetaddr_delete (addr);

      client_iochannel = gnet_tcp_socket_get_io_channel (client);
      g_assert (client_iochannel != NULL);

      client_state = g_new0(ClientState, 1);
      client_state->socket = client;
      client_state->name = name;
      client_state->port = port;
      client_state->watch_flags = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
      client_state->watch = 
	g_io_add_watch (client_iochannel, client_state->watch_flags,
			async_client_iofunc, client_state);

      g_print ("Accepted connection from %s:%d\n", name, port);
    }
  else
    {
      fprintf (stderr, "Server error\n");
      exit (EXIT_FAILURE);
    }
}


/*

  Client IO callback.  Called for errors, input, or output.  When
  there is input, we reset the watch with the output flag (if we
  haven't already).  When there is no more data, the watch is reset
  without the output flag.
  
 */
gboolean
async_client_iofunc (GIOChannel* iochannel, GIOCondition condition, 
		     gpointer data)
{
  ClientState* client_state = (ClientState*) data;
  gboolean rv = TRUE;

  g_assert (client_state != NULL);
  

  /* Check for socket error */
  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
      fprintf (stderr, "Client socket error (%s:%d)\n", 
	       client_state->name, client_state->port);
      goto error;
    }

  /* Check for data to be read (or if the socket was closed) */
  if (condition & G_IO_IN)
    {
      GIOError error;
      guint bytes_read;

      /* Read the data into our buffer */
      error = 
	g_io_channel_read (iochannel, 
			   &client_state->buffer[client_state->n], 
			   sizeof(client_state->buffer) - client_state->n, 
			   &bytes_read);


      /* Check for socket error */
      if (error != G_IO_ERROR_NONE)
	{
	  fprintf (stderr, "Client read error (%s:%d): %d\n", 
		   client_state->name, client_state->port, error);
	  goto error;
	}

      /* Check if we read 0 bytes, which means the connection is
         closed */
      else if (bytes_read == 0)
	{
	  g_print ("Connection from %s:%d closed\n", 
		   client_state->name, client_state->port);
	  goto error;
	}

      /* Otherwise, we read something */
      else
	{
	  guint old_watch_flags;

	  g_assert (bytes_read > 0);

	  old_watch_flags = client_state->watch_flags;
	  client_state->n += bytes_read;

	  /* Add an add watch */
	  client_state->watch_flags |= G_IO_OUT;

	  /* Remove the IN watch if the buffer is full */
	  if (client_state->n == sizeof(client_state->buffer))
	    client_state->watch_flags &= ~G_IO_IN;

	  /* Update watch flags if they changed */
	  if (old_watch_flags != client_state->watch_flags)
	    {
	      g_source_remove (client_state->watch);
	      client_state->watch = 
		g_io_add_watch(iochannel, client_state->watch_flags,
			       async_client_iofunc, client_state);
	      rv = FALSE;
	    }
	}

    }

  if (condition & G_IO_OUT)
    {
      GIOError error;
      guint bytes_written;

      /* Write the data out */
      error = g_io_channel_write(iochannel, client_state->buffer, 
				 client_state->n, &bytes_written);

      if (error != G_IO_ERROR_NONE) 
	{
	  fprintf (stderr, "Client write error (%s:%d): %d\n", 
		   client_state->name, client_state->port, error);
	  goto error;
	}

      else if (bytes_written > 0)
	{
	  guint old_watch_flags;

	  /* Move the memory down some (you wouldn't want to do this
	     in a performance server because it's slow!) */
	  client_state->n -= bytes_written;
	  g_memmove(client_state->buffer, 
		    &client_state->buffer[bytes_written],
		    client_state->n);

	  old_watch_flags = client_state->watch_flags;

	  /* Remove OUT watch if done */
	  if (client_state->n == 0)
	    client_state->watch_flags &= ~G_IO_OUT;

	  /* Add IN watch */
	  client_state->watch_flags |= G_IO_IN;

	  /* Update watch flags if they changed */
	  if (old_watch_flags != client_state->watch_flags)
	    {
	      g_source_remove (client_state->watch);
	      client_state->watch = 
		g_io_add_watch(iochannel, client_state->watch_flags,
			       async_client_iofunc, client_state);
	      rv = FALSE;
	    }
	}
    }

  return rv;

 error:
  clientstate_delete (client_state);
  return FALSE;
}


static void 
async_sig_int (int signum)
{
  gnet_tcp_socket_delete (async_server);
  exit (EXIT_FAILURE);
}
