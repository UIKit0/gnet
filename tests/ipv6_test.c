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


int
main (int argc, char* argv[])
{
  GIPv6Policy ipv6_policy;
  gchar* policy_str = NULL;

  gnet_init ();

  ipv6_policy = gnet_ipv6_get_policy();
  gnet_ipv6_set_policy(ipv6_policy);

  switch (ipv6_policy)
    {
    case GIPV6_POLICY_IPV4_THEN_IPV6:
      policy_str = "GIPV6_POLICY_IPV4_THEN_IPV6";
      break;
    case GIPV6_POLICY_IPV6_THEN_IPV4:
      policy_str = "GIPV6_POLICY_IPV6_THEN_IPV4";
      break;
    case GIPV6_POLICY_IPV4_ONLY:
      policy_str = "GIPV6_POLICY_IPV4_ONLY";
      break;
    case GIPV6_POLICY_IPV6_ONLY:
      policy_str = "GIPV6_POLICY_IPV6_ONLY";
      break;
    default:
      failed = 1;
    }	

/*   if (policy_str) */
/*     g_print ("%s\n", policy_str); */

  if (failed)
    exit (1);

  exit (0);

  return 0;
}
