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

  while (*str++ && len < n) ++len;

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

/* PACK does MEMCPY regardless of endian */
#define PACK(TYPE, VTYPE)					\
  do {								\
    for (mult=(mult?mult:1); mult; --mult)			\
      {								\
        TYPE t;							\
        g_return_val_if_fail (n + sizeof(TYPE) <= len, -1);	\
        t = (TYPE) va_arg (args, VTYPE);                        \
        MEMCPY(buffer, (char*) &t, sizeof(TYPE));               \
        buffer += sizeof(TYPE);					\
        n += sizeof(TYPE); 	                 		\
      }								\
    mult = 0;	 						\
   } while(0)


/* PACK2 does memcpy based on endian */
#define PACK2(TYPENATIVE, VTYPE, TYPESTD, SIZESTD)			\
  do {									\
    for (mult=(mult?mult:1); mult; --mult)				\
      {									\
        if (sizemode == 0)						\
          {								\
             TYPENATIVE t;						\
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= len, -1);	\
             t = (TYPENATIVE) va_arg (args, VTYPE);                    	\
             MEMCPY(buffer, (char*) &t, sizeof(TYPENATIVE));            \
             buffer += sizeof(TYPENATIVE);				\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, -1);	\
             t = (TYPESTD) va_arg (args, VTYPE);               		\
             LEMEMCPY(buffer, (char*) &t, sizeof(TYPESTD));             \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, -1);	\
             t = (TYPESTD) va_arg (args, VTYPE);                       	\
             BEMEMCPY(buffer, (char*) &t, sizeof(TYPESTD));             \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */


/**
 *  gnet_pack:
 *  @format: Pack format (see below)
 *  @buffer: Buffer to pack to
 *  @len: Length of buffer
 *
 *  The pack format string is a list of types.  Each type is
 *  represented by a character.  Most types can be prefixed by an
 *  integer, which represents how many times it is repeated (eg,
 *  "4i2b" is equivalent to "iiiibb".
 *
 *  Native size/order is the default.  If the first character of
 *  FORMAT is < then little endian order and standard size are used.
 *  If the first character is > or !, then big endian (or network)
 *  order and standard size are used.  Standard sizes are 1 byte for
 *  chars, 2 bytes for shorts, and 4 bytes for ints and longs.
 *  x is a pad byte.  The pad byte is the NULL character.
 *
 *  b/B are signed/unsigned chars
 *
 *  h/H are signed/unsigned shorts
 *
 *  i/I are signed/unsigned ints
 *
 *  l/L are signed/unsigned longs
 *
 *  f/D are floats/doubles (always native order/size)
 *  
 *  v is a void pointer (always native size)
 *
 *  s is a zero-terminated string.  REPEAT is repeat.
 *
 *  S is a zero-padded string of maximum length REPEAT.  We write
 *  up-to a NULL character or REPEAT characters, whichever comes
 *  first.  We then write NULL characters up to a total of REPEAT
 *  characters.  Special case: If REPEAT is not specified, we write
 *  the string as a non-NULL-terminated string (note that it can't be
 *  unpacked easily then).
 *
 *  r is a byte array of NEXT bytes.  NEXT is the next argument and is
 *  an integer.  REPEAT is repeat.  (r is from "raw")
 *
 *  R is a byte array of REPEAT bytes.  REPEAT must be specified.
 *
 *  p is a Pascal string.  The string passed is a NULL-termiated
 *  string of less than 256 character.  The string writen is a
 *  non-NULL-terminated string with a byte before the string storing
 *  the string length.  REPEAT is repeat.
 *
 *  Mnemonics: (B)yte, s(H)ort, (I)nteger, (F)loat, (D)ouble, (V)oid
 *  pointer, (S)tring, (R)aw
 *
 *  pack was mostly inspired by Python's pack, with some awareness of
 *  Perl's pack.  We don't do Python 0-repeat-is-alignment.  Submit a
 *  patch if you really want it.
 *
 *  Returns: bytes packed; -1 if error.
 *
 **/
gint
gnet_pack (const gchar* format, gchar* buffer, const guint len, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, len);
  rv = gnet_vpack (format, buffer, len, args);
  va_end (args);

  return rv;
}


