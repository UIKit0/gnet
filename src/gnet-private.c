/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2000-2003  Andrew Lanoix
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

#include "gnet-private.h"
#include "gnet.h"


/* 

   Super-function.

   When creating a listening socket, we need to create the appropriate
   socket and set-up an address for binding.  These operations depend
   on the particular interface, or on IPv6 policy if there is no interface.

 */
int
gnet_private_create_listen_socket (int type, const GInetAddr* iface, int port, struct sockaddr_storage* sa)
{
  int family;
  int sockfd;

  if (iface)
    {
      family = GNET_INETADDR_FAMILY(iface);
      *sa = iface->sa;
      GNET_SOCKADDR_PORT_SET(*sa, g_htons(port));
    }
  else
    {
      GIPv6Policy ipv6_policy;

      ipv6_policy = gnet_ipv6_get_policy();
      if (ipv6_policy == GIPV6_POLICY_IPV4_ONLY)	/* IPv4 */
	{
	  struct sockaddr_in* sa_in;

	  family = AF_INET;

	  sa_in = (struct sockaddr_in*) sa;
	  sa_in->sin_family = AF_INET;
	  GNET_SOCKADDR_SET_SS_LEN(*sa);
	  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
	  sa_in->sin_port = g_htons(port);
	}
#ifdef HAVE_IPV6
      else						/* IPv6 */
	{
	  struct sockaddr_in6* sa_in6;

	  family = AF_INET6;

	  sa_in6 = (struct sockaddr_in6*) sa;
	  sa_in6->sin6_family = AF_INET6;
	  GNET_SOCKADDR_SET_SS_LEN(*sa);
	  memset(&sa_in6->sin6_addr, 0, sizeof(sa_in6->sin6_addr));
	  sa_in6->sin6_port = g_htons(port);
	}
#endif
    }

  sockfd = socket(family, type, 0);

  return sockfd;
}





/**
 * gnet_private_io_channel_new:
 * @sockfd: socket descriptor
 *
 * Create a new IOChannel from a descriptor.  In GLib 2.0, turn off
 * encoding and buffering.
 *
 * Returns: An iochannel.
 *
 **/
GIOChannel* 
gnet_private_io_channel_new (int sockfd) 
{
  GIOChannel* iochannel;

  iochannel = GNET_SOCKET_IO_CHANNEL_NEW(sockfd);
  if (iochannel == NULL)
    return NULL;

#if GLIB_MAJOR_VERSION == 2
  g_io_channel_set_encoding (iochannel, NULL, NULL);
  g_io_channel_set_buffered (iochannel, FALSE);
#endif

  return iochannel;
}



/* 

   Below is the Windows specific code and a general idea of how
   things were implemented.

   In the Windows Port, not much was needed to be done for the sync
   TCP, UDP and Mcast functions. The Async TCP functions needed to be
   rewritten.

   To accomplish this we use one GIOchannel to handle all the TCP
   async functions and their callbacks. This is different from the
   forking *nix version of Gnet.

   The async functions do all the 'parent' code and the functions then
   add the "state" information (containing the user and Gnet's
   bookkeeping data) into private global hashes.  Then they do the
   appropriate WinSock2 WSAAsync* function (the 'child'), which will
   returns a HANDLE.  This HANDLE is typecasted and returned to the
   user as their ID.  This number will also double as a key into the
   hash. Note that WinSock2 WSAAsyncGetXY task handles are
   unique. (sockfd is used in gnet_tcp_socket_new_async)

   When the WinSock async call is complete, it posts a message that
   will be pickup by gnet_MainCallBack(). The wParam in the MSG
   contains the original async task handle returned by the WinSock2
   async functions. The high 16 bits of lParm contain the error
   information. We use the macros defined in WinSock2.h to extract
   this. The 'message' parameter identifies which async callback we
   are dealing with.  Typcased names of these are #define-d in
   gnet.h. We then do any necessary work and then call the gnet async
   function's callback, removing the data record from the correct
   hash. Then these callbacks call the user's callback with the user's
   data.

   If the user calls an async cancel function, then we remove and
   destroy the data from the hash.  A WinSock 2 async call is
   destroyed by WSACancelAsyncRequest(HANDLE);.

	 UPDATE: As of March 2001, many of the problems in glib's iochannel
	 code were fixed so only a few gnet functions still use this method.

   I believe I have taken care of the multi-threaded issues by using
   mutexes.  

*/

