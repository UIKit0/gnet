/* Echo server (Partyline)
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
#include "glib.h"
#include "gnet.h"

#include <signal.h>

static void ob_server_func (GServer* server, GConn* conn, gpointer user_data);
static void ob_client_func (GConn* conn, GConnEvent* event, 
			    gpointer user_data);
static void ob_sig_int (int signum);

static GServer* ob_server = NULL;

GList *connection_list = NULL;

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

  fprintf (stderr, "NOTE: You can not use the standard echoclient with this server.\n");

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


void send_to(gpointer data, gpointer user_data){
   struct _GConn* conn = (struct _GConn*) data;
   gchar* buf = (gchar*) user_data;
   gchar* buf2 = g_memdup (buf, (guint) strlen(buf)+1);
   gnet_conn_write( conn, buf2, (gint) strlen(buf2));
}


static void
ob_server_func (GServer* server, GConn* conn, gpointer user_data)
{
  if (conn)
    {
	  connection_list = g_list_append( connection_list, conn );
      gnet_conn_set_callback (conn, ob_client_func, NULL);
      gnet_conn_set_watch_error (conn, TRUE);
      gnet_conn_readline (conn);
    }
  else	/* Error */
    {
      gnet_server_delete (server);
      exit (EXIT_FAILURE);
    }
}


static void
ob_client_func (GConn* conn, GConnEvent* event, gpointer user_data)
{
  gchar* buf2;
  switch (event->type)
    {
    case GNET_CONN_READ:
      {
		event->buffer[event->length-1] = '\n';
		buf2= g_memdup (event->buffer, event->length+1);
		buf2[event->length]='\0';
		g_list_foreach(connection_list,send_to,buf2);

		gnet_conn_readline (conn);
		break;
      }
    case GNET_CONN_WRITE:
      {
	; /* Do nothing */
	break;
      }

    case GNET_CONN_CLOSE:
    case GNET_CONN_TIMEOUT:
    case GNET_CONN_ERROR:
      {
		connection_list = g_list_remove(connection_list, conn );
		gnet_conn_delete (conn);
	break;
      }
    default:
      g_assert_not_reached ();
    }
}


static void 
ob_sig_int (int signum)
{
  gnet_server_delete (ob_server);
  exit (EXIT_FAILURE);
}
