/* GNet - Networking library
 * Copyright (C) 2000-2002  David Helder
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

#include "gnet-private.h"
#include "inetaddr.h"

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_LINUX_NETLINK_H
#include <usagi_ifaddrs.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif


#ifdef GNET_WIN32

static int inet_pton(int af, const char* src, void* dst);
static const char* inet_ntop(int af, const void *src, char *dst, size_t size);

/* TODO: Use Window's inet_pton/inet_ntop if they ever implement it. */
static int
inet_pton(int af, const char* src, void* dst)
{
  in_addr_t addr;

  addr = inet_addr(src);
  if (addr == INADDR_NONE && strcmp(src, "255.255.255.255"))
    return 0;

  *((in_addr_t*) dst) = addr;
  return 1;
}

static const char* 
inet_ntop(int af, const void* src, char* dst, size_t size)
{
  struct in_addr addr = *(struct in_addr*) src;
  const char* buf;

  buf = inet_ntoa(addr);
  if (buf == NULL)
    return NULL;

  strcpy(dst, buf);

  return dst;
}

#endif /* GNET_WIN32 */


/* **************************************** */

static GList* gnet_gethostbyname(const char* hostname);
static gchar* gnet_gethostbyaddr(const struct sockaddr_storage* sa);
static void   ialist_free (GList* ialist);


#if defined(HAVE_GETHOSTBYNAME_R_GLIB_MUTEX) \
   || defined(HAVE_GETADDRINFO_GLIB_MUTEX)
#  ifndef G_THREADS_ENABLED
#    error Using GLib Mutex but thread are not enabled.
#  endif
G_LOCK_DEFINE (dnslock);
#endif


#if !defined(HAVE_GETADDRINFO)
static GList* hostent2ialist (const struct hostent* he);

static GList*
hostent2ialist (const struct hostent* he)
{
  GList* list = NULL;
  int i;

  if (!he)
    return NULL;

  for (i = 0; he->h_addr_list[i]; ++i)
    {
      GInetAddr* ia;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      ia->sa.ss_family = he->h_addrtype;
      GNET_INETADDR_SET_SS_LEN(ia);
      memcpy (GNET_SOCKADDR_ADDRP(ia->sa), he->h_addr_list[i], he->h_length);
      list = g_list_prepend(list, ia);
    }

  /* list is returned backwards */
  return list;
}

#endif



/* Thread safe gethostbyname.  Maps hostname to list of
   sockaddr_storage pointers. */
GList*
gnet_gethostbyname(const char* hostname)
{
  GList* list = NULL;

  /* **************************************** */
  /* First attempt non-blocking lookup */
  
  struct in_addr inaddr;
#ifdef HAVE_IPV6
  struct in6_addr in6addr;
#endif

  if (inet_pton(AF_INET, hostname, &inaddr) != 0)
    {
      GInetAddr* ia;
      struct sockaddr_in* sa;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa = (struct sockaddr_in*) &ia->sa;

      sa->sin_family = AF_INET;
      GNET_INETADDR_SET_SS_LEN(ia);
      sa->sin_addr = inaddr;

      list = g_list_prepend (list, ia);
      return list;
    }
#ifdef HAVE_IPV6
  else if (inet_pton(AF_INET6, hostname, &in6addr) != 0)
    {
      GInetAddr* ia;
      struct sockaddr_in6* sa;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa = (struct sockaddr_in6*) &ia->sa;

      sa->sin6_family = AF_INET6;
      GNET_INETADDR_SET_SS_LEN(ia);
      sa->sin6_addr = in6addr;
      /* don't set the nice name, it's not necessarily canonical */

      list = g_list_prepend (list, ia);
      return list;
    }
#endif

  /* **************************************** */
  /* DNS lookup: getaddrinfo() */

#ifdef HAVE_GETADDRINFO
  {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    int rv;
    GIPv6Policy policy;

    policy = gnet_ipv6_get_policy();

    memset (&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if (policy == GIPV6_POLICY_IPV4_ONLY)
      hints.ai_family = AF_INET;
    else if (policy == GIPV6_POLICY_IPV6_ONLY)
      hints.ai_family = AF_INET6;

    /* If GLIB_MUTEX is not defined, either getaddrinfo() is
       thread-safe, or we don't have mutexes. */
#   ifdef HAVE_GETADDRINFO_GLIB_MUTEX
    G_LOCK (dnslock);
#   endif

    rv = getaddrinfo (hostname, NULL, &hints, &res);
    if (rv == 0)
      {
	struct addrinfo* i;
	GList* ipv4_list = NULL;
	GList* ipv6_list = NULL;
	
	for (i = res; i != NULL; i = i->ai_next)
	  {
	    GInetAddr* ia;

	    ia = g_new0(GInetAddr, 1);
	    ia->ref_count = 1;
	    memcpy (&ia->sa, i->ai_addr, i->ai_addrlen);

	    if (i->ai_family == PF_INET)
	      ipv4_list = g_list_prepend (ipv4_list, ia);
	    else if (i->ai_family == PF_INET6)
	      ipv6_list = g_list_prepend (ipv6_list, ia);
	    else
	      g_free (ia);
	  }

	if (policy == GIPV6_POLICY_IPV4_ONLY)
	  {
	    list = ipv4_list;
	    g_list_free (ipv6_list);
	  }
	else if (policy == GIPV6_POLICY_IPV4_THEN_IPV6)
	  {
	    list = g_list_concat (ipv6_list, ipv4_list);
	    /* list will be reversed below */
	  }
	else if (policy == GIPV6_POLICY_IPV6_ONLY)
	  {
	    list = ipv6_list;
	    g_list_free (ipv4_list);
	  }
	else if (policy == GIPV6_POLICY_IPV6_THEN_IPV4)
	  {
	    list = g_list_concat (ipv4_list, ipv6_list);
	    /* list will be reversed below */
	  }
      }

    if (res)
      freeaddrinfo (res);

#   ifdef HAVE_GETADDRINFO_GLIB_MUTEX
    G_UNLOCK (dnslock);
#   endif
  }
#else

  /* **************************************** */
  /* DNS lookup: gethostbyname() */

#ifdef HAVE_GETHOSTBYNAME_THREADSAFE
  {
    struct hostent* he;

    he = gethostbyname(hostname);
    list = hostent2ialist(he);
  }
#else
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
  {
    struct hostent result_buf, *result;
    size_t len;
    char* buf;
    int herr;
    int rv;

    len = 1024;
    buf = g_new(gchar, len);

    while ((rv = gethostbyname_r (hostname, &result_buf, buf, len, 
				  &result, &herr)) 
	   == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (!rv)
      list = hostent2ialist(result);

    g_free(buf);
  }

#else
#ifdef HAVE_GETHOSTBYNAME_R_SOLARIS
  {
    size_t len = 8192; /* Stevens suggest this size should be adequate */
    char * buf = NULL;
    struct hostent result;
    struct hostent* he;

    do
      {
	buf = g_renew (gchar, buf, len);
	errno = 0;
	he = gethostbyname_r (hostname, &result, buf, len, &h_errno);
	len += 1024;
      }
    while (errno == ERANGE);

    if (he)
      list = hostent2ialist(&result);

  done:
    g_free(buf);
  }

#else
#ifdef HAVE_GETHOSTBYNAME_R_HPUX
  {
    struct hostent result;
    struct hostent_data buf;
    int rv;

    rv = gethostbyname_r (hostname, &result, &buf);
    if (!rv)
      list = hostent2ialist(&result);
  }

#else 
#ifdef GNET_WIN32
  {
    struct hostent* result;

    WaitForSingleObject(gnet_hostent_Mutex, INFINITE);

    result = gethostbyname(hostname);
    if (result != NULL)
      list = hostent2ialist(result);

    ReleaseMutex(gnet_hostent_Mutex);
  }
#else
  {
    struct hostent* he;

#   ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
    G_LOCK (dnslock);
#   endif

    he = gethostbyname(hostname);
    list = hostent2ialist(he);

#   ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
    G_UNLOCK (dnslock);
#   endif
  }
#endif
#endif
#endif
#endif
#endif
#endif

  if (list)
    list = g_list_reverse (list);

  return list;

}


/* Free list of GInetAddr's.  We assume name hasn't been allocated. */
static void
ialist_free (GList* ialist)
{
  GList* i;

  if (!ialist)
    return;

  for (i = ialist; i != NULL; i = i->next)
    g_free ((GInetAddr*) i->data);
  g_list_free (ialist);
}



/* 

   Thread safe gethostbyaddr (we assume that gethostbyaddr_r follows
   the same pattern as gethostbyname_r, so we don't have special
   checks for it in configure.in.

   Returns: the hostname; NULL on error.
*/

gchar*
gnet_gethostbyaddr(const struct sockaddr_storage* sa)
{
  gchar* rv = NULL;

  /* We assume if there's getaddrinfo(), then there's getnameinfo(). */
#ifdef HAVE_GETADDRINFO
  {
    int gnirv;
    char host[NI_MAXHOST];


#   ifdef HAVE_GETADDRINFO_GLIB_MUTEX
      G_LOCK (dnslock);
#   endif

  again:
    gnirv = getnameinfo((const struct sockaddr*) sa, 
			GNET_SOCKADDR_LEN(*sa),
			host, sizeof(host),
			NULL, 0, NI_NAMEREQD);
    if (gnirv == 0)
      rv = g_strdup (host);
    else if (gnirv == EAGAIN)
      goto again;
    else
      rv = NULL;

#   ifdef HAVE_GETADDRINFO_GLIB_MUTEX
      G_UNLOCK (dnslock);
#   endif

  }
#else
#ifdef HAVE_GETHOSTBYNAME_THREADSAFE
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent* he;

    he = gethostbyaddr(sa_addr, sa_len, sa_type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
  }
#else
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent result_buf, *result;
    size_t len;
    char* buf;
    int herr;
    int res;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyaddr_r (sa_addr, sa_len, sa_type, 
				   &result_buf, buf, len, 	
				   &result, &herr)) 
	   == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (res || result == NULL || result->h_name == NULL)
      goto done;

    rv = g_strdup(result->h_name);

  done:
    g_free(buf);
  }

#else
#ifdef HAVE_GET_HOSTBYNAME_R_SOLARIS
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent result;
    size_t len;
    char* buf;
    int herr;
    int res;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyaddr_r (sa_addr, sa_len, sa_type, &result, buf, len, &herr)) == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (res || hp == NULL || hp->h_name == NULL)
      goto done;

    rv = g_strdup(result->h_name);

  done:
    g_free(buf);
  }

