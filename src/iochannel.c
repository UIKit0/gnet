/* GNet - Networking library
 * Copyright (C) 2000  David Helder
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
#include "gnet.h"


/**
 * gnet_io_channel_writen:
 * @channel: the channel to write to
 * @buf: the buffer to read from
 * @len: length of the buffer
 * @bytes_written: pointer to integer for the function to store the
 * number of bytes writen.
 * 
 * Write all @len bytes in the buffer to the channel.  This is
 * basically a wrapper around g_io_channel_write().  The problem with
 * g_io_channel_write() is that it may not write all the bytes in the
 * buffer and return a short count even when there was not an error
 * (this is rare, but it can happen and is often difficult to detect
 * when it does).
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes writen by modifying
 * the integer pointed to by @bytes_written.
 *
 **/
GIOError
gnet_io_channel_writen (GIOChannel    *channel, 
			gchar         *buf, 
			guint          len,
			guint         *bytes_written)
{
  guint nleft;
  guint nwritten;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buf;
  nleft = len;

  while (nleft > 0)
    {
      if ((error = g_io_channel_write(channel, ptr, nleft, &nwritten))
	  != G_IO_ERROR_NONE)
	{
	  if (error == G_IO_ERROR_AGAIN)
	    nwritten = 0;
	  else
	    break;
	}

      nleft -= nwritten;
      ptr += nwritten;
    }

  *bytes_written = (len - nleft);

  return error;
}



/**
 * gnet_io_channel_readn:
 * @channel: the channel to read from
 * @buf: the buffer to write to
 * @len: the length of the buffer
 * @bytes_read: pointer to integer for the function to store the 
 * number of of bytes read.
 *
 * Read exactly @len bytes from the channel the buffer to the channel.
 * This is basically a wrapper around g_io_channel_read().  The
 * problem with g_io_channel_read() is that it may not read all the
 * bytes wanted and return a short count even when there was not an
 * error (this is rare, but it can happen and is often difficult to
 * detect when it does).
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by @bytes_read.  If @bytes_read is 0, the end of
 * the file has been reached (eg, the socket has been closed).
 *
 **/
GIOError
gnet_io_channel_readn (GIOChannel    *channel, 
		       gchar         *buf, 
		       guint          len,
		       guint         *bytes_read)
{
  guint nleft;
  guint nread;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buf;
  nleft = len;

  while (nleft > 0)
    {
      if ((error = g_io_channel_read(channel, buf, nleft, &nread))
	  !=  G_IO_ERROR_NONE)
	{
	  if (error == G_IO_ERROR_AGAIN)
	    nread = 0;
	  else
	    break;
	}
      else if (nread == 0)
	break;

      nleft -= nread;
      ptr += nread;
    }

  *bytes_read = (len - nleft);

  return error;
}


/**
 * gnet_io_channel_readline:
 * @channel: the channel to read from
 * @buf: the buffer to write to
 * @len: length of the buffer
 * @bytes_read: pointer to integer for the function to store the 
 *   number of of bytes read.
 *
 * Read a line from the channel.  The line will be null-terminated and
 * include the newline character.  If there is not enough room for the
 * line, the line is truncated to fit in the buffer.
 * 
 * Warnings: (in the gotcha sense, not the bug sense)
 * 
 * 1. If the buffer is full and the last character is not a newline,
 * the line was truncated.  So, do not assume the buffer ends with a
 * newline.
 *
 * 2. @bytes_read is actually the number of bytes put in the buffer.
 * That is, it includes the terminating null character.
 * 
 * 3. Null characters can appear in the line before the terminating
 * null (I could send the string "Hello world\0\n").  If this matters
 * in your program, check the string length of the buffer against the
 * bytes read.
 *
 * I hope this isn't too confusing.  Usually the function works as you
 * expect it to if you have a big enough buffer.  If you have the
 * Stevens book, you should be familiar with the semantics.
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by @bytes_read (this number includes the
 * newline).  If an error is returned, the contents of @buf and
 * @bytes_read are undefined.
 * 
 **/
GIOError
gnet_io_channel_readline (GIOChannel    *channel, 
			  gchar         *buf, 
			  guint          len,
			  guint         *bytes_read)
{
  guint n, rc;
  gchar c, *ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buf;

  for (n = 1; n < len; ++n)
    {
    try_again:
      error = gnet_io_channel_readn(channel, &c, 1, &rc);

      if (error == G_IO_ERROR_NONE && rc == 1)		/* read 1 char */
	{
	  *ptr++ = c;
	  if (c == '\n')
	    break;
	}
      else if (error == G_IO_ERROR_NONE && rc == 0)	/* read EOF */
	{
	  if (n == 1)	/* no data read */
	    {
	      *bytes_read = 0;
	      return G_IO_ERROR_NONE;
	    }
	  else		/* some data read */
	    break;
	}
      else
	{
	  if (error == G_IO_ERROR_AGAIN)
	    goto try_again;

	  return error;
	}
    }

  *ptr = 0;
  *bytes_read = n;

  return error;
}



