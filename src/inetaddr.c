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


/* **************************************** */

static gboolean gnet_gethostbyname(const char* hostname, struct sockaddr_storage* sa, gchar** nicename);
static gchar* gnet_gethostbyaddr(const char* addr, size_t length, int type);

/* Testing stuff */
/*  #undef   HAVE_GETHOSTBYNAME_R_GLIBC */
/*  #define  HAVE_GETHOSTBYNAME_R_GLIB_MUTEX */

/* TODO: Move this to an init function */
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
#  ifndef G_THREADS_ENABLED
#    error Using GLib Mutex but thread are not enabled.
#  endif
G_LOCK_DEFINE (gethostbyname);
#endif

/* Thread safe gethostbyname.  The only valid fields are sin_len,
   sin_family, and sin_addr.  Nice name */

gboolean
gnet_gethostbyname(const char* hostname, struct sockaddr_storage* sa, gchar** nicename)
{
  gboolean rv = FALSE;

#ifndef GNET_WIN32
  
  struct in_addr inaddr;
  struct in6_addr in6addr;

  /* Attempt non-blocking lookup */

  if (inet_pton(AF_INET, hostname, &inaddr) != 0)
    {
      struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

      sa_inp->sin_family = AF_INET;
      memcpy(&sa_inp->sin_addr, (char*) &inaddr, sizeof(inaddr));
      if (nicename)
	*nicename = g_strdup (hostname);
      return TRUE;
    }
  else if (inet_pton(AF_INET6, hostname, &in6addr) != 0)
    {
      struct sockaddr_in6* sa_inp = (struct sockaddr_in6*) sa;

      sa_inp->sin6_family = AF_INET6;
      memcpy(&sa_inp->sin6_addr, (char*) &in6addr, sizeof(in6addr));
      if (nicename)
	*nicename = g_strdup (hostname);
      return TRUE;
    }

#endif


  /* HAVE_GETADDRINFO */
 {
   struct addrinfo hints;
   struct addrinfo* res;
   int rv;

   memset (&hints, 0, sizeof(hints));
   hints.ai_socktype = SOCK_STREAM;
   /*    hints.ai_family = AF_INET; */	/* FIX */

   rv = getaddrinfo(hostname, NULL, &hints, &res);
   if (rv != 0)
     {
      fprintf (stderr, "getaddrinfo error: %s\n", gai_strerror(rv));


     return FALSE;
     }
     
   memcpy (sa, res->ai_addr, res->ai_addrlen);
   return TRUE;
 }


#ifdef HAVE_GETHOSTBYNAME_THREADSAFE
  {
    struct hostent* he;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    he = gethostbyname(hostname);
    if (he != NULL && he->h_addr_list[0] != NULL)
      {
	if (sa_inp)
	  {
	    sa_inp->sin_family = he->h_addrtype;
	    memcpy(&sa_inp->sin_addr, he->h_addr_list[0], he->h_length);
	  }

	if (nicename && he->h_name)
	  *nicename = g_strdup(he->h_name);

	rv = TRUE;
      }
  }
#else
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
  {
    struct hostent result_buf, *result;
    size_t len;
    char* buf;
    int herr;
    int res;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyname_r (hostname, &result_buf, buf, len, &result, &herr)) 
	   == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (res || result == NULL || result->h_addr_list[0] == NULL)
      goto done;

    if (sa_inp)
      {
	sa_inp->sin_family = result->h_addrtype;
	memcpy(&sa_inp->sin_addr, result->h_addr_list[0], result->h_length);
      }

    if (nicename && result->h_name)
      *nicename = g_strdup(result->h_name);

    rv = TRUE;

  done:
    g_free(buf);
  }

#else
#ifdef HAVE_GET_HOSTBYNAME_R_SOLARIS
  {
    struct hostent result;
    size_t len;
    char* buf;
    int herr;
    int res;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyname_r (hostname, &result, buf, len, &herr)) == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (res || hp == NULL || hp->h_addr_list[0] == NULL)
      goto done;

    if (sa_inp)
      {
	sa_inp->sin_family = result->h_addrtype;
	memcpy(&sa_inp->sin_addr, result->h_addr_list[0], result->h_length);
      }

    if (nicename && result->h_name)
      *nicename = g_strdup(result->h_name);

    rv = TRUE;

  done:
    g_free(buf);
  }

#else
#ifdef HAVE_GETHOSTBYNAME_R_HPUX
  {
    struct hostent result;
    struct hostent_data buf;
    int res;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    res = gethostbyname_r (hostname, &result, &buf);

    if (res == 0)
      {
	if (sa_inp)
	  {
	    sa_inp->sin_family = result.h_addrtype;
	    memcpy(&sa_inp->sin_addr, result.h_addr_list[0], result.h_length);
	  }
	
	if (nicename && result.h_name)
	  *nicename = g_strdup(result.h_name);

	rv = TRUE;
      }
  }

#else 
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
  {
    struct hostent* he;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    if (!g_threads_got_initialized)
      g_thread_init (NULL);

    G_LOCK (gethostbyname);
    he = gethostbyname(hostname);

    if (he != NULL && he->h_addr_list[0] != NULL)
      {
	if (sa_inp)
	  {
	    sa_inp->sin_family = he->h_addrtype;
	    memcpy(&sa_inp->sin_addr, he->h_addr_list[0], he->h_length);
	  }

	if (nicename && he->h_name)
	  *nicename = g_strdup(he->h_name);

	rv = TRUE;
      }
    G_UNLOCK (gethostbyname);
  }
#else
#ifdef GNET_WIN32
  {
    struct hostent *result;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    WaitForSingleObject(gnet_hostent_Mutex, INFINITE);
    result = gethostbyname(hostname);

    if (result != NULL)
      {
	if (sa_inp)
	  {
	    sa_inp->sin_family = result->h_addrtype;
	    memcpy(&sa_inp->sin_addr, result->h_addr_list[0], result->h_length);
	  }
	
	if (nicename && result->h_name)
	  *nicename = g_strdup(result->h_name);

	ReleaseMutex(gnet_hostent_Mutex);
	rv = TRUE;
   
      }
  }
