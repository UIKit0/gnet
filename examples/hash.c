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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <gnet.h>	/* Or <gnet/gnet.h> when installed. */



int
main (int argc, char* argv[])
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

  gnet_init ();


  /* ******************** */

  if (argc != 2)
    {
      fprintf (stderr, "Usage: hash <filename>\n");
      exit (EXIT_FAILURE);
    }

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
      buffer = g_malloc (length);
      g_assert (fread (buffer, length, 1, file) == 1);
    }
  else
    buffer = NULL;
  fclose (file);

  /* Reminder that on windows, text files have extra "\r" in them 
     where as on *NIX they do not. This will throw off SHA & MD5 
     if you don't filter the extra characters out before calling 
     those functions.
  */

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
    g_free (buffer);

  /* **************************************** */

  exit (EXIT_SUCCESS);

  return 0;
}
