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

typedef enum { NORMAL, ASYNC, OBJECT} ClientType;


static void usage (int status);

static void normal_echoclient (gchar* hostname, gint port);
static void async_echoclient (gchar* hostname, gint port);
static void object_echoclient (gchar* hostname, gint port);


int
main(int argc, char** argv)
{
  ClientType client_type = NORMAL;


  if (argc != 3 && argc != 4)
    usage(EXIT_FAILURE);

  if (argc == 4)
    {
      if (strcmp(argv[1], "--async") == 0)
	client_type = ASYNC;
      else if (strcmp(argv[1], "--object") == 0)
	client_type = OBJECT;
      else
	usage(EXIT_FAILURE);
    }

  switch (client_type)
    {
    case NORMAL:
      g_print ("Normal echo client running\n");
      normal_echoclient(argv[argc-2], atoi(argv[argc-1]));
      break;
    case ASYNC:
      g_print ("Asynchronous echo client running (UNFINISHED)\n");
      async_echoclient(argv[argc-2], atoi(argv[argc-1]));
      break;
    case OBJECT:
      g_print ("Object echo client running\n");
      object_echoclient(argv[argc-2], atoi(argv[argc-1]));
      break;
    default:
      g_assert_not_reached();
    }

  return 0;
}


static void
usage (int status)
{
  g_print ("usage: echoclient [(nothing)|--async|--object] <server> <port>\n");
  exit(status);
}



/* ************************************************************ */


static void
normal_echoclient(gchar* hostname, gint port)
{
  GInetAddr* addr = NULL;
  GTcpSocket* socket = NULL;
  GIOChannel* iochannel = NULL;
  gchar buffer[1024];
  guint n;
  GIOError error = G_IO_ERROR_NONE;

  /* Create the address */
  addr = gnet_inetaddr_new(hostname, port);
  g_assert (addr != NULL);

  /* Create the socket */
  socket = gnet_tcp_socket_new(addr);
  g_assert (socket != NULL);

  /* Get the IOChannel */
  iochannel = gnet_tcp_socket_get_iochannel(socket);
  g_assert (iochannel != NULL);


  while (fgets(buffer, sizeof(buffer), stdin) != 0)
    {
      n = strlen(buffer);
      error = gnet_io_channel_writen(iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      error = gnet_io_channel_readn(iochannel, buffer, n, &n);
      if (error != G_IO_ERROR_NONE) break;

      fwrite(buffer, n, 1, stdout);
    }

  if (error != G_IO_ERROR_NONE) 
    g_print ("IO error (%d)\n", error);

}



/* ************************************************************ */

static void async_client_connfunc (GTcpSocket* socket, GInetAddr* ia,
				   GTcpSocketConnectAsyncStatus status, 
				   gpointer data);

static gboolean async_client_in_iofunc (GIOChannel* iochannel, 
					GIOCondition condition, 
					gpointer data);


static void
async_echoclient (gchar* hostname, gint port)
{
  GMainLoop* main_loop = NULL;

  g_print ("connect to %s %d\n", hostname, port);

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  /* Connect asynchronously */
  gnet_tcp_socket_connect_async (hostname, port, async_client_connfunc, NULL);

  /* Start the main loop */
  g_main_run(main_loop);
}


static void 
async_client_connfunc (GTcpSocket* socket, GInetAddr* ia,
		       GTcpSocketConnectAsyncStatus status, gpointer data)
{
  GIOChannel* in;

  if (status != GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      fprintf (stderr, "Could not connect (status = %d)\n", status);
      exit (EXIT_FAILURE);
    }

  /* Read from stdin */
  in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch(in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 async_client_in_iofunc, NULL);
}


static gboolean
async_client_in_iofunc (GIOChannel* iochannel, GIOCondition condition, 
		     gpointer data)
{
  /* Check for socket error */
  if (condition & G_IO_ERR)
    {
      fprintf (stderr, "Socket error\n");
      goto error;
    }


  /* Check for data to be read (or if the stdin was closed (?)) */
  if (condition & G_IO_IN)
    {
      GIOError error;
      gchar buffer[1024];
      guint bytes_read;

      /* Read the data into our buffer */
      error = g_io_channel_read(iochannel, buffer, 
				sizeof(buffer), &bytes_read);

      /* Check for stdin error */
      if (error != G_IO_ERROR_NONE)
	{
	  fprintf (stderr, "Read error (%d)\n", error);
	  goto error;
	}

      /* Check for EOF */
      else if (bytes_read == 0)
	{
	  /* Really we should free all our resources here, but
             whatever */
	  goto error;
	}

      /* Otherwise, we read something */
      else
	{
	  fwrite (buffer, bytes_read, 1, stdout);

	  /* If there weren't any bytes to write before, then there wasn't a
	     watch on write, so add one. */
/*  	  if (client_state->n == 0) */
/*  	    { */
/*  	      client_state->out_watch =  */
/*  		g_io_add_watch(iochannel, G_IO_OUT,  */
/*  			       nb_client_iofunc, client_state); */
/*  	    } */

/*  	  client_state->n += bytes_read; */
	}
    }

  return TRUE;

 error:
  exit (EXIT_FAILURE);
  return FALSE;
}


#if 0

  GIOChannel* server;


  /* Read stuff from the server */
  server = gnet_tcp_socket_get_iochannel(socket);
  g_io_add_watch(iochannel, 
		 G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 async_server_iofunc, server);

#endif



/* ************************************************************ */

static gboolean ob_in_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			      gpointer data);

