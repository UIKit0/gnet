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

#include "pack.h"
#include <string.h>

static void flipmemcpy(char* dst, char* src, int n);

static int
strlenn(char* str, int n)
{
  int len = 0;

  while (*str && len < n) ++len;

  return len;
}


#define MEMCPY(D,S,N)				\
  do {						\
    if ((N) == 1)				\
      *((char*) (D)) = *((char*) (S));		\
    else if ((N) == 2)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[0];	\
        ((char*) (D))[1] = ((char*) (S))[1];	\
      }						\
    else if ((N) == 4)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[0];	\
        ((char*) (D))[1] = ((char*) (S))[1];	\
        ((char*) (D))[2] = ((char*) (S))[2];	\
        ((char*) (D))[3] = ((char*) (S))[3];	\
      }						\
    else					\
      {						\
	memcpy((D), (S), (N));			\
      }						\
  } while(0)


#define FLIPMEMCPY(D,S,N)			\
  do {						\
    if ((N) == 1)				\
      *((char*) (D)) = *((char*) (S));		\
    else if ((N) == 2)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[1];	\
        ((char*) (D))[1] = ((char*) (S))[0];	\
      }						\
    else if ((N) == 4)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[3];	\
        ((char*) (D))[1] = ((char*) (S))[2];	\
        ((char*) (D))[2] = ((char*) (S))[1];	\
        ((char*) (D))[3] = ((char*) (S))[0];	\
      }						\
    else					\
      {						\
	flipmemcpy((D), (S), (N));		\
      }						\
  } while(0)


static void 
flipmemcpy(char* dst, char* src, int n)
{
  int nn = n;

  for (; n; --n)
    *dst++ = src[nn-n];
}


# if (G_BYTE_ORDER ==  G_LITTLE_ENDIAN)
#   define LEMEMCPY(D,S,N) MEMCPY(D,S,N)
#   define BEMEMCPY(D,S,N) FLIPMEMCPY(D,S,N)
# else
#   define BEMEMCPY(D,S,N) MEMCPY(D,S,N)
#   define LEMEMCPY(D,S,N) FLIPMEMCPY(D,S,N)
# endif


/* **************************************** */

#define PACK(TYPE)						\
  do {								\
    for (mult=(mult?mult:1); mult; --mult)			\
      {								\
        TYPE t;							\
        g_return_val_if_fail (n + sizeof(TYPE) <= len, FALSE);	\
        t = va_arg (args, TYPE);                                \
        MEMCPY(str, (char*) &t, sizeof(TYPE));                  \
        str += sizeof(TYPE);					\
        n += sizeof(TYPE); 	                 		\
      }								\
    mult = 0;	 						\
   } while(0)


#define PACK2(TYPENATIVE, TYPESTD, SIZESTD)				\
  do {									\
    for (mult=(mult?mult:1); mult; --mult)				\
      {									\
        if (sizemode == 0)						\
          {								\
             TYPENATIVE t;						\
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= len, FALSE);\
             t = va_arg (args, TYPENATIVE);                           	\
             MEMCPY(str, (char*) &t, sizeof(TYPENATIVE));              	\
             str += sizeof(TYPENATIVE);					\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, FALSE);	\
             t = va_arg (args, TYPESTD);                           	\
             LEMEMCPY(str, (char*) &t, sizeof(TYPESTD));               	\
             str += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, FALSE);	\
             t = va_arg (args, TYPESTD);                           	\
             BEMEMCPY(str, (char*) &t, sizeof(TYPESTD));               	\
             str += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */


/**

   pack

   The pack format string is a list of types.  Each type is
   represented by a character.  Most types can be prefixed by an
   integer, which represents how many times it is repeated (eg, "4i2b"
   is equivalent to "iiiibb". 

   x is a pad byte.  The pad byte is the NULL character.

   b/B are signed/unsigned chars

   h/H are signed/unsigned shorts

   i/I are signed/unsigned ints

   l/L are signed/unsigned longs

   f/D are floats/doubles (always native order/size)
   
   P is a void pointer (always native order/size)

   s is zero-terminated string.  REPEAT is repeat.

   S is a zero-padded string of maximum length REPEAT.  We write upto
   a NULL character or REPEAT characters, whichever comes first.  We
   then write NULL characters up to a total of REPEAT characters.
   Special case: If REPEAT is not specified, we write the string as a
   non-NULL-terminated string (note that it can't be unpacked easily
   then).

   r is a byte array of NEXT bytes.  NEXT is the next argument and is
     an integer.  REPEAT is repeat.  (r is from "raw")

   R is a byte array of REPEAT bytes.  REPEAT must be specified.

   p is a Pascal string.  The string passed is a NULL-termiated string
   of less than 256 character.  The string writen is a
   non-NULL-terminated string with a byte before the string storing
   the string length.

   Native size/order is the default.  If the first character of FORMAT
   is <, little endian order and standard size is used.  If the first
   character is > or !, big endian (or network) order and standard
   size is used.  Standard sizes are 1 byte for chars, 2 bytes for
   shorts, and 4 bytes for ints and longs.

   Mnemonics: Byte, sHort, Integer, Float, Double, Pointer, String,
   Raw

   pack was mostly inspired by Python's pack, with some awareness of
   Perl's pack.  We don't do Python 0-repeat-is-alignment.  Submit a
   patch if you really want it.

 */