#else
  {
    struct hostent* he;
    struct sockaddr_in* sa_inp = (struct sockaddr_in*) sa;

    he = gethostbyname(hostname);
    if (he != NULL && he->h_addr_list[0] != NULL)
      {
	if (sa_inp)
	  {
	    sa_inp->sin_family = he->h_addrtype;
	    memcpy(&sa_inp->sin_addr, he->h_addr_list[0], he->h_length);
	  }

	if (nicename && he->h_name)
	  *nicename = g_strdup(he->h_name);

	rv = TRUE;
      }
  }
#endif
#endif
#endif
#endif
#endif
#endif

  return rv;
}

/* 

   Thread safe gethostbyaddr (we assume that gethostbyaddr_r follows
   the same pattern as gethostbyname_r, so we don't have special
   checks for it in configure.in.

   Returns: the hostname, NULL if there was an error.
*/

gchar*
gnet_gethostbyaddr(const char* addr, size_t length, int type)
{
  gchar* rv = NULL;


#ifdef HAVE_GETHOSTBYNAME_THREADSAFE
  {
    struct hostent* he;

    he = gethostbyaddr(addr, length, type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
  }
#else
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
  {
    struct hostent result_buf, *result;
    size_t len;
    char* buf;
    int herr;
    int res;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyaddr_r (addr, length, type, &result_buf, buf, len, &result, &herr)) 
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
    struct hostent result;
    size_t len;
    char* buf;
    int herr;
    int res;

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyaddr_r (addr, lenght, type, &result, buf, len, &herr)) == ERANGE)
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
    struct hostent result;
    struct hostent_data buf;
    int res;

    res = gethostbyaddr_r (addr, length, type, &result, &buf);

    if (res == 0)
      rv = g_strdup (result.h_name);
  }