#else
#ifdef HAVE_GETHOSTBYNAME_R_HPUX
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent result;
    struct hostent_data buf;
    int res;

    res = gethostbyaddr_r (sa_addr, sa_len, sa_type, &result, &buf);

    if (res == 0)
      rv = g_strdup (result.h_name);
  }

#else 
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent* he;

    G_LOCK (dnslock);
    he = gethostbyaddr(sa_addr, sa_len, sa_type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
    G_UNLOCK (dnslock);
  }
#else
#ifdef GNET_WIN32
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent* he;

    WaitForSingleObject(gnet_hostent_Mutex, INFINITE);
    he = gethostbyaddr(sa_addr, sa_len, sa_type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
    ReleaseMutex(gnet_hostent_Mutex);
  }
#else
  {
    const char* sa_addr   = GNET_SOCKADDR_ADDRP(*sa);
    size_t      sa_len    = GNET_SOCKADDR_ADDRLEN(*sa);
    int         sa_type   = GNET_SOCKADDR_FAMILY(*sa);

    struct hostent* he;

    he = gethostbyaddr(sa_addr, sa_len, sa_type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
  }
#endif
#endif
#endif
#endif
#endif
#endif
#endif

  return rv;
}



/* **************************************** */


/**
 *  gnet_inetaddr_new
 *  @hostname: host name
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Creates a #GInetAddr from a host name and port.  This function
 *  makes a DNS lookup on the host name so it may block.  The host
 *  name may resolve to multiple addresses.  If this occurs, the first
 *  address in the list is returned.  Use gnet_inetaddr_new_list() to
 *  get the complete list.
 *
 *  Returns: a new #GInetAddr; NULL on error.
 *
 **/
GInetAddr* 
gnet_inetaddr_new (const gchar* hostname, gint port)
{
  GList* ialist;
  GInetAddr* ia = NULL;

  ialist = gnet_gethostbyname (hostname);
  if (!ialist)
    return NULL;

  ia = (GInetAddr*) ialist->data;
  ialist = g_list_remove (ialist, ia);

  GNET_INETADDR_PORT_SET(ia, g_htons(port));

  ialist_free (ialist);

  return ia;
}


/**
 *  gnet_inetaddr_new_list:
 *  @hostname: host name
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Creates a GList of #GInetAddr's from a host name and port.  This
 *  function makes a DNS lookup on the host name so it may block.
 *
 *  Returns: a GList of #GInetAddr's; NULL on error.
 *
 **/
GList*
gnet_inetaddr_new_list (const gchar* hostname, gint port)
{
  GList* ialist;
  GList* i;

  g_return_val_if_fail (hostname != NULL, NULL);

  /* Try to get the host by name (ie, DNS) */
  ialist = gnet_gethostbyname(hostname);
  if (!ialist)
    return NULL;

  /* Set the port */
  for (i = ialist; i != NULL; i = i->next)
    {
      GInetAddr* ia = (GInetAddr*) i->data;
      GNET_INETADDR_PORT_SET(ia, g_htons(port));
    }

  return ialist;
}


/**
 *  gnet_inetaddr_delete_list
 *  @list: GList of #GInetAddr's
 *
 *  Deletes a GList of #GInetAddr's.
 *
 **/
void
gnet_inetaddr_delete_list (GList* list)
{
  GList* i;

  for (i = list; i != NULL; i = i->next)
    gnet_inetaddr_delete ((GInetAddr*) i->data);
  g_list_free (list);
}



/* **************************************** */
/* gnet_inetaddr_new_async()		    */

static void
inetaddr_new_async_cb (GList* ialist, gpointer data);

/**
 *  gnet_inetaddr_new_async
 *  @hostname: host name
 *  @port: port number (0 if the port doesn't matter)
 *  @func: callback function
 *  @data: data to pass to @func on the callback
 * 
 *  Asynchronously creates a #GInetAddr from a host name and port.
 *  The callback is called once the #GInetAddr is created or an error
 *  occurs during lookup.  The callback will not be called during the
 *  call to this function.
 *
 *  See gnet_inetaddr_new_list_async() for implementation notes.
 *
 *  Returns: the ID of the lookup; NULL on failure.  The ID can be used
 *  with gnet_inetaddr_new_async_cancel() to cancel the lookup.
 *
 **/
GInetAddrNewAsyncID 
gnet_inetaddr_new_async (const gchar* hostname, gint port, 
			 GInetAddrNewAsyncFunc func, gpointer data)
{
  GInetAddrNewState*		state;
  GInetAddrNewListAsyncID	list_id;

  state = g_new0(GInetAddrNewState, 1);

  list_id = gnet_inetaddr_new_list_async (hostname, port, inetaddr_new_async_cb, state);
  if (list_id == NULL)
    {
      g_free (state);
      return NULL;
    }

  state->list_id = list_id;
  state->func = func;
  state->data = data;

  return state;
}


/**
 *  gnet_inetaddr_new_async_cancel
 *  @id: ID of the lookup
 *
 *  Cancels an asynchronous #GInetAddr creation that was started with
 *  gnet_inetaddr_new_async().
 *
 **/
void
gnet_inetaddr_new_async_cancel (GInetAddrNewAsyncID async_id)
{
  GInetAddrNewState* state = (GInetAddrNewState*) async_id;

  g_return_if_fail (state);

  if (state->in_callback)
    return;

  gnet_inetaddr_new_list_async_cancel (state->list_id);
  g_free (state);
}


static void
inetaddr_new_async_cb (GList* ialist, gpointer data)
{
  GInetAddrNewState* state = (GInetAddrNewState*) data;

  g_return_if_fail (state);

  state->in_callback = TRUE;

  if (ialist)
    {
      GInetAddr* ia;

      ia = (GInetAddr*) ialist->data;	      /* Get address */
      g_assert (ia);

      ialist = g_list_remove (ialist, ia);    /* Remove from list */
      ialist_free (ialist);		      /* Delete list */

      (*state->func)(ia, state->data);
    }
  else
    {
      (*state->func)(NULL, state->data);
    }

  state->in_callback = FALSE;

  gnet_inetaddr_new_async_cancel (state);
}


/* **************************************** */
/* gnet_inetaddr_new_list_async()	    */


#ifndef GNET_WIN32  /*********** Unix code ***********/

#ifdef HAVE_LIBPTHREAD
static void* inetaddr_new_list_async_pthread (void* arg);
#else

static ssize_t writen (int fd, const void* buf, size_t len);

static ssize_t
writen (int fd, const void* buf, size_t len)
{
  size_t nleft;
  ssize_t nwritten;
  char* ptr;

  nleft = len;
  ptr = (char*) buf;
  while (nleft > 0)
    {
      if ((nwritten = write(fd, ptr, nleft)) < 0)
	{
	  if (errno == EINTR)
	    nwritten = 0;
	  else
	    return -1;
	}
      
      nleft -= nwritten;
      ptr += nwritten;
    }

  return len;
}

#endif


/**
 *  gnet_inetaddr_new_list_async
 *  @hostname: host name
 *  @port: port number (0 if the port doesn't matter)
 *  @func: callback function
 *  @data: data to pass to @func on the callback
 * 
 *  Asynchronously creates a GList of #GInetAddr's from a host name
 *  and port.  The callback is called once the list is created or an
 *  error occurs during lookup.  The callback will not be called
 *  during the call to gnet_inetaddr_new_list_async().  The list
 *  passed in the callback is callee owned.
 *
 *  The Unix version creates a pthread thread which does the lookup.
 *  If pthreads aren't available, it forks and does the lookup.
 *  Forking can be slow or even fail when using operating systems that
 *  copy the entire process when forking.
 *
 *  If you need to lookup hundreds of addresses, we recommend calling
 *  g_main_iteration(FALSE) between calls.  This will help prevent an
 *  explosion of threads or processes.
 *
 *  If you need a more robust library for Unix, look at <ulink
 *  url="http://www.gnu.org/software/adns/adns.html">GNU ADNS</ulink>.
 *  GNU ADNS is under the GNU GPL.  This library does not use threads
 *  or processes.
 *
 *  The Windows version should work fine.  Windows has an asynchronous
 *  DNS lookup function.
 *
 *  Returns: the ID of the lookup; NULL on failure.  The ID can be
 *  used with gnet_inetaddr_new_list_async_cancel() to cancel the
 *  lookup.
 *
 **/
