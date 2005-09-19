/* DNSTest - Tests InetAddr non-blocking functions
 * Copyright (C) 2000-2002  David Helder
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
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include <gnet.h>

#define PORT	0x1234

gchar* host;
gboolean do_reverse = FALSE;
gboolean do_async = FALSE;
gboolean do_single = FALSE;

gint   num_runs = 1;
gint   runs_done = 0;

void lookup_block_single (int run);
void lookup_block_list (int run);

void lookup_async_forward(int run);
void inetaddr_cb(GInetAddr* inetaddr, gpointer data);

void lookup_async_forward_list (int run);
void list_cb(GList* ialist, gpointer data);

void lookup_async_reverse(int run);
void reverse_inetaddr_cb(gchar* name, gpointer data);


void usage(void);




struct ReverseState
{
  GInetAddr* ia;
  gint num;
};



int
main(int argc, char** argv)
{
  extern char* optarg;
  extern int   optind;
  int c;

  gnet_init ();

  /* Parse arguments */
  while ((c = getopt(argc, argv, "arsn:")) != -1) 
    {
      switch (c) 
	{
	case 'a':	
	  do_async = TRUE;
	  break;
	case 'r':
	  do_reverse = TRUE;
	  break;
	case 's':
	  do_single = TRUE;
	  break;
	case 'n':
	  num_runs = atoi(optarg);
	  break;
	case 'h':
	case '?':
	  usage();
	  break;
	}
    }

  if (argc == optind)
    usage();

  host = argv[optind];

  if (!do_async)
    {
      int i;

      for (i = 0; i < num_runs; ++i)
	{
	  if (do_single)
	    lookup_block_single(i);
	  else
	    lookup_block_list(i);
	}
    }
  else
    {
      int i;
      GMainLoop* main_loop = NULL;

      main_loop = g_main_new(FALSE);

      for (i = 0; i < num_runs; ++i)
	{
	  if (!do_reverse)
	    {
	      if (do_single)
		lookup_async_forward(i);
	      else
		lookup_async_forward_list(i);
	    }
	  else
	    lookup_async_reverse(i);
	}

      g_main_run(main_loop);
    }

  exit(EXIT_SUCCESS);
}


void
lookup_block_single (int run)
{
  GInetAddr* ia;
  gchar* name;

  ia = gnet_inetaddr_new (host, PORT);
  if (ia == NULL)
    {
      fprintf (stderr, "DNS lookup for %s failed\n", host);
      exit (EXIT_FAILURE);
    }

  g_assert (gnet_inetaddr_get_port(ia) == PORT);

  if (do_reverse)
    name = gnet_inetaddr_get_name(ia);
  else
    name = gnet_inetaddr_get_canonical_name(ia);
  g_assert (name != NULL);

  g_print ("%d: %s -> %s\n", run, host, name);

  g_free (name);

  gnet_inetaddr_delete (ia);
}


void
lookup_block_list (int run)
{
  GList* ialist;
  GList* i;
  gchar* name;

  ialist = gnet_inetaddr_new_list(host, PORT);
  if (ialist == NULL)
    {
      g_print ("DNS lookup for %s failed\n", host);
      exit (EXIT_FAILURE);
    }

  for (i = ialist; i != NULL; i = i->next)
    {
      GInetAddr* ia;

      ia = (GInetAddr*) i->data;

      g_assert (gnet_inetaddr_get_port(ia) == PORT);

      if (do_reverse)
	name = gnet_inetaddr_get_name(ia);
      else
	name = gnet_inetaddr_get_canonical_name(ia);

      g_assert (name != NULL);

      g_print ("%d: %s -> %s\n", run, host, name);

      g_free (name);
      gnet_inetaddr_delete (ia);
    }

  g_list_free (ialist);
}


/* **************************************** */


void
lookup_async_forward (int run)
{
  gnet_inetaddr_new_async(host, PORT, inetaddr_cb, GINT_TO_POINTER(run));
}


void
inetaddr_cb(GInetAddr* ia, gpointer data)
{
  int i = GPOINTER_TO_INT(data);

  if (ia)
    {
      gchar* cname;

      g_assert (gnet_inetaddr_get_port(ia) == PORT);

      cname = gnet_inetaddr_get_canonical_name(ia);
      if (cname == NULL)
	{
	  g_print ("Reverse DNS lookup failed\n");
	  exit (EXIT_FAILURE);
	}

      g_print ("%d: %s -> %s\n", i, host, cname);

      g_free (cname);
    }

  else
    g_print ("%d: DNS lookup failed\n", i);

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}

/* ******************** */

void
lookup_async_forward_list (int run)
{
  gnet_inetaddr_new_list_async(host, PORT, list_cb, GINT_TO_POINTER(run));
}


void
list_cb(GList* ialist, gpointer data)
{
  int run = GPOINTER_TO_INT(data);

  if (ialist)
    {
      GList* i;

      for (i = ialist; i != NULL; i = i->next)
	{
	  GInetAddr* ia;
	  gchar* cname;

	  ia = (GInetAddr*) i->data;

	  g_assert (gnet_inetaddr_get_port(ia) == PORT);

	  cname = gnet_inetaddr_get_canonical_name(ia);
	  g_assert (cname);

	  g_print ("%d: %s -> %s\n", run, host, cname);

	  g_free (cname);
	  gnet_inetaddr_delete (ia);
	}
      g_list_free (ialist);
    }

  else
    g_print ("%d: DNS lookup failed\n", run);

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}



/* ******************** */

void
lookup_async_reverse (int run)
{
  GInetAddr* ia;
  struct ReverseState* rs;

  ia = gnet_inetaddr_new(host, 0);
  if (ia == NULL)
    {
      g_print ("DNS lookup for %s failed\n", host);
      exit (EXIT_FAILURE);
    }

  rs = g_new0(struct ReverseState, 1);
  rs->ia = ia;
  rs->num = run;

  gnet_inetaddr_get_name_async(ia, reverse_inetaddr_cb, rs);
}


void
reverse_inetaddr_cb (gchar* name, gpointer data)
{
  struct ReverseState* rs = (struct ReverseState*) data;


  if (name)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name(rs->ia);
      if (cname == NULL)
	{
	  g_print ("Reverse DNS lookup for %s failed\n", name);
	  exit (EXIT_FAILURE);
	}

      g_print ("%d: %s -> %s (reverse)\n", rs->num, cname, name);

      g_free(cname);
    }
  else
    g_print ("%d: error\n", rs->num);

  gnet_inetaddr_delete (rs->ia);
  g_free (rs);

  runs_done++;

  if (runs_done == num_runs)
    exit(EXIT_SUCCESS);
}


/* **************************************** */


void
usage(void)
{
  g_print ("dnslookup -a -r -s -n <num runs> <host>\n");
  g_print ("  -a                asynchronous\n");
  g_print ("  -r                reverse lookup\n");
  g_print ("  -s                single forward lookup\n");
  g_print ("  -n <num runs>     number of lookups\n");

  exit (EXIT_FAILURE);
}
