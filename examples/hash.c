/* Computes SHA and MD5 of given file
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <gnet.h>


static void sha_cb (GSHA* sha, gpointer user_data);


int main (int argc, char* argv[])
{
  gchar* filename;
  FILE* file;
  gchar* buffer;
  gint length;
  struct stat filestat;

  gchar* str;

  GMD5* md5;
  GMD5* md5b;

  GSHA* sha;
  GSHA* shab;

  GSHAAsyncID* id;

  GMainLoop* main_loop;

  /* ******************** */


  if (argc != 2)
    g_error ("Usage: hashcheck <filename>\n");

  filename = argv[1];

  /* Open file */
  file = fopen (filename, "rb");
  g_assert (file);

  length = fstat(fileno(file), &filestat);
  g_assert (length == 0);
  length = filestat.st_size;
  g_assert (length >= 0);

  if (length)
    {
      buffer = mmap (NULL, length, PROT_READ, MAP_PRIVATE, fileno(file), 0);
      g_assert (buffer != NULL && ((int) buffer != -1));
    }
  else
    buffer = NULL;

  /* **************************************** */

  /* Create MD5 */
  md5 = gnet_md5_new (buffer, length);	
  g_assert (md5);

  /* Get string */
  str = gnet_md5_get_string (md5);
  g_assert (str);

  g_print ("%s MD5: %s\n", filename, str);

  /* Create MD5 from string */
  md5b = gnet_md5_new_string (str);
  g_assert (md5b);
  g_free (str);

  /* Check if equal */
  g_assert (gnet_md5_equal (md5, md5b));

  gnet_md5_delete (md5);
  gnet_md5_delete (md5b);

  /* **************************************** */

  /* Create SHA */
  sha = gnet_sha_new (buffer, length);	
  g_assert (sha);

  /* Get string */
  str = gnet_sha_get_string (sha);
  g_assert (str);

  g_print ("%s SHA: %s\n", filename, str);

  /* Create SHA from string */
  shab = gnet_sha_new_string (str);
  g_assert (shab);
  g_free (str);

  /* Check if equal */
  g_assert (gnet_sha_equal (sha, shab));

  gnet_sha_delete (sha);
  gnet_sha_delete (shab);

  /* **************************************** */

  if (buffer)
    munmap(buffer, length);

  fclose (file);

  /* **************************************** */

  id = gnet_sha_new_file_async (filename, sha_cb, (gpointer) 0xdeadbeef);
  gnet_sha_new_file_async_cancel (id);

  id = gnet_sha_new_file_async (filename, sha_cb, (gpointer) 0xdeadbeef);

  /* **************************************** */

  main_loop = g_main_new (FALSE);
  g_main_run (main_loop);

  exit (EXIT_SUCCESS);

  return 0;
}


static void
sha_cb (GSHA* sha, gpointer user_data)
{
  g_assert (user_data == (gpointer) 0xdeadbeef);

  if (sha)
    {
      gchar* str;

      str = gnet_sha_get_string (sha);
      g_assert (str);

      g_print ("SHA async: %s\n", str);

      g_free (str);
      gnet_sha_delete (sha);
    }
  else
    g_print ("SHA async ERROR\n");

  exit (EXIT_SUCCESS);
}
