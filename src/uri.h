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


#ifndef _GNET_URI_H
#define _GNET_URI_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* 
   TODO: 

   Handle escaping.  Internally, fields should be unescaped.

   Handle base URIs.  (e.g. gnet_new_uri_from_base (base, path).)

   Add docs.

   Add:
     // Escape and unescape string
     gnet_uri_escape_string (gchar* string);
     gnet_uri_unescape_string (gchar* string);

     // Escape and unescape fields.  When a URI is created, we assume
     // the fields are unescaped (should we?).  This would toggle
     // this.
     gnet_uri_escape_all (GURI* uri);
     gnet_uri_unescape_all (GURI* uri);
     
     // Get escaped string.
     gnet_uri_get_string(GURI* uri);  

     // Set default port.  If the URI port is 80 and the default
     //   port is set to 80, get_string won't print the port.
     gnet_uri_set_default_port (GURI* uri, gint port);	???

   See RFC: ftp://ftp.isi.edu/in-notes/rfc2396.txt

*/

typedef struct _GURI
{
  gchar* scheme;
  gchar* user;
  gchar* password;
  gchar* hostname;
  gint   port;
  gchar* path;
  gchar* query;
  gchar* fragment;
} GURI;


GURI*     gnet_uri_new (const gchar* uri);
GURI*     gnet_uri_new_fields (const gchar* protocol, const gchar* hostname, 
			       const gint port, const gchar* resource);
GURI*	  gnet_uri_new_fields_all (const gchar* protocol, const gchar* user, 
				   const gchar* password, const gchar* hostname, 
				   const gint port, const gchar* resource, 
				   const gchar* query, const gchar* fragment);
GURI*     gnet_uri_clone (const GURI* uri);
void      gnet_uri_delete (GURI* uri);

guint     gnet_uri_hash (gconstpointer p);
gint 	  gnet_uri_equal (gconstpointer p1, gconstpointer p2);

void 	  gnet_uri_set_scheme   (GURI* uri, const gchar* protocol);
void 	  gnet_uri_set_user 	(GURI* uri, const gchar* user);
void 	  gnet_uri_set_password (GURI* uri, const gchar* password);
void 	  gnet_uri_set_hostname (GURI* uri, const gchar* hostname);
void 	  gnet_uri_set_port     (GURI* uri, gint port);
void 	  gnet_uri_set_path	(GURI* uri, const gchar* resource);
void 	  gnet_uri_set_query 	(GURI* uri, const gchar* query);
void 	  gnet_uri_set_fragment (GURI* uri, const gchar* fragment);
	       
gchar* 	  gnet_uri_get_nice_string (const GURI* uri);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_URI_H */
