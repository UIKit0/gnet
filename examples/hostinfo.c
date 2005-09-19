/* Print information about the host
 * Copyright (C) 2000  David Helder
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
#include <glib.h>

#include <gnet.h>


int
main(int argc, char** argv)
{
  GInetAddr* ia;
  GInetAddr* autoia;
  GList* interfaces;
  GList* i;

  gnet_init ();

  /* Print info about me */
  ia = gnet_inetaddr_get_host_addr();
  if (ia != NULL)
    {
      gchar* cname;
      gchar* name;

      name = gnet_inetaddr_get_name(ia);
      if (!name)
	name = g_strdup ("<none>");
      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert (cname != NULL);

      g_print ("Host name is %s (%s)\n", name, cname);
      gnet_inetaddr_delete (ia);
      g_free(name);
      g_free(cname);
    }
  else
    g_print ("Host name is <none> (<none>)\n");

  /* Print interfaces */
  g_print ("Interfaces:\n");

  interfaces = gnet_inetaddr_list_interfaces();

  for (i = interfaces; i != NULL; i = g_list_next(i))
    {
      gchar* name;
      gchar* cname;

      ia = (GInetAddr*) i->data;
      g_assert (ia != NULL);

      name = gnet_inetaddr_get_name(ia);
      g_assert (name != NULL);

      cname = gnet_inetaddr_get_canonical_name(ia);
      g_assert (cname != NULL);

      g_print ("%s (%s)\n", name, cname);
      gnet_inetaddr_delete (ia);
      g_free(name);
    }

  g_list_free(interfaces);

  autoia = gnet_inetaddr_get_internet_interface();
  g_print ("Internet inteface: ");
  if (autoia)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name (autoia);
      g_print ("%s\n", cname);
      g_free (cname);
    }
  else
    g_print ("<none>\n");

  autoia = gnet_inetaddr_autodetect_internet_interface();
  g_print ("Auto-detected internet inteface: ");
  if (autoia)
    {
      gchar* cname;

      cname = gnet_inetaddr_get_canonical_name (autoia);
      g_print ("%s\n", cname);
      g_free (cname);
    }
  else
    g_print ("<none>\n");

  exit(EXIT_SUCCESS);
}