gint
gnet_pack (const gchar* format, gchar* str, const guint len, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, len);
  rv = gnet_vpack (format, str, len, args);
  va_end (args);

  return rv;
}


gint
gnet_pack_strdup (const gchar* format, gchar** str, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, str);
  rv = gnet_vpack (format, *str, 0 /* FIX */, args);
  va_end (args);

  return rv;
}


/* **************************************** */

gint
gnet_vpack (const gchar* format, gchar* str, const guint len, va_list args)
{
  guint n = 0;
  gchar* p = (gchar*) format;
  gint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (str, -1);
  g_return_val_if_fail (len, -1);

  switch (*p)
    {
    case '@':			++p;	break;
    case '<':	sizemode = 1;	++p;	break;
    case '>':	
    case '!':	sizemode = 2;	++p;	break;
    }

  for (; *p; ++p)
    {
      switch (*p)
	{
	case 'x': 
	  {	
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		g_return_val_if_fail (n + 1 <= len, FALSE);
		*str++ = 0;	++n; 
	      }

	    mult = 0;
	    break; 
	  }


	case 'b':  { PACK(gint8); 			break;	}
	case 'B':  { PACK(guint8);			break;	}

	case 'h':  { PACK2(short, gint16, 2); 		break;  }
	case 'H':  { PACK2(unsigned short, guint16, 2); break;  }

	case 'i':  { PACK2(int, gint32, 4); 		break;  }
	case 'I':  { PACK2(unsigned int, guint32, 4); 	break;  }

	case 'l':  { PACK2(long, gint32, 4); 		break;  }
	case 'L':  { PACK2(unsigned long, guint32, 4); 	break;  }

	case 'f':  { PACK(float);			break;  }
	case 'd':  { PACK(double);			break;  }

	case 'P':  { PACK(void*); 			break;	}

	case 's':
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s; 
		int slen;

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		slen = strlen(s);
		g_return_val_if_fail (n + slen + 1 <= len, -1);

		memcpy (str, s, slen + 1);	/* include the 0 */
		str += slen + 1;
		n += slen + 1;
	      }

	    mult = 0; break;
	  }

	case 'S':
	  {
	    char* s;

	    s = va_arg (args, char*);
	    g_return_val_if_fail (p, -1);

	    if (!mult)
	      {
		int slen;

		slen = strlen(s);
		g_return_val_if_fail (n + slen <= len, -1);

		memcpy (str, s, slen);	/* don't include the 0 */
		str += slen;
		n += slen;
	      }
	    else
	      {
		int i;

		g_return_val_if_fail (n + mult <= len, FALSE);

		for (i = 0; i < mult && s[i]; ++i)
		  *str++ = s[i];
		for (; i < mult; ++i)
		  *str++ = 0;

		n += mult;
	      }

	    mult = 0; break;
	  }

	case 'r':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s; 
		int ln;

		s = va_arg (args, char*);
		ln = va_arg (args, int);

		g_return_val_if_fail (s, -1);
		g_return_val_if_fail (n + ln <= len, -1);

		memcpy(str, s, ln);
		str += ln;
		n += ln;
	      }

	    mult = 0; break;
	  }

	case 'R':  
	  { 
	    char* s; 

	    s = va_arg (args, char*);
	    g_return_val_if_fail (s, -1);

	    memcpy (str, s, mult);
	    n += mult;
	    mult = 0; break;
	  }

	case 'p':  
	  { 
	    char* s;
	    int slen;

	    s = va_arg (args, gchar*);
	    g_return_val_if_fail (s, -1);

	    slen = strlen(s);
	    g_return_val_if_fail (n + slen + 1 <= len, FALSE);

	    *str++ = slen;
	    memcpy (str, s, slen);   str += slen;
	    n += slen + 1;
	    mult = 0; break;
	  }

	case '0':  case '1': case '2': case '3': case '4':
	case '5':  case '6': case '7': case '8': case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');
	    break;
	  }

	case ' ': case '\t': case '\n': break;

	default: g_return_val_if_fail (FALSE, -1);
	}
    }

  return n;
}


