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

static SOCKET
gnet_private_create_ipv4_listen_socket (int type, int port, struct sockaddr_storage* sa)
{
  struct sockaddr_in* sa_in;

  sa_in = (struct sockaddr_in*) sa;
  sa_in->sin_family = AF_INET;
  GNET_SOCKADDR_SET_SS_LEN(*sa);
  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = g_htons(port);
  
  return socket (AF_INET, type, 0);
}

static SOCKET
gnet_private_create_ipv6_listen_socket (int type, int port, struct sockaddr_storage* sa)
{
#ifdef HAVE_IPV6
  struct sockaddr_in6* sa_in6;
#ifdef GNET_WIN32
  struct addrinfo Hints, *AddrInfo;
  char port_buff[12];
#endif

  sa_in6 = (struct sockaddr_in6*) sa;

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
  return socket (AF_INET6, type, 0);

#else
  return GNET_INVALID_SOCKET;
#endif
}

SOCKET
gnet_private_create_listen_socket (int type, const GInetAddr* iface, int port, struct sockaddr_storage* sa)
{
  SOCKET sockfd = GNET_INVALID_SOCKET;

  if (iface)
    {
      int family = GNET_INETADDR_FAMILY(iface);
      *sa = iface->sa;
      GNET_SOCKADDR_PORT_SET(*sa, g_htons(port));

      sockfd = socket (family, type, 0);
    }
  else
    {
      GIPv6Policy ipv6_policy;

      ipv6_policy = gnet_ipv6_get_policy();
      switch (ipv6_policy) {
	case GIPV6_POLICY_IPV4_THEN_IPV6:
	  sockfd = gnet_private_create_ipv4_listen_socket (type, port, sa);
	  if (!GNET_IS_SOCKET_VALID(sockfd))
	    sockfd = gnet_private_create_ipv6_listen_socket (type, port, sa);
	  break;
	case GIPV6_POLICY_IPV6_THEN_IPV4:
	  sockfd = gnet_private_create_ipv6_listen_socket (type, port, sa);
	  if (!GNET_IS_SOCKET_VALID(sockfd))
	    sockfd = gnet_private_create_ipv4_listen_socket (type, port, sa);
	  break;
	case GIPV6_POLICY_IPV4_ONLY:
	  sockfd = gnet_private_create_ipv4_listen_socket (type, port, sa);
	  break;
	case GIPV6_POLICY_IPV6_ONLY:
	  sockfd = gnet_private_create_ipv6_listen_socket (type, port, sa);
	  break;
	default:
	  g_assert_not_reached ();
	  break;
      }
    }

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

BOOL WINAPI 
DllMain(HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved);

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
         if( !gnet_initialize_windows_sockets() )
	  {
	  return FALSE; 
	}
 
	/* The WinSock DLL is acceptable. Proceed. */
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
	/*CleanUp WinSock 2 */
	WSACleanup();

	break;
      }
    }

  return TRUE;
}
 
int gnet_initialize_windows_sockets(void)
{
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;

  wVersionRequested = MAKEWORD( 2, 0 );

  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0)
    {
      return FALSE;
    }

  /* Confirm that the WinSock DLL supports 2.0.*/
  /* Note that if the DLL supports versions greater    */
  /* than 2.0 in addition to 2.0, it will still return */
  /* 2.0 in wVersion since that is the version we      */
  /* requested.                                        */

  if (LOBYTE(wsaData.wVersion) != 2 ||
      HIBYTE(wsaData.wVersion) != 0) {
    /* Tell the user that we could not find a usable */
    /* WinSock DLL.                                  */
    WSACleanup();
    return FALSE;
  }

  return TRUE;
}

void gnet_uninitialize_windows_sockets(void)
{
  WSACleanup();
}


#endif