#ifdef GNET_WIN32

static WNDCLASSEX gnetWndClass;
HWND  gnet_hWnd; 
static guint gnet_io_watch_ID;
static GIOChannel *gnet_iochannel;
	
GHashTable *gnet_hash;

HANDLE gnet_Mutex; 
HANDLE gnet_hostent_Mutex;

int gnet_MainCallBack(GIOChannel *iochannel, GIOCondition condition, void *nodata);

LRESULT CALLBACK
GnetWndProc(HWND hwnd,
	    UINT uMsg,
	    WPARAM wParam,
	    LPARAM lParam);

BOOL WINAPI 
DllMain(HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved);

int 
gnet_MainCallBack(GIOChannel *iochannel, GIOCondition condition, void *nodata)
{
  MSG msg;

  gpointer data;
  GInetAddrNewListState* IAstate;
  GInetAddrReverseAsyncState *IARstate;

  int i;

  while ((i = PeekMessage (&msg, gnet_hWnd, 0, 0, PM_REMOVE)))
  {
	switch (msg.message) 
    {
		case IA_NEW_MSG:
		{
			WaitForSingleObject(gnet_Mutex, INFINITE);
			data = g_hash_table_lookup(gnet_hash, (gpointer)msg.wParam);
			g_hash_table_remove(gnet_hash, (gpointer)msg.wParam);
			ReleaseMutex(gnet_Mutex);
		
			IAstate = (GInetAddrNewListState*) data;
			IAstate->errorcode = WSAGETASYNCERROR(msg.lParam); /* NULL if OK */

			/* Now call the callback function */
			gnet_inetaddr_new_list_async_cb(NULL, G_IO_IN, (gpointer)IAstate);

			break;
		}
		case GET_NAME_MSG:
		{
			WaitForSingleObject(gnet_Mutex, INFINITE);
			data = g_hash_table_lookup(gnet_hash, (gpointer)msg.wParam);
			g_hash_table_remove(gnet_hash, (gpointer)msg.wParam);
			ReleaseMutex(gnet_Mutex);

			IARstate = (GInetAddrReverseAsyncState*) data;
			IARstate->errorcode = WSAGETASYNCERROR(msg.lParam); /* NULL if OK */
			
			/* Now call the callback function */
			gnet_inetaddr_get_name_async_cb(NULL, G_IO_IN, (gpointer)IARstate);
			break;
		}
	} /* switch */
  } /* while */

  return 1;
}

