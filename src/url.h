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


#ifndef _GNET_URL_H
#define _GNET_URL_H

/* This module is experimental, buggy, and unstable.  Use at your own risk. */
#ifdef GNET_EXPERIMENTAL 

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* 
   TODO: 

   Handle escaping.  Internally, fields should be unescaped.

   Handle base URLs.  (e.g. gnet_new_url_from_base (base, path).)

   Add docs.

   Add:
     // Escape and unescape string
     gnet_url_escape_string (gchar* string);
     gnet_url_unescape_string (gchar* string);

     // Escape and unescape fields.  When a URL is created, we assume
     // the fields are unescaped (should we?).  This would toggle
     // this.
     gnet_url_escape_all (GURL* url);
     gnet_url_unescape_all (GURL* url);
     
     // Get escaped string.
     gnet_url_get_string(GURL* url);  

     // Set default port.  If the URL port is 80 and the default
     //   port is set to 80, get_string won't print the port.
     gnet_url_set_default_port (GURL* url, gint port);	???

   See RFC: ftp://ftp.isi.edu/in-notes/rfc2396.txt

*/

typedef struct _GURL
{
  gchar* protocol;
  gchar* hostname;
  gint   port;
  gchar* resource;
  gchar* user;
  gchar* password;
  gchar* query;
  gchar* fragment;
} GURL;
  /* TODO: rename "protocol" -> "scheme"? */
  /* TODO: rename "resource" -> "path" */
  /* TODO: Put fields in order.  Don't for now to preserve binary compat. */


GURL*     gnet_url_new (const gchar* url);
GURL*     gnet_url_new_fields (const gchar* protocol, const gchar* hostname, 
			       const gint port, const gchar* resource);
GURL*	  gnet_url_new_fields_all (const gchar* protocol, const gchar* user, 
				   const gchar* password, const gchar* hostname, 
				   const gint port, const gchar* resource, 
				   const gchar* query, const gchar* fragment);
GURL*     gnet_url_clone (const GURL* url);
void      gnet_url_delete (GURL* url);

guint     gnet_url_hash (gconstpointer p);
gint 	  gnet_url_equal (gconstpointer p1, gconstpointer p2);

void 	  gnet_url_set_protocol (GURL* url, const gchar* protocol);
void 	  gnet_url_set_user 	(GURL* url, const gchar* user);
void 	  gnet_url_set_password (GURL* url, const gchar* password);
void 	  gnet_url_set_hostname (GURL* url, const gchar* hostname);
void 	  gnet_url_set_port     (GURL* url, gint port);
void 	  gnet_url_set_resource (GURL* url, const gchar* resource);
void 	  gnet_url_set_query 	(GURL* url, const gchar* query);
void 	  gnet_url_set_fragment (GURL* url, const gchar* fragment);
	       
gchar* 	  gnet_url_get_nice_string (const GURL* url);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_URL_H */
