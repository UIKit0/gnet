/*

  GNet API added by David Helder <dhelder@umich.edu> 2000-6-11.  All
  additions and changes placed in the public domain.

  Files originally from: http://www.gxsnmp.org/CVS/gxsnmp/

 */
/*
 *  sha.h : Implementation of the Secure Hash Algorithm
 *
 * Part of the Python Cryptography Toolkit, version 1.0.0
 *
 * Copyright (C) 1995, A.M. Kuchling
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */
  
/* SHA: NIST's Secure Hash Algorithm */

/* Based on SHA code originally posted to sci.crypt by Peter Gutmann
   in message <30ajo5$oe8@ccu2.auckland.ac.nz>.
   Modified to test for endianness on creation of SHA objects by AMK.
   Also, the original specification of SHA was found to have a weakness
   by NSA/NIST.  This code implements the fixed version of SHA.
*/

/* Here's the first paragraph of Peter Gutmann's posting:
   
The following is my SHA (FIPS 180) code updated to allow use of the "fixed"
SHA, thanks to Jim Gillogly and an anonymous contributor for the information on
what's changed in the new version.  The fix is a simple change which involves
adding a single rotate in the initial expansion function.  It is unknown
whether this is an optimal solution to the problem which was discovered in the
SHA or whether it's simply a bandaid which fixes the problem with a minimum of
effort (for example the reengineering of a great many Capstone chips).
*/

#include <glib.h>
#include "sha.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/* The SHA block size and message digest sizes, in bytes */

#define SHA_DATASIZE    64
#define SHA_DIGESTSIZE  20

/* The structure for storing SHA info */

typedef struct {
               guint32  digest[ 5 ];         /* Message digest */
               guint32  countLo, countHi;    /* 64-bit bit count */
               guint32  data[ 16 ];          /* SHA data buffer */
	       int      Endianness;
               } SHA_CTX;

/* Message digest functions */

static void SHAInit(SHA_CTX* shaInfo);
static void SHAUpdate(SHA_CTX* shaInfo, guint8 const* buffer, guint count);
static void SHAFinal(char *key, SHA_CTX *shaInfo);
static void SHATransform(guint32 *digest, guint32 *data );

/*
 *  sha.c : Implementation of the Secure Hash Algorithm
 *
 * Part of the Python Cryptography Toolkit, version 1.0.0
 *
 * Copyright (C) 1995, A.M. Kuchling
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */
  
/* SHA: NIST's Secure Hash Algorithm */

/* Based on SHA code originally posted to sci.crypt by Peter Gutmann
   in message <30ajo5$oe8@ccu2.auckland.ac.nz>.
   Modified to test for endianness on creation of SHA objects by AMK.
   Also, the original specification of SHA was found to have a weakness
   by NSA/NIST.  This code implements the fixed version of SHA.
*/

/* Here's the first paragraph of Peter Gutmann's posting:
   
The following is my SHA (FIPS 180) code updated to allow use of the "fixed"
SHA, thanks to Jim Gillogly and an anonymous contributor for the information on
what's changed in the new version.  The fix is a simple change which involves
adding a single rotate in the initial expansion function.  It is unknown
whether this is an optimal solution to the problem which was discovered in the
SHA or whether it's simply a bandaid which fixes the problem with a minimum of
effort (for example the reengineering of a great many Capstone chips).
*/


#include <string.h>

/* The SHA f()-functions.  The f1 and f3 functions can be optimized to
   save one boolean operation each - thanks to Rich Schroeppel,
   rcs@cs.arizona.edu for discovering this */

/*#define f1(x,y,z) ( ( x & y ) | ( ~x & z ) )          // Rounds  0-19 */
#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
/*#define f3(x,y,z) ( ( x & y ) | ( x & z ) | ( y & z ) )   // Rounds 40-59 */
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */

/* The SHA Mysterious Constants */

#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

/* SHA initial values */

#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L

/* Note that it may be necessary to add parentheses to these macros if they
   are to be called with expressions as arguments */
/* 32-bit rotate left - kludged with shifts */

#define ROTL(n,X)  ( ( ( X ) << n ) | ( ( X ) >> ( 32 - n ) ) )