GInetAddrNewListAsyncID
gnet_inetaddr_new_list_async (const gchar* hostname, gint port, 
			      GInetAddrNewListAsyncFunc func, 
			      gpointer data)
{
  GInetAddrNewListState* state = NULL;

  g_return_val_if_fail(hostname != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

# ifdef HAVE_LIBPTHREAD			/* Pthread */
  {
    void** args;
    pthread_attr_t attr;
    pthread_t pthread;
    int rv;

    state = g_new0(GInetAddrNewListState, 1);

    args = g_new (void*, 2);
    args[0] = (void*) g_strdup(hostname);
    args[1] = state;

    pthread_mutex_init (&state->mutex, NULL);
    pthread_mutex_lock (&state->mutex);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  create_again:
    rv = pthread_create (&pthread, &attr, 
			 inetaddr_new_list_async_pthread, args);
    if (rv == EAGAIN)	/* Try again */
      {
	sleep(0);	/* Yield the processor */
	goto create_again;
      }
    else if (rv)	/* Error */
      {
	g_warning ("pthread_create error: %s (%d)\n", g_strerror(rv), rv);
	pthread_mutex_unlock (&state->mutex);
	pthread_mutex_destroy (&state->mutex);
	pthread_attr_destroy (&attr);
	g_free (args[0]);
	g_free (state);
	return NULL;
      }

    pthread_attr_destroy (&attr);

  }
# else 					/* Fork */
  {
    int pipes[2];
    pid_t pid = -1;

    /* Open a pipe */
    if (pipe(pipes) == -1)
      return NULL;

    /* Fork to do the look up. */
  fork_again:
    errno = 0;
    /* Child: Do lookup, write result to pipe */
    if ((pid = fork()) == 0)
      {
	int outfd = pipes[1];
	GList* ialist;

	close (pipes[0]);

	/* Try to get the host by name (ie, DNS) */
	if ((ialist = gnet_gethostbyname(hostname)) != NULL)
	  {
	    GList* i;
	    char* buf;
	    int buflen = 0;
	    char* bufp;
	    int rv;

	    /* Calculate size needed for buffer.  The buffer is a list
	       of addresses.  Each address is preceeded by one byte,
	       which is the size of the address.  The last item in the
	       list is a zero. */
	    for (i = ialist; i != NULL; i = i->next)
	      {
		GInetAddr* ia;
		guchar size;

		ia = (GInetAddr*) ialist->data;
		size = GNET_SOCKADDR_ADDRLEN(ia->sa);

		buflen += 1 + size;
	      }
	    buflen++;	/* 1 more byte for the ending zero */

	    /* Copy addresses to buffer */
	    buf = bufp = g_new(guchar, buflen);
	    for (i = ialist; i != NULL; i = i->next)
	      {
		GInetAddr* ia;
		guchar size;

		ia = (GInetAddr*) i->data;
		size = GNET_SOCKADDR_ADDRLEN(ia->sa);

		bufp[0] = size;
		bufp++;

		memcpy (bufp, GNET_SOCKADDR_ADDRP(ia->sa), size);
		bufp += size;
	      }
	    *bufp = '\0';
	    ialist_free (ialist);

	    /* Write buffer */
	    rv = writen (outfd, buf, buflen);
	    if (rv != buflen)
	      g_warning ("Error writing to pipe: %s\n", 
			 g_strerror(errno));
	    close (outfd);
	  }
	else
	  {
	    /* Do nothing.  Exit will close the pipe. */
	  }

	/* Exit (we don't want atexit called, so do _exit instead) */
	_exit(EXIT_SUCCESS);
      }

    /* Parent: Set up state */
    else if (pid > 0)
      {
	close (pipes[1]);

	state = g_new0(GInetAddrNewListState, 1);
	state->pid = pid;
	state->fd = pipes[0];
	state->iochannel = gnet_private_io_channel_new(pipes[0]);
	state->watch = g_io_add_watch(state->iochannel,
				      (G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL),
				      gnet_inetaddr_new_list_async_cb, 
				      state);
      }

    /* Try again */
    else if (errno == EAGAIN)
      {
	sleep(0);	/* Yield the processor */
	goto fork_again;
      }

    /* Else fork failed completely */
    else
      {
	g_warning ("fork error: %s (%d)\n", g_strerror(errno), errno);
	return NULL;
      }
  }
#endif

  /* Finish setting up state for callback */
  g_assert (state);
  state->port = port;
  state->func = func;
  state->data = data;

# ifdef HAVE_LIBPTHREAD
  {
    pthread_mutex_unlock (&state->mutex);
  }
# endif

  return state;
}

#ifdef HAVE_LIBPTHREAD	/* ********** UNIX Pthread ********** */

static gboolean inetaddr_new_list_async_pthread_dispatch (gpointer data);


static void*
inetaddr_new_list_async_pthread (void* arg)
{
  void** args = (void**) arg;
  gchar* name = (gchar*) args[0];
  GInetAddrNewListState* state = (GInetAddrNewListState*) args[1];
  GList* ialist;

  g_free (args);

  /* Do lookup */
  ialist = gnet_gethostbyname (name);
  g_free (name);

  /* Lock */
  pthread_mutex_lock (&state->mutex);

  /* If cancelled, destroy state and exit.  The main thread is no
     longer using state. */
  if (state->is_cancelled)
    {
      ialist_free (state->ias);

      pthread_mutex_unlock (&state->mutex);
      pthread_mutex_destroy (&state->mutex);

      g_free (state);

      return NULL;
    }

  if (ialist)
    {
      GList* i;

      /* Set the port */
      for (i = ialist; i != NULL; i = i->next)
	{
	  GInetAddr* ia = (GInetAddr*) i->data;
          GNET_INETADDR_PORT_SET(ia, g_htons(state->port));
	}

      /* Save the list */
      state->ias = ialist;
    }
  else
    {
      /* Flag failure */
      state->lookup_failed = TRUE;
    }

  /* Add a source for reply */
  state->source = g_idle_add_full (G_PRIORITY_DEFAULT, 
				   inetaddr_new_list_async_pthread_dispatch,
				   state, NULL);

  /* Unlock */
  pthread_mutex_unlock (&state->mutex);

  /* Thread exits... */
  return NULL;
}


static gboolean
inetaddr_new_list_async_pthread_dispatch (gpointer data)
{
  GInetAddrNewListState* state = (GInetAddrNewListState*) data;

  pthread_mutex_lock (&state->mutex);

  state->in_callback = TRUE;

  /* Upcall */
  if (!state->lookup_failed)
    (*state->func)(state->ias, state->data);
  else
    (*state->func)(NULL, state->data);

  state->in_callback = FALSE;

  /* Delete state */
  g_source_remove (state->source);
  pthread_mutex_unlock (&state->mutex);
  pthread_mutex_destroy (&state->mutex);
  g_free (state);

  return FALSE;
}


/**
 *  gnet_inetaddr_new_list_async_cancel
 *  @id: ID of the lookup
 *
 *  Cancels an asynchronous #GInetAddr list creation that was started
 *  with gnet_inetaddr_new_list_async().
 * 
 **/
void
gnet_inetaddr_new_list_async_cancel (GInetAddrNewListAsyncID id)
{
  GInetAddrNewListState* state = (GInetAddrNewListState*) id;

  g_return_if_fail (state);

  if (state->in_callback)
    return;

  /* We don't use in_callback because we'd have to get the mutex to
     access it and if we're in the callback we'd already have the
     mutex and deadlock.  in_callback was mostly meant to prevent
     programmer error.  */

  pthread_mutex_lock (&state->mutex);

  /* Check if the thread has finished and a reply is pending.  If a
     reply is pending, cancel it and delete state. */
  if (state->source)
    {
      g_source_remove (state->source);

      ialist_free (state->ias);

      pthread_mutex_unlock (&state->mutex);
      pthread_mutex_destroy (&state->mutex);

      g_free (state);
    }

  /* Otherwise, the thread is still running.  Set the cancelled flag
     and allow the thread to complete.  When the thread completes, it
     will delete state.  We can't kill the thread because we'd loose
     the resources allocated in gethostbyname_r. */

  else
    {
      state->is_cancelled = TRUE;

      pthread_mutex_unlock (&state->mutex);
    }
}

#else		/* ********** UNIX process ********** */


gboolean 
gnet_inetaddr_new_list_async_cb (GIOChannel* iochannel, 
				 GIOCondition condition, 
				 gpointer data)
{
  GInetAddrNewListState* state = (GInetAddrNewListState*) data;

  g_assert (!state->in_callback);

  /* Read from the pipe */
  if (condition & G_IO_IN)
    {
      gint rv;
      guchar* buf;
      gint space;

      buf = &state->buffer[state->len];
      space = sizeof(state->buffer) - state->len;

      if ((rv = read(state->fd, buf, space)) > 0)
	{
	  guchar* end;
	  gboolean done = FALSE;

	  state->len += rv;
	  end = state->buffer + state->len;

	  /* Parse as many addresses as possible */
	  buf = state->buffer;
	  while (buf != end)
	    {
	      guchar size;
	      GInetAddr* ia;

	      /* Get the size */
	      size = buf[0];
	      buf++;

	      /* Break if done */
	      if (size == 0)
		{
		  done = TRUE;
		  break;
		}

	      /* Break if part of the address is missing.  We will
		 read it later. */
	      if ((buf + size) > end)
		break;

	      /* Skip if a weird size */
	      if (size != 4 && size != 16)
		{
		  buf += size;
		  continue;
		}

	      /* Save the address */
	      ia = g_new0(GInetAddr, 1);
	      ia->ref_count = 1;
	      ia->sa.ss_family = (size == 4)? AF_INET : AF_INET6;
	      GNET_INETADDR_SET_SS_LEN(ia);
	      memcpy(GNET_INETADDR_ADDRP(ia), buf, size);
	      GNET_INETADDR_PORT(ia) = g_htons(state->port);

	      state->ias = g_list_prepend (state->ias, ia);

	      buf += size;
	    }

	  /* If not done, move the rest of the buffer over and read
	     more */
	  if (!done)
	    {
	      int bytes_parsed;

	      bytes_parsed = buf - state->buffer;
	      g_memmove (state->buffer, buf, bytes_parsed);
	      state->len -= bytes_parsed;
	      
	      return TRUE;
	    }

	  /* Otherwise, we're done! */

	  /* Reverse the list */
	  state->ias = g_list_reverse (state->ias);

	  /* Make callback (callee owns list) */
	  state->in_callback = TRUE;
	  (*state->func)(state->ias, state->data);
	  state->ias = NULL;
	  state->in_callback = FALSE;
	  gnet_inetaddr_new_list_async_cancel (state);
	  return FALSE;
	}
      /* otherwise, there was an error.  we shouldn't read 0 - we'll
	 get a zero-length size when they're done sending. */
    }
  /* otherwise, there was an error */

  state->in_callback = TRUE;
  (*state->func)(NULL, state->data);
  state->in_callback = FALSE;
  gnet_inetaddr_new_list_async_cancel(state);
  return FALSE;
}



void
gnet_inetaddr_new_list_async_cancel (GInetAddrNewListAsyncID id)
{
  GInetAddrNewListState* state = (GInetAddrNewListState*) id;

  g_return_if_fail (state);

  /* Ignore if in callback */
  if (state->in_callback)
    return;

  ialist_free (state->ias);
  g_source_remove (state->watch);
  g_io_channel_unref (state->iochannel);

  close (state->fd);
  kill (state->pid, SIGKILL);
  waitpid (state->pid, NULL, 0);

  memset (state, 0, sizeof(*state));
  g_free (state);
}


#endif	/* UNIX */




#else	/*********** Windows code ***********/


GInetAddrNewListAsyncID
gnet_inetaddr_new_list_async (const gchar* hostname, gint port,
			      GInetAddrNewListAsyncFunc func, 
			      gpointer data)
{
  GInetAddr* ia;
  struct sockaddr_in* sa_in;
  GInetAddrNewListState* state;

  g_return_val_if_fail(hostname != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create a new InetAddr */
  ia = g_new0(GInetAddr, 1);
  ia->name = g_strdup(hostname);
  ia->ref_count = 1;

  sa_in = (struct sockaddr_in*) &ia->sa;	  /* WINFIX */
  sa_in->sin_family = AF_INET;
  sa_in->sin_port = g_htons(port);

  /* Create a structure for the call back */
  state = g_new0(GInetAddrNewListState, 1);
  state->ias = NULL;
  state->ias = g_list_prepend(state->ias, ia);
  state->func = func;
  state->data = data;

  state->WSAhandle = (int) WSAAsyncGetHostByName(gnet_hWnd, IA_NEW_MSG, hostname, 
						 state->hostentBuffer, 
						 sizeof(state->hostentBuffer));

  if (!state->WSAhandle)
    {
      gnet_inetaddr_delete (ia);
      g_list_free(state->ias);
      g_free (state);
      return NULL;
    }

  /*get a lock and insert the state into the hash */
  WaitForSingleObject(gnet_Mutex, INFINITE);
  g_hash_table_insert(gnet_hash, (gpointer)state->WSAhandle, (gpointer)state);
  ReleaseMutex(gnet_Mutex);

  return state;
}


gboolean
gnet_inetaddr_new_list_async_cb (GIOChannel* iochannel,
				 GIOCondition condition,
				 gpointer data)
{

  GInetAddrNewListState* state = (GInetAddrNewListState*) data;
  GInetAddr* ia = (GInetAddr*) state->ias->data;
  struct hostent *result;
  struct sockaddr_in *sa_in;

  if (state->errorcode)
    {
      state->in_callback = TRUE;
      (*state->func)(NULL, state->data);
      state->in_callback = FALSE;
      g_free (state);
      return FALSE;
    }

  result = (struct hostent*) state->hostentBuffer;

  sa_in = (struct sockaddr_in*) &ia->sa;
  memcpy(&sa_in->sin_addr, result->h_addr_list[0], result->h_length);

  ia->name = g_strdup(result->h_name);

  state->in_callback = TRUE;
  (*state->func)(state->ias, state->data);
  state->in_callback = FALSE;
  g_free(state);

  return FALSE;
}


void
gnet_inetaddr_new_list_async_cancel (GInetAddrNewListAsyncID id)
{
  GInetAddrNewListState* state = (GInetAddrNewListState*) id;

  g_return_if_fail(state != NULL);
  if (state->in_callback)
    return;

  ialist_free(state->ias);
  WSACancelAsyncRequest((HANDLE)state->WSAhandle);

  /*get a lock and remove the hash entry */
  WaitForSingleObject(gnet_Mutex, INFINITE);
  g_hash_table_remove(gnet_hash, (gpointer)state->WSAhandle);
  ReleaseMutex(gnet_Mutex);
  g_free(state);
}


#endif		/*********** End Windows code ***********/



/**
 *  gnet_inetaddr_new_nonblock:
 *  @hostname: host name
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Creates a #GInetAddr from a host name and port without blocking.
 *  This function does not make a DNS lookup and will fail if creating
 *  the address would require a DNS lookup.
 *
 *  Returns: a new #GInetAddr, or NULL if there was a failure or the
 *  function would block.
 *
 **/
GInetAddr* 
gnet_inetaddr_new_nonblock (const gchar* hostname, gint port)
{
  GInetAddr* ia = NULL;
  struct in_addr inaddr;
#ifdef HAVE_IPV6
  struct in6_addr in6addr;
#endif

  g_return_val_if_fail (hostname, NULL);

  if (inet_pton(AF_INET, hostname, &inaddr) != 0)
    {
      struct sockaddr_in* sa_inp;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa_inp = (struct sockaddr_in*) &ia->sa;

      sa_inp->sin_family = AF_INET;
      GNET_INETADDR_SET_SS_LEN(ia);
      sa_inp->sin_addr = inaddr;
      sa_inp->sin_port = g_htons(port);
    }
#ifdef HAVE_IPV6
  else if (inet_pton(AF_INET6, hostname, &in6addr) != 0)
    {
      struct sockaddr_in6* sa_inp;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa_inp = (struct sockaddr_in6*) &ia->sa;

      sa_inp->sin6_family = AF_INET6;
      GNET_INETADDR_SET_SS_LEN(ia);
      sa_inp->sin6_addr = in6addr;
      sa_inp->sin6_port = g_htons(port);
    }
#endif
  else
    return NULL;

  return ia;
}



/**
 *  gnet_inetaddr_new_bytes
 *  @bytes: address in raw bytes
 *  @length: length of @bytes (4 if IPv4, 16 if IPv6)
 * 
 *  Creates a #GInetAddr from raw bytes.  The bytes should be in
 *  network byte order (big endian).  There should be 4 bytes if it's
 *  an IPv4 address and 16 bytees if it's an IPv6 address.  The port
 *  is set to 0.
 *
 *  Returns: a new #GInetAddr, or NULL if there was a failure.
 *
 **/
GInetAddr* 
gnet_inetaddr_new_bytes (const gchar* bytes, const guint length)
{
  GInetAddr* ia = NULL;

  g_return_val_if_fail (bytes, NULL);

  if (length != 4 && length != 16)
    return NULL;

  ia = g_new0 (GInetAddr, 1);
  ia->ref_count = 1;
  if (length == 4)
    GNET_INETADDR_FAMILY(ia) = AF_INET;
#ifdef HAVE_IPV6
  else
    GNET_INETADDR_FAMILY(ia) = AF_INET6;
#endif
  GNET_INETADDR_SET_SS_LEN(ia);
  memcpy(GNET_INETADDR_ADDRP(ia), bytes, length);

  return ia;
}


/**
 *   gnet_inetaddr_clone:
 *   @inetaddr: a #GInetAddr
 *
 *   Copies a #GInetAddr.
 *
 *   Returns: a copy of @inetaddr.
 *
 **/
GInetAddr* 
gnet_inetaddr_clone(const GInetAddr* inetaddr)
{
  GInetAddr* cia;

  g_return_val_if_fail (inetaddr != NULL, NULL);

  cia = g_new0(GInetAddr, 1);
  cia->ref_count = 1;
  cia->sa = inetaddr->sa;
  if (inetaddr->name != NULL) 
    cia->name = g_strdup(inetaddr->name);

  return cia;
}


/** 
 *  gnet_inetaddr_delete
 *  @inetaddr: a #GInetAddr
 *
 *  Deletes a #GInetAddr.
 *
 **/
void
gnet_inetaddr_delete (GInetAddr* inetaddr)
{
  if (inetaddr != NULL)
    gnet_inetaddr_unref (inetaddr);
}


/**
 *  gnet_inetaddr_ref
 *  @inetaddr: a #GInetAddr
 *
 *  Adds a reference to a #GInetAddr.
 *
 **/
void
gnet_inetaddr_ref (GInetAddr* inetaddr)
{
  g_return_if_fail(inetaddr != NULL);

  inetaddr->ref_count++;
}


/**
 *  gnet_inetaddr_unref
 *  @inetaddr: a #GInetAddr
 *
 *  Removes a reference from a #GInetAddr.  A #GInetAddr is deleted
 *  when the reference count reaches 0.
 *
 **/
void
gnet_inetaddr_unref (GInetAddr* inetaddr)
{
  g_return_if_fail(inetaddr != NULL);

  inetaddr->ref_count--;

  if (inetaddr->ref_count == 0)
    {
      if (inetaddr->name != NULL) 
	g_free (inetaddr->name);
      g_free (inetaddr);
    }
}



/**
 *  gnet_inetaddr_get_name:
 *  @inetaddr: a #GInetAddr
 *
 *  Gets the host name for a #GInetAddr.  This functions makes a
 *  reverse DNS lookup on the address so it may block.  The canonical
 *  name is returned if the address has no host name.
 *
 *  Returns: the host name for the @inetaddr; NULL on error.
 *
 **/
gchar* 
gnet_inetaddr_get_name (/* const */ GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, NULL);

  if (inetaddr->name == NULL)
    {
      gchar* name;

      if ((name = gnet_gethostbyaddr(&inetaddr->sa)) != NULL)
	inetaddr->name = name;
      else
	inetaddr->name = gnet_inetaddr_get_canonical_name(inetaddr);
    }

  g_return_val_if_fail (inetaddr->name, NULL);
  return g_strdup(inetaddr->name);
}



/**
 *  gnet_inetaddr_get_name_nonblock:
 *  @inetaddr: a #GInetAddr
 *
 *  Gets the host name for a #GInetAddr.  This function does not make
 *  a reverse DNS lookup and will fail if getting the name would
 *  require a reverse DNS lookup.
 *
 *  Returns: the host name for the @inetaddr, or NULL if there was an
 *  error or it would require blocking.
 *
 **/
gchar* 
gnet_inetaddr_get_name_nonblock (GInetAddr* inetaddr)
{
  if (inetaddr->name)
    return g_strdup(inetaddr->name);

  return NULL;
}



#ifndef GNET_WIN32  /*********** Unix code ***********/

#ifdef HAVE_LIBPTHREAD
static void* inetaddr_get_name_async_pthread (void* arg);
#endif


/**
 *  gnet_inetaddr_get_name_async:
 *  @inetaddr: a #GInetAddr
 *  @func: callback function
 *  @data: data to pass to @func on the callback
 *
 *  Asynchronously gets the host name for a #GInetAddr.  The callback
 *  is called once the reverse DNS lookup is complete.  The call back
 *  will not be called during the call to this function.
 *
 *  See gnet_inetaddr_new_list_async() for implementation notes.
 *
 *  Returns: the ID of the lookup; NULL on failure.  The ID can be
 *  used with gnet_inetaddr_get_name_async_cancel() to cancel the
 *  lookup.
 *
 **/
GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async (GInetAddr* inetaddr, 
			      GInetAddrGetNameAsyncFunc func,
			      gpointer data)
{
  GInetAddrReverseAsyncState* state = NULL;

  g_return_val_if_fail(inetaddr != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

# ifdef HAVE_LIBPTHREAD			/* Pthread */
  {
    pthread_attr_t attr;
    pthread_t pthread;
    int rv;

    state = g_new0(GInetAddrReverseAsyncState, 1);

    pthread_mutex_init (&state->mutex, NULL);
    pthread_mutex_lock (&state->mutex);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  create_again:
    rv = pthread_create (&pthread, &attr, inetaddr_get_name_async_pthread, state);
    if (rv == EAGAIN)
      {
	sleep(0);	/* Yield the processor */
	goto create_again;
      }
    else if (rv)
      {
	g_warning ("Pthread_create error: %s (%d)\n", g_strerror(rv), rv);
	pthread_mutex_unlock (&state->mutex);
	pthread_mutex_destroy (&state->mutex);
	pthread_attr_destroy (&attr);
	g_free (state);
	return NULL;
      }

    pthread_attr_destroy (&attr);


  }
# else 					/* Fork */
  {
    int pipes[2];
    pid_t pid = -1;

    /* Open a pipe */
    if (pipe(pipes) == -1)
      return NULL;

    /* Fork to do the look up. */
  fork_again:
    errno = 0;
    if ((pid = fork()) == 0)
      {
	int outfd = pipes[1];
	gchar* name;
	guchar len;
	
	close (pipes[0]);

	if (inetaddr->name)
	  name = g_strdup(inetaddr->name);
	else
	  name = gnet_gethostbyaddr(&inetaddr->sa);

	/* Write the name to the pipe.  If we didn't get a name, we
	   just write the canonical name. */
	if (name)
	  {
	    guint lenint = strlen(name);

	    if (lenint > 255)
	      {
		g_warning ("Truncating domain name: %s\n", name);
		name[256] = '\0';
		lenint = 255;
	      }

	    len = lenint;

	    if ((write(outfd, &len, sizeof(len)) == -1) ||
		(write(outfd, name, len) == -1) )
	      g_warning ("Error writing to pipe: %s\n", g_strerror(errno));
	  }
	else
	  {
	    char buffer[INET6_ADDRSTRLEN];
	    const char* rv;

	    rv = inet_ntop(GNET_INETADDR_FAMILY(inetaddr), 
			   GNET_INETADDR_ADDRP(inetaddr),
			   buffer, sizeof(buffer));
	    g_assert (rv);
	    len = strlen(buffer);

	    if ((write(outfd, &len, sizeof(len)) == -1) ||
		(write(outfd, buffer, len) == -1))
	      g_warning ("Error writing to pipe: %s\n", g_strerror(errno));
	  }

	/* Close the socket */
	close (outfd);

	/* Exit (we don't want atexit called, so do _exit instead) */
	_exit(EXIT_SUCCESS);

      }

    /* Set up an IOChannel to read from the pipe */
    else if (pid > 0)
      {
	close (pipes[1]);

	/* Create a structure for the call back */
	state = g_new0(GInetAddrReverseAsyncState, 1);
	state->pid = pid;
	state->fd = pipes[0];
	state->iochannel = gnet_private_io_channel_new(pipes[0]);
	state->watch = g_io_add_watch(state->iochannel,
				      (G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL),
				      gnet_inetaddr_get_name_async_cb, 
				      state);
      }

    /* Try again */
    else if (errno == EAGAIN)
      {
	sleep(0);	/* Yield the processor */
	goto fork_again;
      }

    /* Else there was a goofy error */
    else
      {
	g_warning ("fork error: %s (%d)\n", g_strerror(errno), errno);
	return NULL;
      }
  }
# endif

  /* Set up state for callback */
  g_assert (state);
  state->ia = gnet_inetaddr_clone(inetaddr);
  state->func = func;
  state->data = data;

# ifdef HAVE_LIBPTHREAD
  {
    pthread_mutex_unlock (&state->mutex);
  }
# endif

  return state;
}


#ifdef HAVE_LIBPTHREAD	/* ********** UNIX Pthread ********** */

static gboolean inetaddr_get_name_async_pthread_dispatch (gpointer data);


/**
 *  gnet_inetaddr_get_name_async_cancel:
 *  @id: ID of the lookup
 *
 *  Cancels an asynchronous name lookup that was started with
 *  gnet_inetaddr_get_name_async().
 * 
 */
void
gnet_inetaddr_get_name_async_cancel(GInetAddrGetNameAsyncID id)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) id;

  pthread_mutex_lock (&state->mutex);

  /* Check if the thread has finished and a reply is pending.  If a
     reply is pending, cancel it and delete state. */
  if (state->source)
    {
      g_free (state->name);

      g_source_remove (state->source);

      pthread_mutex_unlock (&state->mutex);
      pthread_mutex_destroy (&state->mutex);

      g_free (state);
    }

  /* Otherwise, the thread is still running.  Set the cancelled flag
     and allow the thread to complete.  When the thread completes, it
     will delete state.  We can't kill the thread because we'd loose
     the resources allocated in gethostbyname_r. */

  else
    {
      state->is_cancelled = TRUE;

      pthread_mutex_unlock (&state->mutex);
    }
}


