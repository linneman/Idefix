/*
 *  http.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

/* -- includes -------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "http.h"
#include "socket_io.h"



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
    HTTP_SOCKET_SEND( socket, linebuf, len, 0 );
  }
  else 
  {
    if( HTTP_SOCKET_SEND( socket, ackbuf, i, 0 ) < 0 )
      error = -2;
    
    fwrite( ackbuf, 1, i, stdout ); 
  }
  
  return error;
}


/*
 *  Invokes CGI handler and returns handler ID if match was found, otherwise -1
 *
 *  handler_return_code - pointer to int variable for storing handler's return code
 */
int _call_cgi_handler( HTTP_OBJ* this, int* handler_return_code )
{
  HTTP_CGI_HASH*    cgi_handler_tab     = this->cgi_handler_tab;
  const int         cgi_handler_tab_top = this->cgi_handler_tab_top;
  char*             handler_url_path;
  char*             url_path = this->url_path;
  int               i, j, found, handler_id = -1;
    
  for( i=0; i < cgi_handler_tab_top; ++i )
  {
    handler_url_path = cgi_handler_tab[i].url_path;
    for( j=0, found = 1; handler_url_path[j]!='\0'  &&  j < HTML_MAX_URL_SIZE; ++j )
    {
      if( url_path[j] != handler_url_path[j] )
      {
        found = 0;
        break;
      }
    }
    
    if( found && ( this->method_id & cgi_handler_tab[i].method_id_mask ) )
    {
      handler_id = cgi_handler_tab[i].handler_id;
      *handler_return_code = (*cgi_handler_tab[i].handler)( this );
      break;
    }
  }
  
  return handler_id;
}


/*!
 *  Parse HTTP header
 *
 *  result is written to:
 *
 *    this->url_path
 *    this->search_path
 *    this->frl
 *    this->mimetyp
 */
static int http_parse_header( HTTP_OBJ* this )
{
  const int       frl_size = HTML_MAX_URL_SIZE + HTML_MAX_PATH_LEN;
  char            *frl;         /* absolute path within local file system for given url */
  char            *url_path;    /* first part of the URL */
  char            *search_path;  /* search path of the URL (separated by ?) */
  
  int             path_sep_idx, error;
  int             i, j;
    
    
  /* allocate temporary used memory  */
  
  this->url_path      = url_path    = OBJ_STACK_ALLOC( HTML_MAX_PATH_LEN );
  this->search_path   = search_path = OBJ_STACK_ALLOC( HTML_MAX_URL_SIZE );
  this->frl           = frl         = OBJ_STACK_ALLOC( frl_size );
  if( this->search_path == NULL )
    return -1;
  
  
  error = _http_get_url_from_request( frl, this->rcvbuf );
  if( error != 0 )
    return error;
  
  /* extract url path */
  path_sep_idx = _http_search_path_index_from_url( frl );
  strncpy( url_path, this->frl, path_sep_idx );
  url_path[path_sep_idx] = '\0';
  
  /* extract search path */
  for( i=path_sep_idx+1, j=0; frl[i] !='\0' && j < HTML_MAX_URL_SIZE; ++i, j++ )
    search_path[j] = frl[i];
  
  search_path[j]='\0';
  
  
  this->mimetyp = _http_get_mime_type_from_filename( url_path );
  
  printf( "URL-PATH: %s\n", url_path );
  printf( "MIME-TYPE: %s\n", HttpMimeTypeTable[this->mimetyp].txt );
  printf( "SEARCH-PATH: %s\n", search_path );
  printf( "---- HTTP ANSWER ------>\n");
  
  /* concatenate resource file name */
  strcpy( frl, HTML_ROOT_DIR );
  strncat( frl, url_path, frl_size - strlen( HTML_ROOT_DIR ) );
  frl[frl_size-1]='\0';
    
  return 0;
}


/*!
 *  Implementation of header generation
 *  
 *  determines resource size and writes appropriate header to socket
 *
 *  in case this->frl != NULL, the content_len is determined from
 *  the given file
 */
