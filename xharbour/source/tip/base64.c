/*
 * $Id: base64.c 9999 2014-05-19 10:56:11Z zsaulius $
 */

#define HB_THREAD_OPTIMIZE_STACK
#ifndef HB_THREAD_SUPPORT
#define HB_THREAD_STUB
#endif

// #include "thread.h"

#include "hbapi.h"
#include "hbapifs.h"
#include "base64.h"
#define XTY62  '+'
#define XTY63  '/'
#define XTYPAD '='

static char Base64Table[ 64 ] = {
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',   'P',
   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e',   'f',
   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',   'v',
   'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', XTY62, XTY63
};

char * Base64Encode( const unsigned char * pcCode, HB_SIZE uCodeLen )
{
   HB_SIZE i, j = 0, tailCnt = uCodeLen % 3;
   char *       pRet = ( char * ) hb_xgrab( 4 * ( ( uCodeLen + 2 ) / 3 ) + 1 );

   //uCodeLen/3 segments.
   for( i = 0; i < uCodeLen / 3; i++, pcCode += 3 )
   {
      pRet[ j++ ] = Base64Table[     pcCode[ 0 ]          >> 2 ];
      pRet[ j++ ] = Base64Table[ ( ( pcCode[ 0 ] & 0x03 ) << 4 ) | ( pcCode[ 1 ] >> 4 ) ];
      pRet[ j++ ] = Base64Table[ ( ( pcCode[ 1 ] & 0x0F ) << 2 ) | ( pcCode[ 2 ] >> 6 ) ];
      pRet[ j++ ] = Base64Table[     pcCode[ 2 ] & 0x3F ];
   }

   if( tailCnt )
   {
      pRet[ j++ ] = Base64Table[ pcCode[ 0 ] >> 2 ];

      if( tailCnt == 1 )
      {
         pRet[ j++ ] = Base64Table[ ( pcCode[ 0 ] & 0x03 ) << 4 ];
         pRet[ j++ ] = XTYPAD;
      }
      else if( tailCnt == 2 )
      {
         pRet[ j++ ] = Base64Table[ ( ( pcCode[ 0 ] & 0x03 ) << 4 ) | ( pcCode[ 1 ] >> 4 ) ];
         pRet[ j++ ] = Base64Table[   ( pcCode[ 1 ] & 0x0F ) << 2 ];
      }
      pRet[ j++ ] = XTYPAD;
   }

   pRet[ j ] = '\0';

   return pRet;
}

//assume input != NULL.
unsigned char * Base64Decode( const char * pcszInput, HB_SIZE * puOutLen )
{
   int             iSize = ( int ) strlen( pcszInput ) + 1;
   char            map[ 256 ], i, c, * pBuf = ( char * ) hb_xgrab( iSize );
   HB_SIZE         j, uBufLen;
   HB_SIZE         uSegCnt;
   HB_SIZE         uTailCnt;
   unsigned char * pRet, * pTmp;
   char *          ptr;

   memset( map, 0xFF, 256 );
   for( i = 'A', map[(unsigned char) 'A' ] =  0; i <  'Z'; map[(unsigned char) i + 1 ] = map[(unsigned char) i ] + 1, i++ );
   for( i = 'a', map[(unsigned char) 'a' ] = 26; i <= 'z'; map[ (unsigned char)i + 1 ] = map[(unsigned char) i ] + 1, i++ );
   for( i = '0', map[(unsigned char) '0' ] = 52; i <= '9'; map[ (unsigned char)i + 1 ] = map[(unsigned char) i ] + 1, i++ );
   map[ XTY62 ] = 62; map[ XTY63 ] = 63;

   uBufLen = 0;
   c       = pcszInput[ 0 ];
   while( c != '\0' && c != XTYPAD )
   {
      pBuf[ uBufLen ] = map[ (unsigned char)c ];

      if( pBuf[ uBufLen ] >= 0 )
         uBufLen++;
      c = *( ++pcszInput );
   }

   pBuf[ uBufLen ] = '\0';

   uSegCnt = uBufLen / 4;
   pRet    = ( unsigned char * ) hb_xgrab( ( uSegCnt + 1 ) * 3 );
   pTmp    = pRet;

   //full segments.
   ptr = pBuf;
   for( j = 0; j < uSegCnt; j++ )
   {
      *pTmp++ = ( ( ptr[ 0 ] << 2 ) | ( ptr[ 1 ] >> 4 ) );
      *pTmp++ = ( ( ptr[ 1 ] << 4 ) | ( ptr[ 2 ] >> 2 ) );
      *pTmp++ = ( unsigned char ) ( ( ptr[ 2 ] << 6 ) |   ptr[ 3 ] );
      ptr    += 4;
   }

   *puOutLen = uSegCnt * 3;
   uTailCnt  = uBufLen % 4;

   if( uTailCnt == 2 )
   {
      *pTmp = ( ( ptr[ 0 ] << 2 ) | ( ptr[ 1 ] >> 4 ) );
      ( *puOutLen )++;
   }
   else if( uTailCnt == 3 )
   {
      *pTmp++        = ( ( ptr[ 0 ] << 2 ) | ( ptr[ 1 ] >> 4 ) );
      *pTmp          = ( ( ptr[ 1 ] << 4 ) | ( ptr[ 2 ] >> 2 ) );
      ( *puOutLen ) += 2;
   }

   hb_xfree( pBuf );

   return pRet;
}