static gboolean
inetaddr_get_name_async_pthread_dispatch (gpointer data)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) data;

  pthread_mutex_lock (&state->mutex);

  /* Upcall */
  (*state->func)(state->name, state->data);

  /* Delete state */
  gnet_inetaddr_delete (state->ia);
  g_source_remove (state->source);
  pthread_mutex_unlock (&state->mutex);
  pthread_mutex_destroy (&state->mutex);
  memset (state, 0, sizeof(*state));
  g_free (state);

  return FALSE;
}


static void* 
inetaddr_get_name_async_pthread (void* arg)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) arg;
  gchar* name;

  /* Lock */
  pthread_mutex_lock (&state->mutex);
     
  /* Do lookup */
  if (state->ia->name)
    name = g_strdup (state->ia->name);
  else
    name = gnet_gethostbyaddr(&state->ia->sa);

  /* If cancelled, destroy state and exit.  The main thread is no
     longer using state. */
  if (state->is_cancelled)
    {
      g_free (name);
      gnet_inetaddr_delete (state->ia);

      pthread_mutex_unlock (&state->mutex);
      pthread_mutex_destroy (&state->mutex);

      g_free (state);

      return NULL;
    }

  /* Copy name to state */
  if (name)
    {
      state->name = name;
    }
  else 	/* Lookup failed: name is canonical name */
    {
      gchar buffer[INET_ADDRSTRLEN];	/* defined in netinet/in.h */
      guchar* p = (guchar*) &(GNET_SOCKADDR_IN(state->ia->sa).sin_addr);

      g_snprintf (buffer, sizeof(buffer), 
		 "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

      state->name = g_strdup(buffer);
    }

  /* Add a source for reply */
  state->source = g_idle_add_full (G_PRIORITY_DEFAULT, 
				   inetaddr_get_name_async_pthread_dispatch,
				   state, NULL);

  /* Unlock */
  pthread_mutex_unlock (&state->mutex);

  /* Thread exits... */
  return NULL;
}




