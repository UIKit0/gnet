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


typedef enum { NORMAL, NONBLOCKING, THREADED} ServerType;
#define ANY_IO_CONDITION  (G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL)

static void normal_echoserver(GTcpSocket* server);
static void nonblocking_echoserver(GTcpSocket* server);
static void threaded_echoserver(GTcpSocket* server);

gboolean nb_server_iofunc (GIOChannel* server, GIOCondition condition, gpointer data);
gboolean nb_client_iofunc (GIOChannel* client, GIOCondition condition, gpointer data);


typedef struct
{
  GTcpSocket* socket;
  gchar buffer[1024];
  guint n;

} ClientState;




int
main(int argc, char** argv)
{
  int port = 0;
  ServerType server_type = NORMAL;
  GTcpSocket* server = NULL;


  if (argc !=  2 && argc != 3)
    {
      g_print ("usage: echoserver [(nothing)|--nonblocking|--threaded] <port> \n");
      g_print ("  (threaded isn't implemented yet)\n");
      exit(EXIT_FAILURE);
    }

  if (argc == 2)
    {
      port = atoi(argv[1]);
    }
  else
    {
      port = atoi(argv[2]);
      if (strcmp(argv[1], "--nonblocking") == 0)
	server_type = NONBLOCKING;
      else if (strcmp(argv[1], "--threaded") == 0)
	server_type = THREADED;
      else
	{
	  g_print ("usage: echoserver [(nothing)|--nonblocking|--threaded] <port> \n");
	  exit(EXIT_FAILURE);
	}
    }


  server = gnet_tcp_socket_server_new(port);
  g_assert (server != NULL);


  switch (server_type)
    {
    case NORMAL:
      normal_echoserver(server);
      break;
    case NONBLOCKING:
      nonblocking_echoserver(server);
      break;
    case THREADED:
      threaded_echoserver(server);
      break;
    default:
      g_assert_not_reached();
    }

  return 0;

}


void
normal_echoserver(GTcpSocket* server)
{
  GTcpSocket* client = NULL;
  gchar buffer[1024];
  guint n;
  GIOChannel* ioclient = NULL;
  GIOError error;


  while ((client = gnet_tcp_socket_server_accept(server)) != NULL)
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
	g_print ("\nReceived error %d (closing socket).\n", error);

      g_io_channel_unref(ioclient);
      gnet_tcp_socket_delete(client);
    }
}




void 
nonblocking_echoserver(GTcpSocket* server)
{
  GIOChannel* iochannel = NULL;
  GMainLoop* main_loop = NULL;

  /* Create the main loop */
  main_loop = g_main_new(FALSE);

  /* Add a watch for incoming clients */
  iochannel = gnet_tcp_socket_get_iochannel(server);
  g_io_add_watch(iochannel, ANY_IO_CONDITION, nb_server_iofunc, server);

  /* Start the main loop */
  g_main_run(main_loop);


}


gboolean
nb_server_iofunc (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GTcpSocket* server = (GTcpSocket*) data;
  g_assert (server != NULL);

/*    g_print ("nb_server_iofunc %d\n", condition); */


  switch (condition)
    {
    case G_IO_IN:
      {
	GTcpSocket* client = NULL;
	GIOChannel* client_iochannel = NULL;
	ClientState* client_state = NULL;

	client = gnet_tcp_socket_server_accept(server);
	g_assert(client != NULL);

	client_iochannel = gnet_tcp_socket_get_iochannel(client);
	g_assert (client_iochannel != NULL);

	client_state = g_new0(ClientState, 1);
	client_state->socket = client;

	g_io_add_watch(client_iochannel, ANY_IO_CONDITION & ~G_IO_OUT, 
		       nb_client_iofunc, client_state);
	
	break;
      }
    default:
      {
	g_print ("nm_server_iofunc condition =  %d\n", condition);
	break;
      }
    }

  return TRUE;
}


gboolean nb_client_iofunc (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  ClientState* client_state = (ClientState*) data;
  GIOError error;

  g_assert (client_state != NULL);

/*    g_print ("nb_client_iofunc %d\n", condition); */


  switch (condition)
    {
    case G_IO_IN:
      {
	guint bytes_read;

	/* Read the data into our buffer */
	error = g_io_channel_read(iochannel, &client_state->buffer[client_state->n], 
				  sizeof(client_state->buffer) - client_state->n, 
				  &bytes_read);

	if (error != G_IO_ERROR_NONE)
	  g_print ("Client read error: %d\n", error);
	else if (bytes_read > 0)
	  {
	    /* If there weren't any bytes to write before, then there wasn't a
	       watch on write, so add one. */
	    if (client_state->n == 0)
	      {
		g_io_add_watch(iochannel, G_IO_OUT, nb_client_iofunc, client_state);
	      }

	    client_state->n += bytes_read;
	  }

	break;
      }
    case G_IO_OUT:
      {
	guint bytes_written;


	/* Write the data out */
	error = g_io_channel_write(iochannel, client_state->buffer, 
				   client_state->n, &bytes_written);

	if (error != G_IO_ERROR_NONE) 
	  g_print ("Client write error: %d\n", error);
	else if (bytes_written > 0)
	  {
	    /* Move the memory down some (you wouldn't want to do this
               in a performance server because it's slow!) */
	    memmove(client_state->buffer, 
		    &client_state->buffer[bytes_written],
		    bytes_written);

	    client_state->n -= bytes_written;
	  }

	return (client_state->n != 0);
      }

    case G_IO_HUP:
      {
	/* Close the connection */
	gnet_tcp_socket_delete(client_state->socket);

	/* Clean up */
	g_free(client_state);

	return FALSE;
      }


    default:
      {
	g_print ("nm_client_iofunc condition =  %d\n", condition);
	break;
      }

    }      

  return TRUE;
}



void
threaded_echoserver(GTcpSocket* server)
{
  g_assert_not_reached();
}