/**
 * gnet_io_channel_readline_strdup:
 * @channel: the channel to read from
 * @buf_ptr: pointer to gchar* for the functin to store the new buffer
 * @bytes_read: pointer to integer for the function to store the 
 *   number of of bytes read.
 *
 * Read a line from the channel.  The line will be null-terminated and
 * include the newline character.  Similarly to g_strdup_printf, a
 * buffer large enough to hold the string will be allocated.
 * 
 * Warnings: (in the gotcha sense, not the bug sense)
 * 
 * 1. If the last character of the buffer is not a newline, the line
 * was truncated by EOF.  So, do not assume the buffer ends with a
 * newline.
 *
 * 2. @bytes_read is actually the number of bytes put in the buffer.
 * That is, it includes the terminating null character.
 * 
 * 3. Null characters can appear in the line before the terminating
 * null (I could send the string "Hello world\0\n").  If this matters
 * in your program, check the string length of the buffer against the
 * bytes read.
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by @bytes_read (this number includes the
 * newline), and the data through the pointer pointed to by @buf_ptr.
 * This data should be freed with g_free().  If an error is returned,
 * the contents of @buf_ptr and @bytes_read are undefined.
 *
 **/
GIOError
gnet_io_channel_readline_strdup (GIOChannel    *channel, 
				 gchar         **buf_ptr, 
				 guint         *bytes_read)
{
  guint rc, n, len;
  gchar c, *ptr, *buf;
  GIOError error = G_IO_ERROR_NONE;

  len = 100;
  buf = (gchar *)g_malloc(len);
  ptr = buf;
  n = 1;

  while (1)
    {
    try_again:
      error = gnet_io_channel_readn(channel, &c, 1, &rc);

      if (error == G_IO_ERROR_NONE && rc == 1)          /* read 1 char */
        {
          *ptr++ = c;
          if (c == '\n')
            break;
        }
      else if (error == G_IO_ERROR_NONE && rc == 0)     /* read EOF */
        {
          if (n == 1)   /* no data read */
            {
              *bytes_read = 0;
	      *buf_ptr = NULL;
	      g_free(buf);

              return G_IO_ERROR_NONE;
            }
          else          /* some data read */
            break;
        }
      else
        {
          if (error == G_IO_ERROR_AGAIN)
            goto try_again;

          g_free(buf);

          return error;
        }

      ++n;

      if (n >= len)
        {
          len *= 2;
          buf = g_realloc(buf, len);
          ptr = buf + n;
        }
    }

  *ptr = 0;
  *buf_ptr = buf;
  *bytes_read = n;

  return error;
}


/* **************************************** */

#ifndef GNET_WIN32	/* New stuff, not ported to Win32 yet */
/* ************************************************************ */

typedef struct _GNetIOChannelWriteAsyncState
{
  GIOChannel* iochannel;

  gchar* buffer;
  guint length;
  guint n;

  GNetIOChannelWriteAsyncFunc func;
  gpointer user_data;

} GNetIOChannelWriteAsyncState;

static gboolean write_async_write_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data);
static gboolean write_async_error_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data);
static gboolean write_async_timeout_cb (gpointer data);



GNetIOChannelWriteAsyncID
gnet_io_channel_write_async(GIOChannel* iochannel, 
			   gchar* buffer, guint length, guint timeout,
			   GNetIOChannelWriteAsyncFunc func, 
			   gpointer user_data)
{
  GNetIOChannelWriteAsyncState* state;

  g_return_val_if_fail (iochannel != NULL, NULL);
  g_return_val_if_fail ((buffer != NULL && length != 0) || (buffer == NULL && length == 0), NULL);
  g_return_val_if_fail (func != NULL, NULL);

  if (buffer == NULL)
    {
      (func)(iochannel, buffer, length, 0, 
	     GNET_IOCHANNEL_WRITE_ASYNC_STATUS_OK, user_data);
      return NULL;
    }

  state = g_new0(GNetIOChannelWriteAsyncState, 1);

  state->iochannel = iochannel;
  state->buffer = buffer;
  state->length = length;
  state->n = 0;

  state->func = func;
  state->user_data = user_data;

  g_io_add_watch(iochannel, G_IO_OUT,  write_async_write_cb, state);
  g_io_add_watch(iochannel, G_IO_ERR | G_IO_HUP | G_IO_NVAL,  write_async_error_cb, state);

  if (timeout > 0)
    g_timeout_add(timeout, write_async_timeout_cb, state);

  return state;
}


