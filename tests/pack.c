/* Gnet-Pack test
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


#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include <gnet.h>

static int failed = 0;

void test_bytes (int test_num, char* s /* binary */, int len, 
		 char* answer /* ascii */);


#define TEST0(NUM, ANSWER, FORMAT, SIZE) do {	\
  gnet_pack (FORMAT, buffer, SIZE);		\
  test_bytes (NUM, buffer, SIZE, ANSWER);       \
  len = gnet_pack_strdup (FORMAT, &str);	\
  test_bytes (NUM+1, buffer, SIZE, ANSWER);	\
  g_free (str);				} while (0)

#define TEST1(NUM, ANSWER, FORMAT, SIZE, ARG1) do {	\
  gnet_pack (FORMAT, buffer, SIZE, ARG1);		\
  test_bytes (NUM, buffer, SIZE, ANSWER);   		\
  len = gnet_pack_strdup (FORMAT, &str, ARG1);		\
  test_bytes (NUM+1, buffer, SIZE, ANSWER);		\
  g_free (str);				} while (0)

#define TEST2(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2) do {	\
  gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2);			\
  test_bytes (NUM, buffer, SIZE, ANSWER);			\
  len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2);		\
  test_bytes (NUM+1, buffer, SIZE, ANSWER);			\
  g_free (str);				} while (0)

#define TEST3(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2, ARG3) do {	\
  gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2, ARG3);		\
  test_bytes (NUM, buffer, SIZE, ANSWER);			\
  len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2, ARG3);	\
  test_bytes (NUM+1, buffer, SIZE, ANSWER);			\
  g_free (str);				} while (0)

#define TEST4(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2, ARG3, ARG4) do {	\
  gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2, ARG3, ARG4);		\
  test_bytes (NUM, buffer, SIZE, ANSWER);				\
  len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2, ARG3, ARG4);	\
  test_bytes (NUM+1, buffer, SIZE, ANSWER);				\
  g_free (str);				} while (0)

#define MEMTEST1(NUM, FORMAT, SIZE, ARG1) do {		\
  gnet_pack (FORMAT, buffer, SIZE, ARG1);		\
  len = gnet_pack_strdup (FORMAT, &str, ARG1);		\
  g_free (str);				} while (0)



