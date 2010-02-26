/*
 *  sockserver.h
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#ifndef _SOCKSERVER_H
#define _SOCKSERVER_H

/* -- public prototypes ----------------------------------------------------------*/


/*******************************************************************************
 * service_socket_loop() 
 *                                                                         */ /*!
 * Reads incoming HTTP requests from socket, and reacts appropriately
 *
 * Function parameters
 *     - ht_root_dir: root directory for static web content 
 *     - port:        port the server is listening to
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int service_socket_loop( const char* ht_root_dir, const int port );


#endif /* #define _SOCKSERVER_H */
