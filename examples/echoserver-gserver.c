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
#include <gnet.h>	/* Or <gnet/gnet.h> when installed. */

#include <signal.h>

static void ob_server_func (GServer* server, GServerStatus status, 
			    GConn* conn, gpointer user_data);
static gboolean ob_client_func (GConn* conn, GConnStatus status, 
				gchar* buffer, gint length, 
				gpointer user_data);
static void ob_sig_int (int signum);

static GServer* ob_server = NULL;


int
main(int argc, char** argv)
{
  int port;
  GServer* server;
  GMainLoop* main_loop;

  gnet_init ();

  if (argc != 2)
    {
      fprintf (stderr, "usage: echoserver <port> \n");
      exit(EXIT_FAILURE);
    }

  port = atoi(argv[argc - 1]);

  /* Create the main loop */
  main_loop = g_main_new (FALSE);

  /* Create the server */
  server = gnet_server_new (NULL, port, ob_server_func, NULL);
  if (!server)
    {
      fprintf (stderr, "Error: Could not start server\n");
      exit (EXIT_FAILURE);
    }

  ob_server = server;
  signal (SIGINT, ob_sig_int);

  /* Start the main loop */
  g_main_run(main_loop);

  exit (EXIT_SUCCESS);
  return 0;
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
