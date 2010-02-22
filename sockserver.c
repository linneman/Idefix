/*
 *  sockserver.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

/* -- includes -------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "http.h"
#include "socket_io.h"
#include "objmem.h"


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
int service_socket_loop(void) 
{
  HTTP_OBJ            http_obj;           /* HTTP thread's object, we keep it on the stack here */
  HTTP_OBJ*           this = & http_obj;
  
  int                 create_socket, new_socket;
  socklen_t           addrlen;
  ssize_t             size;
  struct sockaddr_in  address;
  const int           y = 1;
  int                 error;
  
  if( HTTP_ObjInit( this, HTML_SERVER_NAME ) != 0 )
  {
    fprintf( stderr, "Could not create buffer error!\n" );
    return -1;
  }
  
  printf("Server Started ...");
  if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) > 0)
  {
    printf ( "Socket successfully created\n" );
  }
  else 
  {
    fprintf( stderr, "Could not create socket error!\n" );
    return -1;
  }

  
  setsockopt( create_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
  
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons ( HTML_SERVER_PORT );
  
  if ( bind( create_socket,
            (struct sockaddr *) &address,
            sizeof (address)) != 0 ) 
  {
    fprintf( stderr, "The port %d is already in use!\n", address.sin_port );
    return -1;
  }
  
  listen(create_socket, 5);
  addrlen = sizeof( struct sockaddr_in );
  
  while (1) 
  {
    new_socket = accept ( create_socket,
                         (struct sockaddr *) &address,
                         &addrlen );
    
    if (new_socket > 0)
    {
      printf ("Client (%s) is connected ...\n",
              inet_ntoa (address.sin_addr));
    
      this->socket = new_socket;
      do 
      {
        /* Remember stack frame for later restauration */
        OBJ_ALLOC_STACK_FRAME( this );

      
        size = HTTP_SOCKET_RECV( new_socket, this->rcvbuf, MAX_HTML_BUF_LEN - 1, 0 );
        if( size > 0 )
          this->rcvbuf[size] = '\0';
        
        printf( "received http request!\n");
        // printf( "data sent by client: %s\n", this->rcvbuf );
        
        error = HTTP_ProcessRequest( this );
        if( error < 0 )
        {
          fprintf( stderr, "error while rocessing of http request occured!\n");
        }
        
        
        /*
        printf( "Message to send: " );
        fgets( buffer, MAX_HTML_BUF_LEN, stdin );
        send( new_socket, buffer, strlen (buffer), 0 );
        // flush( new_socket );
        */ 
        
        
        /* Check object's memory and release stack frame */
        OBJ_CHECK( this );
        OBJ_RELEASE_STACK_FRAME( this );
         
      } while( ! error );
      
      close (new_socket);
      }
  }
  
  close (create_socket);
  
  return EXIT_SUCCESS;
}
