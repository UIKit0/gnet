/***********************************************************************
 *                   _     _                     __   _  _
 *   __ _ _ __   ___| |_  | |__   __ _ ___  ___ / /_ | || |
 *  / _` | '_ \ / _ \ __| | '_ \ / _` / __|/ _ \ '_ \| || |_
 * | (_| | | | |  __/ |_  | |_) | (_| \__ \  __/ (_) |__   _|
 *  \__, |_| |_|\___|\__| |_.__/ \__,_|___/\___|\___/   |_|
 *  |___/
 *
 *  created by Alfred Reibenschuh <alfredreibenschuh@gmx.net>,
 *  under the ``GNU Library General Public License´´ (see below).
 *
 ***********************************************************************
 *
 * Copyright (C) 2003 Free Software Foundation
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***********************************************************************/


#include "base64.h"
#include <string.h>
#include <ctype.h>


static gchar gnet_Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define gnet_Pad64	'='


/**
 *  gnet_base64_encode
 *  @src:
 *  @srclen:
 *  @dstlen:
 *  @strict:
 *
 *  Returns:
 *
 **/
gchar*
gnet_base64_encode (gchar* src, gint srclen, gint *dstlen, gint strict) 
{
  gchar* dst;
  gint dstpos;
  gchar input[3];
  gchar output[4];
  gint ocnt;
  gint i;

  if (srclen == 0) 
    return NULL;	/* FIX: Or return ""? */

  /* Calculate required length of dst.  4 bytes of dst are needed for
     every 3 bytes of src. */
  *dstlen = (((srclen + 2) / 3) * 4);
  if (strict)
    *dstlen += (*dstlen / 72);	/* Handle trailing \n */

  dst = g_new(gchar, *dstlen+1);

  /* bulk encoding */
  dstpos = 0;
  ocnt = 0;
  while (srclen >= 3) 
    {
      /*
	Convert 3 bytes of src to 4 bytes of output

	output[0] = input[0] 7:2
	output[1] = input[0] 1:0 input[1] 7:4
	output[2] = input[1] 3:0 input[2] 7:6
	output[3] = input[1] 5:0

       */
      input[0] = *src++;
      input[1] = *src++;
      input[2] = *src++;
      srclen -= 3;

      output[0] = (input[0] >> 2);
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
      output[3] = (input[2] & 0x3f);

      /* ??? FIX: assert? */
      if (dstpos + 4 > *dstlen) 
	return NULL;

      /* Map output to the Base64 alphabet */
      dst[dstpos++] = gnet_Base64[(guint) output[0]];
      dst[dstpos++] = gnet_Base64[(guint) output[1]];
      dst[dstpos++] = gnet_Base64[(guint) output[2]];
      dst[dstpos++] = gnet_Base64[(guint) output[3]];

      /* Add a newline if strict and  */
      if (strict)
	if ((++ocnt % (72/4)) == 0) 
	  dst[dstpos++] = '\n';
    }

  /* Now worry about padding with remaining 1 or 2 bytes */
  if (srclen != 0) 
    {
      input[0] = input[1] = input[2] = '\0';
      for (i = 0; i < srclen; i++) 
	input[i] = *src++;

      output[0] = (input[0] >> 2);
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

      /* ??? */
      if (dstpos + 4 > *dstlen) 
	return NULL;

      dst[dstpos++] = gnet_Base64[(guint) output[0]];
      dst[dstpos++] = gnet_Base64[(guint) output[1]];

      if (srclen == 1)
	dst[dstpos++] = gnet_Pad64;
      else
	dst[dstpos++] = gnet_Base64[(guint) output[2]];

      dst[dstpos++] = gnet_Pad64;
    }

  /* ??? */
  if (dstpos >= *dstlen) 
    return NULL;

  dst[dstpos] = '\0';

  *dstlen = dstpos + 1;

  return dst;
}


/**
 *  gnet_base64_decode
 *  @src:
 *  @srclen:
 *  @dstlen:
 *  @strict:
 *
 *  Returns:
 *
 **/
gchar* 
gnet_base64_decode (gchar* src, gint srclen, gint* dstlen)
{
  gchar* dst;
  gint dstidx, state, ch = 0;
  gchar res;
  gchar* pos;

  if (srclen == 0) 		/* REMOVE? */
    srclen = strlen(src);

  state = 0;
  dstidx = 0;
  res = 0;

  dst = g_new(gchar, srclen+1);
  *dstlen = srclen+1;			/* Dstlen = (srclen / 4) * 3 + 1 */

  while (srclen-- > 0) 
    {
      ch = *src++;
      if (isascii(ch) && isspace(ch)) /* Skip whitespace anywhere */
	continue;
      if (ch == gnet_Pad64) 
	break;
      pos = strchr(gnet_Base64, ch);
      if (pos == 0) /* A non-base64 character */
	{
	  g_free (dst);
	  return NULL;
	}

      switch (state) 
	{
	case 0:
	  dst[dstidx] = ((pos - gnet_Base64) << 2);
	  state = 1;
	  break;

	case 1:
	  dst[dstidx] |= ((pos - gnet_Base64) >> 4);
	  res = (((pos - gnet_Base64) & 0x0f) << 4);
	  dstidx++;
	  state = 2;
	  break;

	case 2:
	  dst[dstidx] = res | ((pos - gnet_Base64) >> 2);
	  res = ((pos - gnet_Base64) & 0x03) << 6;
	  dstidx++;
	  state = 3;
	  break;

	case 3:
	  if (dst != NULL) 
	    dst[dstidx] = res | (pos - gnet_Base64);
	  dstidx++;
	  state = 0;
	  break;

	default:
	  break;
	}
    }

  /*
   * We are done decoding Base-64 chars.  Let's see if we ended
   * on a byte boundary, and/or with erroneous trailing characters.
   */
  if (ch == gnet_Pad64)           /* We got a pad char. */
    {
      switch (state) 
	{
	case 0:             /* Invalid = in first position */
	case 1:             /* Invalid = in second position */
	  g_free (dst);
	  return NULL;
	case 2:             /* Valid, means one byte of info */
                                /* Skip any number of spaces. */
	  while (srclen-- > 0) 
	    {
	      ch = *src++;
	      if (!(isascii(ch) && isspace(ch))) 
		break;
	    }
                                /* Make sure there is another trailing = sign. */
	  if (ch != gnet_Pad64) 
	    {
	      g_free (dst);
	      return NULL;
	    }
                                /* FALLTHROUGH */
	case 3:                 /* Valid, means two bytes of info */
                                /*
                                 * We know this char is an =.  Is there anything but
                                 * whitespace after it?
                                 */
	  while (srclen-- > 0) 
	    {
	      ch = *src++;
	      if (!(isascii(ch) && isspace(ch))) 
		{
		  g_free (dst);
		  return NULL;
		}
	    }
	  /*
	   * Now make sure for cases 2 and 3 that the "extra"
	   * bits that slopped past the last full byte were
	   * zeros.  If we don't check them, they become a
	   * subliminal channel.
	   */
	  if (dst != NULL && res != 0) 
	    {
	      g_free (dst);
	      return NULL;
	    }

	default:
	  {
	    g_assert_not_reached();
	    break;
	  }
	}
    } 
  else 
    {
      /*
       * We ended by seeing the end of the string.  Make sure we
       * have no partial bytes lying around.
       */
      if (state != 0) 
	{
	  g_free (dst);
	  return NULL;
	}
    }

  *dstlen = dstidx;

  return dst;
}


