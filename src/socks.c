/* GNet - Networking library
 * Copyright (C) 2001-2002  David Helder
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

#include "socks-private.h"
#include "socks.h"

G_LOCK_DEFINE_STATIC (socks);
static GInetAddr* socks_server = NULL;
static gboolean   socks_enabled = FALSE;


/**
 *  gnet_socks_get_enabled
 *
 *  Determines whether SOCKS support is enabled.
 *
 *  Returns: TRUE if SOCKS is enabled, FALSE otherwise.
 *
 **/
gboolean
gnet_socks_get_enabled (void)
{
  return socks_enabled;
}


/**
 *  gnet_socks_set_enabled
 *  @enabled: is SOCKS support enabled?
 *
 *  Sets whether SOCKS support is enabled.
 *
 **/
void
gnet_socks_set_enabled (gboolean enabled)
{
  G_LOCK (socks);

  socks_enabled = enabled;

  G_UNLOCK (socks);
}



/**
 *  gnet_socks_get_server
 *
 *  Gets the address of the SOCKS server.  This function checks the
 *  gnet_socks_set_server() value and, if not set, the SOCKS_SERVER
 *  environment variable.  The SOCKS_SERVER enviroment variable should
 *  be in the form HOSTNAME or HOSTNAME:PORT.
 *
 *  Returns: copy of the address or NULL if there is no server.
 *
 **/
GInetAddr*
gnet_socks_get_server (void)
{
  GInetAddr* rv = NULL;

  /* Auto-detect socks server */
  if (!socks_server)
    {
      const gchar* var;

      /* Check SOCKS_SERVER env variable */
      if ((var = g_getenv("SOCKS_SERVER"))) 
	{
	  gchar* hostname;
	  gint port = GNET_SOCKS_PORT;
	  int i;
	  GInetAddr* addr;
	  
	  for (i = 0; var[i] && var[i] != ':'; ++i) 
	    ;
	  if (i == 0) 
	    return NULL;
	  hostname = g_strndup (var, i);

	  if (var[i])
	    {
	      char* ep;
	      port = (gint) strtoul(&var[i+1], &ep, 10);
	      if (*ep != '\0')
		{
		  g_free (hostname);
		  return NULL;
		}
	    }

	  addr = gnet_inetaddr_new (hostname, port);

	  G_LOCK (socks);
	  if (!socks_server)
	    socks_server = addr;
	  G_UNLOCK (socks);
	}
    }

  /* Return copy of socks server */
  G_LOCK (socks);
  if (socks_server)
    rv = gnet_inetaddr_clone (socks_server);
  G_UNLOCK (socks);

  return rv;
}


/**
 *  gnet_socks_set_server:
 *  @ia: SOCKS server address
 *
 *  Sets the address of the SOCKS server.
 *
 **/
void
gnet_socks_set_server (const GInetAddr* ia)
{
  g_return_if_fail (ia);

  G_LOCK (socks);

  if (socks_server)
    gnet_inetaddr_delete (socks_server);
  socks_server = gnet_inetaddr_clone (ia);

  G_UNLOCK (socks);
}