#else		/* ********** UNIX process ********** */


gboolean 
gnet_inetaddr_get_name_async_cb (GIOChannel* iochannel, 
				 GIOCondition condition, 
				 gpointer data)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) data;
  gchar* name = NULL;

  g_return_val_if_fail (state, FALSE);
  g_return_val_if_fail (!state->in_callback, FALSE);

  /* Read from the pipe */
  if (condition & G_IO_IN)
    {
      int rv;
      char* buf;
      int length;

      buf = &state->buffer[state->len];
      length = sizeof(state->buffer) - state->len;

      if ((rv = read(state->fd, buf, length)) >= 0)
	{
	  state->len += rv;

	  /* Return true if there's more to read */
	  if ((state->len - 1) != state->buffer[0])
	    return TRUE;

	  /* Copy the name */
	  name = g_new(gchar, state->buffer[0] + 1);
	  memcpy (name, &state->buffer[1], state->buffer[0]);
	  name[state->buffer[0]] = '\0';
	  if (state->ia->name) 
	    g_free(state->ia->name);
	  state->ia->name = name;

	  /* Remove the watch now in case we don't return immediately */
	  g_source_remove (state->watch);
	  state->watch = 0;

	  /* Call back */
	  state->in_callback = TRUE;
	  (*state->func)(name, state->data);
	  state->in_callback = FALSE;
	  gnet_inetaddr_get_name_async_cancel(state);
	  return FALSE;
	}
      /* otherwise, there was a read error */
    }
  /* otherwise, there was some error */

  /* Call back */
  state->in_callback = TRUE;
  (*state->func)(NULL, state->data);
  state->in_callback = FALSE;
  gnet_inetaddr_get_name_async_cancel(state);
  return FALSE;
}



void
gnet_inetaddr_get_name_async_cancel (GInetAddrGetNameAsyncID id)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) id;

  g_return_if_fail(state != NULL);

  if (state->in_callback)
    return;

  gnet_inetaddr_delete (state->ia);
  if (state->watch)
    g_source_remove (state->watch);
  close (state->fd);
  kill (state->pid, SIGKILL);
  waitpid (state->pid, NULL, 0);

  g_free(state);
}

#endif


#else	/*********** Windows code ***********/


GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async(GInetAddr* inetaddr,
			     GInetAddrGetNameAsyncFunc func,
			     gpointer data)
{

  GInetAddrReverseAsyncState* state;
  struct sockaddr_in* sa_in;

  g_return_val_if_fail(inetaddr != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create a structure for the call back */
  state = g_new0(GInetAddrReverseAsyncState, 1);
  state->ia = gnet_inetaddr_clone(inetaddr);
  state->func = func;
  state->data = data;

  sa_in = (struct sockaddr_in*)&inetaddr->sa;
	
  state->WSAhandle = (int) WSAAsyncGetHostByAddr(gnet_hWnd, GET_NAME_MSG,
						 (const char*)&sa_in->sin_addr,
						 (int) (sizeof(&sa_in->sin_addr)),
						 (int)&sa_in->sin_family,
						 state->hostentBuffer,
						 sizeof(state->hostentBuffer));

  if (!state->WSAhandle)
    {
      g_free(state);
      return NULL;
    }
	
  /*get a lock and insert the state into the hash */
  WaitForSingleObject(gnet_Mutex, INFINITE);
  g_hash_table_insert(gnet_hash, (gpointer)state->WSAhandle, (gpointer)state);
  ReleaseMutex(gnet_Mutex);

  return state;
}

gboolean
gnet_inetaddr_get_name_async_cb (GIOChannel* iochannel,
				 GIOCondition condition,
				 gpointer data)
{
  GInetAddrReverseAsyncState* state;
  gchar* name;
  struct hostent* result;
  state = (GInetAddrReverseAsyncState*) data;


  result = (struct hostent*)state->hostentBuffer;

  if (state->errorcode)
    {
      (*state->func)(NULL, state->data);
      return FALSE;
    }

  state->ia->name = g_strdup(result->h_name);
  name = NULL;
  name = g_strdup(state->ia->name);

  (*state->func)(name, state->data);

  gnet_inetaddr_delete (state->ia);
  g_free(state);
  return FALSE;
}


void
gnet_inetaddr_get_name_async_cancel(GInetAddrGetNameAsyncID id)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) id;

  g_return_if_fail(state != NULL);

  gnet_inetaddr_delete (state->ia);
  WSACancelAsyncRequest((HANDLE)state->WSAhandle);
  /*get a lock and remove the hash entry */
  WaitForSingleObject(gnet_Mutex, INFINITE);
  g_hash_table_remove(gnet_hash, (gpointer)state->WSAhandle);
  ReleaseMutex(gnet_Mutex);
  g_free(state);
}

