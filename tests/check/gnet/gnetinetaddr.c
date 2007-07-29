/* GNet GInetAddr unit test (deterministic, computation-based tests only)
 * Copyright (C) 2002 David Helder
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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
#include "gnetcheck.h"

#include <string.h>

/*** IPv4 tests */

GNET_START_TEST (test_inetaddr_ipv4)
{
  GInetAddr* inetaddr;
  GInetAddr* inetaddr2;
  gchar* cname;
  gchar* cname2;
  gchar bytes[GNET_INETADDR_MAX_LEN];

  /* new (canonical, IPv4) */
  inetaddr = gnet_inetaddr_new ("141.213.11.124", 23);
  fail_unless (inetaddr != NULL);
  
  /* get_port */
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string (cname, "141.213.11.124");
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  fail_unless (inetaddr2 != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  fail_unless (cname2 != NULL);
  fail_unless_equals_string (cname2, "141.213.11.124");
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr2), 42);
  g_free(cname2);

  /* equal, noport_equal */
  fail_unless (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  fail_if (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  fail_unless_equals_int (gnet_inetaddr_hash (inetaddr), 2379549526u);
  /* hash port */
  fail_unless (gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x8d\xd5\xb\x7c", 4);
  fail_unless (inetaddr != NULL);

  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string ("141.213.11.124", cname);
  g_free (cname);

  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 0);
  gnet_inetaddr_set_port (inetaddr, 2345);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);

  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 4);

  gnet_inetaddr_set_bytes (inetaddr, "\x7c\xb\xd5\x8d", 4);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string ("124.11.213.141", cname);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);
  g_free (cname);

  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 4);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  fail_unless (memcmp(bytes, "\x7c\xb\xd5\x8d", 4) == 0);

#ifdef HAVE_IPV6

  /* IPv4 -> IPv6 via set_bytes */
  gnet_inetaddr_set_bytes (inetaddr, "\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
			   "\0\0\0\0"         "\0\0\0\x6f", 16);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  g_free (cname);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);
  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 16);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  fail_unless (!memcmp(bytes, "\x3f\xfe\x0b\x00\x0c\x18\x1f\xff\0\0\0\0\0\0\0\x6f", 16));

#endif

  gnet_inetaddr_delete (inetaddr);
}
GNET_END_TEST;

#define IS_TEST(S,A,F) do {                                     \
  GInetAddr* _inetaddr;                                         \
  gchar* _cname;                                                \
  _inetaddr = gnet_inetaddr_new_nonblock (A, 0);                \
  fail_unless (_inetaddr != NULL);                              \
  _cname = gnet_inetaddr_get_canonical_name (_inetaddr);        \
  fail_unless (_cname != NULL);                                 \
  fail_unless_equals_string (_cname, (A));                      \
  fail_unless (F (_inetaddr));                                  \
  g_free (_cname);                                              \
  gnet_inetaddr_delete (_inetaddr);                             \
} while (0)

GNET_START_TEST (test_inetaddr_is_ipv4)
{
  /* IPv4 */
  IS_TEST ("IPv4", "141.213.11.124", 	     	  gnet_inetaddr_is_ipv4);

  /* IPv4 Loopback */
  IS_TEST ("IPv4 Loopback",  "127.0.0.1",     	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 Loopback2", "127.23.42.129", 	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 !Loopback", "128.23.42.129", 	  !gnet_inetaddr_is_loopback);

  /* IPv4 Multicast */
  IS_TEST ("IPv4 Multicast",   "224.0.0.0",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast2",  "224.0.0.1",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast3",  "239.255.255.255", gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast",  "223.255.255.255", !gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast2", "240.0.0.0", 	  !gnet_inetaddr_is_multicast);

  /* IPv4 Private */
  IS_TEST ("IPv4 Private",   	"10.0.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private2",   	"10.255.255.255",  gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private",   	"9.255.255.255",   !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private2",   	"11.0.0.0", 	   !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private3",   	"172.16.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private4",   	"172.31.255.255",  gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private3",   	"172.15.255.255",  !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private4",   	"172.32.0.0",      !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private5",   	"192.168.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private6",   	"192.168.255.255", gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private5",   	"192.167.255.255", !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private6",   	"192.169.0.0",     !gnet_inetaddr_is_private);


  /* IPv4 Reserved */
  IS_TEST ("IPv4 Reserved",   	"0.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved2",   	"0.0.255.255", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved2",   "1.0.0.0", 	   !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved3",   	"240.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved4",   	"247.255.255.255", gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved3",   "239.255.255.255", !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved4",   "248.0.0.0", 	   !gnet_inetaddr_is_reserved);

  /* Internet */
  IS_TEST ("Internet1", "141.213.11.124", 	   gnet_inetaddr_is_internet);
}
GNET_END_TEST;

