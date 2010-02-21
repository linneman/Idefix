/*
 *  objmem.c
 *  objmem
 *
 *  Created by Otto Linnemann on 14.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 */

#include <string.h>
#include "objmem.h"


/*
 *    Helper function for initializing memory frame with pre- and
 *    post fix section instrumented for access violation detection
 *    
 *    -descTable:   pointer table to memory descriptors
 *    -tableIndex:  index of table entry to be initialized
 *    -size:        size of the memory payload
 *    -handle:      pointer address where addr is written
 * 
 *    -returns:     payload address
 */
 
char*   _OBJ_init_chk_frame( char* addr, const int size, OBJ_MEM_CHK_DESC descTable[], int tableIndex )
{
  OBJ_MEM_CHK_DESC* descPtr;
  
  assert( tableIndex < OBJ_DEBUG_ADDR_TABLE_SIZE );
  descPtr = & descTable[tableIndex];

  memset( addr, OBJ_PREFIX_CHAR, OBJ_PREFIX_LEN );
  memset( addr + OBJ_PREFIX_LEN + size, OBJ_POSTFIX_CHAR, OBJ_POSTFIX_LEN );
  
  descPtr->addr = addr;
  descPtr->size = size;
  
  return( addr + OBJ_PREFIX_LEN );
}