#endif		/*********** End Windows code ***********/


/**
 *  gnet_inetaddr_get_length
 *  @inetaddr: a #GInetAddr
 *
 *  Get the length of a #GInetAddr's address in bytes.  An IPv4
 *  address is 4 bytes long.  An IPv6 address is 16 bytes long.
 *
 *  Returns: the length in bytes.
 *
 **/
gint
gnet_inetaddr_get_length (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr, 0);

  return GNET_INETADDR_ADDRLEN(inetaddr);
}



/**
 *  gnet_inetaddr_get_bytes
 *  @inetaddr: a #GInetAddr
 *  @buffer: buffer to store address in
 *
 *  Get a #GInetAddr's address as bytes.  @buffer should be 4 bytes
 *  long for an IPv4 address or 16 bytes long for an IPv6 address.
 *  Use %GNET_INETADDR_MAX_LEN when allocating a static buffer and
 *  gnet_inetaddr_get_length() when allocating a dynamic buffer.
 *
 **/
void
gnet_inetaddr_get_bytes (const GInetAddr* inetaddr, gchar* buffer)
{
  g_return_if_fail (inetaddr);
  g_return_if_fail (buffer);

  memcpy (buffer, GNET_INETADDR_ADDRP(inetaddr), 
	  GNET_INETADDR_ADDRLEN(inetaddr));
}


/**
 *  gnet_inetaddr_set_bytes
 *  @inetaddr: a #GInetAddr
 *  @bytes: address in raw bytes
 *  @length: length of @bytes
 *
 *  Sets the address of a #GInetAddr from bytes.  @buffer will be
 *  4 bytes long for an IPv4 address and 16 bytes long for an IPv6
 *  address.
 *
 **/
void
gnet_inetaddr_set_bytes (GInetAddr* inetaddr, 
			 const gchar* bytes, gint length)
{
  gint port;

  g_return_if_fail (inetaddr);
  g_return_if_fail (bytes);
  g_return_if_fail (length == 4 || length == 16);

  /* Save the port.  If the family changes and it's location moves, it
     could be trashed. */
  port = GNET_INETADDR_PORT(inetaddr);

  if (length == 4)
    GNET_INETADDR_FAMILY(inetaddr) = AF_INET;
#ifdef HAVE_IPV6
  else if (length == 16)
    GNET_INETADDR_FAMILY(inetaddr) = AF_INET6;
#endif
  GNET_INETADDR_SET_SS_LEN(inetaddr);
  memcpy (GNET_INETADDR_ADDRP(inetaddr), bytes, length);
  GNET_INETADDR_PORT_SET(inetaddr, port);
}




/**
 *  gnet_inetaddr_get_canonical_name:
 *  @inetaddr: a #GInetAddr
 *
 *  Gets the canonical name of a #GInetAddr.  An IPv4 canonical name
 *  is a dotted decimal name (e.g., 141.213.8.59).  An IPv6 canonical
 *  name is a semicoloned hexidecimal name (e.g., 23:de:ad:be:ef).
 *
 *  Returns: the canonical name; NULL on error.
 *
 **/
gchar* 
gnet_inetaddr_get_canonical_name(const GInetAddr* inetaddr)
{
  gchar buffer[INET6_ADDRSTRLEN];	/* defined in netinet/in.h */
  
  g_return_val_if_fail (inetaddr != NULL, NULL);

  if (inet_ntop(GNET_INETADDR_FAMILY(inetaddr), 
		GNET_INETADDR_ADDRP(inetaddr),
		buffer, sizeof(buffer)) == NULL)
    return NULL;
  return g_strdup(buffer);
}


/**
 *  gnet_inetaddr_get_port:
 *  @inetaddr: a #GInetAddr
 *
 *  Gets the port number of a #GInetAddr.  
 *
 *  Returns: the port number.
 */
gint
gnet_inetaddr_get_port(const GInetAddr* inetaddr)
{
  g_return_val_if_fail(inetaddr != NULL, -1);

  return (gint) g_ntohs(GNET_INETADDR_PORT(inetaddr));
}


/**
 *  gnet_inetaddr_set_port:
 *  @inetaddr: a #GInetAddr
 *  @port: new port number
 *
 *  Set the port number of a #GInetAddr.
 *
 **/
void
gnet_inetaddr_set_port(const GInetAddr* inetaddr, gint port)
{
  g_return_if_fail(inetaddr != NULL);

  GNET_INETADDR_PORT_SET(inetaddr, g_htons(port));
}



/* **************************************** */


/**
 *  gnet_inetaddr_is_canonical
 *  @hostname: host name
 *
 *  Checks if the host name is in canonical form.
 *
 *  Returns: TRUE if @name is canonical; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_canonical (const gchar* name)
{
  char buf[16];

  g_return_val_if_fail (name, FALSE);

  if (inet_pton(AF_INET, name, buf) == 1)
    return TRUE;

#ifdef HAVE_IPV6
 {
   if (inet_pton(AF_INET6, name, buf) == 1)
     return TRUE;
 }
#endif

 return FALSE;
}



/**
 *  gnet_inetaddr_is_internet
 *  @inetaddr: a #GInetAddr
 * 
 *  Checks if a #GInetAddr is a sensible internet address.  This mean
 *  it is not private, reserved, loopback, multicast, or broadcast.
 *
 *  Note that private and loopback address are often valid addresses,
 *  so this should only be used to check for general internet
 *  connectivity.  That is, if the address passes, it is reachable on
 *  the internet.
 *
 *  Returns: TRUE if the address is an internet address; FALSE
 *  otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_internet (const GInetAddr* inetaddr)
{
  g_return_val_if_fail(inetaddr != NULL, FALSE);

  if (!gnet_inetaddr_is_private(inetaddr) 	&&
      !gnet_inetaddr_is_reserved(inetaddr) 	&&
      !gnet_inetaddr_is_loopback(inetaddr) 	&&
      !gnet_inetaddr_is_multicast(inetaddr) 	&&
      !gnet_inetaddr_is_broadcast(inetaddr))
    {
      return TRUE;
    }

  return FALSE;
}



/**
 *  gnet_inetaddr_is_private
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is an address reserved for private
 *  networks.  For IPv4, this includes:
 *
 *   10.0.0.0        -   10.255.255.255  (10/8 prefix)
 *   172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
 *   192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
 *
 *  (from RFC 1918.  See also draft-manning-dsua-02.txt)
 *
 *  For IPv6, this includes link local addresses (fe80::/64) and site
 *  local addresses (fec0::/64).
 * 
 *  Returns: TRUE if @inetaddr is private; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_private (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      guint32 addr;

      addr = GNET_INETADDR_SA4(inetaddr).sin_addr.s_addr;
      addr = g_ntohl(addr);

      if ((addr & 0xFF000000) == (10 << 24))
	return TRUE;

      if ((addr & 0xFFF00000) == 0xAC100000)
	return TRUE;

      if ((addr & 0xFFFF0000) == 0xC0A80000)
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      if (IN6_IS_ADDR_LINKLOCAL(&GNET_INETADDR_SA6(inetaddr).sin6_addr) ||
	  IN6_IS_ADDR_SITELOCAL(&GNET_INETADDR_SA6(inetaddr).sin6_addr))
	return TRUE;
    }
#endif

  return FALSE;
}


/**
 *  gnet_inetaddr_is_reserved:
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is reserved for some purpose.  This
 *  excludes addresses reserved for private networks.
 *
 *  For IPv4, we check for:
 *    0.0.0.0/16  (top 16 bits are 0's)
 *    Class E (top 5 bits are 11110)
 *
 *  For IPv6, we check for the 00000000 prefix.
 *    
 *  Returns: TRUE if @inetaddr is reserved; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_reserved (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      guint32 addr;

      addr = GNET_INETADDR_SA4(inetaddr).sin_addr.s_addr;
      addr = g_ntohl(addr);

      if ((addr & 0xFFFF0000) == 0)
	return TRUE;

      if ((addr & 0xF8000000) == 0xF0000000)
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      guint32 high_addr;

      high_addr = GNET_INETADDR_ADDR32(inetaddr, 0);
      high_addr = g_ntohl(high_addr);

      if ((high_addr & 0xFFFF0000) == 0)	/* 0000 0000 prefix */
	return TRUE;
    }
#endif

  return FALSE;
}



/**
 *  gnet_inetaddr_is_loopback:
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is a loopback address.  The IPv4 loopback
 *  addresses have the prefix 127.0.0.1/24.  The IPv6 loopback address
 *  is ::1.
 * 
 *  Returns: TRUE if @inetaddr is a loopback address; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_loopback (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      guint32 addr;

      addr = GNET_INETADDR_SA4(inetaddr).sin_addr.s_addr;
      addr = g_ntohl(addr);

      if ((addr & 0xFF000000) == (IN_LOOPBACKNET << 24))
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      if (IN6_IS_ADDR_LOOPBACK(&GNET_INETADDR_SA6(inetaddr).sin6_addr))
	return TRUE;
    }
#endif

  return FALSE;
}


/**
 *  gnet_inetaddr_is_multicast
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is a multicast address.  IPv4 multicast
 *  addresses are in the range 224.0.0.0 to 239.255.255.255 (ie, the
 *  top four bits are 1110).  IPv6 multicast addresses have the prefix
 *  FF.
 * 
 *  Returns: TRUE if @inetaddr is a multicast address; FALSE
 *  otherwise.
 *
 **/
gboolean 
gnet_inetaddr_is_multicast (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      if (IN_MULTICAST(g_htonl(GNET_INETADDR_SA4(inetaddr).sin_addr.s_addr)))
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      if (IN6_IS_ADDR_MULTICAST(&GNET_INETADDR_SA6(inetaddr).sin6_addr))
	return TRUE;
    }