HB_FUNC( HB_BASE64ENCODE )
{
   HB_THREAD_STUB
   const unsigned char * pcCode   = ( const unsigned char * ) hb_parcx( 1 );
   unsigned int          uCodeLen = hb_parni( 2 );
   char *                szBase64Encode;

   szBase64Encode = Base64Encode( pcCode, uCodeLen );
   hb_retcAdopt( szBase64Encode );
}

HB_FUNC( HB_BASE64DECODE )
{
   HB_THREAD_STUB
   const char *    pcCode = hb_parcx( 1 );
   HB_SIZE    uCodeLen;
   unsigned char * szBase64Encode;

   szBase64Encode = Base64Decode( pcCode, &uCodeLen );

   hb_retclenAdopt( ( char * ) szBase64Encode, uCodeLen );
}

static HB_SIZE filelength( HB_FHANDLE handle )
{
   HB_SIZE nEnd   = hb_fsSeek( handle, 0, 2 );
   HB_SIZE nStart = hb_fsSeek( handle, 0, 0 );

   return nEnd - nStart;
}

static char * filetoBuff( char * f, const char * s )
{

   HB_SIZE i;
   HB_FHANDLE fh = hb_fsOpen( s, 2 );

   i      = hb_fsReadLarge( fh, ( BYTE * ) f, filelength( fh ) );
   f[ i ] = '\0';
   hb_fsClose( fh );
   return f;
}

HB_FUNC( HB_BASE64ENCODEFILE )
{
   HB_THREAD_STUB
   const char *   szInFile    = hb_parcx( 1 );
   const char *   szOutFile   = hb_parcx( 2 );
   const char *   pcCode;
   char *         FromBuffer;
   HB_FHANDLE     fh;
   HB_SIZE            iSize;
   char *         szBase64Encode;

   fh             = hb_fsOpen( szInFile, 2 );
   iSize          = filelength( fh );
   FromBuffer     = ( char * ) hb_xgrab( iSize + 1 );
   hb_fsClose( fh );
   pcCode         = ( char * ) filetoBuff( FromBuffer, szInFile );
   szBase64Encode = Base64Encode( ( const unsigned char * ) pcCode, iSize );
   fh             = hb_fsCreate( szOutFile, 0 );
   hb_fsWriteLarge( fh, szBase64Encode, strlen( szBase64Encode ) );
   hb_fsClose( fh );
   hb_xfree( FromBuffer );
   hb_xfree( szBase64Encode );
}

HB_FUNC( HB_BASE64DECODEFILE )
{
   HB_THREAD_STUB
   const char *   szInFile    = hb_parcx( 1 );
   const char *   szOutFile   = hb_parcx( 2 );
   const char *   pcCode;
   char *         FromBuffer;
   HB_FHANDLE     fh;
   HB_SIZE        iSize;
   char *         szBase64Encode;

   fh             = hb_fsOpen( szInFile, 2 );
   iSize          = filelength( fh );
   FromBuffer     = ( char * ) hb_xgrab( iSize + 1 );
   hb_fsClose( fh );
   pcCode         = ( char * ) filetoBuff( FromBuffer, szInFile );
   szBase64Encode = ( char * ) Base64Decode( pcCode, &iSize  );
   fh             = hb_fsCreate( szOutFile, 0 );
   hb_fsWriteLarge( fh, szBase64Encode, strlen( szBase64Encode ) );
   hb_fsClose( fh );
   hb_xfree( FromBuffer );
}
