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
#include "uri.h"


/**
 *  gnet_uri_new:
 *  @uri: URI string
 *
 *  Create a #GURI from the URI argument.  Fields are set to NULL if
 *  they are not set.  The path does not necessarily start with a /.
 *  The parser is not validating -- it will accept some malformed URI.
 *  
 *  Returns: A new #GURI, or NULL if there was a failure.
 *
 **/
GURI* 
gnet_uri_new (const gchar* uri)
{
  GURI* guri = NULL;
  const gchar* p;
  const gchar* temp;

  g_return_val_if_fail (uri, NULL);

  guri = g_new0 (GURI, 1);

  /* Skip initial whitespace */
  while (*uri && isspace((int)*uri))
    ++uri;
  p = uri;

  /* Scheme */
  temp = p;
  while (*p && *p != ':' && *p != '/' && *p != '?' && *p != '#')
    ++p;
  if (*p == ':')
    {
      guri->scheme = g_strndup (uri, p - uri);
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
	    guri->password = g_strndup(p + 1, hoststart - p - 2);

	  /* User */
	  guri->user = g_strndup(temp, p - temp);
	  p = hoststart;
	}
      else
	p = temp;

      /* Hostname */
      temp = p;
      while (*p && *p != '/' && *p != '?' && *p != '#' && *p != ':') ++p;
      if ((p - temp) == 0) 
	{
	  gnet_uri_delete (guri);
	  return NULL;
	}
      guri->hostname = g_strndup (temp, p - temp);

      /* Port */
      if (*p == ':')
	{
	  for (++p; isdigit((int)*p); ++p)
	    guri->port = guri->port * 10 + (*p - '0');
	}

    }

  /* Path (we are liberal and won't check if it starts with /) */
  temp = p;
  while (*p && *p != '?' && *p != '#')
    ++p;
  if (p != temp)
    guri->path = g_strndup(temp, p - temp);

  /* Query */
  if (*p == '?')
    {
      temp = p + 1;
      while (*p && *p != '#')
        ++p;
      guri->query = g_strndup (temp, p - temp);
    }

  /* Fragment */
  if (*p == '#')
    {
      ++p;
      guri->fragment = g_strdup (p);
    }

  return guri;
}


/**
 *  gnet_uri_new_fields:
 *  @scheme: scheme
 *  @hostname: hostname
 *  @port: port
 *  @path: path
 *
 *  Create a #GURI from the fields.  This is the short version.  Use
 *  gnet_uri_new_fields_all() to specify all fields.
 *
 *  Returns: A new #GURI.
 *
 **/
GURI*     
gnet_uri_new_fields (const gchar* scheme, const gchar* hostname, 
		     const gint port, const gchar* path)
{
  GURI* uri = NULL;

  uri = g_new0 (GURI, 1);
  if (scheme)		uri->scheme = g_strdup (scheme);
  if (hostname)		uri->hostname = g_strdup (hostname);
  uri->port = port;
  if (path)		uri->path = g_strdup (path);

  return uri;
}


/**
 *  gnet_uri_new_fields_all:
 *  @scheme: scheme
 *  @user: user
 *  @password: password
 *  @hostname: hostname
 *  @port: port
 *  @path: path
 *  @query: query
 *  @fragment: fragment
 *
 *  Create a #GURI from the fields.  This is the short version.  Use
 *  gnet_uri_new_fields_all() to specify all fields.
 *
 *  Returns: A new #GURI.
 *
 **/
GURI*
gnet_uri_new_fields_all (const gchar* scheme, const gchar* user, 
			 const gchar* password, const gchar* hostname, 
			 const gint port, const gchar* path, 
			 const gchar* query, const gchar* fragment)
{
  GURI* uri = NULL;

  uri = g_new0 (GURI, 1);
  if (scheme)		uri->scheme   = g_strdup (scheme);
  if (user)		uri->user     = g_strdup (user);
  if (password)		uri->password = g_strdup (password);
  if (hostname)		uri->hostname = g_strdup (hostname);
  uri->port = port;
  if (path)		uri->path     = g_strdup (path);
  if (query)		uri->query    = g_strdup (query);
  if (fragment)		uri->fragment = g_strdup (fragment);

  return uri;
}


