/* DNSTest - Tests InetAddr non-blocking functions
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


#define DO_REVERSE 0
#define VERBOSE 0

void lookup_block(void);
void lookup_nonblock(void);
void inetaddr_cb(GInetAddr* inetaddr, GInetAddrNonblockStatus status, gpointer data);
void reverse_inetaddr_cb(GInetAddr* inetaddr, GInetAddrNonblockStatus status, 
			 gchar* name, gpointer data);


void usage(void);


gint num_runs = 1;
gchar* host = NULL;
gint runs_done = 0;


int
main(int argc, char** argv)
{
  gboolean block = FALSE;

  if (argc < 2 || argc > 4)
    usage();

  host = argv[1];

  if (argc >= 3)
    {

      if (strcmp(argv[2], "block") == 0)
	{
	  g_print ("Using blocking DNS\n");
	  block = TRUE;
	}
      else
	g_print ("Using non-blocking DNS\n");
    }

  if (argc >= 4)
    {
      num_runs = atoi(argv[3]);
    }


  if (block)
    lookup_block();
  else
    lookup_nonblock();

  exit(EXIT_SUCCESS);
}


void
lookup_block(void)
{
  int i;

/*    g_print ("lookup_block\n"); */

  for (i = 0; i < num_runs; ++i)
    {
      GInetAddr* ia;
      gchar* cname;

      ia = gnet_inetaddr_new(host, 0);
      g_assert(ia != NULL);

      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert(cname != NULL);

#if VERBOSE
      g_print ("%d: %s -> %s\n", i, host, cname);
#endif

      g_free(cname);
    }
}




void
lookup_nonblock(void)
{
  int i;
  GMainLoop* main_loop = NULL;

/*    g_print ("lookup_nonblock\n"); */

  main_loop = g_main_new(FALSE);

  for (i = 0; i < num_runs; ++i)
    {

#if (!DO_REVERSE)

      gnet_inetaddr_new_nonblock(host, 0, inetaddr_cb, GINT_TO_POINTER(i));
      
#else

      GInetAddr* ia;

      ia = gnet_inetaddr_new(host, 0);
      g_assert (ia != NULL);

      gnet_inetaddr_get_name_nonblock(ia, reverse_inetaddr_cb, GINT_TO_POINTER(i));

#endif

      g_main_iteration(FALSE);
    }

  g_main_run(main_loop);
}


void
inetaddr_cb(GInetAddr* ia, GInetAddrNonblockStatus status, gpointer data)
{
  int i = GPOINTER_TO_INT(data);

  if (status == GINETADDR_NONBLOCK_STATUS_OK)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert(cname != NULL);

#if VERBOSE
      g_print ("%d: %s -> %s\n", i, host, cname);
#endif

      g_free(cname);
    }
#if VERBOSE
  else
    g_print("%d: error\n", i);
#endif

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}


void
reverse_inetaddr_cb(GInetAddr* ia, GInetAddrNonblockStatus status, 
		    gchar* name, gpointer data)
{
  int i = GPOINTER_TO_INT(data);

  if (status == GINETADDR_NONBLOCK_STATUS_OK)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert(cname != NULL);

#if VERBOSE
      g_print ("%d: %s -> %s\n", i, cname, name);
#endif

      g_free(cname);
      g_free(name);
    }
#if VERBOSE
  else
    g_print("%d: error\n", i);
#endif

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}



void
usage(void)
{
  g_print ("dnstest <host> [block|nonblock] [num-runs]\n");
  exit (EXIT_FAILURE);
}