#endif

  return FALSE;
}


/**
 *  gnet_inetaddr_is_broadcast
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is a broadcast address.  The broadcast
 *  address is 255.255.255.255.  (Network broadcast addresses are
 *  network dependent.)
 * 
 *  Returns: TRUE if @inetaddr is a broadcast address; FALSE
 *  otherwise.
 *
 **/
gboolean 
gnet_inetaddr_is_broadcast (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      if (g_htonl(GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr) == INADDR_BROADCAST)
	return TRUE;
    }
  /* There is no IPv6 broadcast address */

  return FALSE;
}


/**
 *  gnet_inetaddr_is_ipv4:
 *  @inetaddr: a #GInetAddr
 *
 *  Checks if a #GInetAddr is an IPv4 address.
 * 
 *  Returns: TRUE if @inetaddr is an IPv4 address; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_ipv4 (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  return GNET_INETADDR_FAMILY(inetaddr) == AF_INET;
}


/**
 *  gnet_inetaddr_is_ipv6:
 *  @inetaddr: a #GInetAddr
 *
 *  Check if a #GInetAddr is an IPv6 address.
 * 
 *  Returns: TRUE if @inetaddr is an IPv6 address; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_ipv6 (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

#ifdef HAVE_IPV6
  return GNET_INETADDR_FAMILY(inetaddr) == AF_INET6;
#else
  return FALSE;
#endif
}



/* **************************************** */



/**
 *  gnet_inetaddr_hash:
 *  @p: Pointer to an #GInetAddr.
 *
 *  Creates a hash code for a #GInetAddr for use with GHashTable.
 *
 *  Returns: the hash code for @p.
 *
 **/
guint 
gnet_inetaddr_hash (gconstpointer p)
{
  const GInetAddr* ia;
  guint32 port;
  guint32 addr = 0;

  g_assert(p != NULL);

  ia = (const GInetAddr*) p;

  port = (guint32) g_ntohs(GNET_INETADDR_PORT(ia));

  /* We do pay attention to network byte order just in case the hash
     result is saved or sent to a different host.  */

  if (GNET_INETADDR_FAMILY(ia) == AF_INET)
    {
      struct sockaddr_in* sa_in = (struct sockaddr_in*) &ia->sa;

      addr = g_ntohl(sa_in->sin_addr.s_addr);
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(ia) == AF_INET6)
    {
      addr = g_ntohl(GNET_INETADDR_ADDR32(ia, 0)) ^
	g_ntohl(GNET_INETADDR_ADDR32(ia, 1)) ^
	g_ntohl(GNET_INETADDR_ADDR32(ia, 2)) ^
	g_ntohl(GNET_INETADDR_ADDR32(ia, 3));
    }
#endif
  else
    g_assert_not_reached();

  return (port ^ addr);
}



/**
 *  gnet_inetaddr_equal:
 *  @p1: a #GInetAddr.
 *  @p2: another #GInetAddr.
 *
 *  Compares two #GInetAddr's for equality.  IPv4 and IPv6 addresses
 *  are always unequal.
 *
 *  Returns: TRUE if they are equal; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_equal (gconstpointer p1, gconstpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  g_return_val_if_fail (p1, FALSE);
  g_return_val_if_fail (p2, FALSE);

  /* Note network byte order doesn't matter */

  if (GNET_INETADDR_FAMILY(ia1) != GNET_INETADDR_FAMILY(ia2))
    return FALSE;

  if (GNET_INETADDR_FAMILY(ia1) == AF_INET)
    {
      struct sockaddr_in* sa_in1 = (struct sockaddr_in*) &ia1->sa;
      struct sockaddr_in* sa_in2 = (struct sockaddr_in*) &ia2->sa;

      if ((sa_in1->sin_addr.s_addr == sa_in2->sin_addr.s_addr) &&
	  (sa_in1->sin_port == sa_in2->sin_port))
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(ia1) == AF_INET6)
    {
      struct sockaddr_in6* sa_in1 = (struct sockaddr_in6*) &ia1->sa;
      struct sockaddr_in6* sa_in2 = (struct sockaddr_in6*) &ia2->sa;

      if (IN6_ARE_ADDR_EQUAL(&sa_in1->sin6_addr, &sa_in2->sin6_addr) &&
	  (sa_in1->sin6_port == sa_in2->sin6_port))
	return TRUE;
    }
#endif
  else
    g_assert_not_reached();

  return FALSE;
}


/**
 *  gnet_inetaddr_noport_equal
 *  @p1: a #GInetAddr
 *  @p2: another #GInetAddr
 *
 *  Compares two #GInetAddr's for equality, but does not compare the
 *  port numbers.
 *
 *  Returns: TRUE if they are equal; FALSE otherwise.
 *
 **/
gboolean 
gnet_inetaddr_noport_equal (gconstpointer p1, gconstpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  /* Note network byte order doesn't matter */

  if (GNET_INETADDR_FAMILY(ia1) != GNET_INETADDR_FAMILY(ia2))
    return FALSE;

  if (GNET_INETADDR_FAMILY(ia1) == AF_INET)
    {
      struct sockaddr_in* sa_in1 = (struct sockaddr_in*) &ia1->sa;
      struct sockaddr_in* sa_in2 = (struct sockaddr_in*) &ia2->sa;

      if (sa_in1->sin_addr.s_addr == sa_in2->sin_addr.s_addr)
	return TRUE;
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(ia1) == AF_INET6)
    {
      if ((GNET_INETADDR_ADDR32(ia1, 0) ==
	   GNET_INETADDR_ADDR32(ia2, 0)) &&
	  (GNET_INETADDR_ADDR32(ia1, 1) ==
	   GNET_INETADDR_ADDR32(ia2, 1)) &&
	  (GNET_INETADDR_ADDR32(ia1, 2) ==
	   GNET_INETADDR_ADDR32(ia2, 2)) &&
	  (GNET_INETADDR_ADDR32(ia1, 3) ==
	   GNET_INETADDR_ADDR32(ia2, 3)))
	return TRUE;
    }
#endif
  else
    g_assert_not_reached();

  return FALSE;
}



/* **************************************** */

#ifndef GNET_WIN32  /*********** Unix code ***********/

/**
 *  gnet_inetaddr_get_host_name:
 * 
 *  Gets the host's name.
 *
 *  Returns: the name of the host; NULL on error.
 *
 **/
gchar*
gnet_inetaddr_get_host_name (void)
{
  gchar* name = NULL;
  struct utsname myname;
  GInetAddr* addr;

  /* Get system name */
  if (uname(&myname) < 0)
    return NULL;

  /* Do forward lookup */
  addr = gnet_inetaddr_new (myname.nodename, 0);
  if (!addr)
    return NULL;
  
  /* Do backwards lookup */
  name = gnet_inetaddr_get_name (addr);
  if (!name)
    name = g_strdup (myname.nodename);

  gnet_inetaddr_delete (addr);

  return name;
}


#else	/*********** Windows code ***********/
/* (Windows doesn't have uname */

gchar*
gnet_inetaddr_get_host_name (void)
{
  gchar* name = NULL;
  int error = 0;

  name = g_new0(char, 256);
  error = gethostname(name, 256);
  if (error)
    {
      g_free(name);
      return NULL;
    }

  return name;
}

#endif		/*********** End Windows code ***********/


/**
 *  gnet_inetaddr_get_host_addr:
 * 
 *  Get the host's address.
 *
 *  Returns: the address of the host; NULL on error.
 *
 **/
GInetAddr* 
gnet_inetaddr_get_host_addr (void)
{
  gchar* name;
  GInetAddr* ia = NULL;

  name = gnet_inetaddr_get_host_name();
  if (name != NULL)
    {  
      ia = gnet_inetaddr_new(name, 0);
      g_free(name);
    }

  return ia;
}


/* **************************************** */

static GInetAddr* autodetect_internet_interface_ipv4 (void);
static GInetAddr* autodetect_internet_interface_ipv6 (void);


/**
 *  gnet_inetaddr_autodetect_internet_interface
 *
 *  Finds an interface likely to be connected to the internet.  This
 *  function can be used to automatically configure peer-to-peer
 *  applications.  The function relies on heuristics and does not
 *  always work correctly, especially if the host is behind a NAT.
 *
 *  Returns: an address of an internet interface; NULL if it couldn't
 *  find one or on error.
 *
 **/
GInetAddr* 
gnet_inetaddr_autodetect_internet_interface (void)
{
  GInetAddr* iface = NULL;
  GIPv6Policy policy;
  policy = gnet_ipv6_get_policy();
  switch (policy)
    {
    case GIPV6_POLICY_IPV4_THEN_IPV6:
      {
	iface = autodetect_internet_interface_ipv4();
	if (iface) break;
	iface = autodetect_internet_interface_ipv6();
	break;
      }
    case GIPV6_POLICY_IPV6_THEN_IPV4:
      {
	iface = autodetect_internet_interface_ipv6();
	if (iface) break;
	iface = autodetect_internet_interface_ipv4();
	break;
      }
    case GIPV6_POLICY_IPV4_ONLY:
      {
	iface = autodetect_internet_interface_ipv4();
	break;
      }
    case GIPV6_POLICY_IPV6_ONLY:
      {
	iface = autodetect_internet_interface_ipv6();
	break;
      }
    }

  /* If that didn't work, return the first Internet interface. */
  if (!iface)
    iface = gnet_inetaddr_get_internet_interface ();

  return iface;
}


static GInetAddr* 
autodetect_internet_interface_ipv4 (void)
{
  GInetAddr* ia;
  GInetAddr* iface;

  /* First try to get the interface with a route to
     junglemonkey.net (141.213.11.223).  This uses the connected UDP
     socket method described by Stevens.  It does not work on all
     systems.  (see Stevens UNPv1 pp 231-3) */
  ia = gnet_inetaddr_new_nonblock ("141.213.11.223", 0);
  g_assert (ia);

  iface = gnet_inetaddr_get_interface_to (ia);
  gnet_inetaddr_delete (ia);

  /* We want an internet interface */
  if (iface && gnet_inetaddr_is_internet(iface))
    return iface;
  gnet_inetaddr_delete (iface);

  return NULL;
}


