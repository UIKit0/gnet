/* DNSTest - Tests InetAddr non-blocking functions
 * Copyright (C) 2000-2002  David Helder
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


int do_reverse = 0;
int verbose = 1;

void lookup_block(void);
void lookup_async(void);
void inetaddr_cb(GInetAddr* inetaddr, GInetAddrAsyncStatus status, gpointer data);
void reverse_inetaddr_cb(GInetAddr* inetaddr, GInetAddrAsyncStatus status, 
			 gchar* name, gpointer data);


void usage(void);


gint num_runs = 1;
gchar* host = NULL;
gint runs_done = 0;


int
main(int argc, char** argv)
{
  gboolean block = TRUE;

  gnet_init ();

  if (argc < 2 || argc > 4)
    usage();

  host = argv[1];

  if (argc >= 3 && !strcmp(argv[2], "async"))
    block = FALSE;

  if (argc >= 4)
    {
      num_runs = atoi(argv[3]);
    }


  if (block)
    {
      g_print ("Using blocking DNS\n");
      lookup_block();
    }
  else
    {
      g_print ("Using asynchronous DNS\n");
      lookup_async();
    }

  exit(EXIT_SUCCESS);
}


void
lookup_block(void)
{
  int i;

  for (i = 0; i < num_runs; ++i)
    {
      GInetAddr* ia;
      gchar* name;

      ia = gnet_inetaddr_new(host, 0);
      if (ia == NULL)
	{
	  g_print ("DNS lookup for %s failed\n", host);
	  exit (EXIT_FAILURE);
	}

      if (do_reverse)
	name = gnet_inetaddr_get_name(ia);
      else
	name = gnet_inetaddr_get_canonical_name(ia);

      g_assert (name != NULL);

      if (verbose)
	g_print ("%d: %s -> %s\n", i, host, name);

      g_free (name);

      gnet_inetaddr_delete (ia);

    }
}




void
lookup_async(void)
{
  int i;
  GMainLoop* main_loop = NULL;

  main_loop = g_main_new(FALSE);

  for (i = 0; i < num_runs; ++i)
    {
      if (do_reverse)
	{
	  GInetAddr* ia;

	  ia = gnet_inetaddr_new(host, 0);
	  if (ia == NULL)
	    {
	      if (verbose)
		g_print ("DNS lookup for %s failed\n", host);
	      exit (EXIT_FAILURE);
	    }

	  gnet_inetaddr_get_name_async(ia, reverse_inetaddr_cb, 
				       GINT_TO_POINTER(i));
	}
      else
	{
	  gnet_inetaddr_new_async(host, 0, inetaddr_cb, GINT_TO_POINTER(i));
	}

/*        g_main_iteration (FALSE); */
    }

  g_main_run(main_loop);
}


void
inetaddr_cb(GInetAddr* ia, GInetAddrAsyncStatus status, gpointer data)
{
  int i = GPOINTER_TO_INT(data);

  if (status == GINETADDR_ASYNC_STATUS_OK)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name(ia);
      if (cname == NULL)
	{
	  g_print ("Reverse DNS lookup failed\n");
	  exit (EXIT_FAILURE);
	}

      if (verbose)
	g_print ("%d: %s -> %s\n", i, host, cname);

      if (do_reverse) /* Caller owns forward ia, we own reverse ia. */
	gnet_inetaddr_delete (ia);

      g_free (cname);
    }

  else if (verbose)
    g_print("%d: DNS lookup failed\n", i);

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}


void
reverse_inetaddr_cb (GInetAddr* ia, GInetAddrAsyncStatus status, 
		     gchar* name, gpointer data)
{
  int i = GPOINTER_TO_INT(data);

  if (status == GINETADDR_ASYNC_STATUS_OK)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name(ia);
      if (cname == NULL)
	{
	  g_print ("Reverse DNS lookup for %s failed\n", name);
	  exit (EXIT_FAILURE);
	}

      if (verbose)
	g_print ("%d: %s -> %s (reverse)\n", i, cname, name);

      g_free(cname);
    }
  else if (verbose)
    g_print("%d: error\n", i);

  gnet_inetaddr_delete (ia);

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}



void
usage(void)
{
  g_print ("dnstest <host> [block|async] [num-runs]\n");
  exit (EXIT_FAILURE);
}