void
gnet_io_channel_write_async_cancel(GNetIOChannelWriteAsyncID id, 
				  gboolean delete_buffer)
{
  GNetIOChannelWriteAsyncState* state;

  g_return_if_fail (id != NULL);

  state = (GNetIOChannelWriteAsyncState*) id;

  if (delete_buffer)
    g_free(state->buffer);

  while (g_source_remove_by_user_data(state))
    ;

  g_free(state);
}


static gboolean 
write_async_write_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GNetIOChannelWriteAsyncState* state;
  guint bytes_writen;

  state = (GNetIOChannelWriteAsyncState*) data;

  g_return_val_if_fail (iochannel != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (condition == G_IO_OUT, FALSE);
  g_return_val_if_fail (iochannel == state->iochannel, FALSE);

  if (g_io_channel_write(iochannel, 
			 &state->buffer[state->n], 
			 state->length - state->n,
			 &bytes_writen) != G_IO_ERROR_NONE)
    {
      state->func(iochannel, state->buffer, state->length, state->n,
		  GNET_IOCHANNEL_WRITE_ASYNC_STATUS_ERROR, state->user_data);
      gnet_io_channel_write_async_cancel(state, FALSE);
      return FALSE;
    }

  state->n += bytes_writen;

  if (state->n == state->length)
    {
      state->func(iochannel, state->buffer, state->length, state->n,
		  GNET_IOCHANNEL_WRITE_ASYNC_STATUS_OK, state->user_data);
      gnet_io_channel_write_async_cancel(state, FALSE);
      return FALSE;
    }

  return TRUE;
}


static gboolean 
write_async_error_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GNetIOChannelWriteAsyncState* state;

  state = (GNetIOChannelWriteAsyncState*) data;

  g_return_val_if_fail (iochannel != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (iochannel == state->iochannel, FALSE);

  state->func(iochannel, state->buffer, state->length, state->n,
	      GNET_IOCHANNEL_WRITE_ASYNC_STATUS_ERROR, state->user_data);
  gnet_io_channel_write_async_cancel(state, FALSE);

  return FALSE;
}


static gboolean 
write_async_timeout_cb (gpointer data)
{
  GNetIOChannelWriteAsyncState* state;

  state = (GNetIOChannelWriteAsyncState*) data;

  g_return_val_if_fail (state != NULL, FALSE);

  state->func(state->iochannel, state->buffer, state->length, state->n,
	      GNET_IOCHANNEL_WRITE_ASYNC_STATUS_TIMEOUT, state->user_data);
  gnet_io_channel_write_async_cancel(state, FALSE);

  return FALSE;
}


/* ************************************************************ */

typedef struct _GNetIOChannelReadAsyncState
{
  GIOChannel* iochannel;

  gboolean read_one;
  gboolean my_buffer;
  gchar* buffer;
  guint max_len;
  guint length;
  guint offset;
  guint timeout;

  guint read_watch;
  guint other_watch;
  guint timer;

  GNetIOChannelReadAsyncCheckFunc check_func;
  gpointer check_user_data;

  GNetIOChannelReadAsyncFunc func;
  gpointer user_data;

} GNetIOChannelReadAsyncState;

static gboolean read_async_read_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data);
static gboolean read_async_error_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data);
static gboolean read_async_timeout_cb (gpointer data);



GNetIOChannelReadAsyncID
gnet_io_channel_read_async (GIOChannel* iochannel, 
			   gchar* buffer, guint length, guint timeout, 
			   gboolean read_one_byte_at_a_time, 
			   GNetIOChannelReadAsyncCheckFunc check_func, 
			   gpointer check_user_data,
			   GNetIOChannelReadAsyncFunc func, 	    
			   gpointer user_data)
{
  GNetIOChannelReadAsyncState* state;

  g_return_val_if_fail (iochannel, NULL);
  g_return_val_if_fail (check_func, NULL);
  g_return_val_if_fail (func, NULL);
  g_return_val_if_fail (buffer || (!buffer && length), NULL);

  state = g_new0 (GNetIOChannelReadAsyncState, 1);

  state->iochannel = iochannel;
  state->read_one = read_one_byte_at_a_time;

  if (buffer)
    {
      state->my_buffer = FALSE;
      state->buffer = buffer;
      state->max_len = length;
      state->length = length;
      state->offset = 0;
    }
  else
    {
      state->my_buffer = TRUE;
      state->buffer = NULL;
      state->max_len = length;
      state->length = 0;
      state->offset = 0;
    }

  state->check_func = 		check_func;
  state->check_user_data = 	check_user_data;
  state->func = 		func;
  state->user_data = 		user_data;

  state->read_watch = g_io_add_watch(iochannel, G_IO_IN,   
				     read_async_read_cb, state);
  state->other_watch = g_io_add_watch(iochannel, 
				      G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
				      read_async_error_cb, state);

  state->timeout = timeout;
  if (timeout > 0)
    state->timer = g_timeout_add(timeout, read_async_timeout_cb, state);

  return state;
}