static GInetAddr* 
autodetect_internet_interface_ipv6 (void)
{
  GInetAddr* ia;
  GInetAddr* iface;

  /* Try IPv6 using kame.net's address */
  ia = gnet_inetaddr_new_nonblock("3FFE:501:4819:2000:210:F3FF:FE03:4D0", 0);
  g_assert (ia);

  iface = gnet_inetaddr_get_interface_to (ia);
  gnet_inetaddr_delete (ia);

  /* We want an internet interface */
  if (iface && gnet_inetaddr_is_internet(iface))
    return iface;
  gnet_inetaddr_delete (iface);

  return NULL;
}



/**
 *  gnet_inetaddr_get_interface_to:
 *  @inetaddr: a #GInetAddr
 *
 *  Figures out which local interface would be used to send a packet
 *  to @inetaddr.  This works on some systems, but not others.  We
 *  recommend using gnet_inetaddr_autodetect_internet_interface() to
 *  find an Internet interface since it's more likely to work.
 *
 *  Returns: the address of an interface used to route packets to
 *  @inetaddr; NULL if there is no such interface or the system does
 *  not support this check.
 *
 **/
GInetAddr* 
gnet_inetaddr_get_interface_to (const GInetAddr* inetaddr)
{
  int sockfd;
  struct sockaddr_storage myaddr;
  socklen_t len;
  GInetAddr* iface;

  g_return_val_if_fail (inetaddr, NULL);

  sockfd = socket (GNET_INETADDR_FAMILY(inetaddr), SOCK_DGRAM, 0);
  if (sockfd == -1)
    return NULL;

  if (connect (sockfd, &GNET_INETADDR_SA(inetaddr), GNET_INETADDR_LEN(inetaddr)) == -1)
    {
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  len = sizeof (myaddr);
  if (getsockname (sockfd, (struct sockaddr*) &myaddr, &len) != 0)
    {
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  iface = g_new0 (GInetAddr, 1);
  iface->ref_count = 1;
  iface->sa = myaddr;

  return iface;
}



/**
 *  gnet_inetaddr_get_internet_interface
 *
 *  Finds an internet interface.  This function finds the first
 *  interface that is an internet address.  IPv6 policy is followed.
 *
 *  This function does not work on some systems.  We recommend using
 *  gnet_inetaddr_autodetect_internet_interface(), which performs
 *  additional checks.
 *
 *  Returns: the address of an internet interface; NULL if no internet
 *  interfaces or on error.
 *
 **/
GInetAddr* 
gnet_inetaddr_get_internet_interface (void)
{
  GList* interfaces;
  GList* i;
  GInetAddr* ipv4_addr = NULL;
  GInetAddr* ipv6_addr = NULL;
  GInetAddr* iface = NULL;
  GIPv6Policy policy;

  /* Get a list of interfaces */
  interfaces = gnet_inetaddr_list_interfaces ();
  if (interfaces == NULL)
    return NULL;

  /* Find the first IPv4 and IPv6 interfaces */
  for (i = interfaces; i != NULL; i = i->next)
    {
      GInetAddr* ia;

      ia = (GInetAddr*) i->data;

      if (gnet_inetaddr_is_internet (ia))
	{
	  if (ipv4_addr == NULL && gnet_inetaddr_is_ipv4 (ia))
	    ipv4_addr = ia;
	  else if (ipv6_addr == NULL && gnet_inetaddr_is_ipv6 (ia))
	    ipv6_addr = ia;
	}
    }

  /* Choose one based on policy */
  policy = gnet_ipv6_get_policy();
  switch (policy)
    {
    case GIPV6_POLICY_IPV4_THEN_IPV6:
      {
	if (ipv4_addr)
	  iface = ipv4_addr;
	else
	  iface = ipv6_addr;
	break;
      }
    case GIPV6_POLICY_IPV6_THEN_IPV4:
      {
	if (ipv6_addr)
	  iface = ipv6_addr;
	else
	  iface = ipv4_addr;
	break;
      }
    case GIPV6_POLICY_IPV4_ONLY:
      {
	iface = ipv4_addr;
	break;
      }
    case GIPV6_POLICY_IPV6_ONLY:
      {
	iface = ipv6_addr;
	break;
      }
    }

  /* Copy the interface */
  if (iface != NULL)
    iface = gnet_inetaddr_clone (iface);

  /* Delete the interface list */
  for (i = interfaces; i != NULL; i = i->next)
    gnet_inetaddr_delete ((GInetAddr*) i->data);
  g_list_free (interfaces);

  return iface;
}


/**
 *  gnet_inetaddr_is_internet_domainname:
 *  @name: name
 *
 *  Checks if a name is a sensible internet domain name.  This
 *  function uses heuristics. It does not use DNS and will not block.
 *  For example, "localhost" and "10.10.23.42" are not sensible
 *  internet domain names.  (10.10.23.42 is a network address, but not
 *  accessible to the internet at large.)
 *
 *  Returns: TRUE if @name is a sensible Internet domain name; FALSE
 *  otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_internet_domainname (const gchar* name)
{
  GInetAddr* addr;

  g_return_val_if_fail (name, FALSE);

  /* The name can't be localhost or localhost.localdomain */
  if (!strcmp(name, "localhost") || 
      !strcmp(name, "localhost.localdomain"))
    {
      return FALSE;
    }

  /* The name must have a dot in it */
  if (!strchr(name, '.'))
    {
      return FALSE;
    }
	
  /* If it's dotted decimal, make sure it's valid */
  addr = gnet_inetaddr_new_nonblock (name, 0);
  if (addr)
    {
      gboolean rv;

      rv = gnet_inetaddr_is_internet (addr);
      gnet_inetaddr_delete (addr);

      if (!rv)
	return FALSE;
    }

  return TRUE;
}


/* **************************************** */




#ifndef GNET_WIN32		/* Unix specific version */

/**
 *  gnet_inetaddr_list_interfaces
 *
 *  Gets a list of #GInetAddr interfaces's on this host.  This list
 *  includes all "up" Internet interfaces and the loopback interface,
 *  if it exists.
 *
 *  Note that the Windows version supports a maximum of 10 interfaces.
 *  In Windows NT, Service Pack 4 (or higher) is required.
 *
 *  This function may not work on some systems.
 *
 *  Returns: A list of #GInetAddr's representing available interfaces.
 *  The caller should delete the list and the addresses.
 *
 **/
GList* 
gnet_inetaddr_list_interfaces (void)
{
  GList* list = NULL;

  /* Use USAGI project's getifaddr() if available. */ 
#ifdef HAVE_LINUX_NETLINK_H
#  define HAVE_GETIFADDRS 1
#  define getifaddrs usagi_getifaddrs
#  define freeifaddrs usagi_freeifaddrs
#endif

#ifdef HAVE_GETIFADDRS

  int rv;
  struct ifaddrs* ifs;
  struct ifaddrs* i;
  
  rv = getifaddrs (&ifs);
  if (rv != 0)
    return NULL;

  for (i = ifs; i != NULL; i = i->ifa_next)
    {
      struct sockaddr* sa;
      void*  src;
      size_t len;
      GInetAddr* ia;

      /* Skip if not up or is loopback */
      if (!(i->ifa_flags & IFF_UP) ||
	  (i->ifa_flags & IFF_LOOPBACK))
	continue;

      /* Skip if no address */
      if (!i->ifa_addr)
	continue;
      
      /* Get address, or skip if not IPv4 or IPv6 */
      sa = (struct sockaddr*) i->ifa_addr;
      if (sa->sa_family == AF_INET)
	{
	  src = (void*) &((struct sockaddr_in*) sa)->sin_addr;
	  len = sizeof(struct in_addr);
	}
#ifdef HAVE_IPV6
      else if (sa->sa_family == AF_INET6)
	{
	  src = (char*) &((struct sockaddr_in6*) sa)->sin6_addr;
	  len = sizeof(struct in6_addr);
	}
#endif
      else
	continue;

      ia = g_new0 (GInetAddr, 1);
      ia->ref_count = 1;
      GNET_INETADDR_FAMILY(ia) = sa->sa_family;
      GNET_INETADDR_SET_SS_LEN(ia);
      memcpy(GNET_INETADDR_ADDRP(ia), src, len);
      list = g_list_prepend (list, ia);
    }

  freeifaddrs (ifs);

#else	/* Old-fashioned IPv4-only method */

  gint len, lastlen;
  gchar* buf;
  gchar* ptr;
  gint sockfd;
  struct ifconf ifc;
  struct ifreq* ifr;


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
  for (ptr = buf; ptr < (buf + ifc.ifc_len); 
#ifdef HAVE_SOCKADDR_SA_LEN      
      ptr += sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len
#else
      ptr += sizeof(struct ifreq)
#endif
)
    {
      struct sockaddr_storage addr;
      int rv;
      GInetAddr* ia;

      ifr = (struct ifreq*) ptr;

      /* Ignore non-AF_INET */
      if (ifr->ifr_addr.sa_family != AF_INET)
	{
#ifdef HAVE_IPV6
	  if (ifr->ifr_addr.sa_family != AF_INET6)
#endif
	    continue;
	}

      /* Save the address - the next call will clobber it */
      memcpy(&addr, &ifr->ifr_addr, sizeof(addr));
      
      /* Get the flags */
      rv = ioctl(sockfd, SIOCGIFFLAGS, ifr);
      if (rv == -1)
	continue;

      /* Ignore entries that aren't up or loopback.  Someday we'll
	 write an interface structure and include this stuff. */
      if (!(ifr->ifr_flags & IFF_UP) ||
	  (ifr->ifr_flags & IFF_LOOPBACK))
	continue;

      /* Create an InetAddr for this one and add it to our list */
      ia = g_new0 (GInetAddr, 1);
      ia->ref_count = 1;
      ia->sa = addr;
      list = g_list_prepend (list, ia);
    }

  g_free (buf);
#endif

  list = g_list_reverse (list);

  return list;
}


#else /* GNET_WIN32 Windows specific version */

GList* 
gnet_inetaddr_list_interfaces (void)
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
  struct sockaddr_storage addr;
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
      if ((SetFlags & IFF_UP) || (SetFlags & IFF_LOOPBACK))
	{
	  /* Create an InetAddr for this one and add it to our list */
	  ia = g_new0(GInetAddr, 1);
	  memcpy(&ia->sa, &addr, sizeof(addr));
	  ia->ref_count = 1;
	  list = g_list_prepend(list, ia);
	}
    }
  return list;
}

#endif /* end Windows specific gnet_inetaddr_list_interfaces */

