/* Echo server
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

#include <signal.h>


typedef enum { NORMAL, ASYNC, OBJECT} ServerType;

static void normal_echoserver (gint port);
static void async_echoserver (gint port);
static void object_echoserver (gint port);


int
main(int argc, char** argv)
{
  int port = 0;
  ServerType server_type = NORMAL;


  if (argc !=  2 && argc != 3)
    {
      fprintf (stderr, "usage: echoserver [(nothing)|--async|--object] <port> \n");
      exit(EXIT_FAILURE);
    }

  if (argc == 3)
    {
      if (strcmp(argv[1], "--async") == 0)
	server_type = ASYNC;
      else if (strcmp(argv[1], "--object") == 0)
	server_type = OBJECT;
      else
	{
	  fprintf (stderr, "usage: echoserver [(nothing)|--async|--object] <port> \n");
	  exit(EXIT_FAILURE);
	}
    }

  port = atoi(argv[argc - 1]);

  switch (server_type)
    {
    case NORMAL:
      normal_echoserver(port);
      break;
    case ASYNC:
      async_echoserver(port);
      break;
    case OBJECT:
      g_print ("Object echo server running\n");
      object_echoserver(port);
      break;
    default:
      g_assert_not_reached();
    }

  return 0;

}


/* ************************************************************ */

static void normal_sig_int (int signum);

static GTcpSocket* normal_server = NULL;

static void
normal_echoserver (gint p)
{
  GTcpSocket* server;
  GInetAddr*  addr;
  gchar*      name;
  gint	      port;
  GTcpSocket* client = NULL;
  gchar       buffer[1024];
  guint       n;
  GIOChannel* ioclient = NULL;
  GIOError    error;

  /* Create the server */
  server = gnet_tcp_socket_server_new(p);
  if (server == NULL)
    {
      fprintf (stderr, "Could not create server on port %d\n", p);
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
      ioclient = gnet_tcp_socket_get_iochannel(client);
      g_assert (ioclient);

      /* Print the address */
      addr = gnet_tcp_socket_get_inetaddr(client);
      g_assert (addr);
      name = gnet_inetaddr_get_canonical_name (addr);
      g_assert (name);
      port = gnet_inetaddr_get_port (addr);
      g_print ("Accepted connection from %s:%d\n", name, port);

      while ((error = gnet_io_channel_readline(ioclient, buffer, sizeof(buffer), &n)) 
	     == G_IO_ERROR_NONE && (n > 0))
	{
	  error = gnet_io_channel_writen(ioclient, buffer, n, &n);
	  if (error != G_IO_ERROR_NONE) break;
	  fwrite(buffer, n, 1, stdout);
	}

      if (error != G_IO_ERROR_NONE)
	fprintf (stderr, "\nReceived error %d (closing socket).\n", error);

      g_io_channel_unref (ioclient);
      gnet_tcp_socket_delete (client);

      g_print ("Connection from %s:%d closed\n", name, port);

      gnet_inetaddr_delete (addr);
      g_free (name);
    }
}


static void 
normal_sig_int (int signum)
{
  gnet_tcp_socket_delete (normal_server);
  exit (EXIT_FAILURE);
}


/* ************************************************************ */