/* The initial expanding function.  The hash function is defined over an
   80-word expanded input array W, where the first 16 are copies of the input
   data, and the remaining 64 are defined by

        W[ i ] = W[ i - 16 ] ^ W[ i - 14 ] ^ W[ i - 8 ] ^ W[ i - 3 ]

   This implementation generates these values on the fly in a circular
   buffer - thanks to Colin Plumb, colin@nyx10.cs.du.edu for this
   optimization.

   The updated SHA changes the expanding function by adding a rotate of 1
   bit.  Thanks to Jim Gillogly, jim@rand.org, and an anonymous contributor
   for this information */

#define expand(W,i) ( W[ i & 15 ] = ROTL( 1, ( W[ i & 15 ] ^ W[ (i - 14) & 15 ] ^ \
                                                 W[ (i - 8) & 15 ] ^ W[ (i - 3) & 15 ] ) ) )


/* The prototype SHA sub-round.  The fundamental sub-round is:

        a' = e + ROTL( 5, a ) + f( b, c, d ) + k + data;
        b' = a;
        c' = ROTL( 30, b );
        d' = c;
        e' = d;

   but this is implemented by unrolling the loop 5 times and renaming the
   variables ( e, a, b, c, d ) = ( a', b', c', d', e' ) each iteration.
   This code is then replicated 20 times for each of the 4 functions, using
   the next 20 values from the W[] array each time */

#define subRound(a, b, c, d, e, f, k, data) \
    ( e += ROTL( 5, a ) + f( b, c, d ) + k + data, b = ROTL( 30, b ) )

/* Initialize the SHA values */

void
SHAInit(SHA_CTX * shaInfo )
{
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  shaInfo->Endianness = 1;
#else
  shaInfo->Endianness = 0;
#endif

  /* Set the h-vars to their initial values */
  shaInfo->digest[ 0 ] = h0init;
  shaInfo->digest[ 1 ] = h1init;
  shaInfo->digest[ 2 ] = h2init;
  shaInfo->digest[ 3 ] = h3init;
  shaInfo->digest[ 4 ] = h4init;

  /* Initialise bit count */
  shaInfo->countLo = shaInfo->countHi = 0;
}


/* Perform the SHA transformation.  Note that this code, like MD5, seems to
   break some optimizing compilers due to the complexity of the expressions
   and the size of the basic block.  It may be necessary to split it into
   sections, e.g. based on the four subrounds

   Note that this corrupts the shaInfo->data area */

void 
SHATransform(guint32 *digest, guint32 *data )
{
  guint32 A, B, C, D, E;     /* Local vars */
  guint32 eData[ 16 ];       /* Expanded data */

  /* Set up first buffer and local data buffer */
  A = digest[ 0 ];
  B = digest[ 1 ];
  C = digest[ 2 ];
  D = digest[ 3 ];
  E = digest[ 4 ];
  g_memmove( eData, data, SHA_DATASIZE );

  /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
  subRound( A, B, C, D, E, f1, K1, eData[  0 ] );
  subRound( E, A, B, C, D, f1, K1, eData[  1 ] );
  subRound( D, E, A, B, C, f1, K1, eData[  2 ] );
  subRound( C, D, E, A, B, f1, K1, eData[  3 ] );
  subRound( B, C, D, E, A, f1, K1, eData[  4 ] );
  subRound( A, B, C, D, E, f1, K1, eData[  5 ] );
  subRound( E, A, B, C, D, f1, K1, eData[  6 ] );
  subRound( D, E, A, B, C, f1, K1, eData[  7 ] );
  subRound( C, D, E, A, B, f1, K1, eData[  8 ] );
  subRound( B, C, D, E, A, f1, K1, eData[  9 ] );
  subRound( A, B, C, D, E, f1, K1, eData[ 10 ] );
  subRound( E, A, B, C, D, f1, K1, eData[ 11 ] );
  subRound( D, E, A, B, C, f1, K1, eData[ 12 ] );
  subRound( C, D, E, A, B, f1, K1, eData[ 13 ] );
  subRound( B, C, D, E, A, f1, K1, eData[ 14 ] );
  subRound( A, B, C, D, E, f1, K1, eData[ 15 ] );
  subRound( E, A, B, C, D, f1, K1, expand( eData, 16 ) );
  subRound( D, E, A, B, C, f1, K1, expand( eData, 17 ) );
  subRound( C, D, E, A, B, f1, K1, expand( eData, 18 ) );
  subRound( B, C, D, E, A, f1, K1, expand( eData, 19 ) );

  subRound( A, B, C, D, E, f2, K2, expand( eData, 20 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 21 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 22 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 23 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 24 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 25 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 26 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 27 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 28 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 29 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 30 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 31 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 32 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 33 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 34 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 35 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 36 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 37 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 38 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 39 ) );

  subRound( A, B, C, D, E, f3, K3, expand( eData, 40 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 41 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 42 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 43 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 44 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 45 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 46 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 47 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 48 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 49 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 50 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 51 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 52 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 53 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 54 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 55 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 56 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 57 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 58 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 59 ) );

  subRound( A, B, C, D, E, f4, K4, expand( eData, 60 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 61 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 62 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 63 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 64 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 65 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 66 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 67 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 68 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 69 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 70 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 71 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 72 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 73 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 74 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 75 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 76 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 77 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 78 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 79 ) );

  /* Build message digest */
  digest[ 0 ] += A;
  digest[ 1 ] += B;
  digest[ 2 ] += C;
  digest[ 3 ] += D;
  digest[ 4 ] += E;
}

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

