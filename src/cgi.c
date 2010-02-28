/*
 *  cgi.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 24.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "cgi.h"


/*!
 *  Sample common gateway interface (CGI)
 *
 *  Function parameters
 *     - this:      pointer to HTTP Object
 *    
 *  Returnparameter
 *     - R: 0 in case of success, otherwise error code
 */
int TestCgiHandler( struct _HTTP_OBJ* this )
{
  int   len1, len2;
  int   bytes_written;
  int   error;
  char  content1[512];
  char  content2[1024];
  
  this->mimetyp = HTTP_MIME_TEXT_HTML;
  this->disconnect = true;
  
  snprintf( content1, sizeof(content1),
    "<html><body>\n"
    "<h1>This Content has been created dynamically!</h1>\n"
    "<p>************************** TestCgiHandler triggered! **************************</p>\n"
    "<p>*** method_id : %d</p>\n"
    "<p>*** url_path : %s</p>\n"
    "<p>*** search_path : %s</p>\n"
    "<p>*******************************************************************************</p><br />\n\n",
    this->method_id, this->url_path, this->search_path
    );
  len1 = strlen( content1 );
  
  if( this->method_id == HTTP_POST_ID )
  {
    snprintf( content2, sizeof(content2), "<p>POST:</p><code>%s</code></body></html>\n", this->body_ptr );
    len2 = strlen( content2 );
  }
  else 
  {
    content2[0] = '\0'; 
    len2 = 0;
  }

  
  this->content_len = len1 + len2;
  
  error = HTTP_SendHeader( this, HTTP_ACK_OK );
  if( error == HTTP_OK )
  {
    bytes_written = HTTP_SOCKET_SEND( this->socket, content1, len1, 0 );
    bytes_written += HTTP_SOCKET_SEND( this->socket, content2, len2, 0 );
  
    if( bytes_written != this->content_len )
    {
      error = HTTP_CGI_EXEC_ERROR;
    }
  }
  
  return error;
}


/*!
 *  Sample registration of CGI handlers
 *
 *  Function parameters
 *     - this:      pointer to HTTP Object
 *    
 *  Returnparameter
 *     - R: 0 in case of success, otherwise error code
 */
int RegisterCgiHandlers( struct _HTTP_OBJ* this )
{
  int error = 0;


  if( ! error )
    error = HTTP_AddCgiHanlder( this, TestCgiHandler, HTTP_GET_ID, "linnemann" );
  
  if( ! error ) 
    error = HTTP_AddCgiHanlder( this, TestCgiHandler, HTTP_POST_ID, "form" );

  
  return error;
}
