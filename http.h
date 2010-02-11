/*
 *  http.h
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#ifndef _HTTP_H
#define _HTTP_H


/* -- const definitions -----------------------------------------------------------*/


/*!
 *  The port where the HTML server is listen to
 */
#define HTML_SERVER_PORT    3000


/*!
 *  Server's indication in response texts
 */
#define HTML_SERVER_NAME    "Compact HTTP Server"


/*!
 *  This base directory of all HTML pages
 */
#define HTML_ROOT_DIR       "./"



/*!
 *  Maximum size of the received HTML commands ( for POST it can be big! )
 */
#define MAX_HTML_BUF_LEN    10000





/* -- public prototypes ----------------------------------------------------------*/


/*******************************************************************************
 * ProcessHttpRequest() 
 *                                                                         */ /*!
 * Possible HTTP Commands are 
 *    GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT.
 *
 * Not all of them are implemented currently.
 *                                                                              
 * Function parameters
 *     - socket: file descriptor for reading / writing http data
 *     - pbuf:   pointer to buffer of size MAX_HTML_BUF_LEN
 *
 * Returnparameter
 *     - R: 0 in case of success, otherwise error code
 * 
 *******************************************************************************/
int ProcessHttpRequest( int socket, char *pbuf );


#endif /* #ifndef _HTTP_H */
