/* Print information about the host
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

#include <gnet.h>


int
main(int argc, char** argv)
{

  GList* interfaces;
  GList* i;
  GInetAddr* ia;
  gchar* cname;
  gchar* name;

  /* Print info about me */
  ia = gnet_inetaddr_gethostaddr();
  g_assert (ia != NULL);

  name = gnet_inetaddr_get_name(ia);
  g_assert (name != NULL);
  cname = gnet_inetaddr_get_canonical_name(ia);
  g_assert (cname != NULL);

  g_print ("hostname is %s (%s)\n", name, cname);
  g_free(name);
  g_free(cname);


  /* Print interfaces */
  g_print ("interfaces:\n");

  interfaces = gnet_inetaddr_list_interfaces();

  for (i = interfaces; i != NULL; i = g_list_next(i))
    {
      ia = (GInetAddr*) i->data;
      g_assert (ia != NULL);

      name = gnet_inetaddr_get_name(ia);
      g_assert (name != NULL);

      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert (cname != NULL);

      g_print ("%s (%s)\n", name, cname);
      g_free(name);
    }

  g_list_free(interfaces);


  exit(EXIT_SUCCESS);
}
