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

#include "gnet-private.h"
#include "gnet.h"



/**
 *  gnet_private_inetaddr_sockaddr_new:
 *  @sa: sockaddr struct to create InetAddr from.
 *
 *  Create an internet address from a sockaddr struct.  WARNING: This
 *  may go away or be hidden in a future version.
 *
 *  Returns: a new InetAddr, or NULL if there was a failure.  
 *
 **/
GInetAddr* 
gnet_private_inetaddr_sockaddr_new(const struct sockaddr sa)
{
  GInetAddr* ia = g_new0(GInetAddr, 1);

  ia->sa = sa;

  return ia;
}



/**
 *  gnet_private_inetaddr_get_sockaddr:
 *  @ia: Address to get the sockaddr of.
 *
 *  Get the sockaddr struct.  WARNING: This may go away or be hidden
 *  in a future version.
 *
 *  Returns: the sockaddr struct
 **/
struct sockaddr 
gnet_private_inetaddr_get_sockaddr(const GInetAddr* ia)
{
  g_assert(ia != NULL);

  return ia->sa;
}



#ifndef GNET_WIN32		/* Unix specific version */

/**
 *  gnet_private_inetaddr_list_interfaces:
 *
 *  Get a list of InetAddr's on this host (usually there's only one,
 *  but there can be more).  This isn't complete - we only look at
 *  running, non-loopback addresses.
 *
 *  (TODO: Make a NetInterface struct which includes flags so you know
 *  if its mcast or bcast and can have the bcast address, etc.).
 *
 *  Note that the Windows version supports a maximum of 10 interfaces.
 *  In Windows NT, Service Pack 4 (or higher) is required.
 *
 *  Returns: A list of InetAddrs representing available interfaces.
 *
 **/
GList* 
gnet_private_inetaddr_list_interfaces(void)
{
  GList* list = NULL;
  gint len, lastlen;
  gchar* buf;
  gchar* ptr;
  gint sockfd;
  struct ifconf ifc;


  /* Create a dummy socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) return NULL;

  len = 8 * sizeof(struct ifreq);
  lastlen = 0;

  /* Get the list of interfaces.  We might have to try multiple times
     if there are a lot of interfaces. */
  while(1)
    {
      buf = g_new0(gchar, len);
      
      ifc.ifc_len = len;
      ifc.ifc_buf = buf;
      if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
	{
	  /* Might have failed because our buffer was too small */
	  if (errno != EINVAL || lastlen != 0)
	    {
	      g_free(buf);
	      return NULL;
	    }
	}
      else
	{
	  /* Break if we got all the interfaces */
	  if (ifc.ifc_len == lastlen)
	    break;

	  lastlen = ifc.ifc_len;
	}

      /* Did not allocate big enough buffer - try again */
      len += 8 * sizeof(struct ifreq);
      g_free(buf);
    }


  /* Create the list.  Stevens has a much more complex way of doing
     this, but his is probably much more correct portable.  */
  for (ptr = buf; ptr < (buf + ifc.ifc_len); ptr += sizeof(struct ifreq))
    {
      struct ifreq* ifr = (struct ifreq*) ptr;
      struct sockaddr addr;
      GInetAddr* ia;

      /* Ignore non-AF_INET */
      if (ifr->ifr_addr.sa_family != AF_INET)
	continue;

      /* FIX: Skip colons in name?  Can happen if aliases, maybe. */

      /* Save the address - the next call will clobber it */
      memcpy(&addr, &ifr->ifr_addr, sizeof(addr));
      
      /* Get the flags */
      ioctl(sockfd, SIOCGIFFLAGS, ifr);

      /* Ignore entries that aren't running or are loopback.  Someday
	 we'll write an interface structure and include this stuff. */
      if (!(ifr->ifr_flags & IFF_RUNNING) ||
	  (ifr->ifr_flags & IFF_LOOPBACK))
	continue;

      /* Create an InetAddr for this one and add it to our list */
      ia = gnet_private_inetaddr_sockaddr_new(addr);
      if (ia != NULL)
	list = g_list_prepend(list, ia);
    }

  return list;
}

#else GNET_WIN32		/* Windows specific version */

GList* 
gnet_private_inetaddr_list_interfaces(void)
{
  GList* list;
  SOCKET s;
  int wsError;
  DWORD bytesReturned;
  SOCKADDR_IN* pAddrInet;
  u_long SetFlags;
  INTERFACE_INFO localAddr[10];  /* Assumes there will be no more than 10 IP interfaces */
  int numLocalAddr; 
  int i;
  struct sockaddr addr;
  GInetAddr *ia;
  /*SOCKADDR_IN* pAddrInet2; */	/* For testing */

  list = NULL;
  if((s = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0)) == INVALID_SOCKET)
    {
      return NULL;
    }

  /* Enumerate all IP interfaces */
  wsError = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, &localAddr,
		     sizeof(localAddr), &bytesReturned, NULL, NULL);
  if (wsError == SOCKET_ERROR)
    {
      closesocket(s);
      return NULL;
    }

  closesocket(s);

  /* Display interface information */
  numLocalAddr = (bytesReturned/sizeof(INTERFACE_INFO));
  for (i=0; i<numLocalAddr; i++) 
    {
      pAddrInet = (SOCKADDR_IN*)&localAddr[i].iiAddress;
      memcpy(&addr, pAddrInet, sizeof(addr));
      /* pAddrInet2 = (SOCKADDR_IN*)&addr; */	/*For testing */

      SetFlags = localAddr[i].iiFlags;
      if ((SetFlags & IFF_UP) && (!(SetFlags & IFF_LOOPBACK)))
	{
	  /* Create an InetAddr for this one and add it to our list */
	  ia = gnet_private_inetaddr_sockaddr_new(addr);
	  if (ia != NULL)
	    list = g_list_prepend(list, ia);
	}
    }
  return list;
}

