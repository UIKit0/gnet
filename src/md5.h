/**

   This code is in the public domain.  See md5.c for details.

   Authors:
     Colin Plumb [original author]
     David Helder [GNet API]

 */


#ifndef _GNET_MD5_H
#define _GNET_MD5_H

/* This module is experimental, buggy, and unstable.  Use at your own risk. */
#ifdef GNET_EXPERIMENTAL 

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GMD5 GMD5;
#define GNET_MD5_HASH_LENGTH	16	/* in bytes */

GMD5*           gnet_md5_new (const guchar* buffer, guint length);
GMD5*		gnet_md5_new_string (gchar* str);
void            gnet_md5_delete (GMD5* gmd5);

gint 	    	gnet_md5_equal (const gpointer p1, const gpointer p2);
guint		gnet_md5_hash (GMD5* gmd5);

guchar*        	gnet_md5_get_digest (GMD5* gmd5);
gchar*          gnet_md5_get_string (GMD5* gmd5);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_MD5_H */
