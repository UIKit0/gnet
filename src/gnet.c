/* GNet - Networking library
 * Copyright (C) 2000  David Helder
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


#include "gnet-private.h"
#include "gnet.h"


const guint gnet_major_version = GNET_MAJOR_VERSION;
const guint gnet_minor_version = GNET_MINOR_VERSION;
const guint gnet_micro_version = GNET_MICRO_VERSION;
const guint gnet_interface_age = GNET_INTERFACE_AGE;
const guint gnet_binary_age = GNET_BINARY_AGE;


/**
 *  gnet_init:
 *
 *  Initializes the GNet library.  This should be called at the
 *  beginning of any GNet program and before any call to gtk_init().
 *
 **/
void
gnet_init (void)
{
  GList* ifaces;
  GList* i;
  gboolean have_ipv4 = FALSE;
  gboolean have_ipv6 = FALSE;
  GIPv6Policy ipv6_policy;



#ifdef G_THREADS_ENABLED
#ifndef GNET_WIN32 
  if (!g_thread_supported ()) 
    g_thread_init (NULL);
#endif
#endif /* G_THREADS_ENABLED */

  /* FIX: Check environment variable */

  /* Set sensible IPv6 policy.

     We get the list the interfaces.  If we have both IPv4 and IPv6,
     then we use IPV6_THEN_IPV4.  Otherwise, we only use what's available. 


   */
  ipv6_policy = GIPV6_POLICY_IPV4_ONLY;	/* default policy is IPv4 only */

  ifaces = gnet_inetaddr_list_interfaces ();
  for (i = ifaces; i != NULL; i = i->next)
    {
      GInetAddr* iface = (GInetAddr*) i->data;

      if (gnet_inetaddr_is_ipv4(iface))
	have_ipv4 = TRUE;
      else if (gnet_inetaddr_is_ipv6(iface))
	have_ipv6 = TRUE;

      gnet_inetaddr_delete (iface);
    }

  if (have_ipv4 && have_ipv6)
    ipv6_policy = GIPV6_POLICY_IPV6_THEN_IPV4;
  else if (have_ipv4 && !have_ipv6)
    ipv6_policy = GIPV6_POLICY_IPV4_ONLY;
  else if (!have_ipv4 && have_ipv6)
    ipv6_policy = GIPV6_POLICY_IPV6_ONLY;

  gnet_ipv6_set_policy (ipv6_policy);
}
