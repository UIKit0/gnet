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


#ifdef HAVE_IPV6
static gboolean ipv6_detect_envvar (void);
static gboolean ipv6_detect_iface (void);
#endif


/**
 *  gnet_init
 *
 *  Initializes the GNet library.  This should be called at the
 *  beginning of any GNet program and before any call to gtk_init().
 *
 **/
void
gnet_init (void)
{
#ifdef G_THREADS_ENABLED
#ifndef GNET_WIN32 
  if (!g_thread_supported ()) 
    g_thread_init (NULL);
#endif
#endif /* G_THREADS_ENABLED */


  /* Auto-detect IPv6 policy.  Set it to IPv4 if auto-detection fails. */
#ifdef HAVE_IPV6
  if (!ipv6_detect_envvar())
    if (!ipv6_detect_iface())
#endif
      gnet_ipv6_set_policy (GIPV6_POLICY_IPV4_ONLY);

/*    g_print ("ipv6 policy is %d\n", gnet_ipv6_get_policy()); */

}


#ifdef HAVE_IPV6
/* 

   Try to get policy based on environment variables.  We look in the
   environment variable string for a 4 and a 6.  We set policy based
   on which appear and which appears first.

*/

static gboolean
ipv6_detect_envvar (void)
{
  const gchar* envvar;
  char* loc4;
  char* loc6;
  GIPv6Policy policy;

  envvar = g_getenv ("GNET_IPV6_POLICY");
  if (envvar == NULL)
    envvar = g_getenv ("IPV6_POLICY");
  if (envvar == NULL)
     return FALSE;

  loc4 = strchr (envvar, '4');
  loc6 = strchr (envvar, '6');

  if (loc4 && loc6)
    {
      if (loc4 < loc6)
	policy = GIPV6_POLICY_IPV4_THEN_IPV6;
      else
	policy = GIPV6_POLICY_IPV6_THEN_IPV4;
    }
  else if (loc4)
    policy = GIPV6_POLICY_IPV4_ONLY;
  else if (loc6)
    policy = GIPV6_POLICY_IPV6_ONLY;
  else
    return FALSE;

  gnet_ipv6_set_policy (policy);

  return TRUE;
}



/* 

   Auto-detect IPv6 policy.  We get a list of interfaces and set
   policy based on whether IPv4 and/or IPv6 interfaces exist.

   Note if we have both IPv4 and IPv6 that we favor IPv4 over IPv6.
   In my experience, people have better IPv4 connections than IPv6
   connections, which tend to be software tunnels.

*/
static gboolean
ipv6_detect_iface (void)
{
  GList* ifaces;
  GList* i;
  gboolean have_ipv4 = FALSE;
  gboolean have_ipv6 = FALSE;
  GIPv6Policy policy;

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
  g_list_free (ifaces); 

  if (have_ipv4 && have_ipv6)
    policy = GIPV6_POLICY_IPV4_THEN_IPV6;
  else if (have_ipv4 && !have_ipv6)
    policy = GIPV6_POLICY_IPV4_ONLY;
  else if (!have_ipv4 && have_ipv6)
    policy = GIPV6_POLICY_IPV6_ONLY;
  else
    return FALSE;

  gnet_ipv6_set_policy (policy);

  return TRUE;
}
#endif
