/*
 *  cgi.h
 *  WebGui
 *
 *  Created by Otto Linnemann on 24.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#ifndef _CGI_H
#define _CGI_H

#include <stdio.h>
#include "socket_io.h"
#include "http.h"


/*!
 *  Sample common gateway interface (CGI)
 *
 *  Function parameters
 *     - this:      pointer to HTTP Object
 *    
 *  Returnparameter
 *     - R: 0 in case of success, otherwise error code
 */
int TestCgiHandler( struct _HTTP_OBJ* this );


/*!
 *  Sample registration of CGI handlers
 *
 *  Function parameters
 *     - this:      pointer to HTTP Object
 *    
 *  Returnparameter
 *     - R: 0 in case of success, otherwise error code
 */
int RegisterCgiHandlers( struct _HTTP_OBJ* this );


#endif /* #ifndef _CGI_H */