/* GNet - Networking library
 * Copyright (C) 2000-2001  David Helder, David Bolcsfoldi
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


/**
 *  gnet_url_new:
 *  @url: URL string
 *
 *  Create a #GURL from the URL argument.  Fields are set to NULL if
 *  they are not set.  The path does not necessarily start with a /.
 *  The parser is not validating -- it will accept some malformed URL.
 *  
 *  Returns: A new #GURL, or NULL if there was a failure.
 *
 **/
GURL* 
gnet_url_new (const gchar* url)
{
  GURL* gurl = NULL;
  const gchar* p;
  const gchar* temp;

  g_return_val_if_fail (url, NULL);

  gurl = g_new0 (GURL, 1);

  /* Skip initial whitespace */
  while (*url && isspace(*url))
    ++url;
  p = url;

  /* Scheme */
  temp = p;
  while (*p && *p != ':' && *p != '/' && *p != '?' && *p != '#')
    ++p;
  if (*p == ':')
    {
      gurl->protocol = g_strndup (url, p - url);
      ++p;
    }
  else	/* This is a \0 or /, so it must be the hostname */
    p = temp;

  /* Authority */
  if (*p == '/' && p[1] == '/')
    {
      p += 2;

      /* Look for username and password */
      temp = p;
      while (*p && *p != '@' && *p != '/' ) /* Look for @ or / */
	++p;
      if (*p == '@') /* Found user and possibly password */
	{
	  const gchar* hoststart;

	  hoststart = p + 1;
	  p = temp;
          
	  /* Password */
	  while (*p != 0 && *p != ':' && *p != '@')
	    ++p;
	  if (*p == ':') /* Has password */
	    gurl->password = g_strndup(p + 1, hoststart - p - 2);

	  /* User */
	  gurl->user = g_strndup(temp, p - temp);
	  p = hoststart;
	}
      else
	p = temp;

      /* Hostname */
      temp = p;
      while (*p && *p != '/' && *p != '?' && *p != '#' && *p != ':') ++p;
      if ((p - temp) == 0) 
	{
	  gnet_url_delete (gurl);
	  return NULL;
	}
      gurl->hostname = g_strndup (temp, p - temp);

      /* Port */
      if (*p == ':')
	{
	  for (++p; isdigit(*p); ++p)
	    gurl->port = gurl->port * 10 + (*p - '0');
	}

    }

  /* Path (we are liberal and won't check if it starts with /) */
  temp = p;
  while (*p && *p != '?' && *p != '#')
    ++p;
  if (p != temp)
    gurl->resource = g_strndup(temp, p - temp);

  /* Query */
  if (*p == '?')
    {
      temp = p + 1;
      while (*p && *p != '#')
        ++p;
      gurl->query = g_strndup (temp, p - temp);
    }

  /* Fragment */
  if (*p == '#')
    {
      ++p;
      gurl->fragment = g_strdup (p);
    }

  return gurl;
}


/**
 *  gnet_url_new_fields:
 *  @protocol: protocol
 *  @hostname: hostname
 *  @port: port
 *  @resource: path
 *
 *  Create a #GURL from the fields.  This is the short version.  Use
 *  gnet_url_new_fields_all() to specify all fields.
 *
 *  Returns: A new #GURL.
 *
 **/
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


/**
 *  gnet_url_new_fields:
 *  @protocol: protocol
 *  @user: user
 *  @password: password
 *  @hostname: hostname
 *  @port: port
 *  @resource: path
 *  @query: query
 *  @fragment: fragment
 *
 *  Create a #GURL from the fields.  This is the short version.  Use
 *  gnet_url_new_fields_all() to specify all fields.
 *
 *  Returns: A new #GURL.
 *
 **/
