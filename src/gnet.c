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
#include <config.h>


const guint gnet_major_version = GNET_MAJOR_VERSION;
const guint gnet_minor_version = GNET_MINOR_VERSION;
const guint gnet_micro_version = GNET_MICRO_VERSION;
const guint gnet_interface_age = GNET_INTERFACE_AGE;
const guint gnet_binary_age = GNET_BINARY_AGE;


/**
 * gnet_io_channel_writen:
 * @channel: the channel to write to
 * @buf: the buffer to read from
 * @len: length of the buffer
 * @bytes_written: pointer to integer for the function to store the
 * number of bytes writen.
 * 
 * Write all @len bytes in the buffer to the channel.  This is
 * basically a wrapper around g_io_channel_read().  The problem with
 * g_io_channel_write() is that it may not write all the bytes in the
 * buffer and return a short count even when there was not an error
 * (this is rare, but it can happen and is often difficult to detect
 * when it does).
 *
 * Returns: G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes writen by modifying
 * the integer pointed to by bytes_written.
 *
 **/
GIOError
gnet_io_channel_writen (GIOChannel    *channel, 
			gchar         *buf, 
			guint          len,
			guint         *bytes_written)
{
  guint written = 0;
  guint n = 0;
  GIOError error;

  do
    {
      error = g_io_channel_write(channel, &buf[written], 
				 len - written, &n);

      if (error == G_IO_ERROR_NONE)
	written += n;

    }
  while ((written != len) && 
	 (error != G_IO_ERROR_NONE || error == G_IO_ERROR_AGAIN));

  /* TODO: should account for an EINTR in Unix. */

  *bytes_written = written;

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
 * integer pointed to by bytes_read.
 *
 **/
GIOError
gnet_io_channel_readn (GIOChannel    *channel, 
		       gchar         *buf, 
		       guint          len,
		       guint         *bytes_read)
{
  guint read = 0;
  guint n = 0;
  GIOError error;

  do
    {
      error = g_io_channel_read(channel, &buf[read], 
				 len - read, &n);

      if (error == G_IO_ERROR_NONE)
	read += n;
    }
  while ((read != len) && 
	 (error != G_IO_ERROR_NONE || error == G_IO_ERROR_AGAIN));

  /* TODO: should account for an EINTR in Unix. */

  *bytes_read = read;

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
 * If the buffer is full and the last character is not a newline, the
 * line was truncated.  If the buffer is not full and the last
 * character is not a newline, then the channel was closed.  So, do
 * not assume the buffer ends with a newline.
 *
 * Also, @bytes_read is actually the number of bytes put in the
 * buffer.  That is, it includes the terminating null character.
 * 
 * Which is related to another subtlety - null characters can appear
 * in the line your read in before the terminating null.  If this
 * matters in your program, check the string length of the buffer
 * against the bytes read.
 *
 * I hope this isn't too confusing.  Usually the function works as you
 * expect it to if you have a big enough buffer.  If you have the
 * Stevens book, you should be familiar with the semantics.
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by bytes_read.
 * 
 **/
GIOError
gnet_io_channel_readline (GIOChannel    *channel, 
			  gchar         *buf, 
			  guint          len,
			  guint         *bytes_read)
{
  guint read = 0;
  gchar onebyte;
  gchar* p = buf;
  guint n;
  GIOError error = G_IO_ERROR_NONE;


  for (read = 1; read < len; ++read)
    {
    try_again:
      error = gnet_io_channel_readn(channel, &onebyte, 1, &n);

      if (error == G_IO_ERROR_AGAIN)
	goto try_again;

      if ((error != G_IO_ERROR_NONE) || (n != 1))
	break;

      *p = onebyte;
      p++;

      if (onebyte == '\n')
	break;

      /* TODO: should account for an EINTR in Unix. */
    }

  *p = 0;
  *bytes_read = read;

  return error;
}
