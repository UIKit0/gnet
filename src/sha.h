/**

   This code is free (LGPL compatible).  See the top of sha.c for
   details.

   Copyright (C) 1995  A.M. Kuchling [original author]
   Copyright (C) 2000  David Helder [GNet API]

 */


#ifndef _GNET_SHA_H
#define _GNET_SHA_H

/* This module is experimental, buggy, and unstable.  Use at your own risk. */
#ifdef GNET_EXPERIMENTAL

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */c

typedef struct _GSHA GSHA;
#define GNET_SHA_HASH_LENGTH	20	/* in bytes */


typedef gpointer GSHAAsyncID;
typedef void (*GSHAAsyncFunc)(GSHA* sha, gpointer user_data);
/* if sha is NULL, there is an error. */


GSHA*           gnet_sha_new (guint8 const* buffer, guint length);
/* TODO: GSHA*  gnet_sha_new_file (gchar* pathname); */
GSHA*		gnet_sha_new_string (gchar* str);
void            gnet_sha_delete (GSHA* gsha);

/* Not sure if we're going to keep this... */
GSHAAsyncID*    gnet_sha_new_file_async (gchar* pathname, GSHAAsyncFunc func,
					 gpointer user_data);
void		gnet_sha_new_file_async_cancel (GSHAAsyncID* id);

gint 	    	gnet_sha_equal (const gpointer p1, const gpointer p2);
guint		gnet_sha_hash (GSHA* gsha);

guint8*        	gnet_sha_get_digest (GSHA* gsha);
gchar*          gnet_sha_get_string (GSHA* gsha);

/* Copy SHA string into 2 * GNET_SHA_HASH_LENGTH byte buffer */
void	        gnet_sha_copy_string (GSHA* gsha, guint8* buffer);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_SHA_H */
