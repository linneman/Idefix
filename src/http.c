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
#include <stdbool.h>
#include <sys/stat.h>   /* for checking correct file status */
#include "http.h"
#include "socket_io.h"



/* -- const definitions -----------------------------------------------------------*/


/*! 
 *  Current server version in MMmmbb hex format (major.minor.build)
 */
#define HTTP_SERVER_VERSION        ( ( 0 << 16 ) | ( 0 << 8 ) | ( 5 ) )


/*!
 *  The maximum allowed number of characters in an URL
 */
#define HTML_MAX_URL_SIZE     256


/*!
 *  The maximum length of of an absolute path name
 */
#define HTML_MAX_PATH_LEN     256


/*!
 *  Chunksize used for HTML file operations
 */
#define HTML_CHUNK_SIZE       512


/*!
 *  Maximum length of server acknowledge block
 */
#define HTML_MAX_ACK_BLOCK    512


/*!
 *  Number of maximum bytes a status line of
 *  a server answer can have
 */
#define HTML_MAX_STATLINE     80



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
typedef struct 
{
  const int     id;
  const char*   txt;
} HTTP_HASH_TYPE;


/*!
 *  Server error text messages
 */
static const HTTP_HASH_TYPE _httpErrorTab[] =
{
  { HTTP_OK, "ok" },
  { HTTP_HEAP_OVERFLOW, "could not allocate bytes from object's heap" },
  { HTTP_STACK_OVERFLOW, "could not allocate bytes from object's stack" },
  { HTTP_BUFFER_OVERRUN, "internal buffer overrun" },
  { HTTP_MALFORMED_URL, "malformed URL transfered" },
  { HTTP_SEND_ERROR, "send error" },
  { HTTP_WRONG_METHOD, "http method does not exist" },
  { HTTP_CGI_HANLDER_NOT_FOUND, "wrong CGI handler invoked (IMPLEMENTATION BUG)" },
  { HTTP_CGI_EXEC_ERROR, "error occured within cgi execution" },
  { HTTP_TOO_MANY_CGI_HANDLERS, "to many cgi handlers registered" },
  { HTTP_FILE_NOT_FOUND, "static content file like html, jpeg not found" },
  { HTTP_NOT_IMPLEMENTED_YET, "http method of other feature not implmented yet" }
};

const int _httpErrorTabSize = sizeof( _httpErrorTab ) / sizeof( HTTP_HASH_TYPE );


/*
 *  HTTP status code
 */
static const HTTP_HASH_TYPE HttpAckTable[] = 
{
  { 200, "OK" },
  { 404, "Not Found" },
  { 500, "Internal Server Error" },
};


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
  
  /* find the position of the first blank separing the http command from the URL */
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
    return HTTP_MALFORMED_URL;
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
    
  for( i=beg, j=0;  i<end;  ++i)
  {
    c = pbuf[i];
    
    if( c == pbuf[i+1]  &&  ( c=='/' || c=='\\' || c=='.' ) )
      continue; /* do not copy slashes, backslashes or points twice */
    
    url[j++] = c;
  }
  
  /* line termination */
  url[j++] = '\0';
  return HTTP_OK;
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
  int     i = 0, len, error = HTTP_OK;
  
  ackbuf[0] = '\0';
  
  switch( ack_key )
  {
    default:  
      /* Status line */
      snprintf( linebuf, HTML_MAX_STATLINE, "HTTP/1.1 %3d %s\n", HttpAckTable[ack_key].id, HttpAckTable[ack_key].txt );
      len = strlen( linebuf );
      if( i+len >= HTML_MAX_ACK_BLOCK )
      {
        error = HTTP_BUFFER_OVERRUN;
        break;
      }
      strcat( & ackbuf[i], linebuf );
      i += len;
      
      /* Server indication */
      snprintf(  linebuf, HTML_MAX_STATLINE, "Server: %s\n", HTML_SERVER_NAME );
      len = strlen( linebuf );
      if( i+len >= HTML_MAX_ACK_BLOCK )
      {
        error = HTTP_BUFFER_OVERRUN;
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
          error = HTTP_BUFFER_OVERRUN;
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
          error = HTTP_BUFFER_OVERRUN;
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
          error = HTTP_BUFFER_OVERRUN;
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
      error = HTTP_SEND_ERROR;
    
    printf( "-------- HTTP ANSWER HEADER ------->\n");
    fwrite( ackbuf, 1, i, stdout );
    printf( "\n<------- HTTP ANSWER HEADER --------\n");
  }
  
  return error;
}


