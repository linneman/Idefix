/*
 *  http.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

/* -- includes -------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "http.h"



/* -- const definitions -----------------------------------------------------------*/


/*!
 *  The maximum allowed number of characters in an URL
 */
#define HTML_MAX_URL_SIZE   256


/*!
 *  The maximum length of of an absolute path name
 */
#define HTML_MAX_PATH_LEN   256


/*!
 *  Chunksize used for HTML file operations
 */
#define HTML_CHUNK_SIZE     512


/*!
 *  Maximum length of server acknowledge block
 */
#define HTML_MAX_ACK_BLOCK  512


/*!
 *  Number of maximum bytes a status line of
 *  a server answer can have
 */
#define HTML_MAX_STATLINE   80



/*
 *  HTTP methods
 */
#define HTTP_GET              "GET"
#define HTTP_POST             "POST"
#define HTTP_HEAD             "HEAD"
#define HTTP_PUT              "PUT"
#define HTTP_DELETE           "DELETE"
#define HTTP_TRACE            "TRACE"
#define HTTP_OPTIONS          "OPTIONS"
#define HTTP_CONNECT          "CONNECT"

/*  maximum length of HTTP command above */
#define MAX_HTTP_COMMAND_LEN  10


/* -- local types ---------------------------------------------------------------*/


/*
 *  General Hash Type, mapping from ID to textual representation
 */
typedef struct {
  const int     id;
  const char*   txt;
} HTTP_HASH_TYPE;


/*
 *  HTTP status code
 */
static const HTTP_HASH_TYPE HttpAckTable[] = 
{
  { 200, "OK" },
  { 404, "Not Found" },
  { 500, "Internal Server Error" },
};

typedef enum {
  HTTP_ACK_OK,                /* 200 OK */
  HTTP_ACK_NOT_FOUND,         /* 404 Not Found */
  HTTP_ACK_INTERNAL_ERROR     /* 500 Internal Server Error */
} HTTP_ACK_KEY;



/*
 *  HTTP mime types
 */

typedef enum {
  HTTP_MIME_UNDEFINED,
  HTTP_MIME_TEXT_HTML,
  HTTP_MIME_TEXT_CSS,
  HTTP_MIME_TEXT_PLAIN,
  HTTP_MIME_IMAGE_JPEG,
  HTTP_MIME_IMAGE_PNG,
  HTTP_MIME_IMAGE_GIF,
  HTTP_MIME_IMAGE_TIFF,
  HTTP_MIME_APPLICATION_JAVASCRIPT,
  HTTP_MIME_APPLICATION_JSON,
  HTTP_MIME_APPLICATION_XML,
  HTTP_MIME_APPLICATION_INDEX,
  HTTP_MIME_AUDIO_MP4,
  HTTP_MIME_AUDIO_MPEG,
  HTTP_MIME_AUDIO_SPEEX,
  HTTP_MIME_MULTIPART_FORM_DATA,
  HTTP_MIME_MULTIPART_ALTERNATIVE,
} HTTP_MIME_TYPE;


/*
 *  Hash for mime type key/string pair
 */
static const HTTP_HASH_TYPE HttpMimeTypeTable[] =
{
  { HTTP_MIME_UNDEFINED, "undefined" },
  { HTTP_MIME_TEXT_HTML, "text/html" },
  { HTTP_MIME_TEXT_CSS, "text/css" },
  { HTTP_MIME_TEXT_PLAIN, "text/plain" },
  { HTTP_MIME_IMAGE_JPEG, "image/jpeg" },
  { HTTP_MIME_IMAGE_PNG, "image/png" },
  { HTTP_MIME_IMAGE_GIF, "image/gif" },
  { HTTP_MIME_IMAGE_TIFF, "image/tiff" },
  { HTTP_MIME_APPLICATION_JAVASCRIPT, "application/javascript" },
  { HTTP_MIME_APPLICATION_JSON, "application/json" },
  { HTTP_MIME_APPLICATION_XML, "application/xml" },
  { HTTP_MIME_APPLICATION_INDEX, "applicaiton/index" },
  { HTTP_MIME_AUDIO_MP4, "audio/mp4" },
  { HTTP_MIME_AUDIO_MPEG, "audio/mpeg" },
  { HTTP_MIME_AUDIO_SPEEX, "audio/speex" },
  { HTTP_MIME_MULTIPART_FORM_DATA, "multipart/form-data" },
  { HTTP_MIME_MULTIPART_ALTERNATIVE, "mutipart/alternative" }
};