#if HAVE_IPV6

GNET_START_TEST (test_inetaddr_is_ipv6)
{
  /* IPv6 */
  IS_TEST ("!IPv4", "3ffe:b00:c18:1fff::6f", 	  !gnet_inetaddr_is_ipv4);
  IS_TEST ("IPv6", "3ffe:b00:c18:1fff::6f",  	  gnet_inetaddr_is_ipv6);
  IS_TEST ("!IPv6", "141.213.11.124", 	     	  !gnet_inetaddr_is_ipv6);

  /* IPv6 Loopback */
  IS_TEST ("IPv6 Loopback",   "::1", 	      	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback",  "::",           	  !gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback2", "::201",        	  !gnet_inetaddr_is_loopback);

  /* IPv6 Multicast */
  IS_TEST ("IPv6 Multicast",   "ffff::1",         gnet_inetaddr_is_multicast);
  IS_TEST ("IPv6 !Multicast",  "feff::1",         !gnet_inetaddr_is_multicast);

  /* IPv6 Broadcast */
  IS_TEST ("IPv6 Broadcast",   "255.255.255.255", gnet_inetaddr_is_broadcast);
  IS_TEST ("IPv6 !Broadcast",  "255.255.255.254", !gnet_inetaddr_is_broadcast);

  /* IPv6 Private */
  IS_TEST ("IPv6 Private",   	"fe80::",     	   gnet_inetaddr_is_private);
  IS_TEST ("IPv6 Private2",   	"fecf:ffff::",     gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private",   	"fe7f:ffff::",     !gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private2",   	"ff00::",          !gnet_inetaddr_is_private);

  /* IPv6 Reserved */
  IS_TEST ("IPv6 Reserved",   	"::",     	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv6 !Reserved",   	"1::",     	   !gnet_inetaddr_is_reserved);

  /* Internet */
  IS_TEST ("Internet2", "3ffe:b00:c18:1fff::6f",   gnet_inetaddr_is_internet);
  IS_TEST ("!Internet1",  "255.255.255.255",       !gnet_inetaddr_is_internet);
  IS_TEST ("!Internet2",  "ffff::1",     	   !gnet_inetaddr_is_internet);
}
GNET_END_TEST;

#endif /* HAVE_IPV6 */


/*** IPv6 tests ***/

#if HAVE_IPV6
GNET_START_TEST (test_inetaddr_ipv6)
{
  GInetAddr* inetaddr;
  GInetAddr* inetaddr2;
  gchar* cname;
  gchar* cname2;

  /* new (canonical, IPv6) */
  inetaddr = gnet_inetaddr_new ("3ffe:b00:c18:1fff::6f", 23);
  fail_unless (inetaddr != NULL);
  
  /* get_port */
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp(cname, "3ffe:b00:c18:1fff::6f"));
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  fail_unless (inetaddr != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  fail_unless (cname2 != NULL);
  fail_unless (!strcasecmp(cname2, "3ffe:b00:c18:1fff::6f"));
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr2), 42);
  g_free(cname2);

  /* equal, noport_equal */
  fail_unless (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  fail_if (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  fail_unless_equals_int (gnet_inetaddr_hash (inetaddr), 870716602u);
  /* hash port */
  fail_unless (gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
				      "\0\0\0\0"         "\0\0\0\x6f", 16);
  fail_unless (inetaddr != NULL);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 0);
  gnet_inetaddr_delete (inetaddr);
  g_free (cname);
}
GNET_END_TEST;
#endif /* HAVE_IPV6 */

GNET_START_TEST (test_inetaddr_is_internet_domainname)
{
  fail_unless (gnet_inetaddr_is_internet_domainname ("speak.eecs.umich.edu"));
  fail_unless (gnet_inetaddr_is_internet_domainname ("141.213.11.124"));
  fail_if (gnet_inetaddr_is_internet_domainname ("localhost"));
  fail_if (gnet_inetaddr_is_internet_domainname ("localhost.localdomain"));
  fail_if (gnet_inetaddr_is_internet_domainname ("speak"));
}
GNET_END_TEST;

static Suite *
gnetinetaddr_suite (void)
{
  Suite *s = suite_create ("GInetAddr");
  TCase *tc_chain = tcase_create ("inetaddr");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_inetaddr_is_internet_domainname);
  tcase_add_test (tc_chain, test_inetaddr_ipv4);
  tcase_add_test (tc_chain, test_inetaddr_is_ipv4);
#if HAVE_IPV6
  tcase_add_test (tc_chain, test_inetaddr_ipv6);
  tcase_add_test (tc_chain, test_inetaddr_is_ipv6);
#endif

  return s;
}

GNET_CHECK_MAIN (gnetinetaddr);

