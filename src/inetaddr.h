/* GNet - Networking library
 * Copyright (C) 2000, 2002  David Helder
 * Copyright (C) 2000  Andrew Lanoix
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

#ifdef   GNET_WIN32
#include <winsock2.h>	/* This needs to be here */
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *  GInetAddr
 *
 *  GInetAddr is an internet address.
 *
 **/
typedef struct _GInetAddr GInetAddr;




/* ********** */


/**
 *   GInetAddrNewAsyncID:
 * 
 *   ID of an asynchronous GInetAddr creation/lookup started with
 *   gnet_inetaddr_new_async().  The creation can be canceled by
 *   calling gnet_inetaddr_new_async_cancel() with the ID.
 *
 **/
typedef gpointer GInetAddrNewAsyncID;



/**
 *   GInetAddrNewAsyncFunc:
 *   @inetaddr: InetAddr that was looked up (callee owned)
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_async().  Callee owns the address.
 *   The address will be NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrNewAsyncFunc)(GInetAddr* inetaddr, 
				      gpointer data);



/**
 *   GInetAddrNewListAsyncID:
 * 
 *   ID of an asynchronous GInetAddr list creation/lookup started with
 *   gnet_inetaddr_new_list_async().  The creation can be canceled by
 *   calling gnet_inetaddr_new_list_async_cancel() with the ID.
 *
 **/
typedef gpointer GInetAddrNewListAsyncID;



/**
 *   GInetAddrNewListAsyncFunc:
 *   @list: List of GInetAddr's (callee owned)
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_list_async().  Callee owns the
 *   list of GInetAddrs.  The list is NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrNewListAsyncFunc)(GList* list, gpointer data);




/* ********** */

GInetAddr* gnet_inetaddr_new (const gchar* hostname, gint port);


GInetAddrNewAsyncID 
           gnet_inetaddr_new_async (const gchar* hostname, gint port, 
				    GInetAddrNewAsyncFunc func, 
				    gpointer data);
void       gnet_inetaddr_new_async_cancel (GInetAddrNewAsyncID id);


GList*     gnet_inetaddr_new_list (const gchar* hostname, gint port);
void	   gnet_inetaddr_delete_list (GList* list);

GInetAddrNewListAsyncID 
           gnet_inetaddr_new_list_async (const gchar* hostname, gint port, 
					 GInetAddrNewListAsyncFunc func, 
					 gpointer data);
void       gnet_inetaddr_new_list_async_cancel (GInetAddrNewListAsyncID id);


GInetAddr* gnet_inetaddr_new_nonblock (const gchar* hostname, gint port);

GInetAddr* gnet_inetaddr_new_bytes (const guint8* addr, const guint length);

GInetAddr* gnet_inetaddr_clone (const GInetAddr* ia);

void       gnet_inetaddr_delete (GInetAddr* ia);

void 	   gnet_inetaddr_ref (GInetAddr* ia);
void 	   gnet_inetaddr_unref (GInetAddr* ia);


/* ********** */

/**
 *   GInetAddrGetNameAsyncID:
 * 
 *   ID of an asynchronous InetAddr name lookup started with
 *   gnet_inetaddr_get_name_async().  The lookup can be canceled by
 *   calling gnet_inetaddr_get_name_async_cancel() with the ID.
 *
 **/
typedef gpointer GInetAddrGetNameAsyncID;



/**
 *   GInetAddrGetNameAsyncFunc:
 *   @hostname: Canonical name of the address (callee owned)
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_get_name_async().  Callee owns the
 *   name.  The name will be NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrGetNameAsyncFunc)(gchar* hostname,
					  gpointer data);




gchar* gnet_inetaddr_get_name (/* const */ GInetAddr* ia);

gchar* gnet_inetaddr_get_name_nonblock (GInetAddr* ia);

GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async (GInetAddr* ia, 
			      GInetAddrGetNameAsyncFunc func,
			      gpointer data);

void    gnet_inetaddr_get_name_async_cancel (GInetAddrGetNameAsyncID id);


gchar*  gnet_inetaddr_get_canonical_name (const GInetAddr* ia);

gint 	gnet_inetaddr_get_port (const GInetAddr* ia);
void 	gnet_inetaddr_set_port (const GInetAddr* ia, guint port);

/* FIX: Implement */
/*  guint8* gnet_inetaddr_get_bytes (const GInetAddr* ia); */
/*  void    gnet_inetaddr_set_bytes (const GInetAddr* ia, const guint8* addr, const guint length); */


/* ********** */

gboolean gnet_inetaddr_is_canonical (const gchar* hostname);

gboolean gnet_inetaddr_is_internet  (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_private   (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_reserved  (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_loopback  (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_multicast (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_broadcast (const GInetAddr* inetaddr);

gboolean gnet_inetaddr_is_ipv4      (const GInetAddr* inetaddr);
gboolean gnet_inetaddr_is_ipv6      (const GInetAddr* inetaddr);


/* ********** */

guint gnet_inetaddr_hash (gconstpointer p);
gint  gnet_inetaddr_equal (gconstpointer p1, gconstpointer p2);
gint  gnet_inetaddr_noport_equal (gconstpointer p1, gconstpointer p2);


/* ********** */

gchar*     gnet_inetaddr_gethostname (void);
GInetAddr* gnet_inetaddr_gethostaddr (void);


/* ********** */

GInetAddr* gnet_inetaddr_new_any (void);
GInetAddr* gnet_inetaddr_autodetect_internet_interface (void);
GInetAddr* gnet_inetaddr_get_interface_to (const GInetAddr* addr);
GInetAddr* gnet_inetaddr_get_internet_interface (void);

gboolean   gnet_inetaddr_is_internet_domainname (const gchar* hostname);


/* ********** */

GList*     gnet_inetaddr_list_interfaces (void);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_INETADDR_H */
