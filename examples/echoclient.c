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
  gchar* hostname;
  gint port;
  int i;

  gnet_init ();

  if (argc < 3 || argc > 6)
    usage(EXIT_FAILURE);

  i = 1;
  if (strcmp(argv[1], "--normal") == 0)
    {
      client_type = NORMAL;
      i++;
    }
  else if (strcmp(argv[1], "--async") == 0)
    {
      client_type = ASYNC;
      i++;
    }
  else if (strcmp(argv[1], "--object") == 0)
    {
      client_type = OBJECT;
      i++;
    }

  hostname = argv[i++];
  port = atoi(argv[i++]);

  if (i < argc)
    {
      char*      socks_hostname = NULL;
      int        socks_port = GNET_SOCKS_PORT;
      GInetAddr* socks_ia;

      socks_hostname = argv[i++];
      if (i < argc)
	socks_port = atoi(argv[i++]);

      socks_ia = gnet_inetaddr_new (socks_hostname, socks_port);
      if (!socks_ia)
	{
	  fprintf (stderr, "Bad SOCKS server address: %s:%d\n", 
		   socks_hostname, socks_port);
	  exit (EXIT_FAILURE);
	}
      gnet_socks_set_server (socks_ia);
      gnet_inetaddr_delete (socks_ia);

      gnet_socks_set_enabled (TRUE);
    }

  if (gnet_socks_get_enabled())
    {
      GInetAddr* socks_ia;

      socks_ia = gnet_socks_get_server();
      if (socks_ia)
	{
	  gchar* cname;

	  cname = gnet_inetaddr_get_canonical_name(socks_ia);

	  g_print ("SOCKS server is %s:%d\n", cname,
		   gnet_inetaddr_get_port(socks_ia));

	  g_free (cname);
	  gnet_inetaddr_delete (socks_ia);
	}
      else
	g_print ("SOCKS enabled, but no SOCKS server\n");
    }


  switch (client_type)
    {
    case NORMAL:
      g_print ("Normal echo client running\n");
      normal_echoclient(hostname, port);
      break;
    case ASYNC:
      g_print ("Asynchronous echo client running\n");
      async_echoclient(hostname, port);
      break;
    case OBJECT:
      g_print ("Object echo client running\n");
      object_echoclient(hostname, port);
      break;
    default:
      g_assert_not_reached();
    }

  return 0;
}


static void
usage (int status)
{
  g_print ("usage: echoclient [--normal|--async|--object] <server> <port> [SOCKS server] [SOCKS port]\n");
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
  if (!addr)
    {
      fprintf (stderr, "Error: Name lookup for %s failed\n", hostname);
      exit (EXIT_FAILURE);
    }

  /* Create the socket */
  socket = gnet_tcp_socket_new(addr);
  if (!socket)
    {
      fprintf (stderr, "Error: Could not connect to %s:%d\n", hostname, port);
      exit (EXIT_FAILURE);
    }

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

  gnet_inetaddr_delete (addr);
  g_io_channel_unref (iochannel);
  gnet_tcp_socket_delete (socket);
}



/* ************************************************************ */

static void async_client_connfunc (GTcpSocket* socket, GInetAddr* ia,
				   GTcpSocketConnectAsyncStatus status, 
				   gpointer data);

static gboolean async_client_sin_iofunc (GIOChannel* iochannel, 
					 GIOCondition condition, 
					 gpointer data);

static gboolean async_client_in_iofunc (GIOChannel* iochannel, 
					GIOCondition condition, 
					gpointer data);

static GIOChannel* async_in;
static GIOChannel* async_sin;
static GTcpSocket* async_socket;


static void
async_echoclient (gchar* hostname, gint port)
{
  GMainLoop* main_loop = NULL;

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  g_print ("Connecting to %s:%d... ", hostname, port);

  /* Connect asynchronously */
  gnet_tcp_socket_connect_async (hostname, port, async_client_connfunc, NULL);

  /* Start the main loop */
  g_main_run(main_loop);
}


static void 
async_client_connfunc (GTcpSocket* socket, GInetAddr* ia,
		       GTcpSocketConnectAsyncStatus status, gpointer data)
{
  if (status != GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      fprintf (stderr, "Error: Could not connect (status = %d)\n", status);
      exit (EXIT_FAILURE);
    }

  fprintf (stderr, "connected\n");

  async_socket = socket;
  gnet_inetaddr_delete (ia);

  /* Read from socket */
  async_sin = gnet_tcp_socket_get_iochannel (socket);
  g_io_add_watch (async_sin, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		  async_client_sin_iofunc, NULL);
  /* Read from stdin */
  async_in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch(async_in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 async_client_in_iofunc, async_sin);

}


static gboolean
async_client_sin_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			 gpointer data)
{
  /* Check for socket error */
  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
      fprintf (stderr, "Error: Socket error\n");
      goto error;
    }

  /* Check for data to be read */
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
	  fprintf (stderr, "Error: Read error (%d)\n", error);
	  goto error;
	}

      /* Check for EOF */
      else if (bytes_read == 0)
	{
	  g_io_channel_unref (async_in);
	  g_io_channel_unref (async_sin);
	  gnet_tcp_socket_delete (async_socket);
	}

      /* Otherwise, print */
      else
	{
	  fwrite (buffer, 1, bytes_read, stdout);
	}
    }

  return TRUE;

 error:
  exit (EXIT_FAILURE);
  return FALSE;
}


static gboolean
async_client_in_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			gpointer data)
{
  /* Check for socket error */
  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
      fprintf (stderr, "Error: Socket error\n");
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
	  fprintf (stderr, "Error: Read error (%d)\n", error);
	  goto error;
	}

      /* Check for EOF */
      else if (bytes_read == 0)
	{
	  /* Really we should free all our resources here, but
             whatever */
	  goto error;
	}

      /* Otherwise, write to the socket */
      else
	{
	  GIOChannel* sin;
	  GIOError error;
	  guint bytes_written;

	  sin = (GIOChannel*) data;
	  error = gnet_io_channel_writen (sin, buffer, bytes_read, 
					  &bytes_written);
	  g_assert (error == G_IO_ERROR_NONE);
	  g_assert (bytes_written == bytes_read);
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

  g_print ("Connecting to %s:%d... ", hostname, port);

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
      fprintf (stderr, "Error: Socket error\n");
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
	g_print ("connected\n");
	gnet_conn_readline (conn, NULL, 1024, 60000);
	break;
      }

    case GNET_CONN_STATUS_READ:
      {
	fwrite (buffer, length, 1, stdout);
	return TRUE;
	break;
      }

    case GNET_CONN_STATUS_WRITE:
      {
	g_free (buffer);
	break;
      }

    case GNET_CONN_STATUS_CLOSE:
      {
	gnet_conn_delete (conn, TRUE /* delete write buffers */);
	exit (EXIT_SUCCESS);
      }

    case GNET_CONN_STATUS_TIMEOUT:
      {
	gnet_conn_delete (conn, TRUE /* delete write buffers */);
	fprintf (stderr, "Connection timeout\n");
	exit (EXIT_FAILURE);
      }

    case GNET_CONN_STATUS_ERROR:
      {
	gnet_conn_delete (conn, TRUE /* delete write buffers */);
	fprintf (stderr, "Connection failure\n");
	exit (EXIT_FAILURE);
	break;
      }

    default:
      g_assert_not_reached ();
    }

  return FALSE;	/* rv only matters for a read */
}