gint
gnet_pack_strdup (const gchar* format, gchar** buffer, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, buffer);
  rv = gnet_vpack (format, *buffer, 0 /* FIX */, args);
  va_end (args);

  return rv;
}


/* **************************************** */

gint
gnet_vpack (const gchar* format, gchar* buffer, const guint len, va_list args)
{
  guint n = 0;
  gchar* p = (gchar*) format;
  gint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (buffer, -1);
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
		g_return_val_if_fail (n + 1 <= len, -1);
		*buffer++ = 0;	++n; 
	      }

	    mult = 0;
	    break; 
	  }


	case 'b':  { PACK(gint8, int); 				break;	}
	case 'B':  { PACK(guint8, unsigned int);		break;	}

	case 'h':  { PACK2(short, int, gint16, 2); 		break;  }
	case 'H':  { PACK2(unsigned short, unsigned int, guint16, 2); break;  }

	case 'i':  { PACK2(int, int, gint32, 4); 		break;  }
	case 'I':  { PACK2(unsigned int, unsigned int, guint32, 4); break;  }

	case 'l':  { PACK2(long, int, gint32, 4); 		break;  }
	case 'L':  { PACK2(unsigned long, unsigned int, guint32, 4); break;  }

	case 'f':  { PACK(float, double);			break;  }
	case 'd':  { PACK(double, double);			break;  }

	case 'v':  { PACK2(void*, void*, void*, 4); 		break;	}

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

		memcpy (buffer, s, slen + 1);	/* include the 0 */
		buffer += slen + 1;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
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

		memcpy (buffer, s, slen);	/* don't include the 0 */
		buffer += slen;
		n += slen;
	      }
	    else
	      {
		int i;

		g_return_val_if_fail (n + mult <= len, -1);

		for (i = 0; i < mult && s[i]; ++i)
		  *buffer++ = s[i];
		for (; i < mult; ++i)
		  *buffer++ = 0;

		n += mult;
	      }

	    mult = 0; 
	    break;
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

		memcpy(buffer, s, ln);
		buffer += ln;
		n += ln;
	      }

	    mult = 0; 
	    break;
	  }

	case 'R':  
	  { 
	    char* s; 

	    s = va_arg (args, char*);
	    g_return_val_if_fail (s, -1);

	    g_return_val_if_fail (mult, -1);
	    g_return_val_if_fail (n + mult <= len, -1);

	    memcpy (buffer, s, mult);
	    buffer += mult;
	    n += mult;
	    mult = 0; 
	    break;
	  }

	case 'p':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s;
		int slen;

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		slen = strlen(s);
		g_return_val_if_fail (n < 256, -1);
		g_return_val_if_fail (n + slen + 1 <= len, -1);

		*buffer++ = slen;
		memcpy (buffer, s, slen);   
		buffer += slen;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
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
	case 'v':  { size = sizeof(void*); break;}

	  /* string: size = 1 * mult */
	case 's':  { size = 1; break; }

	  /* null-padded string: size = mult ? mult : 0 */
	case 'S':  { size = mult; mult = 1; break; }

	  /* raw: size = 0 */
	case 'r':  { size = 0; break; }
	case 'R':  { size = 0; break; }

	  /* pascal: size = 1 * mult */
	case 'p':  { size = 1; break; }

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
        MEMCPY((char*) t, buffer, sizeof(TYPE));                \
        buffer += sizeof(TYPE);					\
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
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= len, -1);	\
             t = va_arg (args, TYPENATIVE*);                           	\
             MEMCPY((char*) t, buffer, sizeof(TYPENATIVE));             \
             buffer += sizeof(TYPENATIVE);				\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, -1);	\
             t = va_arg (args, TYPESTD*);                           	\
             LEMEMCPY((char*) t, buffer, sizeof(TYPESTD));              \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= len, -1);	\
             t = va_arg (args, TYPESTD*);                           	\
             BEMEMCPY((char*) t, buffer, sizeof(TYPESTD));              \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */

