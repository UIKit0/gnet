/* GNet - Networking library
 * Copyright (C) 2002  David Helder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef _GNET_IPV6_H
#define _GNET_IPV6_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *  GIPv6Policy
 *  @GIPV6_POLICY_IPV4_THEN_IPV6: Use IPv4, then IPv6
 *  @GIPV6_POLICY_IPV6_THEN_IPV4: Use IPv6, then IPv4
 *  @GIPV6_POLICY_IPV4_ONLY: Use IPv4 only
 *  @GIPV6_POLICY_IPV6_ONLY: Use IPv6 only
 * 
 *  Policy for IPv6 use in GNet.  This affects domain name resolution
 *  only.  A name can be resolved to several addresses.  Most
 *  programs will use the first address in the list.  The problem
 *  then is what order should the list be in if there are both IPv4
 *  and IPv6 addresses in the list.  For example, if the system
 *  cannot connect to IPv6 hosts then an IPv6 address should not be
 *  first in the list.
 *  
 *  gnet_init() attempts to set a reasonable default based on the
 *  interfaces available on the host.  If there are only IPv4
 *  interfaces, the policy is set to %GIPV6_POLICY_IPV4_ONLY.  If
 *  there are only IPv6 interfaces, the policy is set to
 *  %GIPV6_POLICY_IPV6_ONLY.  If there are both, the policy is
 *  set to %GIPV6_POLICY_SYSTEM_DEFAULT.
 *
 **/
typedef enum {
  GIPV6_POLICY_IPV4_THEN_IPV6,
  GIPV6_POLICY_IPV6_THEN_IPV4,
  GIPV6_POLICY_IPV4_ONLY,
  GIPV6_POLICY_IPV6_ONLY
} GIPv6Policy;


GIPv6Policy	gnet_ipv6_get_policy (void);
void       	gnet_ipv6_set_policy (GIPv6Policy policy);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_IPV6_H */
