/* Test GNet Inetaddrs (deterministic, computation-based tests only)
 * Copyright (C) 2002  David Helder
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnet.h>


static int failed = 0;

#define TEST(S, C) do {                             	  \
if (C) { /*g_print ("%s: PASS\n", (S)); */        	} \
else   { g_print ("%s: FAIL\n", (S)); failed = 1;  	} \
} while (0)

#define IS_TEST(S, A, F) do {				        \
  GInetAddr* _inetaddr;						\
  gchar* _cname;						\
  _inetaddr = gnet_inetaddr_new_nonblock (A, 0);		\
  TEST(S " (inetaddr new)", _inetaddr != NULL);			\
  _cname = gnet_inetaddr_get_canonical_name (_inetaddr);	\
  TEST(S " (get cname)", _cname != NULL);			\
/*  g_print ("%s\n", _cname);*/					\
  TEST(S " (inetaddr == cname)", !strcmp(_cname, (A)));	\
  TEST(S, F(_inetaddr));					\
  g_free (_cname);						\
  gnet_inetaddr_delete (_inetaddr);				\
} while (0)


int
main (int argc, char* argv[])
{
  GInetAddr* inetaddr;
  GInetAddr* inetaddr2;
  gchar* cname;
  gchar* cname2;
  gchar bytes[GNET_INETADDR_MAX_LEN];

  /* **************************************** */
  /* IPv4 tests */

  /* new (canonical, IPv4) */
  inetaddr = gnet_inetaddr_new ("141.213.11.124", 23);
  TEST ("new", inetaddr != NULL);
  
  /* get_port */
  TEST ("get port", gnet_inetaddr_get_port (inetaddr) == 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  TEST ("set port", gnet_inetaddr_get_port (inetaddr) == 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("cname != NULL", cname != NULL);
  TEST ("cname == 141.213.11.124", !strcmp(cname, "141.213.11.124"));
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  TEST ("clone", inetaddr != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  TEST ("cname2 != NULL", cname2 != NULL);
  TEST ("cname2 == 141.213.11.124", !strcmp(cname2, "141.213.11.124"));
  TEST ("inetaddr2 port", gnet_inetaddr_get_port (inetaddr2) == 42);
  g_free(cname2);

  /* equal, noport_equal */
  TEST ("equal", gnet_inetaddr_equal (inetaddr, inetaddr2));
  TEST ("equal, no port", gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  TEST ("not equal", !gnet_inetaddr_equal (inetaddr, inetaddr2));
  TEST ("equal, no port", gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  TEST ("hash", gnet_inetaddr_hash (inetaddr) == 2379549526u);
  TEST ("hash port", gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x8d\xd5\xb\x7c", 4);
  TEST ("new_bytes", inetaddr);

  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("new_bytes cname", cname);
  TEST ("new_bytes addr", !strcmp("141.213.11.124", cname));
  g_free (cname);

  TEST ("new_bytes port", gnet_inetaddr_get_port (inetaddr) == 0);
  gnet_inetaddr_set_port (inetaddr, 2345);
  TEST ("new_bytes port2", gnet_inetaddr_get_port (inetaddr) == 2345);

  TEST ("new_bytes length", gnet_inetaddr_get_length (inetaddr) == 4);

  gnet_inetaddr_set_bytes (inetaddr, "\x7c\xb\xd5\x8d", 4);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("set_bytes cname", cname);
  TEST ("set_bytes addr", !strcmp("124.11.213.141", cname));
  TEST ("set_bytes port", gnet_inetaddr_get_port (inetaddr) == 2345);
  g_free (cname);

  TEST ("new_bytes length2", gnet_inetaddr_get_length (inetaddr) == 4);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  TEST ("get_bytes addr", !memcmp(bytes, "\x7c\xb\xd5\x8d", 4));

  /* IPv4 -> IPv6 via set_bytes */
  gnet_inetaddr_set_bytes (inetaddr, "\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
			   "\0\0\0\0"         "\0\0\0\x6f", 16);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("set_bytes cname6", cname);
  TEST ("set_bytes addr6", !strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  TEST ("set_bytes port6", gnet_inetaddr_get_port (inetaddr) == 2345);

  TEST ("new_bytes length6", gnet_inetaddr_get_length (inetaddr) == 16);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  TEST ("get_bytes addr", !memcmp(bytes, "\x3f\xfe\x0b\x00\x0c\x18\x1f\xff\0\0\0\0\0\0\0\x6f", 16));

  g_free (cname);
  gnet_inetaddr_delete (inetaddr);


  /* **************************************** */
  /* IPv6 tests */

  /* new (canonical, IPv4) */
  inetaddr = gnet_inetaddr_new ("3ffe:b00:c18:1fff::6f", 23);
  TEST ("new", inetaddr != NULL);
  
  /* get_port */
  TEST ("get port", gnet_inetaddr_get_port (inetaddr) == 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  TEST ("set port", gnet_inetaddr_get_port (inetaddr) == 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("cname != NULL", cname != NULL);
  TEST ("cname == original", !strcasecmp(cname, "3ffe:b00:c18:1fff::6f"));
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  TEST ("clone", inetaddr != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  TEST ("cname2 != NULL", cname2 != NULL);
  TEST ("cname2 == original", !strcasecmp(cname2, "3ffe:b00:c18:1fff::6f"));
  TEST ("inetaddr2 port", gnet_inetaddr_get_port (inetaddr2) == 42);
  g_free(cname2);

  /* equal, noport_equal */
  TEST ("equal", gnet_inetaddr_equal (inetaddr, inetaddr2));
  TEST ("equal, no port", gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  TEST ("not equal", !gnet_inetaddr_equal (inetaddr, inetaddr2));
  TEST ("equal, no port", gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  TEST ("hash2", gnet_inetaddr_hash (inetaddr) == 870716602u);
  TEST ("hash2 port", gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
				      "\0\0\0\0"         "\0\0\0\x6f", 16);
  TEST ("new_bytes", inetaddr);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  TEST ("new_bytes cname", cname);
  TEST ("new_bytes addr", !strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  TEST ("new_bytes port", gnet_inetaddr_get_port (inetaddr) == 0);
  gnet_inetaddr_delete (inetaddr);
  g_free (cname);



  /* **************************************** */
  /* is_XXXX tests			      */

  /* IPv4 */
  IS_TEST ("IPv4", "141.213.11.124", 	     	  gnet_inetaddr_is_ipv4);
  IS_TEST ("!IPv4", "3ffe:b00:c18:1fff::6f", 	  !gnet_inetaddr_is_ipv4);

  /* IPv6 */
  IS_TEST ("IPv6", "3ffe:b00:c18:1fff::6f",  	  gnet_inetaddr_is_ipv6);
  IS_TEST ("!IPv6", "141.213.11.124", 	     	  !gnet_inetaddr_is_ipv6);


  /* IPv4 Loopback */
  IS_TEST ("IPv4 Loopback",  "127.0.0.1",     	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 Loopback2", "127.23.42.129", 	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 !Loopback", "128.23.42.129", 	  !gnet_inetaddr_is_loopback);

  /* IPv6 Loopback */
  IS_TEST ("IPv6 Loopback",   "::1", 	      	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback",  "::",           	  !gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback2", "::201",        	  !gnet_inetaddr_is_loopback);

  /* IPv4 Multicast */
  IS_TEST ("IPv4 Multicast",   "224.0.0.0",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast2",  "224.0.0.1",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast3",  "239.255.255.255", gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast",  "223.255.255.255", !gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast2", "240.0.0.0", 	  !gnet_inetaddr_is_multicast);

  /* IPv6 Multicast */
  IS_TEST ("IPv6 Multicast",   "ffff::1",         gnet_inetaddr_is_multicast);
  IS_TEST ("IPv6 !Multicast",  "feff::1",         !gnet_inetaddr_is_multicast);

  /* IPv4 Broadcast */
  IS_TEST ("IPv6 Broadcast",   "255.255.255.255", gnet_inetaddr_is_broadcast);
  IS_TEST ("IPv6 !Broadcast",  "255.255.255.254", !gnet_inetaddr_is_broadcast);

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

  /* IPv6 Private */
  IS_TEST ("IPv6 Private",   	"fe80::",     	   gnet_inetaddr_is_private);
  IS_TEST ("IPv6 Private2",   	"fecf:ffff::",     gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private",   	"fe7f:ffff::",     !gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private2",   	"ff00::",          !gnet_inetaddr_is_private);


  /* IPv4 Reserved */
  IS_TEST ("IPv4 Reserved",   	"0.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved2",   	"0.0.255.255", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved2",   "1.0.0.0", 	   !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved3",   	"240.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved4",   	"247.255.255.255", gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved3",   "239.255.255.255", !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved4",   "248.0.0.0", 	   !gnet_inetaddr_is_reserved);

  /* IPv6 Reserved */
  IS_TEST ("IPv6 Reserved",   	"::",     	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv6 !Reserved",   	"1::",     	   !gnet_inetaddr_is_reserved);

  /* Internet */
  IS_TEST ("Internet1", "141.213.11.124", 	   gnet_inetaddr_is_internet);
  IS_TEST ("Internet2", "3ffe:b00:c18:1fff::6f",   gnet_inetaddr_is_internet);
  IS_TEST ("!Internet1",  "255.255.255.255",       !gnet_inetaddr_is_internet);
  IS_TEST ("!Internet2",  "ffff::1",     	   !gnet_inetaddr_is_internet);


  /* **************************************** */
  /* Other tests				*/
  TEST ("domainname", gnet_inetaddr_is_internet_domainname ("speak.eecs.umich.edu"));
  TEST ("domainname2", gnet_inetaddr_is_internet_domainname ("141.213.11.124"));
  TEST ("!domainname1", !gnet_inetaddr_is_internet_domainname ("localhost"));
  TEST ("!domainname2", !gnet_inetaddr_is_internet_domainname ("localhost.localdomain"));
  TEST ("!domainname3", !gnet_inetaddr_is_internet_domainname ("speak"));

  if (failed)
    exit (1);

  exit (0);

  return 0;
}
