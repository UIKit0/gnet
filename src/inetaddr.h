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


#ifndef _GNET_INETADDR_H
#define _GNET_INETADDR_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*

  All fields in GInetAddr are private and should be accessed only by
  using the functions below.

 */
typedef struct _GInetAddr GInetAddr;




/* ********** */

/**
 *   GInetAddrAsyncStatus:
 * 
 *   Status of a asynchronous lookup (from gnet_inetaddr_new_async()
 *   or gnet_inetaddr_get_name_async()), passed by GInetAddrAsyncFunc.
 *   More errors may be added in the future, so it's best to compare
 *   against %GINETADDR_ASYNC_STATUS_OK.
 *
 **/
typedef enum {
  GINETADDR_ASYNC_STATUS_OK,
  GINETADDR_ASYNC_STATUS_ERROR
} GInetAddrAsyncStatus;



/**
 *   GInetAddrAsyncFunc:
 *   @inetaddr: InetAddr that was looked up
 *   @status: Status of the lookup
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_async.
 *
 **/
typedef void (*GInetAddrAsyncFunc)(GInetAddr* inetaddr, 
				   GInetAddrAsyncStatus status, 
				   gpointer data);



/**
 *   GInetAddrReverseAsyncFunc:
 *   @inetaddr: InetAddr whose was looked up
 *   @status: Status of the lookup
 *   @name: Nice name of the address
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_async.  Delete the name when
 *   you're done with it.
 *
 **/
typedef void (*GInetAddrReverseAsyncFunc)(GInetAddr* inetaddr, 
					  GInetAddrAsyncStatus status, 
					  gchar* name,
					  gpointer data);




/* ********** */

GInetAddr* gnet_inetaddr_new(const gchar* name, const gint port);

gpointer gnet_inetaddr_new_async(const gchar* name, const gint port, 
				 GInetAddrAsyncFunc func, gpointer data);

void gnet_inetaddr_new_async_cancel(gpointer id);

GInetAddr* gnet_inetaddr_clone(const GInetAddr* ia);

void gnet_inetaddr_delete(GInetAddr* ia);

void gnet_inetaddr_ref(GInetAddr* ia);

void gnet_inetaddr_unref(GInetAddr* ia);


/* ********** */

gchar* gnet_inetaddr_get_name(GInetAddr* ia);

gpointer gnet_inetaddr_get_name_async(GInetAddr* ia, 
				      GInetAddrReverseAsyncFunc func,
				      gpointer data);

void gnet_inetaddr_get_name_async_cancel(gpointer id);

gchar* gnet_inetaddr_get_canonical_name(GInetAddr* ia);

gint gnet_inetaddr_get_port(const GInetAddr* ia);

void gnet_inetaddr_set_port(const GInetAddr* ia, guint port);


/* ********** */

guint gnet_inetaddr_hash(const gpointer p);

gint gnet_inetaddr_equal(const gpointer p1, const gpointer p2);

gint gnet_inetaddr_noport_equal(const gpointer p1, const gpointer p2);


/* ********** */

gchar* gnet_inetaddr_gethostname(void);

GInetAddr* gnet_inetaddr_gethostaddr(void);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_INETADDR_H */