static const int HttpMimeTypeTableSize = sizeof(HttpMimeTypeTable) / sizeof(HTTP_HASH_TYPE);


/*
 *  Hash for mapping file extentions to mime types
 */
static const HTTP_HASH_TYPE HttpFileExtMimeTable[] =
{
  { HTTP_MIME_UNDEFINED, "undefined" },
  { HTTP_MIME_TEXT_HTML, "html" },
  { HTTP_MIME_TEXT_CSS, "css" },
  { HTTP_MIME_TEXT_PLAIN, "txt" },
  { HTTP_MIME_IMAGE_JPEG, "jpg" },
  { HTTP_MIME_IMAGE_JPEG, "jpeg" },
  { HTTP_MIME_IMAGE_PNG, "png" },
  { HTTP_MIME_IMAGE_GIF, "gif" },
  { HTTP_MIME_IMAGE_TIFF, "tiff" },
  { HTTP_MIME_APPLICATION_JAVASCRIPT, "js" },
  { HTTP_MIME_APPLICATION_JSON, "json" },
  { HTTP_MIME_APPLICATION_XML, "xml" },
  { HTTP_MIME_AUDIO_MP4, "mp4" },
  { HTTP_MIME_AUDIO_MPEG, "mpeg" },
  { HTTP_MIME_AUDIO_MPEG, "mpg" },  
  { HTTP_MIME_AUDIO_SPEEX, "speex" }
};

static const int HttpFileExtMimeTableSize = sizeof(HttpFileExtMimeTable) / sizeof(HTTP_HASH_TYPE);




/*
 *  Helper function for determine the mime type in a given string
 */
static HTTP_MIME_TYPE   _http_get_mime_type_from_string( const char *string )
{
  int i, index = HTTP_MIME_UNDEFINED;
  
  for( i=1; i < HttpMimeTypeTableSize; ++i )
  {
    if( strcasestr( string, HttpMimeTypeTable[i].txt ) != NULL )
    {
      index = i;
      break;
    }
  }
  
  return index;
}



/*
 *  Helper function to determine the mime type for a given filename
 */
static HTTP_MIME_TYPE   _http_get_mime_type_from_filename( const char *filename )
{
  int len, i, index = HTTP_MIME_UNDEFINED;
  
  /* determine position of file extension in filename (last dot) */
  len=strlen( filename );
  for( i=len-1; i>=0; i-- )
  {
    if( filename[i] == '.' )
      break;
  }
  
  if( i==0 || i==(len-1) ) /* nothing found */
    return HTTP_MIME_UNDEFINED;
  
  ++i;
  
  for( i=1; i < HttpFileExtMimeTableSize; ++i )
  {
    if( strcasestr( & filename[i], HttpFileExtMimeTable[i].txt ) != NULL )
    {
      index = i;
      break;
    }
  }
  
  return index;  
}



/*!
 *  extract the URL out of an http request
 */
static int _http_get_url_from_request( char url[HTML_MAX_URL_SIZE], char *pbuf )
{
  int   c;
  int   beg = 0, end;
  int   i, j;
  
  /* find the position of the first blank separing the URL command from the URL */
  while( beg < MAX_HTTP_COMMAND_LEN && pbuf[beg] != ' ' )
    ++beg;
  
  if( beg == MAX_HTTP_COMMAND_LEN )
    return -1;
  
  /* strip spaces */
  for( ; beg<HTML_MAX_URL_SIZE && pbuf[beg]==' '; ++beg)
    ;
  
  
  /* find the position of the last blank separating the URL form the http protocol specifier */
  end = beg + HTML_MAX_URL_SIZE - 1;
  for( i=beg; i < end  &&  pbuf[i] != ' '; ++i )
    ;
  
  if( i==end )
    return -2;
  else 
    end = i;
  
  
  /* strip out the following leading characters '.', '/', digits, '\', '*', ':', ';' */
  /* in order to avoid sandboxing violation */
  while( beg < end )
  {
    c = pbuf[beg];
    
    if( isdigit( c ) || c=='.' || c=='/' || c=='\\' || c=='*' || c==':' || c==';' || c < 32 || c > 127)
      ++beg;
    else 
      break;
  }
  
  if( beg == end )
    return -3;
  
  for( i=beg, j=0;  i<end;  ++i)
  {
    c = pbuf[i];
    
    if( c == pbuf[i+1]  &&  ( c=='/' || c=='\\' || c=='.' ) )
      continue; /* do not copy slashes, backslashes or points twice */
    
    url[j++] = c;
  }
  
  /* line termination */
  url[j++] = '\0';
  return 0;
}