void
gnet_io_channel_read_async_cancel (GNetIOChannelReadAsyncID id)
{
  GNetIOChannelReadAsyncState* state;

  g_return_if_fail (id != NULL);

  state = (GNetIOChannelReadAsyncState*) id;

  if (state->read_watch)
    g_source_remove (state->read_watch);
  if (state->other_watch)
    g_source_remove (state->other_watch);
  if (state->timer)
    g_source_remove (state->timer);

  if (state->my_buffer)
    g_free(state->buffer);

  g_free(state);
}


static gboolean 
read_async_read_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GNetIOChannelReadAsyncState* state;
  GIOError error;
  guint bytes_to_read;
  guint bytes_read;
  gint bytes_processed;

  state = (GNetIOChannelReadAsyncState*) data;

  g_return_val_if_fail (iochannel != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (condition == G_IO_IN, FALSE);
  g_return_val_if_fail (iochannel == state->iochannel, FALSE);


  /* Check if we should make the buffer larger */
  if (state->my_buffer && state->length == state->offset)
    {
      if (state->length)
	{
	  state->length *= 2;
	  state->buffer = g_realloc(state->buffer, state->length);
	}
      else
	{
	  state->length = MIN (128, state->max_len);
	  state->buffer = g_malloc(state->length);
	}
    }


  if (state->read_one)
    bytes_to_read = 1;
  else
    bytes_to_read = state->length - state->offset;

  /* Read in some stuff */
  error = g_io_channel_read(iochannel, &state->buffer[state->offset], 
			    bytes_to_read, &bytes_read);
  state->offset += bytes_read;

  if (error == G_IO_ERROR_AGAIN)
    return TRUE;

  else if (error != G_IO_ERROR_NONE)
    {
      state->func (iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_ERROR, 
		   NULL, 0, state->user_data);
      gnet_io_channel_read_async_cancel (state);
      return FALSE;
    }

  /* If we read nothing, that means EOF and we're done.  Note we do
     not send anything that might be in the buffer */
  else if (bytes_read == 0)
    {
      state->func (iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_OK, 
		   NULL, 0, state->user_data);
      gnet_io_channel_read_async_cancel (state);
      return FALSE;
    }

  /* Check if we read something */
 again:
  bytes_processed = (state->check_func)(state->buffer, state->offset, state->check_user_data);
  if (bytes_processed)
    {
      if (!state->func (iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_OK, 
			state->buffer, bytes_processed, state->user_data))
	{
	  gnet_io_channel_read_async_cancel (state);
	  return FALSE;
	}

      /* Move it over */
      g_memmove (state->buffer, &state->buffer[bytes_processed], 
		 state->offset - bytes_processed);
      state->offset -= bytes_processed;
      goto again;
    }

  /* Check if we hit the max length.  If so, it's an error. */
  if (state->offset >= state->max_len)
    {
      state->func(iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_ERROR, 
		  state->buffer, state->offset, state->user_data);
      gnet_io_channel_read_async_cancel (state);
      return FALSE;
    }

  /* Reset the timer */
  if (state->timer)
    {
      g_assert (g_source_remove (state->timer));
      state->timer = g_timeout_add (state->timeout, 
				    read_async_timeout_cb, 
				    state);
    }

  return TRUE;
}


static gboolean 
read_async_error_cb (GIOChannel* iochannel, GIOCondition condition, 
		     gpointer data)
{
  GNetIOChannelReadAsyncState* state;

  state = (GNetIOChannelReadAsyncState*) data;

  g_return_val_if_fail (iochannel, FALSE);
  g_return_val_if_fail (state, FALSE);

  (state->func)(iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_ERROR, 
		NULL, 0, state->user_data);
  gnet_io_channel_read_async_cancel(state);

  return FALSE;
}


