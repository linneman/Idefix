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

/*!
 *  Time out when waiting for incoming data
 */
#define HTTP_RCV_TIME_OUT           3


/*
 *  Abstraction from UNIX funciton send
 *  Can be replaced against any other function for sending bytes 
 *  to e.g. a stream descriptor
 */
#define HTTP_SOCKET_SEND( socket, buffer, len )        http_send_all( (socket), (buffer), (len), 0 ) 


/*
 *  Abstraction from UNIX funciton recv
 *  Can be replaced against any other function for sending bytes 
 *  to e.g. a stream descriptor
 */
#define HTTP_SOCKET_RECV( socket, buffer, len )        http_recv_timedout( (socket), (buffer), (len), 0, HTTP_RCV_TIME_OUT ) 




/*******************************************************************************
 * http_send_all() 
 *                                                                         */ /*!
 * adapter function for sending out ALL bytes within a buffer 
 *                                                                              
 * Function parameters
 *     - socket:    socket to send to
 *     - buffer:    buffer with bytes to send
 *     - length:    number of bytes to send
 *     - flags:     transfered to original UNIX send call
 *    
 * Returnparameter
 *     - R:         number of successfully transmitted bytes
 * 
 *******************************************************************************/
long http_send_all( int socket, const void* buffer, long length, int flags );



/*******************************************************************************
 * http_recv_timedout() 
 *                                                                         */ /*!
 * adapter function for receving data from socket with time out 
 *                                                                              
 * Function parameters
 *     - socket:    socket to send to
 *     - buffer:    buffer where received bytes are stored to
 *     - length:    number of bytes to receive
 *     - flags:     transfered to original UNIX send call
 *     - timeout:   timeout in seconds
 *    
 * Returnparameter
 *     - R:         number of successfully received bytes or 
 *                  -2 in case of timeout
 *                  -1 in case of other error
 * 
 *******************************************************************************/
long http_recv_timedout( int socket, void* buffer, long length, 
  int flags, int timeout);




#endif
