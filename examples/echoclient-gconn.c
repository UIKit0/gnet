/* Echo client (GConn TCP)
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


static gboolean ob_in_iofunc (GIOChannel* iochannel, GIOCondition condition, 
			      gpointer data);

static void ob_conn_func (GConn* conn, GConnEvent* event, gpointer user_data);


static int lines_pending = 0;
static gboolean read_eof = FALSE;


int
main(int argc, char** argv)
{
  gchar* hostname;
  gint port;
  GMainLoop* main_loop;
  GConn* conn;
  GIOChannel* in;

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
  main_loop = g_main_new(FALSE);
  
  /* Create connection object */
  conn = gnet_conn_new (hostname, port, ob_conn_func, NULL);
  g_assert (conn);

  /* Connect */
  gnet_conn_connect (conn);
  gnet_conn_timeout (conn, 30000);  /* 30 second timeout */

  /* Read from stdin */
  in = g_io_channel_unix_new (fileno(stdin));
  g_io_add_watch(in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
		 ob_in_iofunc, conn);

  /* Start the main loop */
  g_main_run (main_loop);

  exit (EXIT_SUCCESS);
  return 0;
}


/* Read line from stdin asynchronously */
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
      error = gnet_io_channel_readline (iochannel, buffer, 
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
	  if (lines_pending == 0)
	    exit (EXIT_SUCCESS);
	  else
	    read_eof = TRUE;
	  return FALSE;
	}

      /* Otherwise, we read something */
      else
	{
	  gnet_conn_write (conn, buffer, bytes_read);
	  lines_pending++;
	}
    }

  return TRUE;

 error:
  exit (EXIT_FAILURE);
  return FALSE;
}


/* Handle GConn events */
static void
ob_conn_func (GConn* conn, GConnEvent* event, gpointer user_data)
{
  switch (event->type)
    {
    case GNET_CONN_CONNECT:
      {
	gnet_conn_timeout (conn, 0);
	gnet_conn_readline (conn);
	break;
      }

    case GNET_CONN_READ:
      {
	/* Write line out */
	event->buffer[event->length - 1] = '\n';
	fwrite (event->buffer, event->length, 1, stdout);

	/* Check if done */
	lines_pending--;
	if (lines_pending == 0 && read_eof)
	  exit (EXIT_SUCCESS);

	/* Read another line */
	gnet_conn_readline (conn);

	break;
      }

    case GNET_CONN_WRITE:
      {
	; /* do nothing */
	break;
      }

    case GNET_CONN_CLOSE:
      {
	gnet_conn_delete (conn);
	exit (EXIT_SUCCESS);
	break;
      }

    case GNET_CONN_TIMEOUT:
      {
	gnet_conn_delete (conn);
	fprintf (stderr, "Connection timeout\n");
	exit (EXIT_FAILURE);
	break;
      }

    case GNET_CONN_ERROR:
      {
	gnet_conn_delete (conn);
	fprintf (stderr, "Connection failure\n");
	exit (EXIT_FAILURE);
	break;
      }

    default:
      g_assert_not_reached ();
    }
}
