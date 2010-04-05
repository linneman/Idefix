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
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "cgi.h"

#define MIN(x,y) ( (x)<(y) ? (x) : (y) )

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
  int   len1, len2;
  int   bytes_written;
  int   error;
  char  content1[512];
  char  content2[1024];
  
  this->mimetyp = HTTP_MIME_TEXT_HTML;
  
  snprintf( content1, sizeof(content1),
    "<html><body>\n"
    "<h1>This Content has been created dynamically!</h1>\n"
    "<p>************************** TestCgiHandler triggered! **************************</p>\n"
    "<p>*** method_id : %d</p>\n"
    "<p>*** url_path : %s</p>\n"
    "<p>*** search_path : %s</p>\n"
    "<p>*******************************************************************************</p><br />\n\n",
    this->method_id, this->url_path, this->search_path
    );
  len1 = strlen( content1 );
  
  /* ensure proper EOS character, avoid buffer overflow */
  this->body_len = MIN( this->body_len, 1000 );
  this->body_ptr[this->body_len] = '\0';
  
  if( this->method_id == HTTP_POST_ID )
  {
    snprintf( content2, sizeof(content2), "<p>POST:</p><code>%s</code></body></html>\n", this->body_ptr );
    len2 = strlen( content2 );
  }
  else 
  {
    content2[0] = '\0'; 
    len2 = 0;
  }

  printf("=====> RECEIVED POST DATA =====>\n");
  printf("%s\n", this->body_ptr );
  printf("<==============================<\n");
  
  this->content_len = len1 + len2;
  
  error = HTTP_SendHeader( this, HTTP_ACK_OK );
  if( error == HTTP_OK )
  {
    /* write header/content separation line */
    bytes_written = HTTP_SOCKET_SEND( this->socket, "\r\n\r\n", 4 );
    bytes_written += HTTP_SOCKET_SEND( this->socket, content1, len1 );
    bytes_written += HTTP_SOCKET_SEND( this->socket, content2, len2 );
  
    if( bytes_written != this->content_len + 4 )
    {
      error = HTTP_CGI_EXEC_ERROR;
    }
  }
  
  return error;
}

/*!
 *  CGI Handler for retrieving Directory within JSON format
 *
 *  Function parameters
 *     - this:      pointer to HTTP Object
 *    
 *  Returnparameter
 *     - R: 0 in case of success, otherwise error code
 */
int DirCgiHandler( struct _HTTP_OBJ* this )
{
  int   bytes_written;
  int   error = HTTP_OK;

  char  content[1024];
  int   content_len = 0, len;
  char  directory[300], filename[300], entrybuf[300];

  DIR*            dp;
  struct dirent*  ep;
  struct stat     s;
  int             filesize;

  this->mimetyp = HTTP_MIME_APPLICATION_JSON;
  
  /* ensure proper EOS character, avoid buffer overflow */
  this->body_len = MIN( this->body_len, 1000 );
  this->body_ptr[this->body_len] = '\0';
  
  /* generate directory in JSON format */
  
  /* insert JSON array header */
  sprintf( content, "[" );

  /* generete intial full path name for directory to be retrieved */
  strcpy( directory, this->ht_root_dir );
  strcat( directory, this->search_path );
  strcat( directory, "/");
  
  /* open directory and iterate through the entries */
  dp = opendir( directory );
  if( dp != NULL )
  {
    while( error == HTTP_OK && (ep = readdir (dp)) != NULL )
    {
      switch(ep->d_type) {
        case DT_REG:
          /* printf("normal file : "); */

          /* determine file size */
          
          /* generete intial full path name for current entry to be retrieved */
          strcpy( filename, directory );
          strcat( filename, ep->d_name );

          if (stat(filename, &s) != -1)
          {
            filesize = s.st_size;
          }
          else
          {
            filesize = 0;
          }

          sprintf( entrybuf,
            "{\"filename\":\"%s\", \"filtetyp\":\"file\", \"size\":\"%d\"},",
            ep->d_name, filesize);
            
          if( content_len + (len = strlen( entrybuf )) > sizeof(content) ) {
            error = HTTP_BUFFER_OVERRUN;
          }
          else {
            strcat( content, entrybuf );
            content_len += len;
          }
          break;

        case DT_DIR:
          if( strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, ".." ) == 0 )
            continue;
        
          sprintf( entrybuf,
            "{\"filename\":\"%s\", \"filtetyp\":\"dir\", \"size\":\"%d\"},",
            ep->d_name, filesize); 
            
          if( content_len + (len = strlen( entrybuf )) > sizeof(content) ) {
            error = HTTP_BUFFER_OVERRUN;
          }
          else {
            strcat( content, entrybuf );
            content_len += len;
          }
          break;

            /* ... */
        case DT_UNKNOWN :
          /* printf("???       : "); */
          break;

        default:
          break;
      }
    }
    closedir (dp);
  }
  else
  {
    error = HTTP_CGI_EXEC_ERROR;
  }

  /* insert XML footer */
  sprintf( content +strlen(content)-1, "]\n");
  content_len += 1;
  
  this->content_len = content_len;
  
  error = HTTP_SendHeader( this, HTTP_ACK_OK );
  if( error == HTTP_OK )
  {
    /* write header/content separation line */
    bytes_written = HTTP_SOCKET_SEND( this->socket, "\r\n\r\n", 4 );
    bytes_written += HTTP_SOCKET_SEND( this->socket, content, content_len );
  
    if( bytes_written != this->content_len + 4 )
    {
      error = HTTP_CGI_EXEC_ERROR;
    }
  }
  
  return error;
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
    error = HTTP_AddCgiHanlder( this, DirCgiHandler, HTTP_GET_ID, "dir" );

  if( ! error )
    error = HTTP_AddCgiHanlder( this, TestCgiHandler, HTTP_GET_ID, "linnemann" );
  
  if( ! error ) 
    error = HTTP_AddCgiHanlder( this, TestCgiHandler, HTTP_POST_ID, "form" );

  
  return error;
}