/**
 *  gnet_uri_clone:
 *  @uri: URI to clone.
 * 
 *  Create a URI from another one.
 *
 *  Returns: a new #GURI.
 *
 **/
GURI*     
gnet_uri_clone (const GURI* uri)
{
  GURI* uri2;

  g_return_val_if_fail (uri, NULL);

  uri2 = g_new0 (GURI, 1);
  uri2->scheme   = g_strdup (uri->scheme);
  uri2->user     = g_strdup (uri->user);
  uri2->password = g_strdup (uri->password);
  uri2->hostname = g_strdup (uri->hostname);
  uri2->port     = uri->port;
  uri2->path 	 = g_strdup (uri->path);
  uri2->query    = g_strdup (uri->query);
  uri2->fragment = g_strdup (uri->fragment);

  return uri2;
}


/** 
 *  gnet_uri_delete:
 *  @uri: #GURI to delete
 *
 *  Delete a #GURI.
 *
 **/
void
gnet_uri_delete (GURI* uri)
{
  if (uri)
    {
      g_free (uri->scheme);
      g_free (uri->user);
      g_free (uri->password);
      g_free (uri->hostname);
      g_free (uri->path);
      g_free (uri->query);
      g_free (uri->fragment);
      g_free (uri);
    }
}


/**
 *  gnet_uri_hash
 *  @p: GURI to get hash value of
 *
 *  Hash the GURI hash value.
 *
 *  Returns: hash value.
 *
 **/
guint
gnet_uri_hash (gconstpointer p)
{
  const GURI* uri = (const GURI*) p;
  guint h = 0;

  g_return_val_if_fail (uri, 0);

  if (uri->scheme)	h =  g_str_hash (uri->scheme);
  if (uri->user)	h ^= g_str_hash (uri->user);
  if (uri->password)	h ^= g_str_hash (uri->password);
  if (uri->hostname)	h ^= g_str_hash (uri->hostname);
  h ^= uri->port;
  if (uri->path)	h ^= g_str_hash (uri->path);
  if (uri->query)	h ^= g_str_hash (uri->query);
  if (uri->fragment)	h ^= g_str_hash (uri->fragment);
  
  return h;
}


#define SAFESTRCMP(A,B) (((A)&&(B))?(strcmp((A),(B))):((A)||(B)))

/**
 *  gnet_uri_equal:
 *  @p1: Pointer to first #GURI.
 *  @p2: Pointer to second #GURI.
 *
 *  Compare two #GURI's.  
 *
 *  Returns: 1 if they are the same; 0 otherwise.
 *
 **/
gint
gnet_uri_equal (gconstpointer p1, gconstpointer p2)
{
  const GURI* uri1 = (const GURI*) p1;
  const GURI* uri2 = (const GURI*) p2;

  g_return_val_if_fail (uri1, 0);
  g_return_val_if_fail (uri2, 0);

  if (uri1->port == uri2->port &&
      !SAFESTRCMP(uri1->scheme, uri2->scheme) &&
      !SAFESTRCMP(uri1->user, uri2->user) &&
      !SAFESTRCMP(uri1->password, uri2->password) &&
      !SAFESTRCMP(uri1->hostname, uri2->hostname) &&
      !SAFESTRCMP(uri1->path, uri2->path) &&
      !SAFESTRCMP(uri1->query, uri2->query) &&
      !SAFESTRCMP(uri1->fragment, uri2->fragment))
    return 1;

  return 0;
}


/**
 *  gnet_uri_set_scheme:
 *  @uri: uri
 *  @scheme: scheme
 *
 *  Set the scheme in the URI.
 *
 **/
void
gnet_uri_set_scheme (GURI* uri, const gchar* scheme)
{
  g_return_if_fail (uri);

  if (uri->scheme)
    {
      g_free (uri->scheme);
      uri->scheme = NULL;
    }

  if (scheme)
    uri->scheme = g_strdup (scheme);
}


/**
 *  gnet_uri_set_user:
 *  @uri: uri
 *  @user: user
 *
 *  Set the user in the URI.
 *
 **/
void
gnet_uri_set_user (GURI* uri, const gchar* user)
{
  g_return_if_fail (uri);

  if (uri->user)
    {
      g_free (uri->user);
      uri->user = NULL;
    }

  if (user)
    uri->user = g_strdup (user);
}