static void 
longReverse(guint32 *buffer, int byteCount, int Endianness )
{
  guint32 value;

  if (Endianness==1) return;
  byteCount /= sizeof( guint32 );
  while( byteCount-- )
    {
      value = *buffer;
      value = ( ( value & 0xFF00FF00L ) >> 8  ) | \
              ( ( value & 0x00FF00FFL ) << 8 );
      *buffer++ = ( value << 16 ) | ( value >> 16 );
    }
}

/* Update SHA for a block of data */

void
SHAUpdate( SHA_CTX *shaInfo, guint8 const *buffer, guint count )
{
  guint32 tmp;
  int dataCount;

  /* Update bitcount */
  tmp = shaInfo->countLo;
  if ( ( shaInfo->countLo = tmp + ( ( guint32 ) count << 3 ) ) < tmp )
    shaInfo->countHi++;             /* Carry from low to high */
  shaInfo->countHi += count >> 29;

  /* Get count of bytes already in data */
  dataCount = ( int ) ( tmp >> 3 ) & 0x3F;

  /* Handle any leading odd-sized chunks */
  if( dataCount )
    {
      guint8 *p = ( guint8 * ) shaInfo->data + dataCount;

      dataCount = SHA_DATASIZE - dataCount;
      if( count < dataCount )
        {
          g_memmove( p, buffer, count );
          return;
        }
      g_memmove( p, buffer, dataCount );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness);
      SHATransform( shaInfo->digest, shaInfo->data );
      buffer += dataCount;
      count -= dataCount;
    }

  /* Process data in SHA_DATASIZE chunks */
  while( count >= SHA_DATASIZE )
    {
      g_memmove( shaInfo->data, buffer, SHA_DATASIZE );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness );
      SHATransform( shaInfo->digest, shaInfo->data );
      buffer += SHA_DATASIZE;
      count -= SHA_DATASIZE;
    }

  /* Handle any remaining bytes of data. */
  g_memmove( shaInfo->data, buffer, count );
}

/* Final wrapup - pad to SHA_DATASIZE-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, MSB-first) */

void
SHAFinal( char *key, SHA_CTX *shaInfo )
{
  int count;
  guint8 *dataPtr;

  /* Compute number of bytes mod 64 */
  count = ( int ) shaInfo->countLo;
  count = ( count >> 3 ) & 0x3F;

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  dataPtr = ( guchar * ) shaInfo->data + count;
  *dataPtr++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = SHA_DATASIZE - 1 - count;

  /* Pad out to 56 mod 64 */
  if( count < 8 )
    {
      /* Two lots of padding:  Pad the first block to 64 bytes */
      memset( dataPtr, 0, count );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness );
      SHATransform( shaInfo->digest, shaInfo->data );

      /* Now fill the next block with 56 bytes */
      memset( shaInfo->data, 0, SHA_DATASIZE - 8 );
    }
  else
      /* Pad block to 56 bytes */
    memset( dataPtr, 0, count - 8 );

  /* Append length in bits and transform */
  shaInfo->data[ 14 ] = shaInfo->countHi;
  shaInfo->data[ 15 ] = shaInfo->countLo;

  longReverse( shaInfo->data, SHA_DATASIZE - 8, shaInfo->Endianness );
  SHATransform( shaInfo->digest, shaInfo->data );
  longReverse(shaInfo->digest, SHA_DIGESTSIZE, shaInfo->Endianness );
  g_memmove(key, shaInfo->digest, SHA_DIGESTSIZE);
}