static gboolean 
read_async_timeout_cb (gpointer data)
{
  GNetIOChannelReadAsyncState* state;

  state = (GNetIOChannelReadAsyncState*) data;

  g_return_val_if_fail (state, FALSE);

  (state->func)(state->iochannel, GNET_IOCHANNEL_READ_ASYNC_STATUS_TIMEOUT, 
		NULL, 0, state->user_data);
  gnet_io_channel_read_async_cancel(state);

  return FALSE;
}


gint
gnet_io_channel_readany_check_func (gchar* buffer, guint length, 
				   gpointer user_data)
{
  return length;
}


gint
gnet_io_channel_readline_check_func (gchar* buffer, guint length, 
				     gpointer user_data)
{
  guint i;

  for (i = 0; i < length; ++i)
    {
      if (buffer[i] == '\n')
	return i + 1;
    }

  return 0;
}
#endif

/* ************************************************************ */

/* 

	Alternative Implementation of Socket GIOChannels for GLIB for Windows 

Copyright (C) Andrew Lanoix

Portions Copyright (C) Peter Mattis, Spencer Kimball, Josh MacDonald, 
											 Owen Taylor and Tor Lillqvist

NOTES:
	Supports multiple watches per socket.
	Supports one GIOChannel per socket.
	OOB data is not supported.

Implementation:
	I have set things up much like the other async calls. I have created
	a hidden window that I use specifically of handling watches on the 
	sockets. The msg.message = socketfd + an offset. The offset is 
	necessary since message < 100 are reserved. A global hash is used to 
	keep state info. The only big difference is that I have a 
	'main record' state that contains the cumulative information 
	about all the watches on one socket iochannel. This contains a GSList 
	that contains info about each specific watch. I did this since Winsock2
	can only handle one watch per socket. I have info to keep track of which
	watches need to be called on that socket of any given GIOCondition.  
	The main record is created when the iochannel is made, and stays around 
	until the channel is destroyed.  

*/

#ifdef GNET_WIN32

const int offset = 101;

typedef struct _GIOWin32Channel GIOWin32Channel;
typedef struct _GIOWin32Watch GIOWin32Watch;

struct _GIOWin32Channel
{
  GIOChannel channel;
  gint fd;
};

struct _GIOWin32Watch {
  GPollFD					pollfd;
  GIOWin32Channel *channel;
  GIOCondition		condition;
  GIOFunc					callback;
	gpointer				user_data;
	long						wincondition;
};

static GIOError g_io_win32_sock_read (GIOChannel *channel, 
				      gchar      *buf, 
				      guint       count,
				      guint      *bytes_written);
static GIOError g_io_win32_sock_write(GIOChannel *channel, 
				      gchar      *buf, 
				      guint       count,
				      guint      *bytes_written);
static void g_io_win32_sock_close(GIOChannel *channel);
static void g_io_win32_free (GIOChannel *channel);

static GIOError g_io_win32_no_seek (GIOChannel *channel,
							gint      offset, 
							GSeekType type);

static guint g_io_win32_sock_add_watch (GIOChannel      *channel,
					gint             priority,
					GIOCondition     condition,
					GIOFunc          func,
					gpointer         user_data,
					GDestroyNotify   notify);

static gboolean g_io_win32_sock_prepare  (gpointer  source_data, 
					  GTimeVal *current_time,
					  gint     *timeout);
static gboolean g_io_win32_sock_check    (gpointer  source_data,
					  GTimeVal *current_time);
static gboolean g_io_win32_sock_dispatch (gpointer  source_data,
					  GTimeVal *current_time,
					  gpointer  user_data);

static void g_io_win32_destroy (gpointer source_data);

GIOFuncs win32_channel_sock_funcs = {
  g_io_win32_sock_read,
  g_io_win32_sock_write,
  g_io_win32_no_seek,
  g_io_win32_sock_close,
  g_io_win32_sock_add_watch,
  g_io_win32_free
};

GSourceFuncs win32_watch_sock_funcs = {
  g_io_win32_sock_prepare,
  g_io_win32_sock_check,
  g_io_win32_sock_dispatch,
  g_io_win32_destroy
};

static GIOError 
g_io_win32_sock_read (GIOChannel *channel, 
		      gchar      *buf, 
		      guint       count,
		      guint      *bytes_read)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *) channel;
  gint result;

  result = recv (win32_channel->fd, buf, count, 0);
  if (result == SOCKET_ERROR)
    {
      *bytes_read = 0;
      switch (WSAGetLastError ())
	{
	case WSAEINVAL:
	  return G_IO_ERROR_INVAL;
	case WSAEWOULDBLOCK:
	case WSAEINTR:
	  return G_IO_ERROR_AGAIN;
	default:
	  return G_IO_ERROR_UNKNOWN;
	}
    }
  else
    {
      *bytes_read = result;
      return G_IO_ERROR_NONE;
    }
}
		       
