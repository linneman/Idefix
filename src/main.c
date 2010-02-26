/*
 *  main.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>   /* for checking root directory existence */
#include "sockserver.h"
#include "http.h"       /* for version information */

#define APP_NAME  "idefix"

/*!
 *  The port where the HTML server is listen to
 */
#define HTML_SERVER_DEFAULT_PORT    80


/*!
 *  This base directory of all HTML pages
 */
#define HTML_DEFAULT_ROOT_DIR       "./"


/*!
 *  Maximum length of a file path
 */
#define HTML_MAX_PATH_LEN           255


/*!
 *  writes help screen to standard out
 */
static void help( void )
{
  printf("%s: Fast and simple HTTP server for embedded applications \n\n", APP_NAME);
  printf("Invocation: %s [ options ]\n\n", APP_NAME );
  printf("Options:\n");
  printf("--port\n-p\n");
  printf("\tSpecifies the port the server is connected to. Port 80 is used\n");
  printf("\tin case nothing is specified.\n\n");
  printf("--rootdir\n-r\n");
  printf("\tSpecifies the root directory where static files are searched\n");
  printf("\tfrom. For empty URL's index.html is retrieved per default.\n\n");
  printf("--version\n-v\n");
  printf("\tPrints version information.\n\n");
  printf("\t--help\n-h\n");
  printf("\tThis help screen. For more informaiton refer also to the man page.\n\n");
  printf("(C) GNU-General Public Licence, written by Otto Linnemann, 02/2010\n"); 
}

/*!
 *  writes version to standard out
 */
static void version( void )
{
  long version = HTTP_GetServerVersion();

  printf("Webserver-Version %d.%d.%d\n", 
    (int) ( ( version >> 16 ) & 0xFF ), 
    (int) ( ( version >>  8 ) & 0xFF ), 
    (int)   ( version & 0xFF )
    );
}


int main( int argc, char* argv[] )
{
  char          root_dir[HTML_MAX_PATH_LEN];
  int           port     = HTML_SERVER_DEFAULT_PORT;
  int           optindex, optchar, error = 0;
  struct stat   root_dir_stat;
  const struct  option long_options[] = 
  {
    { "help",     no_argument,        NULL,   'h' },
    { "version",  no_argument,        NULL,   'v' },
    { "rootdir",  required_argument,  NULL,   'r' },
    { "port",     required_argument,  NULL,   'p' },
    NULL
  };


  /* ignore sigpipe */
  signal( SIGPIPE, SIG_IGN );

  /* setup options */
  strcpy( root_dir, HTML_DEFAULT_ROOT_DIR );
  while( ( optchar = getopt_long( argc, argv, "hvr:p:", long_options, &optindex ) ) != -1 )
  {
    switch( optchar )
    {
      case 'h':
        help();
        error = -1;
        break;
        
      case 'v':
        version();
        error = -1;
        break;
        
      case 'p':
        port = atoi( optarg );
        if( port < 1 || port > 65535 )
        {
          fprintf( stderr, "wrong port specified error!\n");
          return(-1);
        }
        break;
      
      case 'r':
        strncpy( root_dir, optarg, HTML_MAX_PATH_LEN );
        root_dir[HTML_MAX_PATH_LEN-1] = '\0';
        error = stat( root_dir, & root_dir_stat );
        if( !error )
        {
          if( ! ( root_dir_stat.st_mode & S_IFDIR ) )
          {
            error = -1;
          }
        }
        if( error ) 
          fprintf( stderr, "html document root directory %s does not exist or cannot be accessed error!\n", root_dir );
        break;
        
      default:
        fprintf( stderr, "input argument error!\n");
        return -1;
    }
  }

  if( !error )
  {
    /* prints basic configuration info */
    printf("Starting Webserver at port %d and root directory %s ...\n\n", port, root_dir );
    
    /* start single thread server */
    error = service_socket_loop( root_dir, port );
  }

  return error;
}
