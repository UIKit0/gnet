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


#ifndef _GNET_URL_H
#define _GNET_URL_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* 
   TODO: 

   Clean this up.
   Handle URL's per RFC and other relevant documents.
   Handle escape converting.
   Add docs.
   Add:
     gnet_url_get_canonical_string(GURL* url);		??? hmm...
     gnet_url_set_default_port (GURL* url, gint port);	???

*/

typedef struct _GURL
{
  gchar* protocol;
  gchar* hostname;
  gint port;
  gchar* resource;

} GURL;


GURL*     gnet_url_new (const gchar* url);
GURL*     gnet_url_new_fields (const gchar* protocol, const gchar* hostname, 
			       const gint port, const gchar* resource);
GURL*     gnet_url_clone (const GURL* url);
void      gnet_url_delete (GURL* url);
	       
guint     gnet_url_hash (const gpointer p);
gint 	  gnet_url_equal (const gpointer p1, const gpointer p2);

void 	  gnet_url_set_protocol (GURL* url, const gchar* protocol);
void 	  gnet_url_set_hostname (GURL* url, const gchar* hostname);
void 	  gnet_url_set_port     (GURL* url, gint port);
void 	  gnet_url_set_resource (GURL* url, const gchar* resource);
	       
gchar* 	  gnet_url_get_nice_string (const GURL* url);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_URL_H */