static GIOError 
g_io_win32_sock_write(GIOChannel *channel, 
		      gchar      *buf, 
		      guint       count,
		      guint      *bytes_written)
{
  GIOWin32Channel *win32_channel = (GIOWin32Channel *) channel;
  gint result;

  result = send (win32_channel->fd, buf, count, 0);
      
  if (result == SOCKET_ERROR)
    {
      *bytes_written = 0;
      switch (WSAGetLastError ())
	{
	case WSAEINVAL:
	  return G_IO_ERROR_INVAL;
	case WSAEWOULDBLOCK:
	case WSAEINTR:
	  return G_IO_ERROR_AGAIN;
	default:
	  return G_IO_ERROR_UNKNOWN;
	}
    }
  else
    {
      *bytes_written = result;
      return G_IO_ERROR_NONE;
    }
}

static GIOError 
g_io_win32_no_seek (GIOChannel *channel,
		    gint      offset, 
		    GSeekType type)
{
  g_warning ("GNet's g_io_win32_no_seek: unseekable IO channel type");
  return G_IO_ERROR_UNKNOWN;
}


void purge_queue(int sockfd)
{
	BOOL status;
	MSG msg;
	
	status = 1;	
	while (status)
	{
		status = PeekMessage(&msg, gnet_sock_hWnd, sockfd+offset, sockfd+offset, PM_NOREMOVE);
		if (status)
		{
			GetMessage(&msg, gnet_sock_hWnd, sockfd+offset, sockfd+offset);
		}
	}	
}

/* This is called when the iochannel is being closed with g_io_channel_close() */
static void 
g_io_win32_sock_close (GIOChannel *channel)
{
	SocketWatchAsyncState* data;
	gpointer data2;
	SocketWatchAsyncState* state;
	GSList *list;
  GIOWin32Channel *win32_channel = (GIOWin32Channel *) channel;

	
	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = NULL;
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)win32_channel->fd);
	if (data)
		{
			data = (SocketWatchAsyncState*) data;
			
			/* Cancel event posting on the socket */
			WSAAsyncSelect(win32_channel->fd, gnet_sock_hWnd, 0, 0);
			purge_queue(win32_channel->fd);

			/* Delete and free any watches on the socket */
			data2 = data->callbacklist;
			list = (GSList*) data2; /* Typecasting this way seems to make VC happy */
			if (list)
			{
				g_slist_free((GSList*)list);
			}
			data->callbacklist = NULL;

			/* Remove the main record from the hash */
			g_hash_table_remove(gnet_select_hash, (gpointer)win32_channel->fd);
			
			closesocket(win32_channel->fd);

			/* Delete the main record for the socket*/
			g_free(data);
		}
	ReleaseMutex(gnet_select_Mutex);	
}

static void 
g_io_win32_free (GIOChannel *channel)
{
	SocketWatchAsyncState* data;
	gpointer data2;
	SocketWatchAsyncState* state;
	GSList *list;
  GIOWin32Channel *win32_channel = (GIOWin32Channel *) channel;

	if (!win32_channel)
		return;

	/* Cleanup, but don't close the socket fd */

	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = NULL;
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)win32_channel->fd);
	if (data)
	{
		data = (SocketWatchAsyncState*) data;

		/* Cancel event posting on the socket */
		WSAAsyncSelect(win32_channel->fd, gnet_sock_hWnd, 0, 0);
		purge_queue(win32_channel->fd);

		/* Delete and free any watches on the socket */
		data2 = data->callbacklist;
		list = (GSList*) data2; /* Typecasting this way seems to make VC happy */
		if (list)
		{
			g_slist_free((GSList*)list);
		}
		data->callbacklist = NULL;

		/* Remove the main record from the hash */
		g_hash_table_remove(gnet_select_hash, (gpointer)win32_channel->fd);
			
		/* Delete the main record for the socket*/
		g_free(data);
		}
	ReleaseMutex(gnet_select_Mutex);	

}

