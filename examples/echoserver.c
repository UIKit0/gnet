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
      g_print ("usage: echoserver [(nothing)|--async|--object] <port> \n");
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
	  g_print ("usage: echoserver [(nothing)|--async|--object] <port> \n");
	  exit(EXIT_FAILURE);
	}
    }

  port = atoi(argv[argc - 1]);

  switch (server_type)
    {
    case NORMAL:
      g_print ("Normal echo server running\n");
      normal_echoserver(port);
      break;
    case ASYNC:
      g_print ("Asynchronous echo server running\n");
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

void
normal_echoserver(gint port)
{
  GTcpSocket* server;
  GTcpSocket* client = NULL;
  gchar buffer[1024];
  guint n;
  GIOChannel* ioclient = NULL;
  GIOError error;

  /* Create the server */
  server = gnet_tcp_socket_server_new(port);
  g_assert (server != NULL);

  while ((client = gnet_tcp_socket_server_accept (server)) != NULL)
    {
      ioclient = gnet_tcp_socket_get_iochannel(client);
      g_assert (ioclient != NULL);


      while ((error = gnet_io_channel_readline(ioclient, buffer, sizeof(buffer), &n)) 
	     == G_IO_ERROR_NONE && (n > 0))
	{
	  error = gnet_io_channel_writen(ioclient, buffer, n, &n);
	  if (error != G_IO_ERROR_NONE) break;
	  fwrite(buffer, n, 1, stdout);
	}

      if (error != G_IO_ERROR_NONE)
	fprintf (stderr, "\nReceived error %d (closing socket).\n", error);

      g_io_channel_unref(ioclient);
      gnet_tcp_socket_delete(client);
    }
}


/* ************************************************************ */

typedef struct
{
  GTcpSocket* socket;
  GIOChannel* iochannel;
  guint out_watch;
  gchar buffer[1024];
  guint n;

} ClientState;

static void clientstate_delete (ClientState* state);

static gboolean async_server_iofunc (GIOChannel* server, 
				     GIOCondition condition, gpointer data);
static gboolean async_client_iofunc (GIOChannel* client, 
				     GIOCondition condition, gpointer data);



static void
clientstate_delete (ClientState* state)
{
  if (state->out_watch)
    g_source_remove (state->out_watch);
  g_io_channel_unref (state->iochannel);
  gnet_tcp_socket_delete (state->socket);
  g_free(state);
}


static void 
async_echoserver(gint port)
{
  GTcpSocket* server;
  GIOChannel* iochannel = NULL;
  GMainLoop* main_loop = NULL;

  /* Create the server */
  server = gnet_tcp_socket_server_new(port);
  g_assert (server != NULL);

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  /* Add a watch for incoming clients */
  iochannel = gnet_tcp_socket_get_iochannel(server);
  g_io_add_watch(iochannel, 
		 G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 async_server_iofunc, server);

  /* Start the main loop */
  g_main_run(main_loop);
}


static gboolean
async_server_iofunc (GIOChannel* iochannel, GIOCondition condition, 
		     gpointer data)
{
  GTcpSocket* server = (GTcpSocket*) data;
  g_assert (server != NULL);

  /*    g_print ("nb_server_iofunc %d\n", condition); */

  if (condition & G_IO_IN)
    {
      GTcpSocket* client = NULL;
      GIOChannel* client_iochannel = NULL;
      ClientState* client_state = NULL;

      client = gnet_tcp_socket_server_accept_nonblock (server);
      if (!client) return TRUE;

      client_iochannel = gnet_tcp_socket_get_iochannel(client);
      g_assert (client_iochannel != NULL);

      client_state = g_new0(ClientState, 1);
      client_state->socket = client;
      client_state->iochannel = client_iochannel;

      g_io_add_watch (client_iochannel, 
		      G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		      async_client_iofunc, client_state);
    }
  else
    {
      fprintf (stderr, "Server error %d\n", condition);
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

  On some systems GLib has problems if you use more than one watch on
  a file descriptor.  The problem is GLib assumes descriptors can
  appear twice in the array passed to poll(), which is true on some
  systems but not others (Linux).

  See GLib bug 11059 on http://bugs.gnome.org.
  Look at Jungle Monkey for an example of how to work around the bug.

 */
gboolean
async_client_iofunc (GIOChannel* iochannel, GIOCondition condition, 
		     gpointer data)
{
  ClientState* client_state = (ClientState*) data;

  g_assert (client_state != NULL);

/*    fprintf (stderr, "async_client_iofunc %d (in = %d, out = %d, err = %d, hup = %d, pri = %d\n", condition, G_IO_IN, G_IO_OUT, G_IO_ERR, G_IO_HUP, G_IO_PRI); */

  /* Check for socket error */
  if (condition & G_IO_ERR)
    {
      fprintf (stderr, "Client socket error\n");
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
	  fprintf (stderr, "Client read error: %d\n", error);
	  goto error;
	}

      /* Check if we read 0 bytes, which means the connection is
         closed */
      else if (bytes_read == 0)
	{
	  goto error;
	}

      /* Otherwise, we read something */
      else
	{
	  g_assert (bytes_read > 0);

	  /* If there isn't an out_watch, add one. */
	  if (client_state->out_watch == 0)
	    {
	      client_state->out_watch = 
		g_io_add_watch(iochannel, G_IO_OUT, 
			       async_client_iofunc, client_state);
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
	 fprintf (stderr, "Client write error: %d\n", error);
	 clientstate_delete (client_state);
	 return FALSE;
       }

     else if (bytes_written > 0)
       {
	 /* Move the memory down some (you wouldn't want to do this
	    in a performance server because it's slow!) */
	 memmove(client_state->buffer, 
		 &client_state->buffer[bytes_written],
		 bytes_written);

	 client_state->n -= bytes_written;
       }

     /* Check if done */
     if (client_state->n == 0)
       {
	 client_state->out_watch = 0;
	 return FALSE;
       }
   }

  return TRUE;

 error:
  clientstate_delete (client_state);
  return FALSE;
}



/* ************************************************************ */

static void ob_server_func (GServer* server, GServerStatus status, 
			    GConn* conn, gpointer user_data);
static gboolean ob_client_func (GConn* conn, GConnStatus status, 
				gchar* buffer, gint length, 
				gpointer user_data);


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
  g_assert (server);

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
	gnet_conn_readline (conn, NULL, 1024, 60000);
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
	gchar* buffer_copy = g_memdup(buffer, length);
	buffer_copy[length] = '\n';

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
	gnet_conn_delete (conn, TRUE /* delete write buffers */);
	break;
      }

    default:
      g_assert_not_reached ();
    }

  return TRUE;	/* TRUE means read more if status was read, otherwise
                   its ignored */
}
