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
 *  The port where the HTML server is listen to
 */
#define HTML_SERVER_PORT    3000


/*!
 *  Server's indication in response texts
 */
#define HTML_SERVER_NAME    "Compact HTTP Server"


/*!
 *  This base directory of all HTML pages
 */
#define HTML_ROOT_DIR       "./"



/*!
 *  Maximum size of the received HTML commands ( for POST it can be big! )
 */
#define MAX_HTML_BUF_LEN    10000



/*!
 *  Maximum size of memory consumed from one server instance
 */
#define HTTP_OBJ_SIZE       ( MAX_HTML_BUF_LEN + 2000 )



/* -- public types    -----------------------------------------------------------*/

typedef struct 
{
  /* public members */
  char* server_name;    /* name of the http server */
  int   socket;         /* file respectively socket descriptor */
  char* rcvbuf;         /* receiving buffer ( header and body ) */
  
  
  /* private temporary data */
  char* url_path;       /* first part of the URL */
  char* search_path;    /* search path of the URL (separated by ?) */
  char* frl;            /* absolute path within local file system for given url */
  long  content_len;    /* size of the content within body in bytes */
  int   mimetyp;        /* mime typ */
  
  
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
 *     - server_name: server name
 *     - rcvbuf:      pointer to buffer of size MAX_HTML_BUF_LEN
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int HTTP_ObjInit( HTTP_OBJ* this, const char* server_name );


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


#endif /* #ifndef _HTTP_H */