#else 
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
  {
    struct hostent* he;

    if (!g_threads_got_initialized)
      g_thread_init (NULL);

    G_LOCK (gethostbyname);
    he = gethostbyaddr(addr, length, type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
    G_UNLOCK (gethostbyname);
  }
#else
#ifdef GNET_WIN32
  {
    struct hostent* he;

    WaitForSingleObject(gnet_hostent_Mutex, INFINITE);
    he = gethostbyaddr(addr, length, type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
    ReleaseMutex(gnet_hostent_Mutex);
  }
#else
  {
    struct hostent* he;

    he = gethostbyaddr(addr, length, type);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
  }
#endif
#endif
#endif
#endif
#endif
#endif

  return rv;
}



#ifdef GNET_WIN32

/* TODO: Use Window's inet_aton if they ever implement it. */
static int
inet_aton(const char *cp, struct in_addr *inp)
{
  inp->s_addr = inet_addr(cp);
  if (inp->s_addr == INADDR_NONE && strcmp (cp, "255.255.255.255"))
    return 0;
  return 1;
}

#endif /* GNET_WIN32 */




/* **************************************** */
/* TO IMPLEMENT?:			    */

/***
 *
 * Create an internet address from raw bytes.
 *
 **/
/*  InetAddr* inetaddr_bytes_new(const guint8* addr, const gint length); */



/* **************************************** */




/**
 *  gnet_inetaddr_new:
 *  @name: a nice name (eg, mofo.eecs.umich.edu) or a dotted decimal
 *  name (eg, 141.213.8.59).
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Create an internet address from a name and port.  This function
 *  may block.
 *
 *  Returns: a new #GInetAddr, or NULL if there was a failure.
 *
 **/
GInetAddr* 
gnet_inetaddr_new (const gchar* name, gint port)
{
  struct sockaddr_storage sa;
  GInetAddr* ia = NULL;

  g_return_val_if_fail (name != NULL, NULL);

  /* Try to get the host by name (ie, DNS) */
  if (!gnet_gethostbyname(name, &sa, NULL))
    {
      fprintf (stderr, "gnet_inetaddr_new failure!!!\n");

    return NULL;
    }

  ia = g_new0(GInetAddr, 1);
  ia->name = g_strdup(name);
  ia->ref_count = 1;
	  
  memcpy(&ia->sa, &sa, sizeof(ia->sa));
  GNET_INETADDR_PORT(ia) = g_htons(port);

  return ia;
}


#ifndef GNET_WIN32  /*********** Unix code ***********/

#ifdef HAVE_LIBPTHREAD
static void* inetaddr_new_async_pthread (void* arg);
#endif


/**
 *  gnet_inetaddr_new_async:
 *  @name: a nice name (eg, mofo.eecs.umich.edu) or a dotted decimal name
 *    (eg, 141.213.8.59).  You can delete the after the function is called.
 *  @port: port number (0 if the port doesn't matter)
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 * 
 *  Create a GInetAddr from a name and port asynchronously.  The
 *  callback is called once the structure is created or an error
 *  occurs during lookup.  The callback will not be called during the
 *  call to gnet_inetaddr_new_async().  The GInetAddr passed in the
 *  callback is callee owned.
 *
 *  The Unix version creates a pthread thread which does the lookup.
 *  If pthreads aren't available, it forks and does the lookup.
 *  Forking will be slow or even fail when using operating systems
 *  that copy the entire process when forking.  
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
 *  Returns: ID of the lookup which can be used with
 *  gnet_inetaddr_new_async_cancel() to cancel it; NULL on failure.
 *
 **/
GInetAddrNewAsyncID
gnet_inetaddr_new_async (const gchar* name, gint port, 
			 GInetAddrNewAsyncFunc func, gpointer data)
{
  GInetAddr* ia;
  struct sockaddr_in* sa_in;
  GInetAddrAsyncState* state = NULL;

  g_return_val_if_fail(name != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

# ifdef HAVE_LIBPTHREAD			/* Pthread */
  {
    void** args;
    pthread_attr_t attr;
    pthread_t pthread;
    int rv;

    state = g_new0(GInetAddrAsyncState, 1);

    args = g_new (void*, 2);
    args[0] = (void*) g_strdup(name);
    args[1] = state;

    pthread_mutex_init (&state->mutex, NULL);
    pthread_mutex_lock (&state->mutex);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  create_again:
    rv = pthread_create (&pthread, &attr, inetaddr_new_async_pthread, args);
    if (rv == EAGAIN)
      {
	sleep(0);	/* Yield the processor */
	goto create_again;
      }
    else if (rv)
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
	struct sockaddr_storage sa;

	close (pipes[0]);

	/* Try to get the host by name (ie, DNS) */
	if (gnet_gethostbyname(name, &sa, NULL))
	  {
	    guchar size = GNET_SOCKADDR_LEN(sa);
      
	    if ( (write(outfd, &size, sizeof(guchar)) != sizeof(guchar)) ||
		 (write(outfd, &sa.sin_addr, size) != size) )
	      g_warning ("Error writing to pipe: %s\n", g_strerror(errno));
	  }
	else
	  {
	    /* Write a zero */
	    guchar zero = 0;
	    
	    if (write(outfd, &zero, sizeof(zero)) != sizeof(zero))
	      g_warning ("Error writing to pipe: %s\n", g_strerror(errno));
	  }

	/* Exit (we don't want atexit called, so do _exit instead) */
	_exit(EXIT_SUCCESS);
      }

    /* Parent: Set up state */
    else if (pid > 0)
      {
	close (pipes[1]);

	state = g_new0(GInetAddrAsyncState, 1);
	state->pid = pid;
	state->fd = pipes[0];
	state->iochannel = gnet_private_io_channel_new(pipes[0]);
	state->watch = g_io_add_watch(state->iochannel,
				      (G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL),
				      gnet_inetaddr_new_async_cb, 
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

  /* Create a new InetAddr */
  ia = g_new0(GInetAddr, 1);
  ia->name = g_strdup(name);
  ia->ref_count = 1;

  sa_in = (struct sockaddr_in*) &ia->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_port = g_htons(port);

  /* Finish setting up state for callback */
  g_assert (state);
  state->ia = ia;
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

static gboolean inetaddr_new_async_pthread_dispatch (gpointer data);


/**
 *  gnet_inetaddr_new_async_cancel:
 *  @id: ID of the lookup
 *
 *  Cancel an asynchronous GInetAddr creation that was started with
 *  gnet_inetaddr_new_async().
 * 
 */
void
gnet_inetaddr_new_async_cancel (GInetAddrNewAsyncID id)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) id;

  g_return_if_fail (state);

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

      gnet_inetaddr_delete (state->ia);

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
inetaddr_new_async_pthread_dispatch (gpointer data)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) data;

  pthread_mutex_lock (&state->mutex);

  /* Upcall */
  if (!state->lookup_failed)
    (*state->func)(state->ia, GINETADDR_ASYNC_STATUS_OK, state->data);
  else
    (*state->func)(NULL,      GINETADDR_ASYNC_STATUS_ERROR, state->data);

  /* Delete state */
  g_source_remove (state->source);
  pthread_mutex_unlock (&state->mutex);
  pthread_mutex_destroy (&state->mutex);
  memset (state, 0, sizeof(*state));
  g_free (state);

  return FALSE;
}


static void*
inetaddr_new_async_pthread (void* arg)
{
  void** args = (void**) arg;
  gchar* name = (gchar*) args[0];
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) args[1];
  struct sockaddr_storage sa;
  int rv;

  g_free (args);

  /* Do lookup */
  rv = gnet_gethostbyname (name, &sa, NULL);
  g_free (name);

  /* Lock */
  pthread_mutex_lock (&state->mutex);

  /* If cancelled, destroy state and exit.  The main thread is no
     longer using state. */
  if (state->is_cancelled)
    {
      gnet_inetaddr_delete (state->ia);

      pthread_mutex_unlock (&state->mutex);
      pthread_mutex_destroy (&state->mutex);

      g_free (state);

      return NULL;
    }

  if (rv)
    {
      /* Copy result */
      memcpy(&state->ia->sa, &sa, sizeof(state->ia->sa));
    }
  else
    {
      /* Flag failure */
      state->lookup_failed = TRUE;
    }

  /* Add a source for reply */
  state->source = g_idle_add_full (G_PRIORITY_DEFAULT, 
				   inetaddr_new_async_pthread_dispatch,
				   state, NULL);

  /* Unlock */
  pthread_mutex_unlock (&state->mutex);

  /* Thread exits... */
  return NULL;
}


#else		/* ********** UNIX process ********** */


gboolean 
gnet_inetaddr_new_async_cb (GIOChannel* iochannel, 
			    GIOCondition condition, 
			    gpointer data)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) data;

  g_assert (!state->in_callback);

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

	  /* We're done reading.  Copy into the addr if we were
             successful. */
	  if (state->len > 1)
	    {
	      struct sockaddr_in* sa_in;

	      sa_in = (struct sockaddr_in*) &state->ia->sa;
	      memcpy(&sa_in->sin_addr, &state->buffer[1], (state->len - 1));
	    }

	  /* Otherwise, we got a 0 because there was an error */
	  else
	    goto error;

	  /* Call back */
	  state->in_callback = TRUE;
	  (*state->func)(state->ia, GINETADDR_ASYNC_STATUS_OK, state->data);
	  state->in_callback = FALSE;
	  gnet_inetaddr_new_async_cancel (state);
	  return FALSE;
	}
      /* otherwise, there was an error */
    }
  /* otherwise, there was an error */

 error:
  state->in_callback = TRUE;
  (*state->func)(NULL, GINETADDR_ASYNC_STATUS_ERROR, state->data);
  state->in_callback = FALSE;
  gnet_inetaddr_new_async_cancel(state);
  return FALSE;
}



void
gnet_inetaddr_new_async_cancel (GInetAddrNewAsyncID id)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) id;

  g_return_if_fail (state);

  /* Ignore if in callback */
  if (state->in_callback)
    return;

  gnet_inetaddr_delete (state->ia);
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


