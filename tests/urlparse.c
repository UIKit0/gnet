/* Parse and print URL
 * Copyright (C) 2001  David Bolcsfoldi, David Helder
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

#include <stdlib.h>
#include <glib.h>

#define GNET_EXPERIMENTAL 1
#include <gnet.h>


int
main (int argc, char* argv[])
{
  GURL* url = NULL;
  gchar* pretty = NULL;

  if (argc < 2)
    {
      g_print("usage: %s <url>\n", argv[0]);
      exit(EXIT_SUCCESS);
    }
  
  url = gnet_url_new(argv[1]);

  if(url != NULL)
    {
      g_print("URL %s:\n", argv[1]);
      g_print("\tprotocol: %s\n", url->protocol);
      g_print("\thostname: %s\n", url->hostname);
      g_print("\tport: %d\n", url->port);
      g_print("\tuser: %s\n", url->user);
      g_print("\tpassword: %s\n", url->password);
      g_print("\tresource: %s\n", url->resource);
      g_print("\tquery: %s\n", url->query);
      g_print("\tfragment: %s\n", url->fragment);

      pretty = gnet_url_get_nice_string(url);
      g_print("\tpretty print: %s\n", pretty);
      g_free(pretty);
      exit(EXIT_SUCCESS);
    }
  else
    {
      g_print("Failed to parse: %s\n", argv[1]);
      exit(EXIT_FAILURE);
    }

  exit(0);
}
