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
  char  content[1024]; 
  
   sprintf( content, 
    "<html><body>\n" 
    "<h1>This Content has been created dynamically!</h1>"
    "<p>************************** TestCgiHandler triggered! **************************</p>\n" 
    "<p>*** method_id : %d</p>\n"
    "<p>*** url_path : %s</p>\n"
    "<p>*** search_path : %s</p>\n"
    "<p>*******************************************************************************\n\n" 
    "</body></html>\n",
    this->method_id, this->url_path, this->search_path
    ) ;
  
  HTTP_SOCKET_SEND( this->socket, content, strlen( content ), 0 );
    
  return 0;
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
