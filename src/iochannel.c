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

