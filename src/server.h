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


#ifndef _GNET_SERVER_H
#define _GNET_SERVER_H

/* This module is experimental, buggy, and unstable.  Use at your own risk. */
#ifdef GNET_EXPERIMENTAL 

#include <glib.h>
#include "tcp.h"
#include "conn.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



typedef struct _GServer GServer;

typedef enum
{
  GNET_SERVER_STATUS_CONNECT,
  GNET_SERVER_STATUS_ERROR

} GServerStatus;


typedef void (*GServerFunc)(GServer* server,
			    GServerStatus status, 
			    GConn* conn,
			    gpointer user_data);


struct _GServer
{
  GInetAddr* 	iface;
  gint		port;

  GTcpSocket* 	socket;
  GIOChannel* 	iochannel;

  guint		watch_id;

  GServerFunc	func;
  gpointer	user_data;

};


GServer*  gnet_server_new (const GInetAddr* iface, gboolean force_port, 
			   GServerFunc func, gpointer user_data);

void      gnet_server_delete (GServer* server);




#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_SERVER_H */