/* ************************************************************ */
/* Code below is David Helder's API for GNet			*/

struct _GSHA
{
  SHA_CTX	ctx;
  guchar 	digest[GNET_SHA_HASH_LENGTH];
};


GSHA*           
gnet_sha_new (guchar const* buffer, guint length)
{
  GSHA* gsha;

  gsha = g_new0 (GSHA, 1);
  SHAInit (&gsha->ctx);
  SHAUpdate (&gsha->ctx, buffer, length);
  SHAFinal ((gpointer) &gsha->digest, &gsha->ctx);

  return gsha;
}



GSHA*		
gnet_sha_new_string (gchar* str)
{
  GSHA* gsha;
  guint i;

  g_return_val_if_fail (str, NULL);
  g_return_val_if_fail (strlen(str) == (GNET_SHA_HASH_LENGTH * 2), NULL);

  gsha = g_new0 (GSHA, 1);

  for (i = 0; i < (GNET_SHA_HASH_LENGTH * 2); ++i)
    {
      guint val;

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
	gsha->digest[i / 2] |= val;
      else
	gsha->digest[i / 2] = val << 4;
    }

  return gsha;
}


void
gnet_sha_delete (GSHA* gsha)
{
  if (gsha)
    g_free (gsha);
}


/* **************************************** */


typedef struct _GSHAAsyncState
{
  gboolean 	waiting;

  gchar* 	pathname;
  int 		fd;
  GIOChannel* 	iochannel;
  guint 	watch;

  GSHA* 	sha;

  GSHAAsyncFunc func;
  gpointer 	user_data;

} GSHAAsyncState;

#define 	MAX_ASYNC_SHA			8
static guint 	num_async_sha =			0;
static GList*	waiting_async_sha = 		NULL;
static GList*	waiting_async_sha_last = 	NULL;


static gboolean dispatch_sha (GSHAAsyncState* state);
static gboolean sha_cb (GIOChannel* iochannel, 
			GIOCondition condition, gpointer data);


GSHAAsyncID*
gnet_sha_new_file_async (gchar* pathname, GSHAAsyncFunc func,
			 gpointer user_data)
{
  GSHAAsyncState* state;

  g_return_val_if_fail (pathname, NULL);
  g_return_val_if_fail (func, NULL);

  state = g_new0 (GSHAAsyncState, 1);
  state->pathname = g_strdup (pathname);
  state->sha = g_new0 (GSHA, 1);
  SHAInit (&state->sha->ctx);
  state->func = func;
  state->user_data = user_data;

  /* Dispatch now if no one is waiting, otherwise queue */
  if (!num_async_sha)
    {
      if (dispatch_sha (state))
	state = NULL;
    }
  else
    {
      state->waiting = TRUE;
      waiting_async_sha = g_list_prepend (waiting_async_sha, state);
      if (waiting_async_sha_last == NULL)
	waiting_async_sha_last = waiting_async_sha;
    }

  return (GSHAAsyncID) state;
}


/* Return FALSE if ok, TRUE otherwise */
static gboolean
dispatch_sha (GSHAAsyncState* state)
{
  g_return_val_if_fail (state, TRUE);
  g_return_val_if_fail (!state->fd, TRUE);
  g_return_val_if_fail (state->pathname, TRUE);

  state->waiting = FALSE;
  ++num_async_sha;

  /* Fail if file does not exist */
  state->fd = open (state->pathname, O_RDONLY | O_NONBLOCK);
  if (state->fd == -1)
    {
      (state->func)(NULL, state->user_data);
      gnet_sha_new_file_async_cancel ((GSHAAsyncID) state);
      return TRUE;
    }

  state->iochannel = g_io_channel_unix_new (state->fd);
  state->watch = g_io_add_watch_full (state->iochannel, 
				      G_PRIORITY_LOW,
				      G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,  
				      sha_cb, state, NULL);

  return FALSE;
}


