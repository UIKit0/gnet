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

#define GNET_EXPERIMENTAL 1
#include <gnet.h>


static void usage (int status);
static void normal_echoclient (gchar* hostname, gint port);
static void async_echoclient (gchar* hostname, gint port);


int
main(int argc, char** argv)
{
  char*      socks_hostname = NULL;
  int        socks_port = GNET_SOCKS_PORT;
  GInetAddr* socks_ia;

  if (argc < 3 || argc > 5)
    usage(EXIT_FAILURE);
  if (argc >= 4)
    socks_hostname = argv[3];
  if (argc >= 5)
    socks_port = atoi(argv[4]);

  if (socks_hostname)
    {
      socks_ia = gnet_inetaddr_new (socks_hostname, socks_port);
      if (!socks_ia)
	{
	  fprintf (stderr, "Bad SOCKS server address: %s:%d\n", 
		   socks_hostname, socks_port);
	  exit (EXIT_FAILURE);
	}
      gnet_socks_set_server (socks_ia);
      gnet_inetaddr_delete (socks_ia);
    }

  gnet_socks_set_enabled (TRUE);

  socks_ia = gnet_socks_get_server();
  if (socks_ia)
    g_print ("SOCKS server is %s:%d\n",
	     gnet_inetaddr_get_canonical_name(socks_ia),
	     gnet_inetaddr_get_port(socks_ia));
  else
    g_print ("No SOCKS server\n");

/*    normal_echoclient(argv[1], atoi(argv[2])); */
  async_echoclient (argv[1], atoi(argv[2]));

  return 0;
}


static void
usage (int status)
{
  g_print ("usage: socksclient <server> <port> [SOCKS server] [SOCKS port]\n");
  exit(status);
}



/* ************************************************************ */


static void
normal_echoclient (gchar* hostname, gint port)
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

static gboolean async_client_sin_iofunc (GIOChannel* iochannel, 
					 GIOCondition condition, 
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
  GIOChannel* sin;
  GIOChannel* in;

  if (status != GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      fprintf (stderr, "Could not connect (status = %d)\n", status);
      exit (EXIT_FAILURE);
    }

  /* Read from socket */
  sin = gnet_tcp_socket_get_iochannel (socket);
  g_io_add_watch (sin, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		  async_client_sin_iofunc, NULL);

  /* Read from stdin */
  in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch(in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 async_client_in_iofunc, sin);
}


static gboolean
async_client_sin_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			 gpointer data)
{
  /* Check for socket error */
  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
      fprintf (stderr, "Socket error\n");
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

