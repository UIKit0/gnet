/*

  GNet API added by David Helder <dhelder@umich.edu> 2000-6-11.  All
  additions and changes placed in the public domain.

  Files originally from: http://www.gxsnmp.org/CVS/gxsnmp/

 */
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#include "md5.h"
#include <glib.h>
#include <string.h>


/* ************************************************************ */
/* Code below is from Colin Plumb implementation 		*/



struct MD5Context {
	guint32 buf[4];
	guint32 bits[2];
	guchar  in[64];
	int     doByteReverse;
};

static void MD5Init(struct MD5Context *context);
static void MD5Update(struct MD5Context *context, guchar const *buf,
		      guint len);
static void MD5Final(guchar digest[16], struct MD5Context *context);
static void MD5Transform(guint32 buf[4], guint32 const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;




static void byteReverse(guint8 *buf, guint longs);

/*
 * Note: this code is harmless on little-endian machines.
 */
void 
byteReverse(guint8 *buf, guint longs)
{
  guint32 t;
  do 
    {
      t = (guint32) ((guint) buf[3] << 8 | buf[2]) << 16 |
          ((guint) buf[1] << 8 | buf[0]);
      *(guint32 *) buf = t;
      buf += 4;
    } 
  while (--longs);
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void 
MD5Init(struct MD5Context *ctx)
{
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  ctx->doByteReverse = 1;
#else
  ctx->doByteReverse = 0;
#endif
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void 
MD5Update(struct MD5Context *ctx, guint8 const *buf, guint len)
{
  guint32 t;

  /* Update bitcount */

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((guint32) len << 3)) < t)
    ctx->bits[1]++;		/* Carry from low to high */
  ctx->bits[1] += len >> 29;

  t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

  /* Handle any leading odd-sized chunks */

  if (t) 
    {
      guint8 *p = (guint8 *) ctx->in + t;

      t = 64 - t;
      if (len < t) 
        {
          g_memmove(p, buf, len);
          return;
	}
      g_memmove(p, buf, t);
      if (ctx->doByteReverse)
        byteReverse(ctx->in, 16);
      MD5Transform(ctx->buf, (guint32 *) ctx->in);
      buf += t;
      len -= t;
    }
  /* Process data in 64-byte chunks */

  while (len >= 64) 
    {
      g_memmove(ctx->in, buf, 64);
      if (ctx->doByteReverse)
        byteReverse(ctx->in, 16);
      MD5Transform(ctx->buf, (guint32 *) ctx->in);
      buf += 64;
      len -= 64;
    }

  /* Handle any remaining bytes of data. */

  g_memmove(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void 
MD5Final(guint8 digest[16], struct MD5Context *ctx)
{
  guint count;
  guint8 *p;

  /* Compute number of bytes mod 64 */
  count = (ctx->bits[0] >> 3) & 0x3F;

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  p = ctx->in + count;
  *p++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = 64 - 1 - count;

  /* Pad out to 56 mod 64 */
  if (count < 8) 
    {
	/* Two lots of padding:  Pad the first block to 64 bytes */
      memset(p, 0, count);
      if (ctx->doByteReverse)
        byteReverse(ctx->in, 16);
      MD5Transform(ctx->buf, (guint32 *) ctx->in);

      /* Now fill the next block with 56 bytes */
      memset(ctx->in, 0, 56);
    } 
  else 
    {
      /* Pad block to 56 bytes */
      memset(p, 0, count - 8);
    }
  if (ctx->doByteReverse)
    byteReverse(ctx->in, 14);

  /* Append length in bits and transform */
  ((guint32 *) ctx->in)[14] = ctx->bits[0];
  ((guint32 *) ctx->in)[15] = ctx->bits[1];

  MD5Transform(ctx->buf, (guint32 *) ctx->in);
  if (ctx->doByteReverse)
    byteReverse((guint8 *) ctx->buf, 4);
  g_memmove(digest, ctx->buf, 16);
  memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void 
MD5Transform(guint32 buf[4], guint32 const in[16])
{
  register guint32 a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}



/* ************************************************************ */
/* Code below is David Helder's API for GNet			*/

struct _GMD5
{
  struct MD5Context 	ctx;
  gchar 		digest[GNET_MD5_HASH_LENGTH];
};


/**
 *  gnet_md5_new:
 *  @buffer: buffer to hash
 *  @length: length of @buffer
 * 
 *  Create an MD5 hash of @buffer.
 *
 *  Returns: a new #GMD5.
 *
 **/
GMD5*           
gnet_md5_new (const gchar* buffer, guint length)
{
  GMD5* gmd5;

  gmd5 = g_new0 (GMD5, 1);
  MD5Init (&gmd5->ctx);
  MD5Update (&gmd5->ctx, buffer, length);
  MD5Final ((gpointer) &gmd5->digest, &gmd5->ctx);

  return gmd5;
}



/**
 *  gnet_md5_new_string:
 *  @str: hexidecimal string
 * 
 *  Create an MD5 hash from a hexidecimal string.
 *
 *  Returns: a new #GMD5.
 *
 **/
GMD5*		
gnet_md5_new_string (const gchar* str)
{
  GMD5* gmd5;
  guint i;

  g_return_val_if_fail (str, NULL);
  g_return_val_if_fail (strlen(str) >= (GNET_MD5_HASH_LENGTH * 2), NULL);

  gmd5 = g_new0 (GMD5, 1);

  for (i = 0; i < (GNET_MD5_HASH_LENGTH * 2); ++i)
    {
      guint val = 0;

      switch (str[i])
	{
	case '0':	val = 0;	break;
	case '1':	val = 1;	break;
	case '2':	val = 2;	break;
	case '3':	val = 3;	break;
	case '4':	val = 4;	break;
	case '5':	val = 5;	break;
	case '6':	val = 6;	break;
	case '7':	val = 7;	break;
	case '8':	val = 8;	break;
	case '9':	val = 9;	break;
	case 'A':
	case 'a':	val = 10;	break;
	case 'B':
	case 'b':	val = 11;	break;
	case 'C':
	case 'c':	val = 12;	break;
	case 'D':
	case 'd':	val = 13;	break;
	case 'E':
	case 'e':	val = 14;	break;
	case 'F':
	case 'f':	val = 15;	break;
	default:
	  g_return_val_if_fail (FALSE, NULL);
	}

      if (i % 2)
	gmd5->digest[i / 2] |= val;
      else
	gmd5->digest[i / 2] = val << 4;
    }

  return gmd5;
}



/**
 *  gnet_md5_clone
 *  @gmd5: a #GMD5
 * 
 *  Create an MD5 by copying a #GMD5.
 *
 *  Returns: a new #GMD5.
 *
 **/
GMD5*           
gnet_md5_clone (const GMD5* gmd5)
{
  GMD5* gmd52;

  g_return_val_if_fail (gmd5, NULL);

  gmd52      = g_new0 (GMD5, 1);
  gmd52->ctx = gmd5->ctx;
  memcpy (gmd52->digest, gmd5->digest, sizeof(gmd5->digest));

  return gmd52;
}



/** 
 *  gnet_md5_delete
 *  @gmd5: a #GMD5
 *
 *  Delete a #GMD5.
 *
 **/
void
gnet_md5_delete (GMD5* gmd5)
{
  if (gmd5)
    g_free (gmd5);
}



/**
 *  gnet_md5_new_incremental
 *
 *  Create an MD5 hash incrementally.  After creating the #GMD5, call
 *  gnet_md5_update() one or more times to hash more data.  Finally,
 *  call gnet_md5_final() to compute the final hash value.
 *
 *  Returns: a new #GMD5
 *
 **/
GMD5*		
gnet_md5_new_incremental (void)
{
  GMD5* gmd5;

  gmd5 = g_new0 (GMD5, 1);
  MD5Init (&gmd5->ctx);
  return gmd5;
}


/**
 *  gnet_md5_update
 *  @gmd5: a #GMD5
 *  @buffer: buffer to add
 *  @length: length of @buffer
 *
 *  Update the hash with @buffer.  This may be called several times on
 *  a hash created by gnet_md5_new_incremental() before being
 *  finalized by calling gnet_md5_final().
 * 
 **/
void
gnet_md5_update (GMD5* gmd5, const gchar* buffer, guint length)
{
  g_return_if_fail (gmd5);

  MD5Update (&gmd5->ctx, buffer, length);
}


/**
 *  gnet_md5_final
 *  @gmd5: a #GMD5
 *
 *  Calcuate the final hash value of @gmd5.  This is called on an
 *  #GMD5 created by gnet_md5_new_incremental() and updated by one or
 *  more calls to gnet_md5_update().
 *
 **/
void
gnet_md5_final (GMD5* gmd5)
{
  g_return_if_fail (gmd5);

  MD5Final ((gpointer) &gmd5->digest, &gmd5->ctx);
}


/* **************************************** */

/**
 *  gnet_md5_equal
 *  @p1: first #GMD5.
 *  @p2: second #GMD5.
 *
 *  Compare two #GMD5's.  
 *
 *  Returns: 1 if they are the same; 0 otherwise.
 *
 **/
gint
gnet_md5_equal (gconstpointer p1, gconstpointer p2)
{
  GMD5* gmd5a = (GMD5*) p1;
  GMD5* gmd5b = (GMD5*) p2;
  guint i;

  for (i = 0; i < GNET_MD5_HASH_LENGTH; ++i)
    if (gmd5a->digest[i] != gmd5b->digest[i])
      return 0;

  return 1;
}


/**
 *  gnet_md5_hash
 *  @p: a #GMD5
 *
 *  Hash the GMD5 hash value.  This is not the actual MD5 hash, but a
 *  hash of this hash.  This hash can be used with the GLib
 *  GHashTable.
 *
 *  Returns: the hash value.
 *
 **/
guint
gnet_md5_hash (gconstpointer p)
{
  const GMD5* gmd5 = (const GMD5*) p;
  const guint* q;

  g_return_val_if_fail (gmd5, 0);

  q = (const guint*) gmd5->digest;

  return (q[0] ^ q[1] ^ q[2] ^ q[3]);
}


/**
 *  gnet_md5_get_digest
 *  @gmd5: a #GMD5
 *
 *  Get the raw MD5 hash digest.
 *
 *  Returns: callee-owned buffer containing the MD5 hash digest.  The
 *  buffer is %GNET_MD5_HASH_LENGTH bytes long.
 *
 **/
gchar*        	
gnet_md5_get_digest (const GMD5* gmd5)
{
  g_return_val_if_fail (gmd5, NULL);
  
  return (gchar*) gmd5->digest;
}


static gchar bits2hex[16] = { '0', '1', '2', '3', 
			      '4', '5', '6', '7',
			      '8', '9', 'a', 'b',
			      'c', 'd', 'e', 'f' };

/**
 *  gnet_md5_get_string
 *  @gmd5: a #GMD5
 *
 *  Get the digest represented a string.
 *
 *  Returns: a hexadecimal string representing the hash.  The string
 *  is 2 * %GNET_MD5_HASH_LENGTH bytes long and NULL terminated.  The
 *  string is caller owned.
 *
 **/
gchar*          
gnet_md5_get_string (const GMD5* gmd5)
{
  gchar* str;
  guint i;

  g_return_val_if_fail (gmd5, NULL);

  str = g_new (gchar, GNET_MD5_HASH_LENGTH * 2 + 1);
  str[GNET_MD5_HASH_LENGTH * 2] = '\0';

  for (i = 0; i < GNET_MD5_HASH_LENGTH; ++i)
    {
      str[i * 2]       = bits2hex[(gmd5->digest[i] & 0xF0) >> 4];
      str[(i * 2) + 1] = bits2hex[(gmd5->digest[i] & 0x0F)     ];
    }

  return str;
}



/**
 * gnet_md5_copy_string
 * @gmd5: a #GMD5
 * @buffer: buffer at least 2 * %GNET_MD5_HASH_LENGTH bytes long
 *
 * Copy the digest represented as a string into @buffer.  The string
 * is not NULL terminated.
 * 
 **/
void
gnet_md5_copy_string (const GMD5* gmd5, gchar* buffer)
{
  guint i;

  g_return_if_fail (gmd5);
  g_return_if_fail (buffer);

  for (i = 0; i < GNET_MD5_HASH_LENGTH; ++i)
    {
      buffer[i * 2]       = bits2hex[(gmd5->digest[i] & 0xF0) >> 4];
      buffer[(i * 2) + 1] = bits2hex[(gmd5->digest[i] & 0x0F)     ];
    }
}