int
main(int argc, char** argv)
{
  char buffer[1024];
  char* str;
  int len;

  gnet_init ();

  /* **************************************** */
  /* NATIVE */

  TEST0 (10100, "00", "x", 1);

  TEST1 (10200, "17", "b", 1, 0x17);
  TEST1 (10210, "f1", "b", 1, 0xf1);

  TEST1 (10300, "17", "B", 1, 0x17);
  TEST1 (10310, "f1", "B", 1, 0xf1);

#if (G_BYTE_ORDER ==  G_LITTLE_ENDIAN)	/* NATIVE LITTLE ENDIAN */

  TEST1 (10400, "0201", "h", sizeof(short), 0x0102);
  TEST1 (10410, "01f0", "h", sizeof(short), 0xf001);
  TEST1 (10420, "f001", "h", sizeof(short), 0x01f0);

  TEST1 (10500, "0201", "H", sizeof(short), 0x0102);
  TEST1 (10510, "01f0", "H", sizeof(short), 0xf001);
  TEST1 (10520, "f001", "H", sizeof(short), 0x01f0);

  TEST1 (10600, "04030201", "i", sizeof(int), 0x01020304);
  TEST1 (10610, "040302f1", "i", sizeof(int), 0xf1020304);
  TEST1 (10620, "f4030201", "i", sizeof(int), 0x010203f4);

  TEST1 (10700, "04030201", "I", sizeof(int), 0x01020304);
  TEST1 (10710, "040302f1", "I", sizeof(int), 0xf1020304);
  TEST1 (10720, "f4030201", "I", sizeof(int), 0x010203f4);

  TEST1 (10800, "04030201", "l", sizeof(int), 0x01020304);
  TEST1 (10810, "040302f1", "l", sizeof(int), 0xf1020304);
  TEST1 (10820, "f4030201", "l", sizeof(int), 0x010203f4);

  TEST1 (10900, "04030201", "L", sizeof(int), 0x01020304);
  TEST1 (10910, "040302f1", "L", sizeof(int), 0xf1020304);
  TEST1 (10920, "f4030201", "L", sizeof(int), 0x010203f4);

  if (sizeof(void*) == 4)
    {
      TEST1 (11100, "04030201", "v", sizeof(void*), (void*) 0x01020304);
      TEST1 (11110, "040302f1", "v", sizeof(void*), (void*) 0xf1020304);
      TEST1 (11120, "f4030201", "v", sizeof(void*), (void*) 0x010203f4);
    }

  TEST3 (11200, "00010002",         "bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (11210, "0403020108070605", "ii",  8, 0x01020304, 0x05060708);
  TEST2 (11220, "0403020108070605", "2i",  8, 0x01020304, 0x05060708);

#else					/* NATIVE BIG ENDIAN */

  TEST1 (10400, "0102", "h", sizeof(short), 0x0102);
  TEST1 (10410, "f001", "h", sizeof(short), 0xf001);
  TEST1 (10420, "01f0", "h", sizeof(short), 0x01f0);

  TEST1 (10500, "0102", "H", sizeof(short), 0x0102);
  TEST1 (10510, "f001", "H", sizeof(short), 0xf001);
  TEST1 (10520, "01f0", "H", sizeof(short), 0x01f0);

  TEST1 (10600, "01020304", "i", sizeof(int), 0x01020304);
  TEST1 (10610, "f1020304", "i", sizeof(int), 0xf1020304);
  TEST1 (10620, "010203f4", "i", sizeof(int), 0x010203f4);

  TEST1 (10700, "01020304", "I", sizeof(int), 0x01020304);
  TEST1 (10710, "f1020304", "I", sizeof(int), 0xf1020304);
  TEST1 (10720, "010203f4", "I", sizeof(int), 0x010203f4);

  TEST1 (10800, "01020304", "l", sizeof(int), 0x01020304);
  TEST1 (10810, "f1020304", "l", sizeof(int), 0xf1020304);
  TEST1 (10820, "010203f4", "l", sizeof(int), 0x010203f4);

  TEST1 (10900, "01020304", "L", sizeof(int), 0x01020304);
  TEST1 (10910, "f1020304", "L", sizeof(int), 0xf1020304);
  TEST1 (10920, "010203f4", "L", sizeof(int), 0x010203f4);

  if (sizeof(void*) == 4)
    {
      TEST1 (11100, "01020304", "v", sizeof(void*), (void*) 0x01020304);
      TEST1 (11110, "f1020304", "v", sizeof(void*), (void*) 0xf1020304);
      TEST1 (11120, "010203f4", "v", sizeof(void*), (void*) 0x010203f4);
    }

  TEST3 (11200, "00000102", "bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (11201, "0102030405060708", "ii", 8, 0x01020304, 0x05060708);
  TEST2 (11202, "0102030405060708", "2i", 8, 0x01020304, 0x05060708);

#endif

  MEMTEST1 (11000, "f", sizeof(float),  23.43);
  MEMTEST1 (11010, "d", sizeof(double), 43.21);
  


  /* **************************************** */
  /* LITTLE ENDIAN */

  TEST0 (20100, "00", "<x", 1);

  TEST1 (20200, "17", "<b", 1, 0x17);
  TEST1 (20210, "f1", "<b", 1, 0xf1);

  TEST1 (20300, "17", "<B", 1, 0x17);
  TEST1 (20310, "f1", "<B", 1, 0xf1);

  TEST1 (20400, "0201", "<h", 2, 0x0102);
  TEST1 (20410, "01f0", "<h", 2, 0xf001);
  TEST1 (20420, "f001", "<h", 2, 0x01f0);

  TEST1 (20500, "0201", "<H", 2, 0x0102);
  TEST1 (20510, "01f0", "<H", 2, 0xf001);
  TEST1 (20520, "f001", "<H", 2, 0x01f0);

  TEST1 (20600, "04030201", "<i", 4, 0x01020304);
  TEST1 (20610, "040302f1", "<i", 4, 0xf1020304);
  TEST1 (20620, "f4030201", "<i", 4, 0x010203f4);

  TEST1 (20700, "04030201", "<I", 4, 0x01020304);
  TEST1 (20710, "040302f1", "<I", 4, 0xf1020304);
  TEST1 (20720, "f4030201", "<I", 4, 0x010203f4);

  TEST1 (20800, "04030201", "<l", 4, 0x01020304);
  TEST1 (20810, "040302f1", "<l", 4, 0xf1020304);
  TEST1 (20820, "f4030201", "<l", 4, 0x010203f4);

  TEST1 (20900, "04030201", "<L", 4, 0x01020304);
  TEST1 (20910, "040302f1", "<L", 4, 0xf1020304);
  TEST1 (20920, "f4030201", "<L", 4, 0x010203f4);

  MEMTEST1 (21000, "<f", sizeof(float),  23.43);
  MEMTEST1 (21010, "<d", sizeof(double), 43.21);
  
  TEST3 (21100, "00010002", "<bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (21110, "0403020108070605", "<ii", 8, 0x01020304, 0x05060708);
  TEST2 (21120, "0403020108070605", "<2i", 8, 0x01020304, 0x05060708);


  /* **************************************** */
  /* BIG ENDIAN */

  TEST0 (30100, "00", ">x", 1);

  TEST1 (30200, "17", ">b", 1, 0x17);
  TEST1 (30210, "f1", ">b", 1, 0xf1);

  TEST1 (30300, "17", ">B", 1, 0x17);
  TEST1 (30310, "f1", ">B", 1, 0xf1);

  TEST1 (30400, "0102", ">h", 2, 0x0102);
  TEST1 (30410, "f001", ">h", 2, 0xf001);
  TEST1 (30420, "01f0", ">h", 2, 0x01f0);

  TEST1 (30500, "0102", ">H", 2, 0x0102);
  TEST1 (30510, "f001", ">H", 2, 0xf001);
  TEST1 (30520, "01f0", ">H", 2, 0x01f0);

  TEST1 (30600, "01020304", ">i", 4, 0x01020304);
  TEST1 (30610, "f1020304", ">i", 4, 0xf1020304);
  TEST1 (30620, "010203f4", ">i", 4, 0x010203f4);

  TEST1 (30700, "01020304", ">I", 4, 0x01020304);
  TEST1 (30710, "f1020304", ">I", 4, 0xf1020304);
  TEST1 (30720, "010203f4", ">I", 4, 0x010203f4);

  TEST1 (30800, "01020304", ">l", 4, 0x01020304);
  TEST1 (30810, "f1020304", ">l", 4, 0xf1020304);
  TEST1 (30820, "010203f4", ">l", 4, 0x010203f4);

  TEST1 (30900, "01020304", ">L", 4, 0x01020304);
  TEST1 (30910, "f1020304", ">L", 4, 0xf1020304);
  TEST1 (30920, "010203f4", ">L", 4, 0x010203f4);

  MEMTEST1 (31000, ">f", sizeof(float),  23.43);
  MEMTEST1 (31010, ">d", sizeof(double), 43.21);
  
  TEST3 (31200, "00000102", ">bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (31210, "0102030405060708", ">ii", 8, 0x01020304, 0x05060708);
  TEST2 (31220, "0102030405060708", ">2i", 8, 0x01020304, 0x05060708);


  /* **************************************** */
  /* STRINGS */

  TEST2 (40000, "68656c6c6f00746865726500", "ss", 12, "hello", "there");
  TEST2 (40010, "74686572650068656c6c6f00", "2s", 12, "there", "hello");
  TEST1 (40020, "626f6f676572000000000000", "12S", 12, "booger");
  TEST1 (40030, "64617669", "4S", 4, "david");
  TEST1 (40040, "68656c646572", "6S", 6, "helder");
  TEST2 (40050, "64636261", "r", 4, "dcba", 4);
  TEST4 (40060, "6162636465666768", "2r", 8, "abcd", 4, "efgh", 4);
  TEST1 (40070, "64636261", "4R", 4, "dcba"); 
  TEST2 (40080, "6566676861626364", "4R4R", 8, "efgh", "abcd");
  TEST1 (40090, "0461626364", "p", 5, "abcd");
  TEST2 (40100, "04656667680461626364", "2p", 10, "efgh", "abcd");

  /* **************************************** */
  /* FAILURES (these cause gnet warnings) */

#if 0
  gnet_pack ("b", buffer, 0, 0);
  gnet_pack ("B", buffer, 0, 0);
  gnet_pack ("h", buffer, sizeof(short) - 1, 0);
  gnet_pack ("H", buffer, sizeof(unsigned short) - 1, 0);
  gnet_pack ("i", buffer, sizeof(int) - 1, 0);
  gnet_pack ("I", buffer, sizeof(unsigned int) - 1, 0);
  gnet_pack ("l", buffer, sizeof(long) - 1, 0);
  gnet_pack ("L", buffer, sizeof(unsigned long) - 1, 0);
  gnet_pack ("f", buffer, sizeof(float) - 1, 0);
  gnet_pack ("d", buffer, sizeof(double) - 1, 0);
  gnet_pack ("s", buffer, 5, "hello");
  gnet_pack ("ss", buffer, 11, "hello", "world");
  gnet_pack ("12S", buffer, 11, "booger");
  gnet_pack ("4S", buffer, 3, "david");
  gnet_pack ("6S", buffer, 5, "helder");
  gnet_pack ("2r", buffer, 7, "abcd", 4, "efgh", 4);
  gnet_pack ("4R", buffer, 3, "dcba");
  gnet_pack ("4R4R", buffer, 7, "efgh", "abcd");
  gnet_pack ("p", buffer, 4, "abcd");
  gnet_pack ("2p", buffer, 9, "efgh", "abcd");
#endif

  /* **************************************** */

  if (failed)
    {
/*        fprintf (stderr, "FAIL\n"); */
      exit (1);
    }

/*    fprintf (stderr, "PASS\n"); */
  exit (0);

  return 0;
}


static gchar bits2hex[16] = { '0', '1', '2', '3', 
			      '4', '5', '6', '7',
			      '8', '9', 'a', 'b',
			      'c', 'd', 'e', 'f' };

void
test_bytes (int test_num, char* s /* binary */, int len, 
	    char* answer /* ascii */)
{
  int i;

  for (i = 0; i < len; ++i)
    {
      if (!answer[2 * i] || !answer[(2 * i) + 1] ||
	  (bits2hex[(s[i] & 0xf0) >> 4] != answer[2 * i]) ||
	  (bits2hex[(s[i] & 0xf)      ] != answer[(2 * i) + 1]))
      {
	int j;

	fprintf (stderr, "FAILURE: test #%d at byte %d\n", test_num, i);
	fprintf (stderr, "\toutput   : ");
	for (j = 0; j < len; ++j)
	  fprintf (stderr, "%c%c", bits2hex[(s[j] & 0xf0) >> 4], 
		   bits2hex[s[j] & 0xf]);
	fprintf (stderr, "\n\tshould be: %s\n", answer);
	failed = 1;
      }
    }
}

