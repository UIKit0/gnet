/* Test GNet Base64 (deterministic, computation-based tests only)
 * Copyright (C) 2003 Alfred Reibenschuh
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

#include <glib.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _internal_test
#include "../src/base64.h"
#include "../src/base64.c"
#else
#include <gnet.h>
#endif



static int failed = 0;
gchar *sesame    = "Aladdin:open sesame and many many more so come here and see the gold";
gchar *aladin    = "QWxhZGRpbjpvcGVuIHNlc2FtZSBhbmQgbWFueSBtYW55IG1vcmUgc28gY29tZSBoZXJlIGFuZCBzZWUgdGhlIGdvbGQ=";
gchar *aladin_cr = "QWxhZGRpbjpvcGVuIHNlc2FtZSBhbmQgbWFueSBtYW55IG1vcmUgc28gY29tZSBoZXJlIGFu\nZCBzZWUgdGhlIGdvbGQ=";

#define TEST(S, C) do {                             	  \
if (C) { /*g_print ("%s: PASS\n", (S));*/         	} \
else   { g_print ("%s: FAIL\n", (S)); failed = 1;  	} \
} while (0)


int
main (int argc, char* argv[])
{
  gchar* buf1;
  gchar* buf2;
  gint  len1,len2;
  len1=0; 
  buf1=gnet_base64_encode(sesame,strlen(sesame),&len1,0);
  /* length includes the final \0 */
  TEST("base64 encode/length",len1==strlen(aladin)+1);
  TEST("base64 encode/buffer",!strcmp(buf1,aladin));
#ifdef _internal_test
  g_print("org='%s' len=%i\n",aladin,strlen(aladin));
  g_print("act='%s' len=%i\n",buf1,len1);
#endif
  g_free(buf1);
  len1=0; 
  buf1=gnet_base64_encode(sesame,strlen(sesame),&len1,1);
  /* length includes the final \0 */
  TEST("base64 encode/length/strict",len1==strlen(aladin_cr)+1);
  TEST("base64 encode/buffer/strict",!strcmp(buf1,aladin_cr));
#ifdef _internal_test
  g_print("org='%s' len=%i\n",aladin_cr,strlen(aladin_cr));
  g_print("act='%s' len=%i\n",buf1,len1);
#endif
  g_free(buf1);
  len2=0; 
  buf2=gnet_base64_decode(aladin,strlen(aladin),&len2);
  TEST("base64 decode/length",len2==strlen(sesame));
  TEST("base64 decode/buffer",!strcmp(buf2,sesame));
#ifdef _internal_test
  g_print("org='%s' len=%i\n",sesame,strlen(sesame));
  g_print("act='%s' len=%i\n",buf2,len2);
#endif
  g_free(buf2);
  len2=0; 
  buf2=gnet_base64_decode(aladin_cr,strlen(aladin_cr),&len2);
  TEST("base64 decode/length/strict",len2==strlen(sesame));
  TEST("base64 decode/buffer/strict",!strcmp(buf2,sesame));
#ifdef _internal_test
  g_print("org='%s' len=%i\n",sesame,strlen(sesame));
  g_print("act='%s' len=%i\n",buf2,len2);
#endif
  g_free(buf2);
  /* to test for bug reported on mailing list around 24 Oct 2004 */
  len2=0; 
  buf2=gnet_base64_encode("A\x84\0",3,&len2,TRUE);
  TEST("base64 encode/length/strict",len2==strlen("QYQA")+1);
  TEST("base64 encode/buffer/strict",buf2 && !strcmp(buf2,"QYQA"));
  len1=0;    
  buf1=gnet_base64_decode(buf2,len2,&len1);
  TEST("base64 decode/length",len1==3);
  TEST("base64 decode/buffer",buf1 && !strncmp(buf1,"A\x84\0",3));
  g_free(buf1);
  g_free(buf2);

  if (failed)
    exit (1);

  exit (0);

  return 0;
}