GInetAddrNewAsyncID
gnet_inetaddr_new_async (const gchar* name, gint port,
			 GInetAddrNewAsyncFunc func, gpointer data)
{

  GInetAddr* ia;
  struct sockaddr_in* sa_in;
  GInetAddrAsyncState* state;

  g_return_val_if_fail(name != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create a new InetAddr */
  ia = g_new0(GInetAddr, 1);
  ia->name = g_strdup(name);
  ia->ref_count = 1;

  sa_in = (struct sockaddr_in*) &ia->sa;
  sa_in->sin_family = AF_INET;
  sa_in->sin_port = g_htons(port);

  /* Create a structure for the call back */
  state = g_new0(GInetAddrAsyncState, 1);
  state->ia = ia;
  state->func = func;
  state->data = data;

  state->WSAhandle = (int) WSAAsyncGetHostByName(gnet_hWnd, IA_NEW_MSG, name, state->hostentBuffer, sizeof(state->hostentBuffer));

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
gnet_inetaddr_new_async_cb (GIOChannel* iochannel,
			    GIOCondition condition,
			    gpointer data)
{

  GInetAddrAsyncState* state = (GInetAddrAsyncState*) data;
  struct hostent *result;
  struct sockaddr_in *sa_in;

  if (state->errorcode)
    {
      state->in_callback = TRUE;
      (*state->func)(state->ia, GINETADDR_ASYNC_STATUS_ERROR, state->data);
      state->in_callback = FALSE;
      g_free(state);
      return FALSE;
    }

  result = (struct hostent*)state->hostentBuffer;

  sa_in = (struct sockaddr_in*) &state->ia->sa;
  memcpy(&sa_in->sin_addr, result->h_addr_list[0], result->h_length);

  state->ia->name = g_strdup(result->h_name);

  state->in_callback = TRUE;
  (*state->func)(state->ia, GINETADDR_ASYNC_STATUS_OK, state->data);
  state->in_callback = FALSE;
  g_free(state);

  return FALSE;
}


void
gnet_inetaddr_new_async_cancel(GInetAddrNewAsyncID id)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) id;

  g_return_if_fail(state != NULL);
  if (state->in_callback)
    return;

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
 *  gnet_inetaddr_new_nonblock:
 *  @name: a nice name (eg, mofo.eecs.umich.edu) or a dotted decimal name
 *    (eg, 141.213.8.59).  You can delete the after the function is called.
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Create an internet address from a name and port, but don't block
 *  and fail if it would require blocking.  Otherwise, it returns
 *  NULL.  That is, the call will only succeed if the address is in
 *  network address format (i.e., is not a domain name).
 *
 *  Returns: a new #GInetAddr, or NULL if there was a failure or it
 *  would require blocking.
 *
 **/
GInetAddr* 
gnet_inetaddr_new_nonblock (const gchar* name, gint port)
{
  struct in_addr inaddr;
  struct in6_addr in6addr;
  GInetAddr* ia = NULL;

  g_return_val_if_fail (name, NULL);

  if (inet_pton(AF_INET, name, &inaddr) != 0)
    {
      struct sockaddr_in* sa_inp;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa_inp = (struct sockaddr_in*) &ia->sa;

      memcpy(&sa_inp->sin_addr, (char*) &inaddr, sizeof(inaddr));
      sa_inp->sin_port = g_htons(port);
    }
  else if (inet_pton(AF_INET6, name, &in6addr) != 0)
    {
      struct sockaddr_in6* sa_inp;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;
      sa_inp = (struct sockaddr_in6*) &ia->sa;

      memcpy(&sa_inp->sin6_addr, (char*) &in6addr, sizeof(in6addr));
      sa_inp->sin6_port = g_htons(port);
    }
  else
    return NULL;

  return ia;
}



/**
 *   gnet_inetaddr_clone:
 *   @ia: Address to clone
 *
 *   Create an internet address from another one.  
 *
 *   Returns: a new InetAddr, or NULL if there was a failure.
 *
 **/
GInetAddr* 
gnet_inetaddr_clone(const GInetAddr* ia)
{
  GInetAddr* cia;

  g_return_val_if_fail (ia != NULL, NULL);

  cia = g_new0(GInetAddr, 1);
  cia->ref_count = 1;
  cia->sa = ia->sa;
  if (ia->name != NULL) 
    cia->name = g_strdup(ia->name);

  return cia;
}


/** 
 *  gnet_inetaddr_delete:
 *  @ia: GInetAddr to delete
 *
 *  Delete a GInetAddr.
 *
 **/
void
gnet_inetaddr_delete (GInetAddr* ia)
{
  if (ia != NULL)
    gnet_inetaddr_unref(ia);
}


/**
 *  gnet_inetaddr_ref
 *  @ia: GInetAddr to reference
 *
 *  Increment the reference counter of the GInetAddr.
 *
 **/
void
gnet_inetaddr_ref (GInetAddr* ia)
{
  g_return_if_fail(ia != NULL);

  ++ia->ref_count;
}


/**
 *  gnet_inetaddr_unref
 *  @ia: GInetAddr to unreference
 *
 *  Remove a reference from the GInetAddr.  When reference count
 *  reaches 0, the address is deleted.
 *
 **/
void
gnet_inetaddr_unref (GInetAddr* ia)
{
  g_return_if_fail(ia != NULL);

  --ia->ref_count;

  if (ia->ref_count == 0)
    {
      if (ia->name != NULL) 
	g_free (ia->name);
      g_free (ia);
    }
}



/**
 *  gnet_inetaddr_get_name:
 *  @ia: Address to get the name of.
 *
 *  Get the nice name of the address (eg, "mofo.eecs.umich.edu").  The
 *  "nice name" is the domain name if it has one or the canonical name
 *  if it does not.  Be warned that this call may block since it may
 *  need to do a reverse DNS lookup.
 *
 *  Returns: the nice name of the host, or NULL if there was an error.
 *  The caller is responsible for deleting the returned string.
 *
 **/
gchar* 
gnet_inetaddr_get_name (/* const */ GInetAddr* ia)
{
  g_return_val_if_fail (ia != NULL, NULL);

  if (ia->name == NULL)
    {
      gchar* name;

      if ((name = gnet_gethostbyaddr((char*) &((struct sockaddr_in*)&ia->sa)->sin_addr, 
				    sizeof(struct in_addr), AF_INET)) != NULL)
	ia->name = name;
      else
	ia->name = gnet_inetaddr_get_canonical_name(ia);
    }

  g_assert (ia->name != NULL);
  return g_strdup(ia->name);
}



/**
 *  gnet_inetaddr_get_name_nonblock:
 *  @ia: Address to get the name of.
 *
 *  Get the nice name of the address, but don't block and fail if it
 *  would require blocking.
 *
 *  Returns: the nice name of the host, or NULL if there was an error
 *  or it would require blocking.  The caller is responsible for
 *  deleting the returned string.
 *
 **/
gchar* 
gnet_inetaddr_get_name_nonblock (GInetAddr* ia)
{
  if (ia->name)
    return g_strdup(ia->name);

  return NULL;
}



#ifndef GNET_WIN32  /*********** Unix code ***********/

#ifdef HAVE_LIBPTHREAD
static void* inetaddr_get_name_async_pthread (void* arg);
#endif


/**
 *  gnet_inetaddr_get_name_async:
 *  @ia: Address to get the name of.
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 *
 *  Get the nice name of the address (eg, "mofo.eecs.umich.edu").
 *  This function will use the callback once it knows the nice name.
 *  The callback will not be called during the call to
 *  gnet_inetaddr_new_async().
 *
 *  The Unix uses either pthreads or fork().  See the notes for
 *  gnet_inetaddr_new_async().
 *
 *  Returns: ID of the lookup which can be used with
 *  gnet_inetaddrr_get_name_async_cancel() to cancel it; NULL on
 *  failure.
 *
 **/
GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async (GInetAddr* ia, 
			      GInetAddrGetNameAsyncFunc func,
			      gpointer data)
{
  GInetAddrReverseAsyncState* state = NULL;

  g_return_val_if_fail(ia != NULL, NULL);
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
    if ((pid = fork()) == 0)
      {
	int outfd = pipes[1];
	gchar* name;
	guchar len;
	

	close (pipes[0]);

	if (ia->name)
	  name = g_strdup(ia->name);
	else
	  name = gnet_gethostbyaddr((char*) &((struct sockaddr_in*)&ia->sa)->sin_addr, 
				    sizeof(struct in_addr), AF_INET);

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
	    gchar buffer[INET_ADDRSTRLEN];	/* defined in netinet/in.h */
	    guchar* p = (guchar*) &(GNET_SOCKADDR_IN(ia->sa).sin_addr);

	    g_snprintf(buffer, sizeof(buffer), 
		       "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
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
  state->ia = gnet_inetaddr_clone(ia);
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
 *  Cancel an asynchronous nice name lookup that was started with
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
  (*state->func)(state->name, GINETADDR_ASYNC_STATUS_OK, 
		 state->data);

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
    name = gnet_gethostbyaddr((char*) &((struct sockaddr_in*)&state->ia->sa)->sin_addr, 
			      sizeof(struct in_addr), AF_INET);

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

	  /* Call back */
	  state->in_callback = TRUE;
	  (*state->func)(name, GINETADDR_ASYNC_STATUS_OK, state->data);
	  state->in_callback = FALSE;
	  gnet_inetaddr_get_name_async_cancel(state);
	  return FALSE;
	}
      /* otherwise, there was a read error */
    }
  /* otherwise, there was some error */

  /* Call back */
  state->in_callback = TRUE;
  (*state->func)(NULL, GINETADDR_ASYNC_STATUS_ERROR, state->data);
  state->in_callback = FALSE;
  gnet_inetaddr_get_name_async_cancel(state);
  return FALSE;
}



void
gnet_inetaddr_get_name_async_cancel(GInetAddrGetNameAsyncID id)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) id;

  g_return_if_fail(state != NULL);

  if (state->in_callback)
    return;

  gnet_inetaddr_delete (state->ia);
  g_source_remove (state->watch);
  g_io_channel_unref (state->iochannel);

  close (state->fd);
  kill (state->pid, SIGKILL);
  waitpid (state->pid, NULL, 0);

  g_free(state);
}

