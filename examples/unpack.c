/* Gnet-Unpack test/example
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
  char buf[12] =  {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 
		   0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  char bufh[12] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 
		   0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb};
  char hello[20] = "hello\0there";
  char hellop[12] = {0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f,
		     0x05, 0x74, 0x68, 0x65, 0x72, 0x65 };

  char   igint8, igint8_2;
  unsigned char  iguint8;
  short  igint16;
  unsigned short iguint16;
  int  igint32, igint32_2;
  unsigned int iguint32;
  long  lgint32;
  unsigned long lguint32;
  float floaty;
  double doubley;
  void* voidp;
  char* s1, *s2;

  /* **************************************** */

  printf ("************\n");
  printf ("native tests (assumes little endian)\n");

  gnet_unpack ("x", buf, 1);
  printf ("\noutput:    00\n");
  printf ("should be: 00\n");

  /* ********** */

  igint8 = 0x2a;
  gnet_unpack ("b", &buf[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: 1\n");

  igint8 = 0x2a;
  gnet_unpack ("b", &bufh[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: fffffff1\n");

  /* ********** */

  iguint8 = 0x2a;
  gnet_unpack ("B", &buf[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: 1\n");

  iguint8 = 0x2a;
  gnet_unpack ("B", &bufh[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: f1\n");

  /* ********** */

  igint16 = 0x2a2a;
  gnet_unpack ("h", &buf[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: 201\n");

  igint16 = 0x2a2a;
  gnet_unpack ("h", &bufh[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: fffff2f1\n");

  /* ********** */

  iguint16 = 0x2a2a;
  gnet_unpack ("H", &buf[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: 201\n");

  iguint16 = 0x2a2a;
  gnet_unpack ("H", &bufh[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: f2f1\n");


  /* ********** */

  igint32 = 0x2a2a;
  gnet_unpack ("i", &buf[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: 4030201\n");

  igint32 = 0x2a2a;
  gnet_unpack ("i", &bufh[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  iguint32 = 0x2a2a;
  gnet_unpack ("I", &buf[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: 4030201\n");

  iguint32 = 0x2a2a;
  gnet_unpack ("I", &bufh[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  lgint32 = 0x2a2a;
  gnet_unpack ("l", &buf[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: 4030201\n");

  lgint32 = 0x2a2a;
  gnet_unpack ("l", &bufh[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  lguint32 = 0x2a2a;
  gnet_unpack ("L", &buf[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: 4030201\n");

  lguint32 = 0x2a2a;
  gnet_unpack ("L", &bufh[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: f4f3f2f1\n");


  /* ********** */

  floaty = 1;
  gnet_unpack ("f", &buf[1], sizeof(float), &floaty);
  printf ("\noutput:    %f\n", floaty);
  printf ("should be: ?\n");

  doubley = 1;
  gnet_unpack ("d", &bufh[1], sizeof(double), &doubley);
  printf ("\noutput:    %f (size = %d)\n", doubley, sizeof(double));
  printf ("should be: ?\n");


  /* ********** */

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack ("v", &buf[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: 4030201\n");

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack ("v", &bufh[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: f4f3f2f1\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("little endian tests\n");

  gnet_unpack ("<x", buf, 1);
  printf ("\noutput:    00\n");
  printf ("should be: 00\n");

  /* ********** */

  igint8 = 0x2a;
  gnet_unpack ("<b", &buf[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: 1\n");

  igint8 = 0x2a;
  gnet_unpack ("<b", &bufh[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: fffffff1\n");

  /* ********** */

  iguint8 = 0x2a;
  gnet_unpack ("<B", &buf[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: 1\n");

  iguint8 = 0x2a;
  gnet_unpack ("<B", &bufh[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: f1\n");

  /* ********** */

  igint16 = 0x2a2a;
  gnet_unpack ("<h", &buf[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: 201\n");

  igint16 = 0x2a2a;
  gnet_unpack ("<h", &bufh[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: fffff2f1\n");

  /* ********** */

  iguint16 = 0x2a2a;
  gnet_unpack ("<H", &buf[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: 201\n");

  iguint16 = 0x2a2a;
  gnet_unpack ("<H", &bufh[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: f2f1\n");


  /* ********** */

  igint32 = 0x2a2a;
  gnet_unpack ("<i", &buf[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: 4030201\n");

  igint32 = 0x2a2a;
  gnet_unpack ("<i", &bufh[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  iguint32 = 0x2a2a;
  gnet_unpack ("<I", &buf[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: 4030201\n");

  iguint32 = 0x2a2a;
  gnet_unpack ("<I", &bufh[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  lgint32 = 0x2a2a;
  gnet_unpack ("<l", &buf[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: 4030201\n");

  lgint32 = 0x2a2a;
  gnet_unpack ("<l", &bufh[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: f4f3f2f1\n");

  /* ********** */

  lguint32 = 0x2a2a;
  gnet_unpack ("<L", &buf[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: 4030201\n");

  lguint32 = 0x2a2a;
  gnet_unpack ("<L", &bufh[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: f4f3f2f1\n");


  /* ********** */

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack ("<P", &buf[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: 4030201\n");

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack ("<P", &bufh[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: f4f3f2f1\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("big endian tests\n");

  gnet_unpack (">x", buf, 1);
  printf ("\noutput:    00\n");
  printf ("should be: 00\n");

  /* ********** */

  igint8 = 0x2a;
  gnet_unpack (">b", &buf[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: 1\n");

  igint8 = 0x2a;
  gnet_unpack (">b", &bufh[1], 1, &igint8);
  printf ("\noutput:    %x\n", igint8);
  printf ("should be: fffffff1\n");

  /* ********** */

  iguint8 = 0x2a;
  gnet_unpack (">B", &buf[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: 1\n");

  iguint8 = 0x2a;
  gnet_unpack (">B", &bufh[1], 1, &iguint8);
  printf ("\noutput:    %x\n", iguint8);
  printf ("should be: f1\n");

  /* ********** */

  igint16 = 0x2a2a;
  gnet_unpack (">h", &buf[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: 102\n");

  igint16 = 0x2a2a;
  gnet_unpack (">h", &bufh[1], 2, &igint16);
  printf ("\noutput:    %x\n", igint16);
  printf ("should be: fffff1f2\n");

  /* ********** */

  iguint16 = 0x2a2a;
  gnet_unpack (">H", &buf[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: 102\n");

  iguint16 = 0x2a2a;
  gnet_unpack (">H", &bufh[1], 2, &iguint16);
  printf ("\noutput:    %x\n", iguint16);
  printf ("should be: f1f2\n");


  /* ********** */

  igint32 = 0x2a2a;
  gnet_unpack (">i", &buf[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: 1020304\n");

  igint32 = 0x2a2a;
  gnet_unpack (">i", &bufh[1], 4, &igint32);
  printf ("\noutput:    %x\n", igint32);
  printf ("should be: f1f2f3f4\n");

  /* ********** */

  iguint32 = 0x2a2a;
  gnet_unpack (">I", &buf[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: 1020304\n");

  iguint32 = 0x2a2a;
  gnet_unpack (">I", &bufh[1], 4, &iguint32);
  printf ("\noutput:    %x\n", iguint32);
  printf ("should be: f1f2f3f4\n");

  /* ********** */

  lgint32 = 0x2a2a;
  gnet_unpack (">l", &buf[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: 1020304\n");

  lgint32 = 0x2a2a;
  gnet_unpack (">l", &bufh[1], 4, &lgint32);
  printf ("\noutput:    %x\n", lgint32);
  printf ("should be: f1f2f3f4\n");

  /* ********** */

  lguint32 = 0x2a2a;
  gnet_unpack (">L", &buf[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: 1020304\n");

  lguint32 = 0x2a2a;
  gnet_unpack (">L", &bufh[1], 4, &lguint32);
  printf ("\noutput:    %x\n", lguint32);
  printf ("should be: f1f2f3f4\n");


  /* ********** */

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack (">P", &buf[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: 1020304\n");

  voidp = (void*) 0xaaaaaaaa;
  gnet_unpack (">P", &bufh[1], sizeof(void*), &voidp);
  printf ("\noutput:    %x\n", voidp);
  printf ("should be: f1f2f3f4\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("native combinations (assumes little endian)\n");

  igint8 = igint16 = igint8_2 = 0;
  gnet_unpack ("bhb", &buf[1], 4, &igint8, &igint16, &igint8_2);
  printf ("\noutput:    %x %x %x\n", igint8, igint16, igint8_2);
  printf ("should be: 1 302 4\n");

  igint32 = igint32_2 = 0;
  gnet_unpack ("ii", &buf[1], 8, &igint32, &igint32_2);
  printf ("\noutput:    %x %x\n", igint32, igint32_2);
  printf ("should be: 4030201 8070605\n");

  igint32 = igint32_2 = 0;
  gnet_unpack ("2i", &buf[1], 8, &igint32, &igint32_2);
  printf ("\noutput:    %x %x\n", igint32, igint32_2);
  printf ("should be: 4030201 8070605\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("big endian combinations\n");

  igint8 = igint16 = igint8_2 = 0;
  gnet_unpack (">bhb", &buf[1], 4, &igint8, &igint16, &igint8_2);
  printf ("\noutput:    %x %x %x\n", igint8, igint16, igint8_2);
  printf ("should be: 1 203 4\n");

  igint32 = igint32_2 = 0;
  gnet_unpack (">ii", &buf[1], 8, &igint32, &igint32_2);
  printf ("\noutput:    %x %x\n", igint32, igint32_2);
  printf ("should be: 1020304 5060708\n");

  igint32 = igint32_2 = 0;
  gnet_unpack (">2i", &buf[1], 8, &igint32, &igint32_2);
  printf ("\noutput:    %x %x\n", igint32, igint32_2);
  printf ("should be: 1020304 5060708\n");


  /* **************************************** */

  printf ("\n\n********************\n");
  printf ("strings\n");

  s1 = s2 = NULL;
  gnet_unpack ("ss", hello, 12, &s1, &s2);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  s1 = s2 = NULL;
  gnet_unpack ("2s", hello, 12, &s1, &s2);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  s1 = NULL;
  gnet_unpack ("8S", hello, 6, &s1);
  printf ("\noutput:    %s (%d)\n", s1, s1[7]);
  printf ("should be: hello (0)\n");

  s1 = NULL;
  gnet_unpack ("6S", hello, 6, &s1);
  printf ("\noutput:    %s\n", s1);
  printf ("should be: hello\n");

  s1 = NULL;
  gnet_unpack ("4S", hello, 6, &s1);
  printf ("\noutput:    %s\n", s1);
  printf ("should be: hell\n");

  s1 = s2 = NULL;
  gnet_unpack ("6S6S", hello, 12, &s1, &s2);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  s1 = s2 = NULL;
  gnet_unpack ("r", hello, 6, &s1, 6);
  printf ("\noutput:    %s\n", s1);
  printf ("should be: hello\n");

  s1 = s2 = NULL;
  gnet_unpack ("2r", hello, 12, &s1, 6, &s2, 6);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  s1 = NULL;
  gnet_unpack ("6R", hello, 6, &s1);
  printf ("\noutput:    %s\n", s1);
  printf ("should be: hello\n");

  s1 = s2 = NULL;
  gnet_unpack ("6R6R", hello, 12, &s1, &s2);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  s1 = NULL;
  gnet_unpack ("p", hellop, 6, &s1);
  printf ("\noutput:    %s\n", s1);
  printf ("should be: hello\n");

  s1 = s2 = NULL;
  gnet_unpack ("2p", hellop, 12, &s1, &s2);
  printf ("\noutput:    %s %s\n", s1, s2);
  printf ("should be: hello there\n");

  return 0;
}

