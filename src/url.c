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
#include "url.h"


GURL* 
gnet_url_new (const gchar* url)
{
  GURL* gurl = NULL;
  const gchar* p;
  const gchar* temp;

  g_return_val_if_fail (url, NULL);

  gurl = g_new0 (GURL, 1);

  /* Skip initial whitespace */
  while (*url != 0 && isspace(*url))
    ++url;
  p = url;

  /* Read in protocol (protocol is optional) */
  temp = p;
  while (*p != 0 && *p != ':' && *p != '/')
    ++p;
  if (*p == ':' && p[1] == '/')
    {
      gurl->protocol = g_strndup (url, p - url);
      while (*p != 0 && (*p == ':' || *p == '/')) ++p;
    }
  else	/* This is a \0 or /, so it must be the hostname */
    p = temp;

  /* Read in the hostname */
  temp = p;
  while (*p != 0 && *p != '/' && *p != ':') ++p;
  if ((p - temp) == 0) 
    {
      gnet_url_delete (gurl);
      return NULL;
    }
  gurl->hostname = g_strndup (temp, p - temp);

  /* Read in the port */
  if (*p == ':')
    {
      temp = ++p;
      while (*p != 0 && *p != '/') ++p;
      if ((p - temp) != 0)
	{
	  gchar* port_str = g_strndup (temp, p - temp);
	  gurl->port = atoi (port_str);
	  g_free (port_str);
	}
    }

  /* Read in the file */
  if (*p == 0)
    gurl->resource = g_strdup("/");
  else
    gurl->resource = g_strdup(p);

  return gurl;
}


GURL*     
gnet_url_new_fields (const gchar* protocol, const gchar* hostname, 
		   const gint port, const gchar* resource)
{
  GURL* url = NULL;

  url = g_new0 (GURL, 1);
  if (protocol)		url->protocol = g_strdup (protocol);
  if (hostname)		url->hostname = g_strdup (hostname);
  url->port = port;
  if (resource)		url->resource = g_strdup (resource);

  return url;
}


GURL*     
gnet_url_clone (const GURL* url)
{
  GURL* url2;

  g_return_val_if_fail (url, NULL);

  url2 = g_new0 (GURL, 1);
  url2->protocol = g_strdup (url->protocol);
  url2->hostname = g_strdup (url->hostname);
  url2->port = url->port;
  url2->resource = g_strdup (url->resource);

  return url2;
}


void
gnet_url_delete (GURL* url)
{
  if (url)
    {
      g_free (url->protocol);
      g_free (url->hostname);
      g_free (url->resource);
      g_free (url);
    }
}


guint
gnet_url_hash (const gpointer p)
{
  const GURL* url = (const GURL*) p;
  guint h = 0;

  g_return_val_if_fail (url, 0);

  if (url->protocol)	h =  g_str_hash (url->protocol);
  if (url->hostname)	h ^= g_str_hash (url->hostname);
  h ^= url->port;
  if (url->resource)	h ^= g_str_hash (url->resource);
  
  return h;
}


#define SAFESTRCMP(A,B) ((!(A) && !(B)) || ((A) && (B) && !strcmp((A),(B))))

gint
gnet_url_equal (const gpointer p1, const gpointer p2)
{
  const GURL* url1 = (const GURL*) p1;
  const GURL* url2 = (const GURL*) p2;

  g_return_val_if_fail (url1, 0);
  g_return_val_if_fail (url2, 0);

  if (url1->port == url2->port &&
      SAFESTRCMP(url1->protocol, url2->protocol) &&
      SAFESTRCMP(url1->hostname, url2->hostname) &&
      SAFESTRCMP(url1->resource, url2->resource))
    return 1;

  return 0;
}


void
gnet_url_set_protocol (GURL* url, const gchar* protocol)
{
  g_return_if_fail (url);

  if (url->protocol)
    {
      g_free (url->protocol);
      url->protocol = NULL;
    }

  if (protocol)
    url->protocol = g_strdup (protocol);
}


void
gnet_url_set_hostname (GURL* url, const gchar* hostname)
{
  g_return_if_fail (url);

  if (url->hostname)
    {
      g_free (url->hostname);
      url->hostname = NULL;
    }

  if (hostname)
    url->hostname = g_strdup (hostname);
}


void	
gnet_url_set_port (GURL* url, gint port)
{
  url->port = port;
}


void
gnet_url_set_resource (GURL* url, const gchar* resource)
{
  g_return_if_fail (url);

  if (url->resource)
    {
      g_free (url->resource);
      url->resource = NULL;
    }

  if (resource)
    {
      if (resource[0] == '/')
	url->resource = g_strdup (resource);
      else
	url->resource = g_strconcat ("/", resource, NULL);
    }
}


gchar*
gnet_url_get_nice_string (const GURL* url)
{
  gchar* rv = NULL;
  gchar* resource = NULL;

  g_return_val_if_fail (url, NULL);
  g_return_val_if_fail (url->hostname, NULL);

  if (url->resource)
    {
      if (*url->resource == '/')
	resource = g_strdup (url->resource);
      else
	resource = g_strconcat ("/", url->resource, NULL);
    }
  else
    resource = g_strdup ("");

  if (url->port)
    rv = g_strdup_printf ("%s:%d%s", url->hostname, url->port, resource);
  else
    {
      if (*resource)
	rv = g_strconcat (url->hostname, resource, NULL);
      else
	rv = g_strdup (url->hostname);
    }

  if (url->protocol)
    {
      gchar* temp;

      temp = g_strconcat (url->protocol, "://", rv, NULL);
      g_free (rv);
      rv = temp;
    }

  g_free (resource);

  return rv;
}