/*!
 *  returns the position of the search path in an URL
 *  ( first question mark character ) or position of last character if not found
 */
static int _http_search_path_index_from_url( char url[HTML_MAX_URL_SIZE] )
{
  int i;
  
  for( i=0; i<HTML_MAX_URL_SIZE && url[i]!='\0'; ++i )
    if( url[i] == '?' )
      break;
  
  return i;
}



/*!
 *  generate acknowledge info block
 */
static int _http_ack( int socket, const HTTP_ACK_KEY ack_key, const char* mime_type, const long content_len, const char* add_ons )
{
  
  char    linebuf[HTML_MAX_STATLINE];
  char    ackbuf[HTML_MAX_ACK_BLOCK];
  int     i = 0, len, error = 0;
  
  ackbuf[0] = '\0';
  
  switch( ack_key )
  {
    default:  
      /* Status line */
      snprintf( linebuf, HTML_MAX_STATLINE, "HTTP/1.1 %3d %s\n", HttpAckTable[ack_key].id, HttpAckTable[ack_key].txt );
      len = strlen( linebuf );
      if( i+len >= HTML_MAX_ACK_BLOCK )
      {
        error = -1;
        break;
      }
      strcat( & ackbuf[i], linebuf );
      i += len;
      
      /* Server indication */
      snprintf(  linebuf, HTML_MAX_STATLINE, "Server: %s\n", HTML_SERVER_NAME );
      len = strlen( linebuf );
      if( i+len >= HTML_MAX_ACK_BLOCK )
      {
        error = -1;
        break;
      }
      strcat( & ackbuf[i], linebuf );
      i += len;
      
      
      /* Content length */
      if( content_len >  0 )
      {
        snprintf(  linebuf, HTML_MAX_STATLINE, "Content-Length: %ld\n", content_len );
        len = strlen( linebuf );
        if( i+len >= HTML_MAX_ACK_BLOCK )
        {
          error = -1;
          break;
        }
        strcat( & ackbuf[i], linebuf );
        i += len;
      }
      
      /* Content Type */
      if( mime_type != NULL )
      {
        snprintf(  linebuf, HTML_MAX_STATLINE, "Content-Type: %s\n", mime_type );
        len = strlen( linebuf );
        if( i+len >= HTML_MAX_ACK_BLOCK )
        {
          error = -1;
          break;
        }
        strcat( & ackbuf[i], linebuf );
        i += len;
      }
      
      /* Add-Ons */
      if( add_ons != NULL )
      {
        snprintf(  linebuf, HTML_MAX_STATLINE, "%s\n", add_ons );
        len = strlen( linebuf );
        if( i+len >= HTML_MAX_ACK_BLOCK )
        {
          error = -1;
          break;
        }
        strcat( & ackbuf[i], linebuf );
        i += len;
      }
      
      /* Remove trailing CR's */
      while( ackbuf[i-1] == '\n' && i > 1 )
        --i;
      ackbuf[i++] = '\0';
      break;
  }
  
  if( error )
  {
    snprintf( linebuf, HTML_MAX_STATLINE, "HTTP/1.1 %3d %s\n", HttpAckTable[HTTP_ACK_INTERNAL_ERROR].id, HttpAckTable[HTTP_ACK_INTERNAL_ERROR].txt );
    len = strlen( linebuf );
    send( socket, linebuf, len, 0 );
  }
  else 
  {
    if( send( socket, ackbuf, i, 0 ) < 0 )
      error = -2;
    
    fwrite( ackbuf, 1, i, stdout ); 
  }
  
  return error;
}



/*!
 *  Process HTTP GET command
 */