#endif


#else	/*********** Windows code ***********/


GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async(GInetAddr* ia,
			     GInetAddrGetNameAsyncFunc func,
			     gpointer data)
{

  GInetAddrReverseAsyncState* state;
  struct sockaddr_in* sa_in;

  g_return_val_if_fail(ia != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Create a structure for the call back */
  state = g_new0(GInetAddrReverseAsyncState, 1);
  state->ia = gnet_inetaddr_clone(ia);
  state->func = func;
  state->data = data;

  sa_in = (struct sockaddr_in*)&ia->sa;
	
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
      (*state->func)(NULL, GINETADDR_ASYNC_STATUS_ERROR, state->data);
      return FALSE;
    }

  state->ia->name = g_strdup(result->h_name);
  name = NULL;
  name = g_strdup(state->ia->name);

  (*state->func)(name, GINETADDR_ASYNC_STATUS_OK, state->data);

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
 *  gnet_inetaddr_get_canonical_name:
 *  @ia: Address to get the canonical name of.
 *
 *  Get the "canonical" name of an address (eg, for IP4 the dotted
 *  decimal name 141.213.8.59).
 *
 *  Returns: NULL if there was an error.  The caller is responsible
 *  for deleting the returned string.
 *
 **/
gchar* 
gnet_inetaddr_get_canonical_name(const GInetAddr* ia)
{
  gchar buffer[INET6_ADDRSTRLEN];	/* defined in netinet/in.h */
  
  g_return_val_if_fail (ia != NULL, NULL);

  fprintf (stderr, "fam = %d\n", GNET_INETADDR_FAMILY(ia));

  if (inet_ntop(GNET_INETADDR_FAMILY(ia), 
		GNET_INETADDR_ADDRP(ia),
		buffer, sizeof(buffer)) == NULL)
    return NULL;

  return g_strdup(buffer);
}


/**
 *  gnet_inetaddr_get_port:
 *  @ia: Address to get the port number of.
 *
 *  Get the port number.
 *  Returns: the port number.
 */
gint
gnet_inetaddr_get_port(const GInetAddr* ia)
{
  g_return_val_if_fail(ia != NULL, -1);

  return (gint) g_ntohs(GNET_INETADDR_PORT(ia));
}


/**
 *  gnet_inetaddr_set_port:
 *  @ia: Address to set the port number of.
 *  @port: New port number
 *
 *  Set the port number.
 *
 **/
void
gnet_inetaddr_set_port(const GInetAddr* ia, guint port)
{
  g_return_if_fail(ia != NULL);

  GNET_INETADDR_PORT(ia) = g_htons(port);
}



/* **************************************** */


/**
 *  gnet_inetaddr_is_canonical:
 *  @name: Name to check
 *
 *  Check if the domain name is canonical.  For IPv4, a canonical name
 *  is a dotted decimal name (eg, 141.213.8.59).
 *
 *  Returns: TRUE if @name is canonical; FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_canonical (const gchar* name)
{
  struct in6_addr inaddr;

  g_return_val_if_fail (name, FALSE);

  return (inet_pton(AF_INET,  name, &inaddr) != 0) ||
         (inet_pton(AF_INET6, name, &inaddr) != 0);
}



/**
 *  gnet_inetaddr_is_internet:
 *  @inetaddr: Address to check
 * 
 *  Check if the address is a sensible internet address.  This mean it
 *  is not private, reserved, loopback, multicast, or broadcast.
 *
 *  Note that private and loopback address are often valid addresses,
 *  so this should only be used to check for general Internet
 *  connectivity.  That is, if the address passes, it is reachable on
 *  the Internet.
 *
 *  Returns: TRUE if the address is an 'Internet' address; FALSE
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
 *  gnet_inetaddr_is_private:
 *  @inetaddr: Address to check
 *
 *  Check if the address is an address reserved for private networks
 *  or something else.  This includes:
 *
 *   10.0.0.0        -   10.255.255.255  (10/8 prefix)
 *   172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
 *   192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
 *
 *  (from RFC 1918.  See also draft-manning-dsua-02.txt)
 * 
 *  Returns: TRUE if the address is reserved for private networks;
 *  FALSE otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_private (const GInetAddr* inetaddr)
{
  guint addr;

  g_return_val_if_fail (inetaddr != NULL, FALSE);

  addr = GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr; /* FIX */
  addr = g_ntohl(addr);

  if ((addr & 0xFF000000) == (10 << 24))
    return TRUE;

  if ((addr & 0xFFF00000) == 0xAC100000)
    return TRUE;

  if ((addr & 0xFFFF0000) == 0xC0A80000)
    return TRUE;

  return FALSE;
}