static gboolean ob_conn_func (GConn* conn, GConnStatus status, 
			      gchar* buffer, gint length, gpointer user_data);



static void
object_echoclient (gchar* hostname, gint port)
{
  GMainLoop* main_loop = NULL;
  GConn* conn;
  GIOChannel* in;

  g_print ("connect to %s %d\n", hostname, port);

  /* Create the main loop */
  main_loop = g_main_new(FALSE);
  
  /* Create connection object */
  conn = gnet_conn_new (hostname, port, ob_conn_func, NULL);
  g_assert (conn);

  /* Connect */
  gnet_conn_connect (conn, 0);

  /* Read from stdin */
  in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch(in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 ob_in_iofunc, conn);

  /* Start the main loop */
  g_main_run (main_loop);
}


static gboolean
ob_in_iofunc (GIOChannel* iochannel, GIOCondition condition, 
	      gpointer data)
{
  GConn* conn = (GConn*) data;

  /* Check for socket error */
  if (condition & G_IO_ERR)
    {
      fprintf (stderr, "Socket error\n");
      goto error;
    }


  /* Check for data to be read (or if the stdin was closed (?)) */
  if (condition & G_IO_IN)
    {
      GIOError error;
      gchar buffer[1024];
      guint bytes_read;

      /* Read the data into our buffer */
      error = g_io_channel_read(iochannel, buffer, 
				sizeof(buffer), &bytes_read);

      /* Check for stdin error */
      if (error != G_IO_ERROR_NONE)
	{
	  fprintf (stderr, "Read error (%d)\n", error);
	  goto error;
	}

      /* Check for EOF */
      else if (bytes_read == 0)
	{
	  /* Really we should free all our resources here, but
             whatever */
	  goto error;
	}

      /* Otherwise, we read something */
      else
	{
	  gnet_conn_write (conn, g_memdup(buffer, bytes_read), 
			   bytes_read, 0);
	}
    }

  return TRUE;

 error:
  exit (EXIT_FAILURE);
  return FALSE;
}



static gboolean
ob_conn_func (GConn* conn, GConnStatus status, 
	      gchar* buffer, gint length, gpointer user_data)
{
  switch (status)
    {

    case GNET_CONN_STATUS_CONNECT:
      {
	fprintf (stderr, "connect\n");
	gnet_conn_readline (conn, NULL, 1024, 60000);
	break;
      }

    case GNET_CONN_STATUS_READ:
      {
	fwrite (buffer, length, 1, stdout);
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
