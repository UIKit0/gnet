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

  printf ("\n\n************\n");
  printf ("native tests (assumes little endian)\n");

  gnet_pack ("x", buffer, 1);
  print_bytes (buffer, 1);
  printf ("should be: 00\n");

  /* ********** */

  gnet_pack ("b", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack ("b", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack ("B", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack ("B", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack ("h", buffer, sizeof(short), 0x0102);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 02 01\n");

  gnet_pack ("h", buffer, sizeof(short), 0xf001);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 01 f0\n");

  gnet_pack ("h", buffer, sizeof(short), 0x01f0);
  print_bytes (buffer, sizeof(short));
  printf ("should be: f0 01\n");

  /* ********** */

  gnet_pack ("H", buffer, sizeof(unsigned short), 0x0102);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 02 01\n");

  gnet_pack ("H", buffer, sizeof(unsigned short), 0xf001);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 01 f0\n");

  gnet_pack ("H", buffer, sizeof(unsigned short), 0x01f0);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: f0 01\n");

  /* ********** */

  gnet_pack ("i", buffer, sizeof(int), 0x01020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("i", buffer, sizeof(int), 0xf1020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("i", buffer, sizeof(int), 0x010203f4);
  print_bytes (buffer, sizeof(int));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("I", buffer, sizeof(unsigned int), 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("I", buffer, sizeof(unsigned int), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("I", buffer, sizeof(unsigned int), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("l", buffer, sizeof(long), 0x01020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("l", buffer, sizeof(long), 0xf1020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("l", buffer, sizeof(long), 0x010203f4);
  print_bytes (buffer, sizeof(long));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("L", buffer, sizeof(unsigned long), 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("L", buffer, sizeof(unsigned long), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("L", buffer, sizeof(unsigned long), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("f", buffer, sizeof(float), 23.43);
  print_bytes (buffer, sizeof(float));
  printf ("should be: ?\n");

  gnet_pack ("d", buffer, sizeof(double), 43.22);
  print_bytes (buffer, sizeof(double));
  printf ("should be: ?\n");

  /* ********** */

  gnet_pack ("P", buffer, sizeof(void*), (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("P", buffer, sizeof(void*), (void*) 0xf1020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("P", buffer, sizeof(void*), (void*) 0x010203f4);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: f4 03 02 01\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("little endian tests\n");


  gnet_pack ("<x", buffer, 1);
  print_bytes (buffer, 1);
  printf ("should be: 00\n");

  /* ********** */

  gnet_pack ("<b", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack ("<b", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack ("<B", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack ("<B", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack ("<h", buffer, sizeof(short), 0x0102);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 02 01\n");

  gnet_pack ("<h", buffer, sizeof(short), 0xf001);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 01 f0\n");

  gnet_pack ("<h", buffer, sizeof(short), 0x01f0);
  print_bytes (buffer, sizeof(short));
  printf ("should be: f0 01\n");

  /* ********** */

  gnet_pack ("<H", buffer, sizeof(unsigned short), 0x0102);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 02 01\n");

  gnet_pack ("<H", buffer, sizeof(unsigned short), 0xf001);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 01 f0\n");

  gnet_pack ("<H", buffer, sizeof(unsigned short), 0x01f0);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: f0 01\n");

  /* ********** */

  gnet_pack ("<i", buffer, sizeof(int), 0x01020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("<i", buffer, sizeof(int), 0xf1020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("<i", buffer, sizeof(int), 0x010203f4);
  print_bytes (buffer, sizeof(int));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("<I", buffer, sizeof(unsigned int), 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("<I", buffer, sizeof(unsigned int), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("<I", buffer, sizeof(unsigned int), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("<l", buffer, sizeof(long), 0x01020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("<l", buffer, sizeof(long), 0xf1020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("<l", buffer, sizeof(long), 0x010203f4);
  print_bytes (buffer, sizeof(long));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("<L", buffer, sizeof(unsigned long), 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("<L", buffer, sizeof(unsigned long), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("<L", buffer, sizeof(unsigned long), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: f4 03 02 01\n");

  /* ********** */

  gnet_pack ("<f", buffer, sizeof(float), 23.43);
  print_bytes (buffer, sizeof(float));
  printf ("should be: ?\n");

  gnet_pack ("<d", buffer, sizeof(double), 43.22);
  print_bytes (buffer, sizeof(double));
  printf ("should be: ?\n");

  /* ********** */

  gnet_pack ("<P", buffer, sizeof(void*), (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 04 03 02 01\n");

  gnet_pack ("<P", buffer, sizeof(void*), (void*) 0xf1020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 04 03 02 f1\n");

  gnet_pack ("<P", buffer, sizeof(void*), (void*) 0x010203f4);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: f4 03 02 01\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("big endian tests\n");

  gnet_pack (">x", buffer, 1);
  print_bytes (buffer, 1);
  printf ("should be: 00\n");

  /* ********** */

  gnet_pack (">b", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack (">b", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack (">B", buffer, 1, 0x17);
  print_bytes (buffer, 1);
  printf ("should be: 17\n");

  gnet_pack (">B", buffer, 1, 0xf1);
  print_bytes (buffer, 1);
  printf ("should be: f1\n");

  /* ********** */

  gnet_pack (">h", buffer, sizeof(short), 0x0102);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 01 02\n");

  gnet_pack (">h", buffer, sizeof(short), 0xf001);
  print_bytes (buffer, sizeof(short));
  printf ("should be: f0 01\n");

  gnet_pack (">h", buffer, sizeof(short), 0x01f0);
  print_bytes (buffer, sizeof(short));
  printf ("should be: 01 f0\n");

  /* ********** */

  gnet_pack (">H", buffer, sizeof(unsigned short), 0x0102);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 01 02\n");

  gnet_pack (">H", buffer, sizeof(unsigned short), 0xf001);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: f0 01\n");

  gnet_pack (">H", buffer, sizeof(unsigned short), 0x01f0);
  print_bytes (buffer, sizeof(unsigned short));
  printf ("should be: 01 f0\n");

  /* ********** */

  gnet_pack (">i", buffer, sizeof(int), 0x01020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 01 02 03 04\n");

  gnet_pack (">i", buffer, sizeof(int), 0xf1020304);
  print_bytes (buffer, sizeof(int));
  printf ("should be: f1 02 03 04\n");

  gnet_pack (">i", buffer, sizeof(int), 0x010203f4);
  print_bytes (buffer, sizeof(int));
  printf ("should be: 01 02 03 f4\n");

  /* ********** */

  gnet_pack (">I", buffer, sizeof(unsigned int), 0x01020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 01 02 03 04\n");

  gnet_pack (">I", buffer, sizeof(unsigned int), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: f1 02 03 04\n");

  gnet_pack (">I", buffer, sizeof(unsigned int), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned int));
  printf ("should be: 01 02 03 f4\n");

  /* ********** */

  gnet_pack (">l", buffer, sizeof(long), 0x01020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 01 02 03 04\n");

  gnet_pack (">l", buffer, sizeof(long), 0xf1020304);
  print_bytes (buffer, sizeof(long));
  printf ("should be: f1 02 03 04\n");

  gnet_pack (">l", buffer, sizeof(long), 0x010203f4);
  print_bytes (buffer, sizeof(long));
  printf ("should be: 01 02 03 f4\n");

  /* ********** */

  gnet_pack (">L", buffer, sizeof(unsigned long), 0x01020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 01 02 03 04\n");

  gnet_pack (">L", buffer, sizeof(unsigned long), 0xf1020304);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: f1 02 03 04\n");

  gnet_pack (">L", buffer, sizeof(unsigned long), 0x010203f4);
  print_bytes (buffer, sizeof(unsigned long));
  printf ("should be: 01 02 03 f4\n");

  /* ********** */

  gnet_pack (">f", buffer, sizeof(float), 23.43);
  print_bytes (buffer, sizeof(float));
  printf ("should be: ?\n");

  gnet_pack (">d", buffer, sizeof(double), 43.22);
  print_bytes (buffer, sizeof(double));
  printf ("should be: ?\n");

  /* ********** */

  gnet_pack (">P", buffer, 4, (void*) 0x01020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 01 02 03 04\n");

  gnet_pack (">P", buffer, 4, (void*) 0xf1020304);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: f1 02 03 04\n");

  gnet_pack (">P", buffer, 4, (void*) 0x010203f4);
  print_bytes (buffer, sizeof(void*));
  printf ("should be: 01 02 03 f4\n");

  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("native combinations (assumes little endian)\n");

  gnet_pack ("bhb", buffer, 4, 0x00, 0x0001, 0x02);
  print_bytes (buffer, 4);
  printf ("should be: 00 01 00 02\n");

  gnet_pack ("ii", buffer, 8, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);
  printf ("should be: 04 03 02 01 08 07 06 05\n");

  gnet_pack ("2i", buffer, 8, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);
  printf ("should be: 04 03 02 01 08 07 06 05\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("big endian combinations \n");

  gnet_pack (">bhb", buffer, 4, 0, 1, 2);
  print_bytes (buffer, 4);
  printf ("should be: 00 00 01 02\n");

  gnet_pack (">ii", buffer, 8, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);
  printf ("should be: 01 02 03 04 05 06 07 08\n");

  gnet_pack (">2i", buffer, 8, 0x01020304, 0x05060708);
  print_bytes (buffer, 8);
  printf ("should be: 01 02 03 04 05 06 07 08\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("strings\n");

  gnet_pack ("ss", buffer, 12, "hello", "there");
  print_bytes (buffer, 12);
  printf ("should be: 68 65 6c 6c 6f 00 74 68 65 72 65 00\n");

  gnet_pack ("2s", buffer, 12, "there", "hello");
  print_bytes (buffer, 12);
  printf ("should be: 74 68 65 72 65 00 68 65 6c 6c 6f 00\n");

  gnet_pack ("12S", buffer, 12, "booger");
  print_bytes (buffer, 12);
  printf ("should be: 62 6f 6f 67 65 72 00 00 00 00 00 00\n");

  gnet_pack ("4S", buffer, 4, "david");
  print_bytes (buffer, 4);
  printf ("should be: 64 61 76 69\n");

  gnet_pack ("6S", buffer, 6, "helder");
  print_bytes (buffer, 6);
  printf ("should be: 68 65 6c 64 65 72\n");

  gnet_pack ("r", buffer, 4, "dcba", 4);
  print_bytes (buffer, 4);
  printf ("should be: 64 63 62 61\n");

  gnet_pack ("2r", buffer, 8, "abcd", 4, "efgh", 4);
  print_bytes (buffer, 8);
  printf ("should be: 61 62 63 64 65 66 67 68\n");

  gnet_pack ("4R", buffer, 4, "dcba");
  print_bytes (buffer, 4);
  printf ("should be: 64 63 62 61\n");

  gnet_pack ("4R4R", buffer, 8, "efgh", "abcd");
  print_bytes (buffer, 8);
  printf ("should be: 65 66 67 68 61 62 63 64\n");

  gnet_pack ("p", buffer, 5, "abcd");
  print_bytes (buffer, 5);
  printf ("should be: 04 61 62 63 64\n");

  gnet_pack ("2p", buffer, 10, "efgh", "abcd");
  print_bytes (buffer, 10);
  printf ("should be: 04 65 66 67 68 04 61 62 63 64\n");


  /* **************************************** */

#if 0
  printf ("\n\n********************\n");
  printf ("failures\n");

  gnet_pack ("b", buffer, 0, 0);
  printf ("fail\n");

  gnet_pack ("B", buffer, 0, 0);
  printf ("fail\n");

  gnet_pack ("h", buffer, sizeof(short) - 1, 0);
  printf ("fail\n");

  gnet_pack ("H", buffer, sizeof(unsigned short) - 1, 0);
  printf ("fail\n");

  gnet_pack ("i", buffer, sizeof(int) - 1, 0);
  printf ("fail\n");

  gnet_pack ("I", buffer, sizeof(unsigned int) - 1, 0);
  printf ("fail\n");

  gnet_pack ("l", buffer, sizeof(long) - 1, 0);
  printf ("fail\n");

  gnet_pack ("L", buffer, sizeof(unsigned long) - 1, 0);
  printf ("fail\n");

  gnet_pack ("f", buffer, sizeof(float) - 1, 0);
  printf ("fail\n");

  gnet_pack ("d", buffer, sizeof(double) - 1, 0);
  printf ("fail\n");

  gnet_pack ("s", buffer, 5, "hello");
  printf ("fail\n");

  gnet_pack ("ss", buffer, 11, "hello", "world");
  printf ("fail\n");

  gnet_pack ("12S", buffer, 11, "booger");
  printf ("fail\n");

  gnet_pack ("4S", buffer, 3, "david");
  printf ("fail\n");

  gnet_pack ("6S", buffer, 5, "helder");
  printf ("fail\n");

  gnet_pack ("2r", buffer, 7, "abcd", 4, "efgh", 4);
  printf ("fail\n");

  gnet_pack ("4R", buffer, 3, "dcba");
  printf ("fail\n");

  gnet_pack ("4R4R", buffer, 7, "efgh", "abcd");
  printf ("fail\n");

  gnet_pack ("p", buffer, 4, "abcd");
  printf ("fail\n");

  gnet_pack ("2p", buffer, 9, "efgh", "abcd");
  printf ("fail\n");

#endif

  /* **************************************** */

  return 0;
}


static gchar bits2hex[16] = { '0', '1', '2', '3', 
			      '4', '5', '6', '7',
			      '8', '9', 'a', 'b',
			      'c', 'd', 'e', 'f' };

void
print_bytes(char* s, int len)
{
  int i;

  printf ("\n");
  printf ("output   : ");

  for (i = 0; i < len; ++i)
    printf ("%c%c ", bits2hex[(s[i] & 0xf0) >> 4], 
	             bits2hex[s[i] & 0xf]);

  printf ("\n");
}
