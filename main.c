/*
 *  main.c
 *  WebGui
 *
 *  Created by Otto Linnemann on 10.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#include "sockserver.h"

int main( int argc, char* argv[] )
{
  service_socket_loop();

  return 0;
}
