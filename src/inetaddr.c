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
#include "inetaddr.h"

#include <config.h>


static gboolean gnet_inetaddr_new_async_cb (GIOChannel* iochannel, 
					    GIOCondition condition, 
					    gpointer data);

typedef struct _GInetAddrAsyncState 
{
  GInetAddr* ia;
  GInetAddrNewAsyncFunc func;
  gpointer data;
  pid_t pid;
  int fd;
  guint watch;
  guchar buffer[16];
  int len;

} GInetAddrAsyncState;



static gboolean gnet_inetaddr_get_name_async_cb (GIOChannel* iochannel, 
						 GIOCondition condition, 
						 gpointer data);

typedef struct _GInetAddrReverseAsyncState 
{
  GInetAddr* ia;
  GInetAddrGetNameAsyncFunc func;
  gpointer data;
  pid_t pid;
  int fd;
  guint watch;
  guchar buffer[256 + 1];	/* I think a domain name can only be 256 characters... */
  int len;

} GInetAddrReverseAsyncState;


/* **************************************** */

static gboolean gnet_gethostbyname(const char* hostname, struct sockaddr_in* sa, gchar** nicename);
static gchar* gnet_gethostbyaddr(const char* addr, size_t length, int type);

/* Testing stuff */
/*  #undef   HAVE_GETHOSTBYNAME_R_GLIBC */
/*  #define  HAVE_GETHOSTBYNAME_R_GLIB_MUTEX */

/* TODO: Move this to an init function */
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
G_LOCK_DEFINE (gethostbyname);
#endif

/* Thread safe gethostbyname.  The only valid fields are sin_len,
   sin_family, and sin_addr.  Nice name */