GURL*
gnet_url_new_fields_all (const gchar* protocol, const gchar* user, 
			 const gchar* password, const gchar* hostname, 
			 const gint port, const gchar* resource, 
			 const gchar* query, const gchar* fragment)
{
  GURL* url = NULL;

  url = g_new0 (GURL, 1);
  if (protocol)		url->protocol = g_strdup (protocol);
  if (user)		url->hostname = g_strdup (user);
  if (password)		url->hostname = g_strdup (password);
  if (hostname)		url->hostname = g_strdup (hostname);
  url->port = port;
  if (resource)		url->resource = g_strdup (resource);
  if (query)		url->resource = g_strdup (query);
  if (fragment)		url->resource = g_strdup (fragment);

  return url;
}


/**
 *  gnet_url_clone:
 *  @url: URL to clone.
 * 
 *  Create a URL from another one.
 *
 *  Returns: a new #GURL.
 *
 **/
GURL*     
gnet_url_clone (const GURL* url)
{
  GURL* url2;

  g_return_val_if_fail (url, NULL);

  url2 = g_new0 (GURL, 1);
  url2->protocol = g_strdup (url->protocol);
  url2->user     = g_strdup (url->user);
  url2->password = g_strdup (url->password);
  url2->hostname = g_strdup (url->hostname);
  url2->port     = url->port;
  url2->resource = g_strdup (url->resource);
  url2->query    = g_strdup (url->query);
  url2->fragment = g_strdup (url->fragment);

  return url2;
}


/** 
 *  gnet_url_delete:
 *  @url: #GURL to delete
 *
 *  Delete a #GURL.
 *
 **/
void
gnet_url_delete (GURL* url)
{
  if (url)
    {
      g_free (url->protocol);
      g_free (url->user);
      g_free (url->password);
      g_free (url->hostname);
      g_free (url->resource);
      g_free (url->query);
      g_free (url->fragment);
      g_free (url);
    }
}


/**
 *  gnet_url_hash
 *  @p: GURL to get hash value of
 *
 *  Hash the GURL hash value.
 *
 *  Returns: hash value.
 *
 **/
guint
gnet_url_hash (gconstpointer p)
{
  const GURL* url = (const GURL*) p;
  guint h = 0;

  g_return_val_if_fail (url, 0);

  if (url->protocol)	h =  g_str_hash (url->protocol);
  if (url->user)	h ^= g_str_hash (url->user);
  if (url->password)	h ^= g_str_hash (url->password);
  if (url->hostname)	h ^= g_str_hash (url->hostname);
  h ^= url->port;
  if (url->resource)	h ^= g_str_hash (url->resource);
  if (url->query)	h ^= g_str_hash (url->query);
  if (url->fragment)	h ^= g_str_hash (url->fragment);
  
  return h;
}


#define SAFESTRCMP(A,B) (((A)&&(B))?(strcmp((A),(B))):((A)||(B)))

/**
 *  gnet_url_equal:
 *  @p1: Pointer to first #GURL.
 *  @p2: Pointer to second #GURL.
 *
 *  Compare two #GURL's.  
 *
 *  Returns: 1 if they are the same; 0 otherwise.
 *
 **/
gint
gnet_url_equal (gconstpointer p1, gconstpointer p2)
{
  const GURL* url1 = (const GURL*) p1;
  const GURL* url2 = (const GURL*) p2;

  g_return_val_if_fail (url1, 0);
  g_return_val_if_fail (url2, 0);

  if (url1->port == url2->port &&
      !SAFESTRCMP(url1->protocol, url2->protocol) &&
      !SAFESTRCMP(url1->user, url2->user) &&
      !SAFESTRCMP(url1->password, url2->password) &&
      !SAFESTRCMP(url1->hostname, url2->hostname) &&
      !SAFESTRCMP(url1->resource, url2->resource) &&
      !SAFESTRCMP(url1->query, url2->query) &&
      !SAFESTRCMP(url1->fragment, url2->fragment))
    return 1;

  return 0;
}


/**
 *  gnet_url_set_protocol:
 *  @url: url
 *  @protocol: protocol
 *
 *  Set the protocol in the URL.
 *
 **/
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


