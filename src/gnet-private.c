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
SOCKET
gnet_private_create_listen_socket (int type, const GInetAddr* iface, int port, struct sockaddr_storage* sa)
{
  int family = 0;
  SOCKET sockfd;
#ifdef GNET_WIN32
  struct addrinfo Hints, *AddrInfo;
  char port_buff[12];
#endif

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

	  sa_in6 = (struct sockaddr_in6*) sa;
	  family = AF_INET6;

  #ifndef GNET_WIN32    /* Unix */

	sa_in6->sin6_family = AF_INET6;
	GNET_SOCKADDR_SET_SS_LEN(*sa);
	memset(&sa_in6->sin6_addr, 0, sizeof(sa_in6->sin6_addr));
	sa_in6->sin6_port = g_htons(port);

  #else                 /* Windows */

    /* A simple memset does not work for some reason on Windows */
	sprintf(port_buff, "%d", port);
	memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = AF_INET6;
    Hints.ai_socktype = type;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
	pfn_getaddrinfo(NULL, port_buff, &Hints, &AddrInfo);
	memcpy(sa_in6, AddrInfo->ai_addr, AddrInfo->ai_addrlen);
	pfn_freeaddrinfo(AddrInfo);

  #endif
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
gnet_private_io_channel_new (SOCKET sockfd) 
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

#ifdef GNET_WIN32

static WNDCLASSEX gnetWndClass;
HWND  gnet_hWnd; 
static guint gnet_io_watch_ID;
static GIOChannel *gnet_iochannel;

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

/* This function is still necessary (even though it does nothing) since it 
	presence works around an issue in the Glib main loop. */

int 
gnet_MainCallBack(GIOChannel *iochannel, GIOCondition condition, void *nodata)
{
  MSG msg;
  int i;

  while ((i = PeekMessage (&msg, gnet_hWnd, 0, 0, PM_REMOVE)))
  {
	switch (msg.message) 
    {
		case IA_NEW_MSG:
		{
			break;
		}
		case GET_NAME_MSG:
		{
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
