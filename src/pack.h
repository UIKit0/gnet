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

#ifndef _GNET_PACK_H
#define _GNET_PACK_H

#include "glib.h"

gint gnet_pack (const gchar* format, gchar* str, const guint len, ...);
gint gnet_vpack (const gchar* format, gchar* str, const guint len, va_list args);
gint gnet_pack_strdup (const gchar* format, gchar** str, ...); /* NOT DONE */
     
gint gnet_calcsize (const gchar* format); /* UNTESTED */
     
gint gnet_unpack (const gchar* format, gchar* str, gint len, ...);
gint gnet_vunpack (const gchar* format, gchar* str, gint len, va_list args);


#endif _GNET_PACK_H
