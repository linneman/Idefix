/*
 *  socket_io.c
 *  idefix
 *
 *  Created by Otto Linnemann on 04.03.10.
 *  Copyright 2010 ScanSoft Aachen. All rights reserved.
 *
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


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
ssize_t http_send_all( int socket, const void* buffer, size_t length, int flags ) 
{
    int total = 0;          /* how many bytes we've sent */
    int bytesleft = length; /* how many we have left to send */
    int n;

    while(total < length) {
        n = send( socket, buffer + total, bytesleft, flags );
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return total; 
}




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

ssize_t http_recv_timedout( int socket, void* buffer, size_t length, 
  int flags, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    /* set up the file descriptor set */
    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    /* set up the struct timeval for the timeout */
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    /* wait until timeout or data received */
    n = select( socket + 1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    /* data must be here, so do a normal recv() */
    return recv( socket, buffer, length, 0);
}

