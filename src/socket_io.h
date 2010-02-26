/*
 *  socket_io.h
 *
 *  abstract interface for accessing sockets
 *
 *  WebGui
 *
 *  Created by Otto Linnemann on 11.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#ifndef _SOCKET_IO
#define _SOCKET_IO


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


/*
 *  Abstraction from UNIX funciton send
 *  Can be replaced against any other function for sending bytes 
 *  to e.g. a stream descriptor
 */
#define HTTP_SOCKET_SEND( socket, buffer, len, flags )        send( (socket), (buffer), (len), (flags) ) 


/*
 *  Abstraction from UNIX funciton recv
 *  Can be replaced against any other function for sending bytes 
 *  to e.g. a stream descriptor
 */
#define HTTP_SOCKET_RECV( socket, buffer, len, flags )        recv( (socket), (buffer), (len), (flags) ) 


#endif