/**
 *  gnet_url_set_user:
 *  @url: url
 *  @user: user
 *
 *  Set the user in the URL.
 *
 **/
void
gnet_url_set_user (GURL* url, const gchar* user)
{
  g_return_if_fail (url);

  if (url->user)
    {
      g_free (url->user);
      url->user = NULL;
    }

  if (user)
    url->user = g_strdup (user);
}


/**
 *  gnet_url_set_password:
 *  @url: url
 *  @password: password
 *
 *  Set the password in the URL.
 *
 **/
void
gnet_url_set_password (GURL* url, const gchar* password)
{
  g_return_if_fail (url);

  if (url->password)
    {
      g_free (url->password);
      url->password = NULL;
    }

  if (password)
    url->password = g_strdup (password);
}


/**
 *  gnet_url_set_hostname:
 *  @url: url
 *  @hostname: hostname
 *
 *  Set the hostname in the URL.
 *
 **/
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


/**
 *  gnet_url_set_port:
 *  @url: url
 *  @port: port
 *
 *  Set the port in the URL.
 *
 **/
void	
gnet_url_set_port (GURL* url, gint port)
{
  url->port = port;
}


/**
 *  gnet_url_set_resource:
 *  @url: url
 *  @resource: path
 *
 *  Set the path in the URL.
 *
 **/
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
    url->resource = g_strdup (resource);
}



/**
 *  gnet_url_set_query:
 *  @url: url
 *  @query: query
 *
 *  Set the query in the URL.
 *
 **/
void
gnet_url_set_query (GURL* url, const gchar* query)
{
  g_return_if_fail (url);

  if (url->query)
    {
      g_free (url->query);
      url->query = NULL;
    }

  if (query)
    url->query = g_strdup (query);
}


/**
 *  gnet_url_set_fragment:
 *  @url: url
 *  @fragment: fragment
 *
 *  Set the fragment in the URL.
 *
 **/
void
gnet_url_set_fragment (GURL* url, const gchar* fragment)
{
  g_return_if_fail (url);

  if (url->fragment)
    {
      g_free (url->fragment);
      url->fragment = NULL;
    }

  if (fragment)
    url->fragment = g_strdup (fragment);
}


/**
 *  gnet_url_get_nice_string:
 *  @url: URL
 *
 *  Convert the URL to a human-readable string.  Currently, no
 *  escaping or unescaping is done.
 *
 *  Returns: Nice, caller-owned string.
 *
 **/
gchar*
gnet_url_get_nice_string (const GURL* url)
{
  gchar* rv = NULL;
  GString* buffer = NULL;
  
  g_return_val_if_fail (url, NULL);

  buffer = g_string_sized_new (16);

  if (url->protocol)
    g_string_sprintfa (buffer, "%s:", url->protocol);

  if (url->user || url->password || url->hostname || url->port)
    g_string_append (buffer, "//");

  if (url->user)
    {
      buffer = g_string_append (buffer, url->user);

      if (url->password)
	g_string_sprintfa (buffer, ":%s", url->password);

      buffer = g_string_append_c (buffer, '@');
    }

  if (url->hostname)
    buffer = g_string_append (buffer, url->hostname); 

  if (url->port)
    g_string_sprintfa (buffer, ":%d", url->port);

  if (url->resource)
    {
      if (*url->resource == '/' ||
	  !(url->user || url->password || url->hostname || url->port))
	g_string_append (buffer, url->resource);
      else
	g_string_sprintfa (buffer, "/%s", url->resource);
    }

  if (url->query)
    g_string_sprintfa (buffer, "?%s", url->query);

  if (url->fragment)
    g_string_sprintfa (buffer, "#%s", url->fragment);
  
  /* Free only GString not data contained, return the data instead */
  rv = buffer->str;
  g_string_free (buffer, FALSE); 
  return rv;
}
