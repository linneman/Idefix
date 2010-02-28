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
#include "cgi.h"

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
int service_socket_loop( const char* ht_root_dir, const int port ) 
{
  HTTP_OBJ            http_obj;           /* HTTP thread's object, we keep it on the stack here */
  HTTP_OBJ*           this = & http_obj;
  
  int                 create_socket, new_socket;
  socklen_t           addrlen;
  struct sockaddr_in  address;
  const int           y = 1;
  int                 error;
  
  /* intialize HTTP object */
  if( ( error = HTTP_ObjInit( this, HTML_SERVER_NAME, ht_root_dir, port ) ) != 0 )
  {
    fprintf( stderr, "Could not create buffer error!\n" );
    return error;
  }

  /* register CGI handlers (cgi.c) */
  if( ( error = RegisterCgiHandlers( this ) ) != 0 )
  {
    fprintf( stderr, "Could not register CGI handlers!\n" );
    return error;    
  }

  /* create socket */ 
  printf("Server Started\n");
  if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) > 0)
  {
    printf ( "Socket successfully created\n" );
  }
  else 
  {
    fprintf( stderr, "Could not create socket error!\n" );
    return create_socket;
  }


  setsockopt( create_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
  
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons ( this->port );
  
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
    printf("Waiting for client connections ...\n");
    new_socket = accept ( create_socket,
                         (struct sockaddr *) &address,
                         &addrlen );
    
    if (new_socket > 0)
    {
      printf ("Client (%s) is connected!\n",
              inet_ntoa (address.sin_addr));
    
      this->socket = new_socket;
      do 
      {
        error = HTTP_ProcessRequest( this );
        if( error < 0 )
        {
          fprintf( stderr, 
            "Error while rocessing of http request occured:\n\t%s!\n",
            HTTP_GetErrorMsg(error) 
            );
        }
      } while( !error && !this->disconnect );
      
      close (new_socket);
      }
  }
  
  close (create_socket);
  
  return EXIT_SUCCESS;
}