static gboolean 
sha_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GSHAAsyncState* state =  (GSHAAsyncState*) data;
  gchar buffer [4096];

  g_return_val_if_fail (state, FALSE);

  if (condition == G_IO_IN)
    {
      GIOError error;
      guint bytes_read;

      error = g_io_channel_read (iochannel, buffer, sizeof(buffer), &bytes_read);
      if (error == G_IO_ERROR_AGAIN)
	return TRUE;
      if (error != G_IO_ERROR_NONE)
	goto error;

      if (bytes_read)
	{
	  SHAUpdate (&state->sha->ctx, buffer, bytes_read);
	  return TRUE;
	}
      else
	{
	  SHAFinal ((gpointer) &state->sha->digest, &state->sha->ctx);

	  (state->func)(state->sha, state->user_data);

	  state->sha = NULL;
	  gnet_sha_new_file_async_cancel ((GSHAAsyncID) state);

	  return FALSE;
	}
    }
  else
    {
    error:
      (state->func)(NULL, state->user_data);
      gnet_sha_new_file_async_cancel ((GSHAAsyncID) state);
    }

  return FALSE;
}


/* Cancel or delete a SHA async state.  This is used internally to
   delete states. */
void
gnet_sha_new_file_async_cancel (GSHAAsyncID* id)
{
  GSHAAsyncState* state = (GSHAAsyncState*) id;

  g_return_if_fail (id);

  g_free (state->pathname);
  gnet_sha_delete (state->sha);

  if (state->fd)
    close (state->fd);
  if (state->iochannel)
    g_io_channel_unref (state->iochannel);
  if (state->watch)
    g_source_remove (state->watch);

  if (state->waiting)
    {
      g_return_if_fail (waiting_async_sha);
      g_return_if_fail (waiting_async_sha_last);

      if (waiting_async_sha_last->data == state)
	waiting_async_sha_last = waiting_async_sha_last->prev;

      waiting_async_sha = g_list_remove (waiting_async_sha, state);
    }
  else
    {
      --num_async_sha;
      if (waiting_async_sha_last && num_async_sha < MAX_ASYNC_SHA)
	{
	  GList* last;
	  GSHAAsyncState* next_state;

	  last = waiting_async_sha_last;
	  next_state = (GSHAAsyncState*) last->data;

	  waiting_async_sha_last = waiting_async_sha_last->prev;
	  waiting_async_sha = g_list_remove_link (waiting_async_sha, last);

	  dispatch_sha (next_state);
	}
    }

  g_free (state);
}


/* **************************************** */


gint
gnet_sha_equal (const gpointer p1, const gpointer p2)
{
  GSHA* gshaa = (GSHA*) p1;
  GSHA* gshab = (GSHA*) p2;
  guint i;

  for (i = 0; i < GNET_SHA_HASH_LENGTH; ++i)
    if (gshaa->digest[i] != gshab->digest[i])
      return 0;

  return 1;
}


guint
gnet_sha_hash (GSHA* gsha)
{
  guint* p;

  g_return_val_if_fail (gsha, 0);

  p = (guint*) gsha->digest;

  return (p[0] ^ p[1] ^ p[2] ^ p[3] ^ p[4]);
}


guchar*        	
gnet_sha_get_digest (GSHA* gsha)
{
  g_return_val_if_fail (gsha, NULL);
  
  return gsha->digest;
}


static gchar bits2hex[16] = { '0', '1', '2', '3', 
			      '4', '5', '6', '7',
			      '8', '9', 'a', 'b',
			      'c', 'd', 'e', 'f' };

gchar*          
gnet_sha_get_string (GSHA* gsha)
{
  gchar* str;

  g_return_val_if_fail (gsha, NULL);

  str = g_new (gchar, GNET_SHA_HASH_LENGTH * 2 + 1);

  gnet_sha_copy_string (gsha, str);
  str[GNET_SHA_HASH_LENGTH * 2] = '\0';

  return str;
}



void
gnet_sha_copy_string (GSHA* gsha, guchar* buffer)
{
  guint i;

  g_return_if_fail (gsha);
  g_return_if_fail (buffer);

  for (i = 0; i < GNET_SHA_HASH_LENGTH; ++i)
    {
      buffer[i * 2]       = bits2hex[(gsha->digest[i] & 0xF0) >> 4];
      buffer[(i * 2) + 1] = bits2hex[(gsha->digest[i] & 0x0F)     ];
    }
}