/* Not used but required*/
LRESULT CALLBACK
GnetWndProc(HWND hwnd,        /* handle to window */
	    UINT uMsg,        /* message identifier */
	    WPARAM wParam,    /* first message parameter */
	    LPARAM lParam)    /* second message parameter */
{ 

    switch (uMsg) 
    { 
        case WM_CREATE: 
            /* Initialize the window. */
            return 0; 
 
        case WM_PAINT: 
            /* Paint the window's client area. */ 
            return 0; 
 
        case WM_SIZE: 
            /* Set the size and position of the window. */ 
            return 0; 
 
        case WM_DESTROY: 
            /* Clean up window-specific data objects. */
            return 0; 
 
        /* 
          Process other messages. 
        */ 
 
        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 
    return 0; 
} 


BOOL WINAPI 
DllMain(HINSTANCE hinstDLL,  /* handle to DLL module */
	DWORD fdwReason,     /* reason for calling functionm */
	LPVOID lpvReserved   /* reserved */)
{

  switch(fdwReason) 
    {
    case DLL_PROCESS_ATTACH:
      /* The DLL is being mapped into process's address space */
      /* Do any required initialization on a per application basis, return FALSE if failed */
      {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD( 2, 0 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	  {
				/* Tell the user that we could not find a usable */
				/* WinSock DLL.                                  */
	    return FALSE;
	  }
 
	/* Confirm that the WinSock DLL supports 2.0.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.0 in addition to 2.0, it will still return */
	/* 2.0 in wVersion since that is the version we      */
	/* requested.                                        */
 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
	     HIBYTE( wsaData.wVersion ) != 0 ) {
	  /* Tell the user that we could not find a usable */
	  /* WinSock DLL.                                  */
	  WSACleanup();
	  return FALSE; 
	}
 
	/* The WinSock DLL is acceptable. Proceed. */

	/* Setup and register a windows class that we use for our GIOchannel */
	gnetWndClass.cbSize = sizeof(WNDCLASSEX); 
	gnetWndClass.style = CS_SAVEBITS; /* doesn't matter, need something? */ 
	gnetWndClass.lpfnWndProc = (WNDPROC) GnetWndProc; 
	gnetWndClass.cbClsExtra = 0; 
	gnetWndClass.cbWndExtra = 0; 
	gnetWndClass.hInstance = hinstDLL; 
	gnetWndClass.hIcon = NULL; 
	gnetWndClass.hCursor = NULL; 
	gnetWndClass.hbrBackground = NULL; 
	gnetWndClass.lpszMenuName = NULL; 
	gnetWndClass.lpszClassName = "Gnet"; 
	gnetWndClass.hIconSm = NULL; 

	if (!RegisterClassEx(&gnetWndClass))
	  {
	    return FALSE;	
	  }

	gnet_hWnd  = CreateWindowEx
	  (
	   0,
	   "Gnet", 
	   "none",
	   WS_OVERLAPPEDWINDOW, 
	   CW_USEDEFAULT, 
	   CW_USEDEFAULT, 
	   CW_USEDEFAULT, 
	   CW_USEDEFAULT, 
	   (HWND) NULL, 
	   (HMENU) NULL, 
	   hinstDLL, 
	   (LPVOID) NULL);  

	if (!gnet_hWnd) 
	  {
	    return FALSE;
	  }

	gnet_iochannel = g_io_channel_win32_new_messages((unsigned int)gnet_hWnd);

	/* Add a watch */
	gnet_io_watch_ID = g_io_add_watch(gnet_iochannel,
					  (GIOCondition)(G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL),
					  gnet_MainCallBack, 
					  NULL);

	gnet_hash = g_hash_table_new(NULL, NULL);	

	gnet_Mutex = CreateMutex( 
				 NULL,                       /* no security attributes */
				 FALSE,                      /* initially not owned */
				 "gnet_Mutex");  /* name of mutex */

	if (gnet_Mutex == NULL) 
	  {
	    return FALSE;
	  }

	gnet_hostent_Mutex = CreateMutex( 
					 NULL,                       /* no security attributes */
					 FALSE,                      /* initially not owned */
					 "gnet_hostent_Mutex");  /* name of mutex */

	if (gnet_hostent_Mutex == NULL) 
	  {
	    return FALSE;
	  }
	break;
      }
    case DLL_THREAD_ATTACH:
      /* A thread is created. Do any required initialization on a per thread basis*/
      {
	/*Nothing needs to be done. */
	break;
      }
    case DLL_THREAD_DETACH:
      /* Thread exits with  cleanup */
      {	
	/*Nothing needs to be done. */
	break;
      }
    case DLL_PROCESS_DETACH:
      /* The DLL unmapped from process's address space. Do necessary cleanup */
      {
	g_source_remove(gnet_io_watch_ID);
	g_free(gnet_iochannel);
/* 	DestroyWindow(gnet_hWnd); */
/* FIX DestroyWindow causes problems according to DanielKO.  It works
   if removed.  Andy - take a look at this.  -DAH */

	/*CleanUp WinSock 2 */
	WSACleanup();

	break;
      }
    }

  return TRUE;
}
 
#endif