static guint 
g_io_win32_sock_add_watch (GIOChannel    *channel,
			   gint           priority,
			   GIOCondition   condition,
			   GIOFunc        func,
			   gpointer       user_data,
			   GDestroyNotify notify)
{
	gpointer data;
	SocketWatchAsyncState* state;
	long watch_event;
	gint status;
  GIOWin32Channel *win32_channel = (GIOWin32Channel *) channel;
	GIOWin32Watch *watch;

  g_io_channel_ref (channel);

	/*
	Conversion Table:
	Win32        Unix G_IO_
	FD_CONNECT   IN
	FD_READ      IN
	FD_WRITE     OUT
	FD_OOB       PRI
	FD_ACCEPT    IN
	*/

	watch_event = 0;
	if (condition & G_IO_PRI)
	{
		watch_event = watch_event|FD_OOB;
	}
	if (condition & G_IO_IN)
	{
		watch_event = watch_event|FD_ACCEPT|FD_READ|FD_CONNECT;
	}
	if (condition & G_IO_OUT)
	{
		watch_event = watch_event|FD_WRITE;
	}

	/* Need to get the main record */
	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = NULL;
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)win32_channel->fd);
	if (!data)
	{
		return -1; /* The main record should always be there */
	}

	state = (SocketWatchAsyncState*) data;

	/* Now create new GIOWin32Watch as the data and add it to the list */
	watch  = g_new0(GIOWin32Watch, 1);
	watch->pollfd.fd = win32_channel->fd;
	watch->condition = condition;
	watch->user_data = user_data;
	watch->callback = func;
	watch->channel = win32_channel;
	watch->wincondition = watch_event;

	/* Now update the master watch record for the socket iochannel */
	state->winevent = state->winevent | watch_event;

	/* Add the new watch to the socket watch list */
	state->callbacklist = g_slist_append(state->callbacklist, (gpointer)watch);

	purge_queue(win32_channel->fd);

	/* Note: WSAAsunc automatically sets the socket to noblocking mode */
  status = WSAAsyncSelect(win32_channel->fd, gnet_sock_hWnd, win32_channel->fd+offset, state->winevent);

	ReleaseMutex(gnet_select_Mutex);

	if (status == SOCKET_ERROR)
  {		
		return -1;
  }

	g_main_add_poll (&watch->pollfd, priority);
	return g_source_add (priority, TRUE, &win32_watch_sock_funcs, watch, user_data, notify);
}

gint remove_check(gconstpointer a, gconstpointer b)
{
	/* Ignore b, all we want to know is if we flagged this one to be removed if 
	the watch condition flags were set to be zero */

	GIOWin32Watch* watch = (GIOWin32Watch*) a;

	if (!watch)
		return 1;

	if ((watch->condition == 0) || (watch->wincondition == 0))
		{
			return 0; /* Then this record needs to be delete */
		}

	return 1;
}

void sock_dispatch(gpointer data1, gpointer data2)
{
	GIOWin32Watch* watch = (GIOWin32Watch*) data1;
	GIOCondition *win_condition = data2;
	GIOCondition cond_for_cb;
	GIOChannel *channel;
	gboolean returnval;
	u_long arg;

	cond_for_cb = watch->condition & *win_condition;
	if(cond_for_cb)
	{
		channel = (GIOChannel*) watch->channel;

		/* Force the socket into blocking mode */
		arg = 0;
		ioctlsocket(watch->pollfd.fd, FIONBIO, &arg);

		returnval = (*watch->callback)(channel, cond_for_cb, watch->user_data);

		/* Force the socket back into nonblocking mode */
		arg = 1;
		ioctlsocket(watch->pollfd.fd, FIONBIO, &arg);

		if (returnval == FALSE) /* Flag to delete later */
		{ 
			watch->wincondition = 0;
			watch->condition = 0;
		}

	}
}


int 
gnet_socket_watch_cb(GIOChannel *iochannel, GIOCondition condition, void *nodata)
{
	MSG msg;
  gpointer data;
  SocketWatchAsyncState *state;

	GIOCondition *win_condition;
	GSList *element, *thelist;
	gboolean flag;

  GetMessage (&msg, NULL, 0, 0);

	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)msg.wParam);
	ReleaseMutex(gnet_select_Mutex);

	state = (SocketWatchAsyncState*) data;
	state->errorcode = WSAGETSELECTERROR(msg.lParam);

	/*specifies the network event that has occurred */
	state->eventcode = WSAGETSELECTEVENT(msg.lParam); 

	win_condition = g_new0(GIOCondition, 1);

	if (state->errorcode)
	{
		*win_condition = *win_condition|G_IO_ERR;
	}
	if (state->eventcode|FD_CONNECT)
	{
		*win_condition = *win_condition|G_IO_IN;
  }
	if (state->eventcode|FD_OOB)
	{
		*win_condition = *win_condition|G_IO_PRI;
  }
	if (state->eventcode|FD_ACCEPT)
	{
		*win_condition = *win_condition|G_IO_IN;
  }
	if (state->eventcode|FD_READ)
	{
		*win_condition = *win_condition|G_IO_IN;	
	}
	if (state->eventcode|FD_WRITE)
	{
		*win_condition = *win_condition|G_IO_OUT;
	}

	/* Dispatch the watches */
	g_slist_foreach(state->callbacklist, sock_dispatch, (gpointer)win_condition); 
	g_free(win_condition);

	/* Now remove any watches that have conditions of zero- ie. flagged to be removed */
	flag = 1;
	thelist = ((GSList*)state->callbacklist);

	while (flag)
	{	

		if (!thelist)
			return 1;

		flag = 0;
		/* Find a watch flagged  for deletion */
		element = g_slist_find_custom((GSList*)thelist, (gpointer)NULL, (GCompareFunc)remove_check);
		thelist = ((GSList*)state->callbacklist);

		if (element)
		{
			flag = 1;

			/* Remove that watch */
			g_io_win32_destroy((gpointer)element->data); 
		}
	}

	return 1;
}