static int http_get( int socket, char *pbuf )
{
  FILE*           fp;
  const int       tmp_size = HTML_MAX_URL_SIZE + HTML_MAX_PATH_LEN;
  char            tmp[tmp_size];
  char            url_path[HTML_MAX_PATH_LEN];
  char            search_path[HTML_MAX_URL_SIZE];
  char            buf[HTML_CHUNK_SIZE];
  int             path_sep_idx, error;
  long            file_size;
  HTTP_MIME_TYPE  mimetype;
  char*           p_ack_add_on_str;
  int             i, j, bytes_read, bytes_written;
  
  printf("received GET command: %s\n", pbuf );
  
  error = _http_get_url_from_request( tmp, pbuf );
  if( error != 0 )
    return error;
  
  /* extract url path */
  path_sep_idx = _http_search_path_index_from_url( tmp );
  strncpy( url_path, tmp, path_sep_idx );
  url_path[path_sep_idx] = '\0';
  
  /* extract search path */
  for( i=path_sep_idx+1, j=0; tmp[i] !='\0' && j < HTML_MAX_URL_SIZE; ++i, j++ )
    search_path[j] = tmp[i];
  
  search_path[j]='\0';
  
  
  mimetype = _http_get_mime_type_from_filename( url_path );
  
  printf( "URL-PATH: %s\n", url_path );
  printf( "MIME-TYPE: %s\n", HttpMimeTypeTable[mimetype].txt );
  printf( "SEARCH-PATH: %s\n", search_path );
  printf( "---- HTTP ANSWER ------>\n");
  
  /* concatenate resource file name */
  strcpy( tmp, HTML_ROOT_DIR );
  strncat( tmp, url_path, tmp_size - strlen( HTML_ROOT_DIR ) );
  tmp[tmp_size-1]='\0';
  
  /* generate server response, open requested resource */
  fp = fopen( tmp, "r" );
  if( fp == NULL )
  {
    _http_ack( socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
    return -4;
  }
  
  /* compute the file size of the requested resource and generate ack */
  fseek( fp, 0, SEEK_END );
  file_size = ftell( fp );
  fseek( fp, 0, SEEK_SET );
  
  /* determine whether we put the string "Conneciton:close\n" in the ack message ( only in case for html pages ) */
  if( mimetype == HTTP_MIME_TEXT_HTML )
  {
    p_ack_add_on_str = "Connection: close\n";
  }
  else 
  {
    p_ack_add_on_str = NULL;
  }
  
  /* send the ack message */
  error = _http_ack( socket, HTTP_ACK_OK, HttpMimeTypeTable[mimetype].txt, file_size, p_ack_add_on_str );
  if( error )
  {
    fclose( fp );
    return error;
  }
  
  /* write header/content separation line */
  if( send( socket, "\n\n", 2, 0 ) < 0 )
    error = -1;
  fwrite( "\n\n", 1, 2, stdout ); 
  
  /* read file blockwise and send it to the server */
  do {
    bytes_read = fread( buf, sizeof(char), HTML_CHUNK_SIZE, fp );
    bytes_written = send( socket, buf, bytes_read, 0 );
    fwrite( buf, 1, bytes_read, stdout ); 
  } while( bytes_read > 0  &&  bytes_written == bytes_read );
  
  printf("\n");
  
  fclose( fp );
  
  // in case of html we indicate connection close by error code
  error = -1;
  
  return error;
}


/*!
 *  Process HTTP POST command
 */
static int http_post( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP HEAD command
 */
static int http_head( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP PUT command
 */
static int http_put( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP DELETE command
 */
static int http_delete( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP TRACE command
 */
static int http_trace( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP OPTIONS command
 */
static int http_options( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP CONNECT command
 */
static int http_connect( int socket, char *pbuf )
{
  /*
   *  not implemented yet
   */
  return -1;
}



/* -- public prototypes ----------------------------------------------------------*/


/*******************************************************************************
 * ProcessHttpRequest() 
 *                                                                         */ /*!
 * Possible HTTP Commands are 
 *    GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT.
 *
 * Not all of them are implemented currently.
 *                                                                              
 * Function parameters
 *     - socket: file descriptor for reading / writing http data
 *     - pbuf:   pointer to buffer of size MAX_HTML_BUF_LEN
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int ProcessHttpRequest( int socket, char *pbuf )
{
  int retcode = 0;
  
  if( strncmp( pbuf, HTTP_GET, sizeof(HTTP_GET)-1 ) == 0 )
  {
    retcode = http_get( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_POST, sizeof(HTTP_POST)-1 ) == 0 )
  {
    retcode = http_post( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_HEAD, sizeof(HTTP_HEAD)-1) == 0 )
  {
    retcode = http_head( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_PUT, sizeof(HTTP_PUT)-1 ) == 0 )
  {
    retcode = http_put( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_DELETE, sizeof(HTTP_DELETE)-1 ) == 0 )
  {
    retcode = http_delete( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_TRACE, sizeof(HTTP_TRACE)-1 ) == 0 )
  {
    retcode = http_trace( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_OPTIONS, sizeof(HTTP_OPTIONS)-1 ) == 0 )
  {
    retcode = http_options( socket, pbuf );
  }
  else if( strncmp( pbuf, HTTP_CONNECT, sizeof(HTTP_CONNECT)-1 ) == 0 )
  {
    retcode = http_connect( socket, pbuf );
  }  
  else 
  {
    retcode = -1;
  }
  
  return( retcode );
}