/**
 *  gnet_inetaddr_is_reserved:
 *  @inetaddr: Address to check
 *
 *  Check if the address is reserved for 'something'.  This excludes
 *  address reserved for private networks.
 *
 *  We check for:
 *    0.0.0.0/16  (top 16 bits are 0's)
 *    Class E (top 5 bits are 11110)
 *
 *  Returns: TRUE if the address is reserved for something; FALSE
 *  otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_reserved (const GInetAddr* inetaddr)
{
  guint addr;

  g_return_val_if_fail (inetaddr != NULL, FALSE);

  addr = GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr; /* FIX */
  addr = g_ntohl(addr);

  if ((addr & 0xFFFF0000) == 0)
    return TRUE;

  if ((addr & 0xF8000000) == 0xF0000000)
    return TRUE;

  return FALSE;
}



/**
 *  gnet_inetaddr_is_loopback:
 *  @inetaddr: Address to check
 *
 *  Check if the address is a loopback address.  Loopback addresses
 *  have the prefix 127.0.0.1/16.
 * 
 *  Returns: TRUE if the address is a loopback address; FALSE
 *  otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_loopback (const GInetAddr* inetaddr)
{
  guint addr;

  g_return_val_if_fail (inetaddr != NULL, FALSE);

  addr = GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr; /* FIX */
  addr = g_ntohl(addr);

  if ((addr & 0xFF000000) == (127 << 24))
    return TRUE;

  return FALSE;
}


/**
 *  gnet_inetaddr_is_multicast:
 *  @inetaddr: Address to check
 *
 *  Check if the address is a multicast address.  Multicast address
 *  are in the range 224.0.0.1 - 239.255.255.255 (ie, the top four
 *  bits are 1110).
 * 
 *  Returns: TRUE if the address is a multicast address; FALSE
 *  otherwise.
 *
 **/
gboolean 
gnet_inetaddr_is_multicast (const GInetAddr* inetaddr)
{
  guint addr;

  g_return_val_if_fail (inetaddr != NULL, FALSE);

  addr = GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr; /* FIX */
  addr = g_htonl(addr);

  if ((addr & 0xF0000000) == 0xE0000000)
    return TRUE;

  return FALSE;
}


/**
 *  gnet_inetaddr_is_broadcast:
 *  @inetaddr: Address to check
 *
 *  Check if the address is a broadcast address.  The broadcast
 *  address is 255.255.255.255.  (Network broadcast address are
 *  network dependent.)
 * 
 *  Returns: TRUE if the address is a broadcast address; FALSE
 *  otherwise.
 *
 **/
gboolean 
gnet_inetaddr_is_broadcast (const GInetAddr* inetaddr)
{
  guint addr;

  g_return_val_if_fail (inetaddr != NULL, FALSE);

  addr = GNET_SOCKADDR_IN(inetaddr->sa).sin_addr.s_addr; /* FIX */

  if (addr == 0xFFFFFFFF)
    return TRUE;

  return FALSE;
}


/**
 *  gnet_inetaddr_is_ipv4:
 *  @inetaddr: Address to check
 *
 *  Check if the address is an IPv4 address
 * 
 *  Returns: TRUE if the address is an IPv4 address; FALSE
 *  otherwise.
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
 *  @inetaddr: Address to check
 *
 *  Check if the address is an IPv6 address
 * 
 *  Returns: TRUE if the address is an IPv6 address; FALSE
 *  otherwise.
 *
 **/
gboolean
gnet_inetaddr_is_ipv6 (const GInetAddr* inetaddr)
{
  g_return_val_if_fail (inetaddr != NULL, FALSE);

  return GNET_INETADDR_FAMILY(inetaddr) == AF_INET6;
}



/* **************************************** */



/**
 *  gnet_inetaddr_hash:
 *  @p: Pointer to an #GInetAddr.
 *
 *  Hash the address.  This is useful for glib containers.
 *
 *  Returns: hash value.
 *
 **/
guint 
gnet_inetaddr_hash (gconstpointer p)
{
  const GInetAddr* ia;
  guint32 port;
  guint32 addr;

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
  else if (GNET_INETADDR_FAMILY(ia) == AF_INET6)
    {
      struct sockaddr_in6* sa_in6 = (struct sockaddr_in6*) &ia->sa;

      /* FIX: Is NBO right here? */
      addr = g_ntohl(sa_in6->sin6_addr.s6_addr32[0]) ^
	g_ntohl(sa_in6->sin6_addr.s6_addr32[1]) ^
	g_ntohl(sa_in6->sin6_addr.s6_addr32[2]) ^
	g_ntohl(sa_in6->sin6_addr.s6_addr32[3]);
    }
  else
    g_assert_not_reached();

  return (port ^ addr);
}



