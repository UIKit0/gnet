/* GNet - Networking library
 * Copyright (C) 2001  Marius Eriksen, David Helder
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

#ifndef _GNET_SOCKS_PRIVATE_H
#define _GNET_SOCKS_PRIVATE_H

#include "gnet-private.h"

struct socks4_h {
	u_char vn;
	u_char cd;
	guint16 dport;
	guint32 dip;
	u_char userid;
};

struct socks5_h {
	u_char vn;
	u_char cd;
	u_char rsv;
	u_char atyp;	
	guint32 dip;
	guint32 dport;
};

#define GNET_DEFAULT_SOCKS_VERSION 5

GInetAddr* gnet_private_get_socks_server(void);
int gnet_private_negotiate_socks_server(GTcpSocket*, const GInetAddr*);
int gnet_private_negotiate_socks4(GIOChannel*, const GInetAddr*);
int gnet_private_negotiate_socks5(GIOChannel*, const GInetAddr*);

#endif /* _GNET_SOCKS_PRIVATE_H */
