/***********************************************************************
 *                   _     _                     __   _  _
 *   __ _ _ __   ___| |_  | |__   __ _ ___  ___ / /_ | || |
 *  / _` | '_ \ / _ \ __| | '_ \ / _` / __|/ _ \ '_ \| || |_
 * | (_| | | | |  __/ |_  | |_) | (_| \__ \  __/ (_) |__   _|
 *  \__, |_| |_|\___|\__| |_.__/ \__,_|___/\___|\___/   |_|
 *  |___/
 *
 *  created by Alfred Reibenschuh <alfredreibenschuh@gmx.net>,
 *  under the ``GNU Library General Public License´´ (see below).
 *
 ***********************************************************************
 *
 * Copyright (C) 2003 Free Software Foundation
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***********************************************************************/

#ifndef _GNET_BASE64_H
#define _GNET_BASE64_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GNET_BASE64_RAW       0
#define GNET_BASE64_STRICT    1

gchar* gnet_base64_encode (gchar* src, gint srclen, gint* dstlen, gint strict);
gchar* gnet_base64_decode (gchar* src, gint srclen, gint* dstlen);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_BASE64_H */