/**
 *  gnet_inetaddr_equal:
 *  @p1: Pointer to first #GInetAddr.
 *  @p2: Pointer to second #GInetAddr.
 *
 *  Compare two #GInetAddr's.  
 *
 *  Returns: 1 if they are the same; 0 otherwise.
 *
 **/
gint 
gnet_inetaddr_equal(gconstpointer p1, gconstpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  g_return_val_if_fail (p1, 0);
  g_return_val_if_fail (p2, 0);

  /* Note network byte order doesn't matter */

  if (GNET_INETADDR_FAMILY(ia1) != GNET_INETADDR_FAMILY(ia2))
    return 0;

  if (GNET_INETADDR_FAMILY(ia1) == AF_INET)
    {
      struct sockaddr_in* sa_in1 = (struct sockaddr_in*) &ia1->sa;
      struct sockaddr_in* sa_in2 = (struct sockaddr_in*) &ia2->sa;

      return ((sa_in1->sin_addr.s_addr == sa_in2->sin_addr.s_addr) &&
	      (sa_in1->sin_port == sa_in2->sin_port));
    }
  else if (GNET_INETADDR_FAMILY(ia1) == AF_INET6)
    {
      struct sockaddr_in6* sa_in1 = (struct sockaddr_in6*) &ia1->sa;
      struct sockaddr_in6* sa_in2 = (struct sockaddr_in6*) &ia2->sa;

      return ((sa_in1->sin6_addr.s6_addr32[0] == 
	       sa_in2->sin6_addr.s6_addr32[0]) &&
	      (sa_in1->sin6_addr.s6_addr32[1] == 
	       sa_in2->sin6_addr.s6_addr32[1]) &&
	      (sa_in1->sin6_addr.s6_addr32[2] == 
	       sa_in2->sin6_addr.s6_addr32[2]) &&
	      (sa_in1->sin6_addr.s6_addr32[3] == 
	       sa_in2->sin6_addr.s6_addr32[3]) &&
	      (sa_in1->sin6_port == sa_in2->sin6_port));
    }
  else
    g_assert_not_reached();

  return 0;
}


/**
 *  gnet_inetaddr_noport_equal:
 *  @p1: Pointer to first GInetAddr.
 *  @p2: Pointer to second GInetAddr.
 *
 *  Compare two #GInetAddr's, but does not compare the port numbers.
 *
 *  Returns: 1 if they are the same; 0 otherwise.
 *
 **/
gint 
gnet_inetaddr_noport_equal(gconstpointer p1, gconstpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  /* Note network byte order doesn't matter */

  if (GNET_INETADDR_FAMILY(ia1) != GNET_INETADDR_FAMILY(ia2))
    return 0;

  if (GNET_INETADDR_FAMILY(ia1) == AF_INET)
    {
      struct sockaddr_in* sa_in1 = (struct sockaddr_in*) &ia1->sa;
      struct sockaddr_in* sa_in2 = (struct sockaddr_in*) &ia2->sa;

      return (sa_in1->sin_addr.s_addr == sa_in2->sin_addr.s_addr);
    }
  else if (GNET_INETADDR_FAMILY(ia1) == AF_INET6)
    {
      struct sockaddr_in6* sa_in1 = (struct sockaddr_in6*) &ia1->sa;
      struct sockaddr_in6* sa_in2 = (struct sockaddr_in6*) &ia2->sa;

      return ((sa_in1->sin6_addr.s6_addr32[0] == 
	       sa_in2->sin6_addr.s6_addr32[0]) &&
	      (sa_in1->sin6_addr.s6_addr32[1] == 
	       sa_in2->sin6_addr.s6_addr32[1]) &&
	      (sa_in1->sin6_addr.s6_addr32[2] == 
	       sa_in2->sin6_addr.s6_addr32[2]) &&
	      (sa_in1->sin6_addr.s6_addr32[3] == 
	       sa_in2->sin6_addr.s6_addr32[3]));
    }
  else
    g_assert_not_reached();

  return 0;
}



/* **************************************** */

#ifndef GNET_WIN32  /*********** Unix code ***********/

/**
 *  gnet_inetaddr_gethostname:
 * 
 *  Get the primary host's name.
 *
 *  Returns: the name of the host; NULL if there was an error.  The
 *  caller is responsible for deleting the returned string.
 *
 **/
gchar*
gnet_inetaddr_gethostname(void)
{
  gchar* name = NULL;
  struct utsname myname;

  if (uname(&myname) < 0)
    return NULL;

  if (!gnet_gethostbyname(myname.nodename, NULL, &name))
    return NULL;

  return name;
}


#else	/*********** Windows code ***********/
/* (Windows doesn't have uname */

gchar*
gnet_inetaddr_gethostname(void)
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
 *  gnet_inetaddr_gethostaddr:
 * 
 *  Get the primary host's #GInetAddr.
 *
 *  Returns: the #GInetAddr of the host; NULL if there was an error.
 *  The caller is responsible for deleting the returned #GInetAddr.
 *
 **/
GInetAddr* 
gnet_inetaddr_gethostaddr(void)
{
  gchar* name;
  GInetAddr* ia = NULL;

  name = gnet_inetaddr_gethostname();
  if (name != NULL)
    {  
      ia = gnet_inetaddr_new(name, 0);
      g_free(name);
    }

  return ia;
}


/* **************************************** */




/**
 *  gnet_inetaddr_new_any:
 *
 *  Create a #GInetAddr with the address INADDR_ANY and port 0.  This
 *  is useful for creating default addresses for binding.  The
 *  address's name will be "<INADDR_ANY>".
 *
 *  Returns: INADDR_ANY #GInetAddr.
 *
 **/