/**
 *  gnet_uri_set_password:
 *  @uri: uri
 *  @password: password
 *
 *  Set the password in the URI.
 *
 **/
void
gnet_uri_set_password (GURI* uri, const gchar* password)
{
  g_return_if_fail (uri);

  if (uri->password)
    {
      g_free (uri->password);
      uri->password = NULL;
    }

  if (password)
    uri->password = g_strdup (password);
}


/**
 *  gnet_uri_set_hostname:
 *  @uri: uri
 *  @hostname: hostname
 *
 *  Set the hostname in the URI.
 *
 **/
void
gnet_uri_set_hostname (GURI* uri, const gchar* hostname)
{
  g_return_if_fail (uri);

  if (uri->hostname)
    {
      g_free (uri->hostname);
      uri->hostname = NULL;
    }

  if (hostname)
    uri->hostname = g_strdup (hostname);
}


/**
 *  gnet_uri_set_port:
 *  @uri: uri
 *  @port: port
 *
 *  Set the port in the URI.
 *
 **/
void	
gnet_uri_set_port (GURI* uri, gint port)
{
  uri->port = port;
}


/**
 *  gnet_uri_set_path:
 *  @uri: uri
 *  @path: path
 *
 *  Set the path in the URI.
 *
 **/
void
gnet_uri_set_path (GURI* uri, const gchar* path)
{
  g_return_if_fail (uri);

  if (uri->path)
    {
      g_free (uri->path);
      uri->path = NULL;
    }

  if (path)
    uri->path = g_strdup (path);
}



/**
 *  gnet_uri_set_query:
 *  @uri: uri
 *  @query: query
 *
 *  Set the query in the URI.
 *
 **/
void
gnet_uri_set_query (GURI* uri, const gchar* query)
{
  g_return_if_fail (uri);

  if (uri->query)
    {
      g_free (uri->query);
      uri->query = NULL;
    }

  if (query)
    uri->query = g_strdup (query);
}


/**
 *  gnet_uri_set_fragment:
 *  @uri: uri
 *  @fragment: fragment
 *
 *  Set the fragment in the URI.
 *
 **/
void
gnet_uri_set_fragment (GURI* uri, const gchar* fragment)
{
  g_return_if_fail (uri);

  if (uri->fragment)
    {
      g_free (uri->fragment);
      uri->fragment = NULL;
    }

  if (fragment)
    uri->fragment = g_strdup (fragment);
}


/**
 *  gnet_uri_get_nice_string:
 *  @uri: URI
 *
 *  Convert the URI to a human-readable string.  Currently, no
 *  escaping or unescaping is done.
 *
 *  Returns: Nice, caller-owned string.
 *
 **/
gchar*
gnet_uri_get_nice_string (const GURI* uri)
{
  gchar* rv = NULL;
  GString* buffer = NULL;
  
  g_return_val_if_fail (uri, NULL);

  buffer = g_string_sized_new (16);

  if (uri->scheme)
    g_string_sprintfa (buffer, "%s:", uri->scheme);

  if (uri->user || uri->password || uri->hostname || uri->port)
    g_string_append (buffer, "//");

  if (uri->user)
    {
      buffer = g_string_append (buffer, uri->user);

      if (uri->password)
	g_string_sprintfa (buffer, ":%s", uri->password);

      buffer = g_string_append_c (buffer, '@');
    }

  if (uri->hostname)
    buffer = g_string_append (buffer, uri->hostname); 

  if (uri->port)
    g_string_sprintfa (buffer, ":%d", uri->port);

  if (uri->path)
    {
      if (*uri->path == '/' ||
	  !(uri->user || uri->password || uri->hostname || uri->port))
	g_string_append (buffer, uri->path);
      else
	g_string_sprintfa (buffer, "/%s", uri->path);
    }

  if (uri->query)
    g_string_sprintfa (buffer, "?%s", uri->query);

  if (uri->fragment)
    g_string_sprintfa (buffer, "#%s", uri->fragment);
  
  /* Free only GString not data contained, return the data instead */
  rv = buffer->str;
  g_string_free (buffer, FALSE); 
  return rv;
}