/**
 *  gnet_unpack:
 *  @format: Unpack format (see below)
 *  @buffer: Buffer to unpack from
 *  @len: Length of buffer
 *
 *  The unpack format string is a list of types.  Each type is
 *  represented by a character.  Most types can be prefixed by an
 *  integer, which represents how many times it is repeated (eg,
 *  "4i2b" is equivalent to "iiiibb".
 *
 *  In unpack, the arguments must be pointers to the appropriate type.
 *  Strings and byte arrays are allocated dynamicly (by g_new).  The
 *  caller is responsible for g_free()-ing it.
 *
 *  Native size/order is the default.  If the first character of
 *  FORMAT is < then little endian order and standard size are used.
 *  If the first character is > or !, then big endian (or network)
 *  order and standard size are used.  Standard sizes are 1 byte for
 *  chars, 2 bytes for shorts, and 4 bytes for ints and longs.
 * 
 *  x is a pad byte.  The byte is skipped and not stored.  We do not
 *  check its value.
 *
 *  b/B are signed/unsigned chars
 *
 *  h/H are signed/unsigned shorts (h is from sHort)
 * 
 *  i/I are signed/unsigned ints
 * 
 *  l/L are signed/unsigned longs
 * 
 *  f/D are floats/doubles (always native order/size)
 *    
 *  v is a void pointer (always native size)
 * 
 *  s is a zero-terminated string.  REPEAT is repeat.
 * 
 *  S is a zero-padded string of length REPEAT.  We read REPEAT
 *  characters or until a NULL character.  Any remaining characters are
 *  filled in with 0's.  REPEAT must be specified.
 * 
 *  r is a byte array of NEXT bytes.  NEXT is the next argument and is
 *  an integer.  REPEAT is repeat.  (r is from "raw")
 * 
 *  R is a byte array of REPEAT bytes.  REPEAT must be specified.
 * 
 *  String/byte array memory is allocated by unpack.
 * 
 *  Mnemonics: sHort, Integer, Float, Double, Pointer, String, Raw
 * 
 *  unpack was mostly inspired by Python's unpack, with some awareness
 *  of Perl's unpack.
 * 
 *  Returns: bytes unpacked; -1 if error.
 * */
gint 
gnet_unpack (const gchar* format, gchar* buffer, gint len, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, len);
  rv = gnet_vunpack (format, buffer, len, args);
  va_end (args);

  return rv;
}


gint 
gnet_vunpack (const gchar* format, gchar* buffer, gint len, va_list args)
{
  guint n = 0;
  gchar* p = (gchar*) format;
  gint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (buffer, -1);

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

	    buffer += mult;
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

	case 'v':  { UNPACK2(void*, void*, 4); 		break;	}

	case 's':
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char** sp; 
		int slen;

		sp = va_arg (args, char**);
		g_return_val_if_fail (sp, -1);

		slen = strlenn(buffer, len - n);
		g_return_val_if_fail (n + slen <= len, FALSE);

		*sp = g_new(gchar, slen + 1);
		memcpy (*sp, buffer, slen);
		(*sp)[slen] = 0;
		buffer += slen + 1;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
	  }

	case 'S':
	  { 
	    char** sp; 
	    int slen;

	    g_return_val_if_fail (mult, -1);

	    sp = va_arg (args, char**);
	    g_return_val_if_fail (sp, -1);

	    slen = MIN(mult, strlenn(buffer, len - n));
	    g_return_val_if_fail (n + slen <= len, -1);

	    *sp = g_new(gchar, mult + 1);
	    memcpy (*sp, buffer, slen);
	    while (slen < mult + 1) sp[slen++] = 0;
	    buffer += mult;
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
		memcpy(*sp, buffer, ln);
		buffer += ln;
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
	    memcpy(*sp, buffer, mult);
	    buffer += mult;
	    n += mult;
	    mult = 0; 
	    break;
	  }

	case 'p':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char** sp;
		int slen;

		sp = va_arg (args, char**);
		g_return_val_if_fail (sp, -1);
		g_return_val_if_fail (n + 1 <= len, FALSE);

		slen = *buffer++; 
		++n;
		g_return_val_if_fail (n + slen <= len, FALSE);

		*sp = g_new(char, slen + 1);
		memcpy (*sp, buffer, slen); 
		(*sp)[slen] = 0;
		buffer += slen;
		n += slen;
	      }

	    mult = 0;
	    break;
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

