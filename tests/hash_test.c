/* MD5/SHA test
 * Copyright (C) 2000, 2002  David Helder
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnet.h>



int
main (int argc, char* argv[])
{
  gchar* buffer;

  gchar* str;

  GMD5* md5;
  GMD5* md5b;

  GSHA* sha;
  GSHA* shab;

  gnet_init ();


  /* **************************************** */

  buffer = g_malloc (20000);
  g_assert (buffer);
  memset (buffer, 'A', 20000);

  /* Create MD5 */
  md5 = gnet_md5_new (buffer, 20000);	
  g_assert (md5);

  /* Get string */
  str = gnet_md5_get_string (md5);
  g_assert (str);
  g_free (str);

  /* Create MD5 from string */
  md5b = gnet_md5_new_string ("0af181fb57f1eefb62b74081bbddb155");
  g_assert (md5b);

  /* Check if equal */
  g_assert (gnet_md5_equal (md5, md5b));

  gnet_md5_delete (md5);
  gnet_md5_delete (md5b);

  /* **************************************** */

  /* Create SHA */
  sha = gnet_sha_new (buffer, 20000);	
  g_assert (sha);

  /* Get string */
  str = gnet_sha_get_string (sha);
  g_assert (str);
  g_free (str);

  /* Create SHA from string */
  shab = gnet_sha_new_string ("b9624c14586d4668cba0b2759229a49f1ea355b6");
  g_assert (shab);

  /* Check if equal */
  g_assert (gnet_sha_equal (sha, shab));

  gnet_sha_delete (sha);
  gnet_sha_delete (shab);

  /* **************************************** */

  if (buffer)
    g_free (buffer);

  /* **************************************** */

  exit (EXIT_SUCCESS);

  return 0;
}
