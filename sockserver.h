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
 *     - currently none
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int service_socket_loop(void);


#endif /* #define _SOCKSERVER_H */