gboolean
gnet_gethostbyname(const char* hostname, struct sockaddr_in* sa, gchar** nicename)
{
  gboolean rv = FALSE;

#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
  {
    struct hostent result_buf, *result;
    size_t len;
    char* buf;
    int herr;
    int res;

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

    if (sa)
      {
	sa->sin_family = result->h_addrtype;
	memcpy(&sa->sin_addr, result->h_addr_list[0], result->h_length);
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

    len = 1024;
    buf = g_new(gchar, len);

    while ((res = gethostbyname_r (hostname, &result, buf, len, &herr)) == ERANGE)
      {
	len *= 2;
	buf = g_renew (gchar, buf, len);
      }

    if (res || hp == NULL || hp->h_addr_list[0] == NULL)
      goto done;

    if (sa)
      {
	sa->sin_family = result->h_addrtype;
	memcpy(&sa->sin_addr, result->h_addr_list[0], result->h_length);
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

    res = gethostbyname_r (hostname, &result, &buf);

    if (res == 0)
      {
	if (sa)
	  {
	    sa->sin_family = result.h_addrtype;
	    memcpy(&sa->sin_addr, result.h_addr_list[0], result.h_length);
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

    G_LOCK (gethostbyname);
    he = gethostbyname(hostname);
    G_UNLOCK (gethostbyname);

    if (he != NULL && he->h_addr_list[0] != NULL)
      {
	if (sa)
	  {
	    sa->sin_family = he->h_addrtype;
	    memcpy(&sa->sin_addr, he->h_addr_list[0], he->h_length);
	  }

	if (nicename && he->h_name)
	  *nicename = g_strdup(he->h_name);

	rv = TRUE;
      }
  }
#else
  {
    struct hostent* he;

    he = gethostbyname(hostname);
    if (he != NULL && he->h_addr_list[0] != NULL)
      {
	if (sa)
	  {
	    sa->sin_family = he->h_addrtype;
	    memcpy(&sa->sin_addr, he->h_addr_list[0], he->h_length);
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

  return rv;
}

/* 

   Thread safe gethostbyaddr (we assume that gethostbyaddr_r follows
   the same pattern as gethostbyname_r, so we don't have special
   checks for it in configure.in.

   Returns the hostname, NULL if there was an error.
*/

gchar*
gnet_gethostbyaddr(const char* addr, size_t length, int type)
{
  gchar* rv = NULL;

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

    G_LOCK (gethostbyname);
    he = gethostbyaddr(addr, length, type);
    G_UNLOCK (gethostbyname);
    if (he != NULL && he->h_name != NULL)
      rv = g_strdup(he->h_name);
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

  return rv;
}





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
 *  @name: a nice name (eg, mofo.eecs.umich.edu) or a dotted decimal name
 *    (eg, 141.213.8.59).  You can delete the after the function is called.
 *  @port: port number (0 if the port doesn't matter)
 * 
 *  Create an internet address from a name and port.  
 *
 *  Returns: a new InetAddr, or NULL if there was a failure.  
 *
 **/
GInetAddr* 
gnet_inetaddr_new(const gchar* name, const gint port)
{
  struct sockaddr_in* sa_in;
  struct in_addr inaddr;
  GInetAddr* ia = NULL;

  g_return_val_if_fail(name != NULL, NULL);

  /* Try to read the name as if were dotted decimal */
  if (inet_aton(name, &inaddr) != 0)
    {
      ia = g_new0(GInetAddr, 1);

      ia->ref_count = 1;
      sa_in = (struct sockaddr_in*) &ia->sa;
      sa_in->sin_family = AF_INET;
      sa_in->sin_port = g_htons(port);
      memcpy(&sa_in->sin_addr, (char*) &inaddr, sizeof(struct in_addr));
    }

  else
    {
      struct sockaddr_in sa;

      /* Try to get the host by name (ie, DNS) */
      if (gnet_gethostbyname(name, &sa, NULL))
	{
	  ia = g_new0(GInetAddr, 1);
	  ia->name = g_strdup(name);
	  ia->ref_count = 1;
	  
	  sa_in = (struct sockaddr_in*) &ia->sa;
	  sa_in->sin_family = AF_INET;
	  sa_in->sin_port = g_htons(port);
	  memcpy(&sa_in->sin_addr, &sa.sin_addr, 4);
	}
    }

  return ia;
}



/**
 *  gnet_inetaddr_new_async:
 *  @name: a nice name (eg, mofo.eecs.umich.edu) or a dotted decimal name
 *    (eg, 141.213.8.59).  You can delete the after the function is called.
 *  @port: port number (0 if the port doesn't matter)
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 * 
 *  Create a GInetAddr from a name and port asynchronously.  Once the
 *  structure is created, it will call the callback.  It may call the
 *  callback before the function returns.  It will call the callback
 *  if there is a failure.
 *
 *  This is EXPERIMENTAL.  It is based on forking, which can cause
 *  some problems.  In general, this will work ok for most programs
 *  most of the time.  It will be slow or even fail when using
 *  operating systems that copy the entire process when forking.
 *
 *  If you need to lookup a lot of addresses, I recommend calling
 *  g_main_iteration(FALSE) between calls.  This will help prevent an
 *  explosion of processes.
 *
 *  If you need a more robust library, look at <ulink
 *  url="http://www.gnu.org/software/adns/adns.html">GNU ADNS</ulink>.
 *  GNU ADNS is under the GNU GPL.
 *
 *  Returns: ID of the lookup which can be used with
 *  gnet_inetaddr_new_async_cancel() to cancel it; NULL on immediate
 *  success or failure.
 *
 **/
GInetAddrNewAsyncID
gnet_inetaddr_new_async(const gchar* name, const gint port, 
			GInetAddrNewAsyncFunc func, gpointer data)
{
  pid_t pid = -1;
  int pipes[2];
  struct in_addr inaddr;

  g_return_val_if_fail(name != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* Try to read the name as if were dotted decimal */
  if (inet_aton(name, &inaddr) != 0)
    {
      GInetAddr* ia = NULL;
      struct sockaddr_in* sa_in;

      ia = g_new0(GInetAddr, 1);
      ia->ref_count = 1;

      sa_in = (struct sockaddr_in*) &ia->sa;
      sa_in->sin_family = AF_INET;
      sa_in->sin_port = g_htons(port);
      memcpy(&sa_in->sin_addr, (char*) &inaddr, sizeof(struct in_addr));

      (*func)(ia, GINETADDR_ASYNC_STATUS_OK, data);
      return NULL;
    }

  /* That didn't work - we need to fork */

  /* Open a pipe */
  if (pipe(pipes) == -1)
    {
      (*func)(NULL, GINETADDR_ASYNC_STATUS_ERROR, data);
      return NULL;
    }

  /* Fork to do the look up. */
 fork_again:
  errno = 0;
  if ((pid = fork()) == 0)
    {
      struct sockaddr_in sa;

      /* Try to get the host by name (ie, DNS) */
      if (gnet_gethostbyname(name, &sa, NULL))
	{
	  guchar size = 4;	/* FIX for IPv6 */

	  if ( (write(pipes[1], &size, sizeof(guchar)) == -1) ||
	       (write(pipes[1], &sa.sin_addr, size) == -1) )
	    g_warning ("Problem writing to pipe\n");
	}
      else
	{
	  /* Write a zero */
	  guchar zero = 0;

	  if (write(pipes[1], &zero, sizeof(zero)) == -1)
	    g_warning ("Problem writing to pipe\n");
	}

      /* Close the socket */
      close(pipes[1]);

      /* Exit (we don't want atexit called, so do _exit instead) */
      _exit(EXIT_SUCCESS);
    }

  /* Set up an IOChannel to read from the pipe */
  else if (pid > 0)
    {
      GInetAddr* ia;
      struct sockaddr_in* sa_in;
      GInetAddrAsyncState* state;

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
      state->pid = pid;
      state->fd = pipes[0];

      /* Add a watch */
      state->watch = g_io_add_watch(g_io_channel_unix_new(pipes[0]),
				    GNET_ANY_IO_CONDITION,
				    gnet_inetaddr_new_async_cb, 
				    state);

      return state;
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
      g_warning ("Fork error: %s (%d)\n", g_strerror(errno), errno);
      (*func)(NULL, GINETADDR_ASYNC_STATUS_ERROR, data);
    }

  return NULL;
}



static gboolean 
gnet_inetaddr_new_async_cb (GIOChannel* iochannel, 
			    GIOCondition condition, 
			    gpointer data)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) data;
  GInetAddrAsyncStatus status = GINETADDR_ASYNC_STATUS_ERROR;

  /* Read from the pipe */
  if (condition | G_IO_IN)
    {
      int rv;
      char* buf;
      int length;

      buf = &state->buffer[state->len];
      length = sizeof(state->buffer) - state->len;

      if ((rv = read(state->fd, buf, length)) > 0)
	{
	  struct sockaddr_in* sa_in;

	  state->len += rv;

	  /* Return true if there's more to read */
	  if ((state->len - 1) != state->buffer[0])
	    return TRUE;

	  sa_in = (struct sockaddr_in*) &state->ia->sa;

	  /* We are done! */
	  memcpy(&sa_in->sin_addr, &state->buffer[1], state->len);

	  status = GINETADDR_ASYNC_STATUS_OK;

	  close(state->fd);
	  waitpid(state->pid, NULL, 0);
	}
    }


  /* Call back */
  (*state->func)(state->ia, status, state->data);
  g_free(state);

  return FALSE;
}



/**
 *  gnet_inetaddr_new_async_cancel:
 *  @id: ID of the lookup
 *
 *  Cancel an asynchronous GInetAddr creation that was started with
 *  gnet_inetaddr_new_async().
 * 
 */
void
gnet_inetaddr_new_async_cancel(GInetAddrNewAsyncID id)
{
  GInetAddrAsyncState* state = (GInetAddrAsyncState*) id;

  g_return_if_fail(state != NULL);

  gnet_inetaddr_delete (state->ia);
  g_source_remove (state->watch);

  close (state->fd);
  kill (state->pid, SIGKILL);
  waitpid (state->pid, NULL, 0);

  g_free(state);
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
gnet_inetaddr_delete(GInetAddr* ia)
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
gnet_inetaddr_ref(GInetAddr* ia)
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
gnet_inetaddr_unref(GInetAddr* ia)
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
 *  Get the nice name of the address (eg, "mofo.eecs.umich.edu").  Be
 *  warned that this call may block since it may need to do a reverse
 *  DNS lookup.
 *
 *  Returns: NULL if there was an error.  The caller is responsible
 *  for deleting the returned string.
 *
 **/
gchar* 
gnet_inetaddr_get_name(GInetAddr* ia)
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
 *  gnet_inetaddr_get_name_async:
 *  @ia: Address to get the name of.
 *  @func: Callback function.
 *  @data: User data passed when callback function is called.
 *
 *  Get the nice name of the address (eg, "mofo.eecs.umich.edu").
 *  This function will use the callback once it knows the nice name.
 *  It may even call the callback before it returns.  The callback
 *  will be called if there is an error.
 *
 *  This is EXPERIMENTAL.  It uses the same mechanism as
 *  gnet_inetaddr_new_async() - see the notes for that function.
 *
 *  Returns: ID of the lookup which can be used with
 *  gnet_inetaddrr_get_name_async_cancel() to cancel it; NULL on
 *  immediate success or failure.
 *
 **/
GInetAddrGetNameAsyncID
gnet_inetaddr_get_name_async(GInetAddr* ia, 
			     GInetAddrGetNameAsyncFunc func,
			     gpointer data)
{
  g_return_val_if_fail(ia != NULL, NULL);
  g_return_val_if_fail(func != NULL, NULL);

  /* If we already know the name, just copy that */
  if (ia->name != NULL)
    {
      (func)(ia, GINETADDR_ASYNC_STATUS_OK, g_strdup(ia->name), data);
    }
  
  /* Otherwise, fork and look it up */
  else
    {
      pid_t pid = -1;
      int pipes[2];


      /* Open a pipe */
      if (pipe(pipes) == -1)
	{
	  (func)(ia, GINETADDR_ASYNC_STATUS_ERROR, NULL, data);
	  return NULL;
	}


      /* Fork to do the look up. */
    fork_again:
      if ((pid = fork()) == 0)
	{
	  gchar* name;
	  guchar len;

	  /* Write the name to the pipe.  If we didn't get a name, we
             just write the canonical name. */
	  if ((name = gnet_gethostbyaddr((char*) &((struct sockaddr_in*)&ia->sa)->sin_addr, 
					sizeof(struct in_addr), AF_INET)) != NULL)
	    {
	      guint lenint = strlen(name);

	      if (lenint > 255)
		{
		  g_warning ("Truncating domain name: %s\n", name);
		  name[256] = '\0';
		  lenint = 255;
		}

	      len = lenint;

	      if ((write(pipes[1], &len, sizeof(len)) == -1) ||
		  (write(pipes[1], name, len) == -1) )
		g_warning ("Problem writing to pipe\n");

	      g_free(name);
	    }
	  else
	    {
	      gchar buffer[INET_ADDRSTRLEN];	/* defined in netinet/in.h */
	      guchar* p = (guchar*) &(GNET_SOCKADDR_IN(ia->sa).sin_addr);

	      g_snprintf(buffer, sizeof(buffer), 
			 "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	      len = strlen(buffer);

	      if ((write(pipes[1], &len, sizeof(len)) == -1) ||
		  (write(pipes[1], buffer, len) == -1))
		g_warning ("Problem writing to pipe\n");
	    }

	  /* Close the socket */
	  close(pipes[1]);

	  /* Exit (we don't want atexit called, so do _exit instead) */
	  _exit(EXIT_SUCCESS);

	}

      /* Set up an IOChannel to read from the pipe */
      else if (pid > 0)
	{
	  GInetAddrReverseAsyncState* state;

	  /* Create a structure for the call back */
	  state = g_new0(GInetAddrReverseAsyncState, 1);
	  state->ia = ia;
	  state->func = func;
	  state->data = data;
	  state->pid = pid;
	  state->fd = pipes[0];

	  /* Add a watch */
	  state->watch = g_io_add_watch(g_io_channel_unix_new(pipes[0]),
					GNET_ANY_IO_CONDITION,
					gnet_inetaddr_get_name_async_cb, 
					state);
	  return state;
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
	  g_warning ("Fork error: %s (%d)\n", g_strerror(errno), errno);
	  (*func)(ia, GINETADDR_ASYNC_STATUS_ERROR, NULL, data);
	}
    }

  return NULL;
}



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

  g_return_if_fail(state != NULL);

  gnet_inetaddr_delete (state->ia);
  g_source_remove (state->watch);

  close (state->fd);
  kill (state->pid, SIGKILL);
  waitpid (state->pid, NULL, 0);

  g_free(state);
}



static gboolean 
gnet_inetaddr_get_name_async_cb (GIOChannel* iochannel, 
				 GIOCondition condition, 
				 gpointer data)
{
  GInetAddrReverseAsyncState* state = (GInetAddrReverseAsyncState*) data;
  GInetAddrAsyncStatus status = GINETADDR_ASYNC_STATUS_ERROR;
  gchar* name = NULL;

  /* Read from the pipe */
  if (condition | G_IO_IN)
    {
      int rv;
      char* buf;
      int length;

      buf = &state->buffer[state->len];
      length = sizeof(state->buffer) - state->len;

      if ((rv = read(state->fd, buf, length)) > 0)
	{
	  state->len += rv;

	  /* Return true if there's more to read */
	  if ((state->len - 1) != state->buffer[0])
	    return TRUE;

	  /* Copy the name */
	  name = g_new(gchar, state->buffer[0] + 1);
	  strncpy(name, &state->buffer[1], state->buffer[0]);
	  state->ia->name = name;

	  status = GINETADDR_ASYNC_STATUS_OK;

	  close(state->fd);
	  waitpid(state->pid, NULL, 0);
	}
    }


  /* Call back */
  (*state->func)(state->ia, status, name, state->data);
  g_free(state);

  return FALSE;
}




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
gnet_inetaddr_get_canonical_name(GInetAddr* ia)
{
  gchar buffer[INET_ADDRSTRLEN];	/* defined in netinet/in.h */
  guchar* p = (guchar*) &(GNET_SOCKADDR_IN(ia->sa).sin_addr);
  
  g_return_val_if_fail (ia != NULL, NULL);

  g_snprintf(buffer, sizeof(buffer), 
	     "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
  
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

  return (gint) g_ntohs(((struct sockaddr_in*) &ia->sa)->sin_port);
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

  ((struct sockaddr_in*) &ia->sa)->sin_port = g_htons(port);
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
gnet_inetaddr_hash(const gpointer p)
{
  const GInetAddr* ia;
  guint32 port;
  guint32 addr;

  g_assert(p != NULL);

  ia = (const GInetAddr*) p;
  /* We do pay attention to network byte order just in case the hash
     result is saved or sent to a different host.  */
  port = (guint32) g_ntohs(((struct sockaddr_in*) &ia->sa)->sin_port);
  addr = g_ntohl(((struct sockaddr_in*) &ia->sa)->sin_addr.s_addr);

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
gnet_inetaddr_equal(const gpointer p1, const gpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  g_assert(p1 != NULL && p2 != NULL);

  /* Note network byte order doesn't matter */
  return ((GNET_SOCKADDR_IN(ia1->sa).sin_addr.s_addr ==
	   GNET_SOCKADDR_IN(ia2->sa).sin_addr.s_addr) &&
	  (GNET_SOCKADDR_IN(ia1->sa).sin_port ==
	   GNET_SOCKADDR_IN(ia2->sa).sin_port));
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
gnet_inetaddr_noport_equal(const gpointer p1, const gpointer p2)
{
  const GInetAddr* ia1 = (const GInetAddr*) p1;
  const GInetAddr* ia2 = (const GInetAddr*) p2;

  g_assert(p1 != NULL && p2 != NULL);

  /* Note network byte order doesn't matter */
  return (GNET_SOCKADDR_IN(ia1->sa).sin_addr.s_addr ==
	  GNET_SOCKADDR_IN(ia2->sa).sin_addr.s_addr);
}



/* **************************************** */


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
  struct utsname myname;
  gchar* name = NULL;

  if (uname(&myname) < 0)
    return NULL;

  if (!gnet_gethostbyname(myname.nodename, NULL, &name))
    return NULL;

  return name;
}


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