GIOChannel*
gnet_io_channel_win32_new_stream_socket (int socket)
{
	gpointer data;
  GIOWin32Channel *win32_channel;
  GIOChannel *channel;
	SocketWatchAsyncState* state;

	g_return_val_if_fail(socket > 0, NULL);

	/* Check to see if there is already an iochannel on the same socket. 
	   If so, return that.*/

	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = NULL;
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)socket);
	if(data)
	{
		state = (SocketWatchAsyncState*) data;
		channel = state->channel;
	}
	else
	{
		win32_channel = g_new (GIOWin32Channel, 1);
		channel = (GIOChannel *) win32_channel;

		g_io_channel_init (channel);
		channel->funcs = &win32_channel_sock_funcs;
		win32_channel->fd = socket;

		/* Need to create the main record here, not on the first watch */
		state = g_new0(SocketWatchAsyncState, 1);
		state->channel = channel;
		state->fd = win32_channel->fd;

		/*using sockfd as the key into the 'select' hash */
		g_hash_table_insert(gnet_select_hash, 
		      (gpointer) win32_channel->fd, 
		      (gpointer) state);	
	}
	ReleaseMutex(gnet_select_Mutex);

  return channel;
}

/* Not used */
static gboolean g_io_win32_sock_prepare  (gpointer  source_data, 
					  GTimeVal *current_time,
					  gint     *timeout)
{
  *timeout = -1; 

  return FALSE;
}

/* Not used */
static gboolean g_io_win32_sock_check    (gpointer  source_data,
					  GTimeVal *current_time)
{
	return 0;
}

/* Not used */
static gboolean g_io_win32_sock_dispatch (gpointer source_data, 
			GTimeVal *current_time,
			gpointer user_data)

{
	return 1;
}

void regen_winevent(gpointer data1, gpointer data2)
{
	GIOWin32Watch* watch = (GIOWin32Watch*) data1;
	long *win_event = data2;

	*win_event = watch->wincondition | *win_event;
}

/* This is called by g_source_remove*() */
void g_io_win32_destroy (gpointer source_data) 
{
	SocketWatchAsyncState* state;
	gpointer data;
	GSList *list;
	long *win_event;
  GIOWin32Watch *watch = source_data;
	gint refcount;

	WaitForSingleObject(gnet_select_Mutex, INFINITE);
	data = NULL;
	data = g_hash_table_lookup(gnet_select_hash, (gpointer)watch->pollfd.fd);
	if (!data)
	{	
		return; /* This should never happen */
	}

	state = (SocketWatchAsyncState*) data;
	refcount = state->channel->ref_count;

	g_main_remove_poll ((GPollFD*)&watch->pollfd);
  g_io_channel_unref((GIOChannel*)state->channel);
	/* The above unref might have called this function if ref=1, so we should not 
	proced if data is no longer valid */

	if (refcount == 1)
	{
		ReleaseMutex(gnet_select_Mutex);
		return;
	}

	list = state->callbacklist;
	list = g_slist_remove((GSList*)list, (gpointer)watch);
	state->callbacklist = list;

	if (list) 
	{
		win_event = g_new0(long, 1);

		/* Update the winevent condition on the main watch */
		g_slist_foreach(state->callbacklist, regen_winevent, (gpointer)win_event); 

		state->winevent = *win_event;

		/* Update the watch */
		purge_queue(state->fd);
		WSAAsyncSelect(state->fd, gnet_sock_hWnd, state->fd+offset, state->winevent);

		g_free(win_event);
	}
	else
	{
		/* Then there are no more watches on the iochannel */
		
		/* Cancel winsock2 watch on socketfd */
		WSAAsyncSelect(state->fd, gnet_sock_hWnd, 0, 0);
		purge_queue(state->fd);

		/* Don't remove the main record */
		state->winevent = 0;

	}
	ReleaseMutex(gnet_select_Mutex);
}

#endif