GInetAddr* 
gnet_inetaddr_new_any (void)
{
  GInetAddr* ia;
  struct sockaddr_in* sa_in;

  ia = g_new0 (GInetAddr, 1);
  ia->ref_count = 1;
  sa_in = (struct sockaddr_in*) &ia->sa;  		/* FIX */
  sa_in->sin_addr.s_addr = g_htonl(INADDR_ANY);
  sa_in->sin_port = 0;
  ia->name = g_strdup ("0.0.0.0");

  return ia;
}



/**
 *  gnet_inetaddr_autodetect_internet_interface:
 *
 *  Find an Internet interface.  Usually, this interface routes
 *  packets to and from the Internet.  It can be used to automatically
 *  configure simple servers that must advertise their address.  This
 *  sometimes doesn't work correctly when the user is behind a NAT.
 *
 *  Returns: Address of an Internet interface; NULL if it couldn't
 *  find one or there was an error.
 *
 **/
GInetAddr* 
gnet_inetaddr_autodetect_internet_interface (void)
{
  GInetAddr* jm_addr;
  GInetAddr* iface;

  /* First try to get the interface with a route to
     junglemonkey.net (141.213.11.1).  This uses the connected UDP
     socket method described by Stevens.  It does not work on all
     systems.  (see Stevens UNPv1 pp 231-3) */
  jm_addr = gnet_inetaddr_new_nonblock ("141.213.11.1", 0);
  g_assert (jm_addr);

  iface = gnet_inetaddr_get_interface_to (jm_addr);
  gnet_inetaddr_delete (jm_addr);

  /* We want an internet interface */
  if (iface && gnet_inetaddr_is_internet(iface))
    return iface;
  gnet_inetaddr_delete (iface);

  /* Try getting an internet interface from the list via
     SIOCGIFCONF. (see Stevens UNPv1 pp 428-) */
  iface = gnet_inetaddr_get_internet_interface ();

  return iface;
}



/**
 *  gnet_inetaddr_get_interface_to:
 *  @addr: address
 *
 *  Figure out which local interface would be used to send a packet to
 *  @addr.  This works on some systems, but not others.  We recommend
 *  using gnet_inetaddr_autodetect_internet_interface() to find an
 *  Internet interface since it's more likely to work.
 *
 *  Returns: Address of an interface used to route packets to @addr;
 *  NULL if there is no such interface or the system does not support
 *  this check.
 *
 **/
GInetAddr* 
gnet_inetaddr_get_interface_to (const GInetAddr* addr)
{
  int sockfd;
  struct sockaddr_storage myaddr;
  socklen_t len;
  GInetAddr* iface;

  g_return_val_if_fail (addr, NULL);

  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1)
    return NULL;

  if (connect (sockfd, &GNET_INETADDR_SA(addr), GNET_INETADDR_LEN(addr)) == -1)
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
  memcpy (&iface->sa, (char*) &myaddr, sizeof (iface->sa));

  return iface;
}



/**
 *  gnet_inetaddr_get_internet_interface:
 *
 *  Find an Internet interface.  This just calls
 *  gnet_inetaddr_list_interfaces() and returns the first one that
 *  passes gnet_inetaddr_is_internet().  This works well on some
 *  systems, but not so well on others.  We recommend using
 *  gnet_inetaddr_autodetect_internet_interface() to find an Internet
 *  interface since it's more likely to work.
 *
 *  Returns: Address of an Internet interface; NULL if there is no
 *  such interface or the system does not support this check.
 *
 **/
GInetAddr* 
gnet_inetaddr_get_internet_interface (void)
{
  GInetAddr* iface = NULL;
  GList* interfaces;
  GList* i;

  /* Get a list of interfaces */
  interfaces = gnet_inetaddr_list_interfaces ();
  if (interfaces == NULL)
    return NULL;

  /* Find the first interface that's an internet interface */
  for (i = interfaces; i != NULL; i = i->next)
    {
      GInetAddr* ia;

      ia = (GInetAddr*) i->data;

      if (gnet_inetaddr_is_internet (ia))
	{
	  iface = gnet_inetaddr_clone (ia);
	  break;
	}
    }

  /* If we didn't find one, return the first interface. */
  if (iface == NULL)
    iface = gnet_inetaddr_clone ((GInetAddr*) interfaces->data);

  /* Delete the interface list */
  for (i = interfaces; i != NULL; i = i->next)
    gnet_inetaddr_delete ((GInetAddr*) i->data);
  g_list_free (interfaces);

  return iface;
}





/**
 *  gnet_inetaddr_is_internet_domainname:
 *  @name: Domain name to check
 *
 *  Check if the domain name is a sensible Internet domain name.  This
 *  function uses heuristics and does not use DNS (or even block).
 *  For example, "localhost" and "10.10.23.42" are not sensible
 *  Internet domain names.  (10.10.23.42 is a network address, but not
 *  accessible to the Internet at large.)
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
 *  gnet_inetaddr_list_interfaces:
 *
 *  Get a list of #GInetAddr interfaces's on this host.  This list
 *  includes all "up" Internet interfaces and the loopback interface,
 *  if it exists.
 *
 *  Note that the Windows version supports a maximum of 10 interfaces.
 *  In Windows NT, Service Pack 4 (or higher) is required.
 *
 *  Returns: A list of #GInetAddr's representing available interfaces.
 *  The caller should delete the list and the addresses.
 *
 **/
GList* 
gnet_inetaddr_list_interfaces (void)
{
  GList* list = NULL;
  gint len, lastlen;
  gchar* buf;
  gchar* ptr;
  gint sockfd;
  struct ifconf ifc;
  struct ifreq* ifr;

  /* FIX: Use getifaddrs().  The technique below does not work with
     IPv6 addresses (sockaddr is too small).  Glibc 2.3 will have
     getifaddrs.  *BSD already has it.  We could write (or borrow) our
     own getifaddrs(). */

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
      struct sockaddr addr;
      int rv;
      GInetAddr* ia;

      ifr = (struct ifreq*) ptr;

      /* Ignore non-AF_INET */
      if (ifr->ifr_addr.sa_family != AF_INET &&
	  ifr->ifr_addr.sa_family != AF_INET6 )
	continue;

      /* FIX: Skip colons in name?  Can happen if aliases, maybe. */

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
      memcpy(&ia->sa, &addr, sizeof(addr));
      list = g_list_prepend (list, ia);
    }

  g_free (buf);

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