/* **************************************** */

gint
gnet_calcsize (const gchar* format)
{
  guint n = 0;
  gchar* p = (gchar*) format;
  gint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  if (!format)
    return 0;

  switch (*p)
    {
    case '@':					++p;	break;
    case '<':	sizemode = 1;			++p;	break;
    case '>':	
    case '!':	sizemode = 2;			++p;	break;
    }

  for (; *p; ++p)
    {
      int size = 0;

      switch (*p)
	{
	case 'b':case 'B':case 'x':	{ size = 1; break; }
	case 'h':case 'H': { size = (sizemode?2:sizeof(short));	break; }
	case 'i':case 'I': { size = (sizemode?4:sizeof(int));	break; }
	case 'l':case 'L': { size = (sizemode?4:sizeof(long));	break; }
	case 'f':  { size = sizeof(float); break; }
	case 'd':  { size = sizeof(double); break;}
	case 'P':  { size = sizeof(void*); break;}

	case 's':  { size = 1; 	break; }		/* string 	*/
	case 'S':  { size = mult; mult = 1; break; }	/* zero string  */
	case 'r':  { size = 1; break; }			/* raw		*/
	case 'R':  { size = 0; break; }			/* raw w/ len	*/
	case 'p':  { size = 1; break; }			/* pascal	*/

	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');

	    break;
	  }

	case ' ':case '\t':case '\n':  break;

	default:  g_return_val_if_fail (FALSE, -1);
	}

      if (size)
	{
	  n += size * (mult?mult:1);
	  mult = 0;  
	}
    }

  return n;
}


/* **************************************** */

#define UNPACK(TYPE)						\
  do {								\
    for (mult=(mult?mult:1); mult; --mult)			\
      {								\
        TYPE* t;						\
        g_return_val_if_fail (n + sizeof(TYPE) <= len, FALSE);	\
        t = va_arg (args, TYPE*);                               \
        MEMCPY((char*) t, str, sizeof(TYPE));                  	\
        str += sizeof(TYPE);					\
        n += sizeof(TYPE); 	                 		\
      }								\
    mult = 0;	 						\
   } while(0)


#define UNPACK2(TYPENATIVE, TYPESTD, SIZESTD)				\
  do {									\
    for (mult=(mult?mult:1); mult; --mult)				\
      {									\
        if (sizemode == 0)						\
          {								\
             TYPENATIVE* t;						\
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= len, FALSE);\
             t = va_arg (args, TYPENATIVE*);                           	\
             MEMCPY((char*) t, str, sizeof(TYPENATIVE));              	\
             str += sizeof(TYPENATIVE);					\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, FALSE);	\
             t = va_arg (args, TYPESTD*);                           	\
             LEMEMCPY((char*) t, str, sizeof(TYPESTD));               	\
             str += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, FALSE);	\
             t = va_arg (args, TYPESTD*);                           	\
             BEMEMCPY((char*) t, str, sizeof(TYPESTD));               	\
             str += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */

/**

   unpack

   x is a pad byte.  The byte is skipped and not stored.  We do not
     check its value.

   b/B are signed/unsigned chars

   h/H are signed/unsigned shorts (h is from sHort)

   i/I are signed/unsigned ints

   l/L are signed/unsigned longs

   f/D are floats/doubles (always native order/size)
   
   P is a void pointer (always native order/size)

   s is zero-terminated string of maximum length REPEAT.  REPEAT is
     optional.  If REPEAT is less than the string length, the string
     is truncated and NULL is added.

   S is a zero-padded string of length REPEAT.  We read REPEAT
   characters or until a NULL character.  Any remaining characters are
   filled in with 0's.  REPEAT must be specified.

   r is a byte array of NEXT bytes.  NEXT is the next argument and is
     an integer.  REPEAT is repeat.  (r is from "raw")

   R is a byte array of REPEAT bytes.  REPEAT must be specified.

   String/byte array memory is allocated by unpack.  It is the
   caller's responsibility to unallocate it.

   Native size/order is the default.  If the first character of FORMAT
     is <, little endian order and standard size is used.  If the
     first character is > or !, big endian (or network) order and
     standard size is used.  Standard sizes are 1 byte for chars, 2
     bytes for shorts, and 4 bytes for ints and longs.

   Mnemonics: sHort, Integer, Float, Double, Pointer, String, Raw

   unpack was mostly inspired by Python's unpack, with some awareness
     of Perl's unpack.

 */