/*
 *  find CGI handler and returns handler ID if match was found, otherwise -1
 */
static int _find_cgi_handler( HTTP_OBJ* this )
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
      break;
    }
  }
  
  return handler_id;
}


/*
 *  Invokes CGI handler of given ID and returns handlers error code
 */
static int _call_cgi_handler( HTTP_OBJ* this, int handler_id )
{
  HTTP_CGI_HASH*    cgi_handler_tab     = this->cgi_handler_tab;
  const int         cgi_handler_tab_top = this->cgi_handler_tab_top;
  int               i, error;
    
  for( i=0; i < cgi_handler_tab_top; ++i )
  {
    if( handler_id == cgi_handler_tab[i].handler_id )
      break;    
  }
  
  if( i != cgi_handler_tab_top )
  {
    error = (*cgi_handler_tab[i].handler)( this );
  }
  else 
  {
    /* handler id does not exist */
    error = HTTP_CGI_HANLDER_NOT_FOUND;
  }

  return error;
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
  if( this->frl == NULL )
    return HTTP_STACK_OVERFLOW;
  
  /* get url */
  error = _http_get_url_from_request( frl, this->rcvbuf );
  if( error != 0 )
    return error;
  
  /* in case its empty use default URL */
  if( strlen( frl) == 0 )
    strcpy( frl, HTTP_DEFAULT_URL_PATH );
  
  /* extract url path */
  path_sep_idx = _http_search_path_index_from_url( frl );
  strncpy( url_path, this->frl, path_sep_idx );
  url_path[path_sep_idx] = '\0';
  
  /* extract search path */
  j=0;
  if( frl[path_sep_idx] == '?' )
  {
    for( i=path_sep_idx+1; frl[i] !='\0' && j < HTML_MAX_URL_SIZE; ++i, j++ )
      search_path[j] = frl[i];
  }
  search_path[j]='\0';

  this->mimetyp = _http_get_mime_type_from_filename( url_path );
  
  printf( "URL-PATH: %s\n", url_path );
  printf( "MIME-TYPE: %s\n", HttpMimeTypeTable[this->mimetyp].txt );
  printf( "SEARCH-PATH: %s\n", search_path );
  
  /* concatenate resource file name */
  strcpy( frl, this->ht_root_dir );
  strncat( frl, url_path, frl_size - strlen( this->ht_root_dir ) );
  frl[frl_size-1]='\0';
    
  return HTTP_OK;
}


/*!
 *  Determine length of static content
 */
static void _http_set_content_length_to_file_len( HTTP_OBJ* this )
{
  FILE* fp;

  if( this->frl != NULL )
  {
    /* generate server response, open requested resource */
    fp = fopen( this->frl, "r" );
    if( fp != NULL )
    {
      /* compute the file size of the requested resource and generate ack */
      fseek( fp, 0, SEEK_END );
      this->content_len = ftell( fp );
      fseek( fp, 0, SEEK_SET );
      fclose( fp );
    }
  }
}


/*!
 *  Process HTTP HEAD command (wrapper)
 */