static int _http_head( HTTP_OBJ* this )
{
  FILE*           fp;
  char*           p_ack_add_on_str;
  int             error = 0, error2;
      
  if( this->frl != NULL )
  {
    /* generate server response, open requested resource */
    fp = fopen( this->frl, "r" );
    if( fp == NULL )
    {
      _http_ack( this->socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
      return -4;
    }
    
    /* compute the file size of the requested resource and generate ack */
    fseek( fp, 0, SEEK_END );
    this->content_len = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    fclose( fp );
  }
  
  /* determine whether we put the string "Conneciton:close\n" in the ack message ( only in case for html pages ) */
  if( this->mimetyp == HTTP_MIME_TEXT_HTML )
  {
    /* in case of html we indicate connection close by positive result code */
    error = 1;
    p_ack_add_on_str = "Connection: close\n";
  }
  else 
  {
    p_ack_add_on_str = NULL;
  }
  
  /* send the ack message */
  error2 = _http_ack( this->socket, HTTP_ACK_OK, HttpMimeTypeTable[this->mimetyp].txt, this->content_len, p_ack_add_on_str );
  if( error2 )
  {
    return error2;
  }
  
  /* write header/content separation line */
  if( HTTP_SOCKET_SEND( this->socket, "\n\n", 2, 0 ) < 0 )
    error = -1;
    
  return error;
}


/*!
 *  Process HTTP HEAD command (wrapper)
 */
static int http_head( HTTP_OBJ* this )
{
  int error;
  
  printf("received HEAD command: %s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }


  return _http_head( this );
}


/*!
 *  Process HTTP GET command
 */
static int http_get( HTTP_OBJ* this )
{
  FILE*           fp;
  char            buf[HTML_CHUNK_SIZE];
  int             bytes_read, bytes_written;
  int             error = 0;
  int             handler_return_code;
  
  printf("received GET command: %s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }
    
  if( _call_cgi_handler( this, & handler_return_code ) >= 0 )
  {
    /* if cgi handler was found, do nothing but indicate to disconnect stream */
    error = 1;
  }
  else 
  {
    /* otherwise deliver static content (html, javascript, jpeg, etc) */
  
    /* generate header */
    error = _http_head( this );
    if( error < 0 )
    {
      return error;
    }
  
    /* open and copy static content from file system */
    fp = fopen( this->frl, "r" );
    if( fp == NULL )
    {
      _http_ack( this->socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
      return -5;
    }
    
    /* read file blockwise and send it to the server */
    do {
      bytes_read = fread( buf, sizeof(char), HTML_CHUNK_SIZE, fp );
      bytes_written = HTTP_SOCKET_SEND( this->socket, buf, bytes_read, 0 );
      // fwrite( buf, 1, bytes_read, stdout ); 
    } while( bytes_read > 0  &&  bytes_written == bytes_read );
    
    printf("\n");
    
    fclose( fp );
  }
  
  return error;
}


/*!
 *  Process HTTP POST command
 */
static int http_post( HTTP_OBJ* this )
{
  int         handler_return_code;
  int         error = 0;
   
  printf("received POST command: %s\n", this->rcvbuf );
  printf("->: \n%s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }

  /* invoke CGI handler */ 
  if( _call_cgi_handler( this, & handler_return_code ) >= 0 )
  {
    /* if cgi handler was found, do nothing but indicate to disconnect stream */
    error = 1;
  }
  else 
  {
    /* generate page not found error */
    _http_ack( this->socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
    error = -1;
  }

  return error;
}


/*!
 *  Process HTTP PUT command
 */
static int http_put( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP DELETE command
 */
static int http_delete( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP TRACE command
 */
static int http_trace( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP OPTIONS command
 */
static int http_options( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return -1;
}

/*!
 *  Process HTTP CONNECT command
 */
static int http_connect( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return -1;
}



/* -- public prototypes ----------------------------------------------------------*/


/*******************************************************************************
 * HTTP_ObjInit() 
 *                                                                         */ /*!
 * Initialize main HTTP instance. It keeps a local heap holding persistent
 * data required for the complete lifetime of a server thread and a local
 * stack for storing temporary information required for processing one http
 * request.
 *                                                                              
 * Function parameters
 *     - server_name: server name
 *     - rcvbuf:      pointer to buffer of size MAX_HTML_BUF_LEN
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_ObjInit( HTTP_OBJ* this, const char* server_name )
{
  /* Initialize object internals */
  memset( this, 0, sizeof( HTTP_OBJ ) );
  OBJ_INIT( this );
  
  /* initialize components */
  this->server_name = OBJ_HEAP_ALLOC( strlen( server_name ) + 1 );
  if( this->server_name == NULL )
    return -1;

  strcpy( this->server_name, server_name ); 
  this->socket = -1;

  this->rcvbuf = OBJ_HEAP_ALLOC( MAX_HTML_BUF_LEN );
  if( this->rcvbuf == NULL )
    return -1;
  
  
  return 0;
}


/*******************************************************************************
 * HTTP_ProcessRequest() 
 *                                                                         */ /*!
 * Possible HTTP Commands are 
 *    GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT.
 *
 * Not all of them are implemented currently.
 *                                                                              
 * Function parameters
 *     - this:   pointer to HTTP Object
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_ProcessRequest( HTTP_OBJ* this )
{
  int retcode = 0;
  
  if( strncmp( this->rcvbuf, HTTP_GET, sizeof(HTTP_GET)-1 ) == 0 )
  {
    this->method_id = HTTP_GET_ID;
    retcode = http_get( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_POST, sizeof(HTTP_POST)-1 ) == 0 )
  {
    this->method_id = HTTP_POST_ID;
    retcode = http_post( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_HEAD, sizeof(HTTP_HEAD)-1) == 0 )
  {
    this->method_id = HTTP_HEAD_ID;
    retcode = http_head( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_PUT, sizeof(HTTP_PUT)-1 ) == 0 )
  {
    this->method_id = HTTP_PUT_ID;
    retcode = http_put( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_DELETE, sizeof(HTTP_DELETE)-1 ) == 0 )
  {
    this->method_id = HTTP_DELETE_ID;
    retcode = http_delete( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_TRACE, sizeof(HTTP_TRACE)-1 ) == 0 )
  {
    this->method_id = HTTP_TRACE_ID;
    retcode = http_trace( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_OPTIONS, sizeof(HTTP_OPTIONS)-1 ) == 0 )
  {
    this->method_id = HTTP_OPTIONS_ID;
    retcode = http_options( this );
  }
  else if( strncmp( this->rcvbuf, HTTP_CONNECT, sizeof(HTTP_CONNECT)-1 ) == 0 )
  {
    this->method_id = HTTP_CONNECT_ID;
    retcode = http_connect( this );
  }  
  else 
  {
    retcode = -1;
  }
  
  return( retcode );
}


/*
 *  helper comparison function for sorting the cgi_hash_tab
 */
static int _cgi_hash_comp( const void* a, const void* b )
{
  return strlen( ((HTTP_CGI_HASH *)b)->url_path ) - strlen( ((HTTP_CGI_HASH *)a)->url_path );
}


/*******************************************************************************
 * HTTP_AddCgiHanlder() 
 *                                                                         */ /*!
 * Add a CGI handler to a given HTTP object. 
 *                                                                              
 * Function parameters
 *     - this:      pointer to HTTP Object
 *     - handler:   cgi handler
 *     - method:    METHOD to trigger handler ( usually HTTP_GET_ID and/or HTTP_POST_ID )
 *     - url_path:  trigger url, more specific search paths are served first
 *    
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_AddCgiHanlder( 
  HTTP_OBJ* this, 
  HTTP_CGI_HANDLER handler, 
  const int method_id_mask, 
  const char* url_path )
{
  HTTP_CGI_HASH*  hashPtr;
  int             handler_id = this->cgi_handler_tab_top;

  /* check for handler table overflow */
  if( ! ( handler_id < HTTP_MAX_CGI_HANDLERS ) )
    return -1;
    
  /* insert handler into http object */
  hashPtr = & this->cgi_handler_tab[handler_id];
  hashPtr->handler        = handler;
  hashPtr->handler_id     = handler_id;
  hashPtr->method_id_mask = method_id_mask;
  hashPtr->url_path       = OBJ_HEAP_ALLOC( strlen( url_path ) + 1 );
  strcpy( hashPtr->url_path, url_path );
  ++this->cgi_handler_tab_top;
  
  /* sort handler table from most to least specific URL paths */
  qsort( this->cgi_handler_tab, this->cgi_handler_tab_top, sizeof( HTTP_CGI_HASH ), _cgi_hash_comp );
  
  return 0;
}
