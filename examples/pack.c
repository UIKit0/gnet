/* Gnet-Pack test/example
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


#include <glib.h>
#include <stdio.h>

void print_bytes(char* s, int len);


int
main()
{
  char buffer[1024];

  /* **************************************** */

  gnet_pack ("x", buffer, 1024);
  print_bytes (buffer, 1);

  gnet_pack ("b", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack ("B", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack ("h", buffer, 1024, 0x0102);
  print_bytes (buffer, sizeof(short));

  gnet_pack ("H", buffer, 1024, 0x0304);
  print_bytes (buffer, sizeof(unsigned short));

  gnet_pack ("i", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(int));

  gnet_pack ("I", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));

  gnet_pack ("l", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(long));

  gnet_pack ("L", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));

  gnet_pack ("f", buffer, 1024, 23.43);
  print_bytes (buffer, sizeof(float));

  gnet_pack ("d", buffer, 1024, 43.22);
  print_bytes (buffer, sizeof(double));

  gnet_pack ("P", buffer, 1024, (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));

  /* **************************************** */

  printf ("********************\n");

  gnet_pack ("<x", buffer, 1024);
  print_bytes (buffer, 1);

  gnet_pack ("<b", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack ("<B", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack ("<h", buffer, 1024, 0x0102);
  print_bytes (buffer, sizeof(short));

  gnet_pack ("<H", buffer, 1024, 0x0304);
  print_bytes (buffer, sizeof(unsigned short));

  gnet_pack ("<i", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(int));

  gnet_pack ("<I", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));

  gnet_pack ("<l", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(long));

  gnet_pack ("<L", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));

  gnet_pack ("<f", buffer, 1024, 23.43);
  print_bytes (buffer, sizeof(float));

  gnet_pack ("<d", buffer, 1024, 43.22);
  print_bytes (buffer, sizeof(double));

  gnet_pack ("<P", buffer, 1024, (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));

  /* **************************************** */

  printf ("********************\n");

  gnet_pack (">x", buffer, 1024);
  print_bytes (buffer, 1);

  gnet_pack (">b", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack (">B", buffer, 1024, 23);
  print_bytes (buffer, 1);

  gnet_pack (">h", buffer, 1024, 0x0102);
  print_bytes (buffer, sizeof(short));

  gnet_pack (">H", buffer, 1024, 0x0304);
  print_bytes (buffer, sizeof(unsigned short));

  gnet_pack (">i", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(int));

  gnet_pack (">I", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));

  gnet_pack (">l", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(long));

  gnet_pack (">L", buffer, 1024, 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));

  gnet_pack (">f", buffer, 1024, 23.43);
  print_bytes (buffer, sizeof(float));

  gnet_pack (">d", buffer, 1024, 43.22);
  print_bytes (buffer, sizeof(double));

  gnet_pack (">P", buffer, 1024, (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));

  /* **************************************** */

  printf ("********************\n");

  gnet_pack ("bhb", buffer, 1024, 0, 1, 2);
  print_bytes (buffer, 4);

  gnet_pack ("ii", buffer, 1024, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);

  gnet_pack ("2i", buffer, 1024, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);


  /* **************************************** */

  printf ("********************\n");

  gnet_pack ("ss", buffer, 1024, "hello", "there");
  print_bytes (buffer, 12);

  gnet_pack ("2s", buffer, 1024, "there", "hello");
  print_bytes (buffer, 12);


  return 0;
}


void
print_bytes(char* s, int len)
{
  int i;

  for (i = 0; i < len; ++i)
    printf ("%d ", (unsigned char) s[i]);

  printf ("\n");
}
