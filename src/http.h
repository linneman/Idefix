/*
 *  http.h
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#ifndef _HTTP_H
#define _HTTP_H

#include <stdio.h>
#include "objmem.h"


/* -- const definitions -----------------------------------------------------------*/


/*!
 *  Server's indication in response texts
 */
#define HTML_SERVER_NAME            "Compact HTTP Server"


/*!
 *  Default URL in case nothing is specified from client
 */
#define HTTP_DEFAULT_URL_PATH       "index.html"


/*!
 *  Maximum size of the received HTML commands ( for POST it can be big! )
 */
#define MAX_HTML_BUF_LEN            10000


/*!
 *  Maximum size of memory consumed from one server instance
 */
#define HTTP_OBJ_SIZE               ( MAX_HTML_BUF_LEN + 2000 )


/*!
 *  Maximum allowed CGI handlers
 */
#define HTTP_MAX_CGI_HANDLERS       20


/*!
 *  If 1 TCP connections are kept alive
 */
#define HTTP_KEEP_ALIVE             1


/*!
 *  HTTP method ID's
 */
#define HTTP_GET_ID                 0x01
#define HTTP_POST_ID                0x02
#define HTTP_HEAD_ID                0x04
#define HTTP_PUT_ID                 0x08
#define HTTP_DELETE_ID              0x10
#define HTTP_TRACE_ID               0x20
#define HTTP_OPTIONS_ID             0x40
#define HTTP_CONNECT_ID             0x80
 
/*!
 *  Servers internal error codes
 */
#define HTTP_OK                     (   0 )
#define HTTP_HEAP_OVERFLOW          (  -1 )   /* could not allocate bytes from object's heap */
#define HTTP_STACK_OVERFLOW         (  -2 )   /* could not allocate bytes from object's stack */
#define HTTP_BUFFER_OVERRUN         (  -3 )   /* in example for given URL */
#define HTTP_MALFORMED_URL          (  -4 )   /* comprehensive URL check failed */
#define HTTP_SEND_ERROR             (  -5 )   /* in example due to sigpipe */
#define HTTP_RCV_ERROR              (  -6 )   /* error while receiving data from socket */
#define HTTP_RECV_TIMEOUT           (  -7 )   /* time out while waiting for incomming data */
#define HTTP_WRONG_METHOD           (  -8 )   /* http method does not exist */
#define HTTP_CGI_HANLDER_NOT_FOUND  (  -9 )   /* implementation bug if happens */
#define HTTP_CGI_EXEC_ERROR         ( -10 )   /* error occured within cgi execution */
#define HTTP_TOO_MANY_CGI_HANDLERS  ( -11 )   /* only HTTP_MAX_CGI_HANDLERS allowed */
#define HTTP_FILE_NOT_FOUND         ( -12 )   /* static content file like html, jpeg not found */
#define HTTP_NOT_IMPLEMENTED_YET    ( -13 )   /* http method of other feature not implmented yet */
#define HTTP_HEADER_ERROR           ( -14 )   /* malfromed http header */
#define HTTP_POST_DATA_TOO_BIG      ( -15 )   /* too many bytes in post block */
#define HTTP_POST_IO_ERROR          ( -16 )   /* could not read http post data */


/*!
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
  HTTP_MIME_IMAGE_ICON,
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


/*!
 *  HTTP Header status codes
 */
typedef enum {
  HTTP_ACK_OK,                      /* 200 OK */
  HTTP_ACK_NOT_FOUND,               /* 404 Not Found */
  HTTP_ACK_INTERNAL_ERROR           /* 500 Internal Server Error */
} HTTP_ACK_KEY;

  
  
/* -- public types    -----------------------------------------------------------*/


/*!
 *  CGI callback handler
 *
 *  Function parameters
 *      - this:      pointer to HTTP_OBJ of used thread
 *      - method:    HTTP method (GET or POST )
 *      - url_path:  url_path for triggering handler
 *
 *   Returnparameter
 *
 *      - R:         0 in case of success, otherwise error code
 */
struct _HTTP_OBJ;
typedef int (* HTTP_CGI_HANDLER)( struct _HTTP_OBJ* this );


/*
 *  hash type for CGI handlers
 */
typedef struct 
{
  int               handler_id;
  int               method_id_mask;
  char*             url_path;
  HTTP_CGI_HANDLER  handler;
} HTTP_CGI_HASH;


/*!
 *  Pseudo HTTP class
 */
typedef struct _HTTP_OBJ
{
  /* public members */
  char* server_name;    /* name of the http server */
  int   port;           /* server is listening to port */
  int   socket;         /* file respectively socket descriptor */
  char* rcvbuf;         /* receiving buffer ( header and body ) */
  char* body_ptr;       /* pointer to http body */
  int   header_len;     /* length of the http request header */
  int   body_len;       /* length of the http request body */
  long  content_len;    /* lenght of content which is sent back to server */
  int   mimetyp;        /* mime typ */
  // int   disconnect;     /* disconnect request if not zero */
  char* ht_root_dir;    /* root directory for static web content */
  
  /* private temporary data */
  int   method_id;      /* http method ID */
  char* url_path;       /* first part of the URL */
  char* search_path;    /* search path of the URL (separated by ?) */
  char* frl;            /* absolute path within local file system for given url */
  int   keep_alive;     /* is set to 1 when header key Connection: keep-alive given */
  
  
  
  /* cgi handler table, handlers with more specific search paths are served first */
  HTTP_CGI_HASH    cgi_handler_tab[HTTP_MAX_CGI_HANDLERS];
  int              cgi_handler_tab_top;
  
  /*
   * declare local memory management struct members with 
   * macro OBJ_DELCARE_LOCAL_HEAP( size ) 
   */

  OBJ_DELCARE_LOCAL_HEAP( HTTP_OBJ_SIZE );
  /*
  # This expands to:
  #
  # char		objmem[  size  ];
  # char*   heapPtr;		# initialize to  objmem 
  # char*   stackPtr;		# initialize to objmem + size
  # char*   framePtrTab[ OBJ_MAX_FRAMES ];
  # int     framePtrIndex;
  */
} HTTP_OBJ;


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
);


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
int HTTP_ProcessRequest( HTTP_OBJ* this );


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
  const char* url_path );


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
int HTTP_SendHeader( HTTP_OBJ* this, HTTP_ACK_KEY ack_key );


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
const char* HTTP_GetErrorMsg( int error );


/*******************************************************************************
 * HTTP_GetServerVersion() 
 *                                                                         */ /*!
 * retrieve current server version
 *                                                                              
 * Returnparameter
 *     - R:         server version in MMmmbb hex format (major.minor.build) 
 * 
 *******************************************************************************/
long HTTP_GetServerVersion( void );


#endif /* #ifndef _HTTP_H */
