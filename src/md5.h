/**

   This code is in the public domain.  See md5.c for details.

   Authors:
     Colin Plumb [original author]
     David Helder [GNet API]

 */


#ifndef _GNET_MD5_H
#define _GNET_MD5_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GMD5 GMD5;
#define GNET_MD5_HASH_LENGTH	16	/* in bytes */


GMD5*   gnet_md5_new (const guchar* buffer, guint length);
GMD5*	gnet_md5_new_string (const gchar* str);
GMD5*   gnet_md5_clone (const GMD5* gmd5);
void    gnet_md5_delete (GMD5* gmd5);
	
GMD5*	gnet_md5_new_incremental (void);
void	gnet_md5_update (GMD5* gmd5, const guchar* buffer, guint length);
void	gnet_md5_final (GMD5* gmd5);
	
gint 	gnet_md5_equal (gconstpointer p1, gconstpointer p2);
guint	gnet_md5_hash (gconstpointer p);
	
guint8* gnet_md5_get_digest (const GMD5* gmd5);
gchar*  gnet_md5_get_string (const GMD5* gmd5);
	
void	gnet_md5_copy_string (const GMD5* gmd5, guchar* buffer);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_MD5_H */