gint 
gnet_unpack (const gchar* format, gchar* str, gint len, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, len);
  rv = gnet_vunpack (format, str, len, args);
  va_end (args);

  return rv;
}


gint 
gnet_vunpack (const gchar* format, gchar* str, gint len, va_list args)
{
  guint n = 0;
  gchar* p = (gchar*) format;
  gint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (str, -1);

  switch (*p)
    {
    case '@':					++p;	break;
    case '<':	sizemode = 1;			++p;	break;
    case '>':	
    case '!':	sizemode = 2;			++p;	break;
    }

  for (; *p; ++p)
    {
      switch (*p)
	{
	case 'x': 
	  {	
	    mult = mult? mult:1;
	    g_return_val_if_fail (n + mult <= len, FALSE);

	    str += mult;
	    n += mult;

	    mult = 0;
	    break; 
	  }

	case 'b':  { UNPACK(gint8); 			break;	}
	case 'B':  { UNPACK(guint8);			break;	}

	case 'h':  { UNPACK2(short, gint16, 2); 	break;  }
	case 'H':  { UNPACK2(unsigned short, guint16, 2);break;  }

	case 'i':  { UNPACK2(int, gint32, 4); 		break;  }
	case 'I':  { UNPACK2(unsigned int, guint32, 4); break;  }

	case 'l':  { UNPACK2(long, gint32, 4); 		break;  }
	case 'L':  { UNPACK2(unsigned long, guint32, 4);break;  }

	case 'f':  { UNPACK(float);			break;  }
	case 'd':  { UNPACK(double);			break;  }

	case 'P':  { UNPACK(void*); 			break;	}

	case 's':
	  { 
	    char** sp; 

	    sp = va_arg (args, char**);
	    g_return_val_if_fail (sp, -1);

	    if (!mult)
	      {
		int slen = strlenn(str, len - n);
		g_return_val_if_fail (n + slen <= len, FALSE);

		*sp = g_new(gchar, slen + 1);
		memcpy (*sp, str, slen);
		sp[slen] = 0; /* need - might not be in str */
		str += slen + 1;
		n += slen + 1;
	      }
	    else
	      {
		g_return_val_if_fail (n + mult <= len, FALSE);

		*sp = g_new(gchar, mult + 1);
		memcpy (*sp, str, mult);
		sp[mult] = 0; /* need - might not be in str */
		str += mult + 1;
		n += mult + 1;
	      }

	    mult = 0; break;
	  }

	case 'S':
	  { 
	    char** sp; 
	    int read;

	    sp = va_arg (args, char**);
	    g_return_val_if_fail (sp, -1);

	    g_return_val_if_fail (mult, -1);

	    read = MIN(mult, strlenn(str, len - n));
	    g_return_val_if_fail (n + read <= len, -1);

	    *sp = g_new(gchar, read);
	    memcpy (*sp, str, read);
	    while (read++ < mult) **sp = 0;
	    str += mult;
	    n += mult;

	    mult = 0; break;
	  }


	case 'r':  /* r is the same as s, in this case. */
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char** sp; 
		int ln;

		sp = va_arg (args, char**);
		ln = va_arg (args, int);

		g_return_val_if_fail (sp, -1);
		g_return_val_if_fail (n + ln <= len, FALSE);

		*sp = g_new(char, ln);
		memcpy(*sp, str, ln);
		str += ln;
		n += ln;
	      }

	    mult = 0; break;
	  }

	case 'R':  
	  { 
	    char** sp; 

	    sp = va_arg (args, char**);
	    g_return_val_if_fail (sp, -1);

	    g_return_val_if_fail (mult, -1);
	    g_return_val_if_fail (n + mult <= len, -1);

	    *sp = g_new(char, mult);
	    memcpy(*sp, str, mult);
	    str += mult;
	    n += mult;
	    mult = 0; break;
	  }

	case 'p':  
	  { 
	    char** sp;
	    int slen;

	    sp = va_arg (args, char**);
	    g_return_val_if_fail (sp, -1);
	    g_return_val_if_fail (n + 1 <= len, FALSE);

	    slen = *str++; ++n;
	    g_return_val_if_fail (n + slen <= len, FALSE);

	    *sp = g_new(char, slen + 1);
	    memcpy (*sp, str, slen); 
	    (*sp)[slen] = 0;
	    str += slen;
	    n += slen;
	    mult = 0; break;
	  }

	case '0':  case '1': case '2': case '3': case '4':
	case '5':  case '6': case '7': case '8': case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');
	    break;
	  }

	case ' ': case '\t': case '\n': break;

	default: g_return_val_if_fail (FALSE, -1);
	}
    }

  return n;
}

