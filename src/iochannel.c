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
 * @channel: Channel to write to
 * @buffer: Buffer to read from
 * @length: Length of @buffer
 * @bytes_written: Pointer to integer for us to store the
 *   number of bytes written (optional)
 * 
 * Write all @length bytes in @buffer to @channel.  If @bytes_written
 * is set, the number of bytes written is stored in the integer it
 * points to.  This is only useful in determining the number of bytes
 * actually written if an error occurs.
 *
 * This function is essentially a wrapper around g_io_channel_write().
 * The problem with g_io_channel_write() is that it may not write all
 * the bytes in the buffer and return a short count even when there
 * was not an error.  This is rare, but possible, and often difficult
 * to detect when it does happen.
 *
 * Returns: %G_IO_ERROR_NONE if successful; something else otherwise.
 * The number of bytes written is stored in the integer pointed to by
 * @bytes_written.
 *
 **/
GIOError
gnet_io_channel_writen (GIOChannel* channel, 
			gpointer    buffer, 
			guint       length,
			guint*      bytes_written)
{
  guint nleft;
  guint nwritten;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buffer;
  nleft = length;

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

  if (bytes_written)
    *bytes_written = (length - nleft);

  return error;
}


/**
 * gnet_io_channel_readn:
 * @channel: channel to read from
 * @buffer: buffer to write to
 * @length: length of the buffer
 * @bytes_read: Pointer to integer for the function to store the 
 * number of of bytes read (optional)
 *
 * Read exactly @length bytes from @channel into @buffer.  If
 * @bytes_read is set, the number of bytes read is stored in the
 * integer it points to.  This is only useful in determining the
 * number of bytes actually read if an error occurs.
 *
 * This function is essentially a wrapper around g_io_channel_read().
 * The problem with g_io_channel_read() is that it may not read all
 * the bytes requested and return a short count even when there was
 * not an error (this is rare, but it can happen and is often
 * difficult to detect when it does).
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by @bytes_read.  If @bytes_read is 0, the end of
 * the file has been reached (eg, the socket has been closed).
 *
 **/
GIOError
gnet_io_channel_readn (GIOChannel* channel, 
		       gpointer    buffer, 
		       guint       length,
		       guint*      bytes_read)
{
  guint nleft;
  guint nread;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buffer;
  nleft = length;

  while (nleft > 0)
    {
      if ((error = g_io_channel_read(channel, ptr, nleft, &nread))
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

  if (bytes_read)
    *bytes_read = (length - nleft);

  return error;
}


/**
 * gnet_io_channel_readline:
 * @channel: the channel to read from
 * @buffer: the buffer to write to
 * @length: length of the buffer
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
 * newline).  If an error is returned, the contents of @buffer and
 * @bytes_read are undefined.
 * 
 **/
GIOError
gnet_io_channel_readline (GIOChannel* channel, 
			  gchar*      buffer, 
			  guint       length,
			  guint*      bytes_read)
{
  guint n, rc;
  gchar c, *ptr;
  GIOError error = G_IO_ERROR_NONE;

  ptr = buffer;

  for (n = 1; n < length; ++n)
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
 * @bufferp: pointer to gchar* for the functin to store the new buffer
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
 * newline), and the data through the pointer pointed to by @bufferp.
 * This data should be freed with g_free().  If an error is returned,
 * the contents of @bufferp and @bytes_read are undefined.
 *
 **/
GIOError
gnet_io_channel_readline_strdup (GIOChannel* channel, 
				 gchar**     bufferp, 
				 guint*      bytes_read)
{
  guint rc, n, length;
  gchar c, *ptr, *buf;
  GIOError error = G_IO_ERROR_NONE;

  length = 100;
  buf = (gchar *)g_malloc(length);
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
	      *bufferp = NULL;
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

      if (n >= length)
        {
          length *= 2;
          buf = g_realloc(buf, length);
          ptr = buf + n - 1;
        }
    }

  *ptr = 0;
  *bufferp = buf;
  *bytes_read = n;

  return error;
}