static int http_head( HTTP_OBJ* this )
{
  int             error = 0;
  int             handler_id;
  struct stat     file_stat;
  
  printf("received HEAD command: %s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }
    
  /* check whether CGI handler exists */
  if( ( handler_id = _find_cgi_handler( this ) ) >= 0 )
  {
    /* but do not invoke for head method */
  }
  else
  {
    /* otherwise check for static content (html, javascript, jpeg, etc) */
 
    /* check for correct file status ( must be ordinary file, no directory ) */
    error = stat( this->frl, & file_stat );
    if( !error )
    {
      if( !( file_stat.st_mode & S_IFREG ) )
      {
        return HTTP_FILE_NOT_FOUND;
      }
    }    
          
    /* set content length in http header and always request disconnection */
    _http_set_content_length_to_file_len( this );
    this->disconnect = true;
    
    /* generate header */
    error = HTTP_SendHeader( this, HTTP_ACK_OK );
    if( error < 0 )
    {
      return error;
    }
  }
  
  return error;
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
  int             handler_id;
  struct stat     file_stat;
  
  printf("received GET command: %s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }
    
  /* check whether CGI handler exists */
  if( ( handler_id = _find_cgi_handler( this ) ) >= 0 )
  {
    /* and if so invoke it */
    error = _call_cgi_handler( this, handler_id );
  }
  else 
  {
    /* otherwise deliver static content (html, javascript, jpeg, etc) */
    
    /* check for correct file status ( must be ordinary file, no directory ) */
    error = stat( this->frl, & file_stat );
    if( !error )
    {
      if( !( file_stat.st_mode & S_IFREG ) )
      {
        return HTTP_FILE_NOT_FOUND;
      }
    }
    
    /* set content length in http header */
    _http_set_content_length_to_file_len( this );
    
    /* connection has to be closed after static html content is delivered */
    if( this->mimetyp == HTTP_MIME_TEXT_HTML )
    {
      this->disconnect = true;
    }
    
    /* generate header */
    error = HTTP_SendHeader( this, HTTP_ACK_OK );
    if( error < 0 )
    {
      return error;
    }

    /* open and copy static content from file system */
    fp = fopen( this->frl, "r" );
    if( fp == NULL )
    {
      _http_ack( this->socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
      return HTTP_FILE_NOT_FOUND;
    }
    
    /* read file blockwise and send it to the server */
    do {
      bytes_read = fread( buf, sizeof(char), HTML_CHUNK_SIZE, fp );
      bytes_written = HTTP_SOCKET_SEND( this->socket, buf, bytes_read, 0 );
    } while( bytes_read > 0  &&  bytes_written == bytes_read );
    
    fclose( fp );
  }
  
  return error;
}


/*!
 *  Process HTTP POST command
 */
static int http_post( HTTP_OBJ* this )
{
  int         handler_id;
  int         error = 0;
   
  printf("received POST command: %s\n", this->rcvbuf );

  /* retrieve url_path, search_path and frl (file resouce location) */
  error = http_parse_header( this );
  if( error < 0 )
  {
    return error;
  }

  /* check whether CGI handler exists */
  if( ( handler_id = _find_cgi_handler( this ) ) >= 0 )
  {
    /* and if so invoke it */
    error = _call_cgi_handler( this, handler_id );
  }
  else 
  {
    /* when no handler exists generate page not found error */
    _http_ack( this->socket, HTTP_ACK_NOT_FOUND, NULL, 0, NULL );
    error = HTTP_CGI_HANLDER_NOT_FOUND;
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
  return HTTP_NOT_IMPLEMENTED_YET;
}

/*!
 *  Process HTTP DELETE command
 */
static int http_delete( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return HTTP_NOT_IMPLEMENTED_YET;
}

/*!
 *  Process HTTP TRACE command
 */
static int http_trace( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return HTTP_NOT_IMPLEMENTED_YET;
}

/*!
 *  Process HTTP OPTIONS command
 */
static int http_options( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return HTTP_NOT_IMPLEMENTED_YET;
}

/*!
 *  Process HTTP CONNECT command
 */
static int http_connect( HTTP_OBJ* this )
{
  /*
   *  not implemented yet
   */
  return HTTP_NOT_IMPLEMENTED_YET;
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
 *     - this:        pointer to HTTP object
 *     - server_name: server name
 *     - ht_root_dir: root directory for static web content 
 *     - port:        port the server is listening to
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_ObjInit( 
  HTTP_OBJ*   this, 
  const char* server_name, 
  const char* ht_root_dir,
  const int   port
)
{
  int len;

  /* Initialize object internals */
  memset( this, 0, sizeof( HTTP_OBJ ) );
  OBJ_INIT( this );
  
  /* initialize components */
  this->server_name = OBJ_HEAP_ALLOC( strlen( server_name ) + 1 );
  if( this->server_name == NULL )
    return HTTP_HEAP_OVERFLOW;

  strcpy( this->server_name, server_name ); 
  this->socket = -1;

  /* 2: in case of trailing '/' and '\0' */
  this->ht_root_dir = OBJ_HEAP_ALLOC( ( len = strlen( ht_root_dir ) ) + 2 ); 
  if( this->ht_root_dir == NULL )
    return HTTP_HEAP_OVERFLOW;
  
  strcpy( this->ht_root_dir, ht_root_dir );
  if( this->ht_root_dir[len-1] != '/' )
    strcpy( & this->ht_root_dir[len], "/" );
  
  this->port = port;

  this->rcvbuf = OBJ_HEAP_ALLOC( MAX_HTML_BUF_LEN );
  if( this->rcvbuf == NULL )
    return HTTP_HEAP_OVERFLOW;
    
  
  return HTTP_OK;
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
  int retcode = HTTP_OK;
  
  /* reset internal states first */
  this->content_len = 0;
  this->mimetyp = HTTP_MIME_UNDEFINED;
  this->url_path      = NULL;
  this->search_path   = NULL;
  this->frl           = NULL;
  this->disconnect    = false;
  
  /* invoke HTTP method handler */
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
    retcode = HTTP_WRONG_METHOD;
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
    return HTTP_TOO_MANY_CGI_HANDLERS;
    
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
  
  return HTTP_OK;
}


/*******************************************************************************
 * HTTP_SendHeader() 
 *                                                                         */ /*!
 * Sends HTTP Header of given HTTP Object 
 *                                                                              
 * Function parameters
 *     - this:      pointer to HTTP Object
 *     - ack_key:   acknowledge code ( HTTP_ACK_OK, HTTP_ACK_NOT_FOUND, HTTP_ACK_INTERNAL_ERROR )
 *    
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_SendHeader( HTTP_OBJ* this, HTTP_ACK_KEY ack_key )
{
  const char*     p_ack_add_on_str;
  int             error = HTTP_OK;
  
  /* determine whether we put the string "Conneciton:close\n" in the ack message ( mostly the case for html pages ) */
  if( this->disconnect )
  {
    p_ack_add_on_str = "Connection: close\n";
  }
  else 
  {
    p_ack_add_on_str = NULL;
  }
  
  /* send the ack message */
  error = _http_ack( this->socket, ack_key, HttpMimeTypeTable[this->mimetyp].txt, this->content_len, p_ack_add_on_str );
  if( error )
  {
    return error;
  }
  
  /* write header/content separation line */
  if( HTTP_SOCKET_SEND( this->socket, "\n\n", 2, 0 ) < 0 )
    error = HTTP_SEND_ERROR;
    
  return error;
}


/*******************************************************************************
 * HTTP_GetErrorMsg() 
 *                                                                         */ /*!
 * retrieve pointer to constant string with error message 
 *                                                                              
 * Function parameters
 *     - error:     error code
 *    
 * Returnparameter
 *     - R:         pointer to ASCII error text representation
 * 
 *******************************************************************************/
const char* HTTP_GetErrorMsg( int error )
{
  int i;
 
  for( i=0; i < _httpErrorTabSize; ++ i )
  {
    if( _httpErrorTab[i].id == error )
      return _httpErrorTab[i].txt;
  }
   
  return "undefined error code";
}


/*******************************************************************************
 * HTTP_GetServerVersion() 
 *                                                                         */ /*!
 * retrieve current server version
 *                                                                              
 * Returnparameter
 *     - R:         server version in MMmmbb hex format (major.minor.build) 
 * 
 *******************************************************************************/
long HTTP_GetServerVersion( void )
{
  return HTTP_SERVER_VERSION;
}