typedef struct
{
  GTcpSocket* socket;
  gchar* name;
  gint port;
  GIOChannel* iochannel;
  guint watch_flags;
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


static void
clientstate_delete (ClientState* state)
{
  g_source_remove (state->watch);
  g_io_channel_unref (state->iochannel);
  gnet_tcp_socket_delete (state->socket);
  g_free (state->name);
  g_free (state);
}


static void 
async_echoserver(gint p)
{
  GTcpSocket* server;
  GInetAddr*  addr;
  gchar*      name;
  gint	      port;
  GMainLoop*  main_loop = NULL;

  /* Create the server */
  server = gnet_tcp_socket_server_new(p);
  if (server == NULL)
    {
      fprintf (stderr, "Could not create server on port %d\n", p);
      exit (EXIT_FAILURE);
    }

  async_server = server;
  signal (SIGINT, async_sig_int);

  /* Print the address */
  addr = gnet_tcp_socket_get_inetaddr(server);
  g_assert (addr);
  name = gnet_inetaddr_get_canonical_name (addr);
  g_assert (name);
  port = gnet_inetaddr_get_port (addr);
  g_print ("Async echoserver running on %s:%d\n", name, port);
  gnet_inetaddr_delete (addr);
  g_free (name);

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  /* Wait asyncy for incoming clients */
  gnet_tcp_socket_server_accept_async (server, async_accept, NULL);

  /* Start the main loop */
  g_main_run(main_loop);
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
      addr = gnet_tcp_socket_get_inetaddr(client);
      g_assert (addr);
      name = gnet_inetaddr_get_canonical_name (addr);
      g_assert (name);
      port = gnet_inetaddr_get_port (addr);
      g_print ("Accepted connection from %s:%d\n", name, port);
      gnet_inetaddr_delete (addr);

      client_iochannel = gnet_tcp_socket_get_iochannel (client);
      g_assert (client_iochannel != NULL);

      client_state = g_new0(ClientState, 1);
      client_state->socket = client;
      client_state->name = name;
      client_state->port = port;
      client_state->iochannel = client_iochannel;
      client_state->watch_flags = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
      client_state->watch = 
	g_io_add_watch (client_iochannel, client_state->watch_flags,
			async_client_iofunc, client_state);
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
	g_io_channel_read(iochannel, 
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
	  g_assert (bytes_read > 0);

	  /* If there isn't an OUT watch, add one. */
	  if (!(client_state->watch_flags & G_IO_OUT))
	    {
	      g_source_remove (client_state->watch);
	      client_state->watch_flags |= G_IO_OUT;
	      client_state->watch = 
		g_io_add_watch(iochannel, client_state->watch_flags,
			       async_client_iofunc, client_state);
	      rv = FALSE;
	    }

	  client_state->n += bytes_read;
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
	 /* Move the memory down some (you wouldn't want to do this
	    in a performance server because it's slow!) */
	 client_state->n -= bytes_written;
	 g_memmove(client_state->buffer, 
		   &client_state->buffer[bytes_written],
		   client_state->n);
       }

     /* Remove OUT watch if done */
     if (client_state->n == 0)
       {
	 client_state->watch_flags &= ~G_IO_OUT;
	 g_source_remove (client_state->watch);
	 client_state->watch = 
	   g_io_add_watch(iochannel, client_state->watch_flags,
			  async_client_iofunc, client_state);
	 rv = FALSE;
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



/* ************************************************************ */

static void ob_server_func (GServer* server, GServerStatus status, 
			    GConn* conn, gpointer user_data);
static gboolean ob_client_func (GConn* conn, GConnStatus status, 
				gchar* buffer, gint length, 
				gpointer user_data);
static void ob_sig_int (int signum);

static GServer* ob_server = NULL;


static void
object_echoserver (gint port)
{
  GMainLoop* main_loop;
  GInetAddr* addr;
  GServer* server;

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  /* Create the interface */
  addr = gnet_inetaddr_new_any ();
  gnet_inetaddr_set_port (addr, port);

  /* Create the server */
  server = gnet_server_new (addr, TRUE, ob_server_func, NULL);
  if (!server)
    {
      fprintf (stderr, "Error: Could not start server\n");
      exit (EXIT_FAILURE);
    }

  ob_server = server;
  signal (SIGINT, ob_sig_int);

  /* Start the main loop */
  g_main_run(main_loop);
}


static void
ob_server_func (GServer* server, GServerStatus status, 
		struct _GConn* conn, gpointer user_data)
{
  switch (status)
    {
    case GNET_SERVER_STATUS_CONNECT:
      {
	conn->func = ob_client_func;
	gnet_conn_readline (conn, NULL, 1024, 30000);
	break;
      }

    case GNET_SERVER_STATUS_ERROR:
      {
	gnet_server_delete (server);
	exit (EXIT_FAILURE);
	break;
      }
    }
}


static gboolean
ob_client_func (GConn* conn, GConnStatus status, 
		gchar* buffer, gint length, gpointer user_data)
{
  switch (status)
    {
    case GNET_CONN_STATUS_READ:
      {
	gchar* buffer_copy;

	buffer_copy = g_memdup (buffer, length);

	gnet_conn_write (conn, buffer_copy, length, 0);
	break;
      }

    case GNET_CONN_STATUS_WRITE:
      {
	g_free (buffer);
	break;
      }

    case GNET_CONN_STATUS_CLOSE:
    case GNET_CONN_STATUS_TIMEOUT:
    case GNET_CONN_STATUS_ERROR:
      {
	gnet_conn_delete (conn, TRUE);
	break;
      }

    default:
      g_assert_not_reached ();
    }

  return TRUE;	/* TRUE means read more if status was read, otherwise
                   its ignored */
}



static void 
ob_sig_int (int signum)
{
  gnet_server_delete (ob_server);
  exit (EXIT_FAILURE);
}