#endif /* end Windows specific gnet_private_inetaddr_list_interfaces */




/* TODO: Need to port this to Solaris, Windows */

#if 0

/**
 *  gnet_udp_socket_get_MTU:
 *  @us: GUdpSocket to get MTU from.
 *
 *  Get the MTU for outgoing packets.  
 *
 *  Returns: MTU; -1 if unknown.
 *
 **/
gint
gnet_udp_socket_get_MTU(GUdpSocket* us)
{
  struct ifreq ifr;

  /* FIX: Not everyone has ethernet, right? */
  strncpy (ifr.ifr_name, "eth0", sizeof (ifr.ifr_name));
  if (!ioctl(us->sockfd, SIOCGIFMTU, &ifr)) 
    return ifr.ifr_mtu;

  return -1;
}

#endif




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

   I believe I have taken care of the multi-threaded issues by using
   mutexes.  

*/

#ifdef GNET_WIN32

WNDCLASSEX gnetWndClass;
HWND  gnet_hWnd; 
guint gnet_io_watch_ID;
GIOChannel *gnet_iochannel;
	
GHashTable *gnet_hash;
GHashTable *gnet_select_hash; /* gnet_tcp_socket_new_async needs its own hash */
HANDLE gnet_Mutex; 
HANDLE gnet_select_Mutex;
HANDLE gnet_hostent_Mutex;


int 
gnet_MainCallBack(GIOChannel *iochannel, GIOCondition condition, void *nodata)
{
  MSG msg;

  gpointer data;
  GInetAddrAsyncState *IAstate;
  GInetAddrReverseAsyncState *IARstate;
  GTcpSocketAsyncState *TCPNEWstate;

  /*Take the msg off the message queue */
  GetMessage (&msg, NULL, 0, 0);
 
  switch (msg.message) 
    {
    case IA_NEW_MSG:
      {
	WaitForSingleObject(gnet_Mutex, INFINITE);
	data = g_hash_table_lookup(gnet_hash, (gpointer)msg.wParam);
	g_hash_table_remove(gnet_hash, (gpointer)msg.wParam);
	ReleaseMutex(gnet_Mutex);
		
	IAstate = (GInetAddrAsyncState*) data;
	IAstate->errorcode = WSAGETASYNCERROR(msg.lParam); /* NULL if OK */

	/* Now call the callback function */
	gnet_inetaddr_new_async_cb(NULL, G_IO_IN, (gpointer)IAstate);

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
    case TCP_SOCK_MSG:
      {
	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)msg.wParam);
	g_hash_table_remove(gnet_select_hash, (gpointer)msg.wParam);
	ReleaseMutex(gnet_select_Mutex);
			
	TCPNEWstate = (GTcpSocketAsyncState*) data;
	TCPNEWstate->errorcode = WSAGETSELECTERROR(msg.lParam);
	gnet_tcp_socket_new_async_cb(NULL, G_IO_IN, (gpointer) TCPNEWstate);
	break;
      }
    }

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


gboolean
RemoveHashEntry(gpointer key, gpointer value, gpointer user_data)
{
  g_free(value);
  return TRUE;
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

	/* Setup and register a windows class that we use for out GIOchannel */
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
	gnet_select_hash = g_hash_table_new(NULL, NULL);
	

	gnet_Mutex = CreateMutex( 
				 NULL,                       /* no security attributes */
				 FALSE,                      /* initially not owned */
				 "gnet_Mutex");  /* name of mutex */

	if (gnet_Mutex == NULL) 
	  {
	    return FALSE;
	  }

	gnet_select_Mutex = CreateMutex( 
					NULL,                       /* no security attributes */
					FALSE,                      /* initially not owned */
					"gnet_select_Mutex");  /* name of mutex */

	if (gnet_select_Mutex == NULL) 
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
	DestroyWindow(gnet_hWnd);

	WaitForSingleObject(gnet_Mutex, INFINITE);
	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	g_hash_table_foreach_remove(gnet_hash, RemoveHashEntry, NULL);
	g_hash_table_foreach_remove(gnet_select_hash, RemoveHashEntry, NULL);
	g_hash_table_destroy(gnet_select_hash);
	g_hash_table_destroy(gnet_hash);
	ReleaseMutex(gnet_Mutex);
	ReleaseMutex(gnet_select_Mutex);
	ReleaseMutex(gnet_hostent_Mutex);

	/*CleanUp WinSock 2 */
	WSACleanup();

	break;
      }
    }

  return TRUE;
}
 
#endif
