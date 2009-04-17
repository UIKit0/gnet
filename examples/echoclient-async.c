/* Echo client (async TCP)
 * Copyright (C) 2000-2003  David Helder
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
#include <errno.h>
#include <glib.h>
#include <gnet.h>


static void async_client_connfunc (GTcpSocket* socket,
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

static int lines_pending = 0;
static gboolean read_eof = FALSE;


int
main(int argc, char** argv)
{
  gchar* hostname;
  gint port;
  GMainLoop* main_loop;

  gnet_init ();

  /* Parse args */
  if (argc != 3)
    {
      g_print ("usage: %s <server> <port>\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  hostname = argv[1];
  port = atoi(argv[2]);

  /* Create the main loop */
  main_loop = g_main_new (FALSE);

  /* Connect asynchronously */
  gnet_tcp_socket_connect_async (hostname, port, async_client_connfunc, NULL);

  /* Start the main loop */
  g_main_run (main_loop);

  exit (EXIT_SUCCESS);
  return 0;
}


static void 
async_client_connfunc (GTcpSocket* socket, 
		       GTcpSocketConnectAsyncStatus status, gpointer data)
{
  if (status != GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      fprintf (stderr, "Error: Could not connect (status = %d)\n", status);
      exit (EXIT_FAILURE);
    }

  async_socket = socket;

  /* Read from socket */
  async_sin = gnet_tcp_socket_get_io_channel (socket);
  g_io_add_watch (async_sin, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		  async_client_sin_iofunc, NULL);

  /* Read from stdin */
  async_in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch (async_in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		  async_client_in_iofunc, async_sin);

}

#include <errno.h> /* FIX */

static gboolean
async_client_sin_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			 gpointer data)
{
/*    fprintf (stderr, "async_client_sin_iofunc %d\n", condition); */

/*    IN/HUP/ERR */

  /* Check for socket error */
  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
      fprintf (stderr, "Error: Socket error (%d, %d)\n", condition, errno);
      goto error;
    }

  /* Check for data to be read */
  if (condition & G_IO_IN)
    {
      GIOError error;
      gchar buffer[1024];
      gsize bytes_read;

      /* Read the data into our buffer */
      error = g_io_channel_read (iochannel, buffer, 
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
	  gnet_tcp_socket_delete (async_socket);
	  exit (EXIT_SUCCESS);
	  return FALSE;
	}

      /* Otherwise, print */
      else
	{
	  gint i;

	  if (fwrite (buffer, 1, bytes_read, stdout) != 1) {
            fprintf (stderr, "Error: fwrite to stdout failed: %s\n", g_strerror (errno));
          }

	  for (i = 0; i < bytes_read; ++i)
	    if (buffer[i] == '\n')
	      lines_pending--;

	  if (lines_pending == 0 && read_eof)
	    exit (EXIT_SUCCESS);
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
/*    fprintf (stderr, "async_client_in_iofunc %d\n", condition); */

  /* Check for input error (we check HUP last) */
  if (condition & (G_IO_ERR | G_IO_NVAL))
    {
      fprintf (stderr, "Error: Input error\n");
      goto error;
    }

  /* Check for data to be read (or if the stdin was closed (?)) */
  if (condition & G_IO_IN)
    {
      GIOError error;
      gchar buffer[1024];
      gsize bytes_read;

      /* Read the data into our buffer */
      error = gnet_io_channel_readline (iochannel, buffer, 
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
	  if (lines_pending == 0)
	    exit (EXIT_SUCCESS);
	  else
	    read_eof = TRUE;
	  return FALSE;
	}

      /* Otherwise, write to the socket */
      else
	{
	  GIOChannel* sin;
	  GIOError error;
	  gsize bytes_written;

	  sin = (GIOChannel*) data;
	  error = gnet_io_channel_writen (sin, buffer, bytes_read, 
					  &bytes_written);
/*  	  g_print ("# write %d\n", bytes_written); */
	  g_assert (error == G_IO_ERROR_NONE);
	  g_assert (bytes_written == bytes_read);

	  lines_pending++;
	}
    }

  if (condition & G_IO_HUP)
    goto error;

  return TRUE;

 error:
  exit (EXIT_FAILURE);
  return FALSE;
}
