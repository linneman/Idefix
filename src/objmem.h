/*
 *  objmem.h
 *  objmem, thread safe and fragementation free memory management
 *
 *  Created by Otto Linnemann on 12.02.10.
 *  Copyright 2010 GNU General Public Licence. All rights reserved.
 *
 *  @todo: testing of CHECK_STACK_FRAME, add CHECK_HEAP_FRAME, docu
 */
 
 
/* 
 Usage: Declaration of Object ObjX
 
  # "Object" declaration with local heap (long term) and stack (short term)
  # All methods must declare a "this" pointer to the corresponding object 
 
  typedef struct 
  {
    # public members
    int 		a;
    char*   name;
    
    #
    # declare local memory management struct members with 
    # macro OBJ_DELCARE_LOCAL_HEAP( size ) 
    #

    DECLARE( SizeOfObjX );

    # This expands to:
    #
    # char		objmem[  size  ];
    # char*   heapPtr;		# initialize to  objmem 
    # char*   stackPtr;		# initialize to objmem + size
    # char*   framePtrTab[ OBJ_MAX_FRAMES ];
    # int     framePtrIndex;
  } OBJ_X;



 # Initializer for ObjX
 #
 # additional memory is taken from heap because
 # it must stay valid during the full life time
 # of the object
 
 int ObjX_init( OBJ_X* this, const char* name )
 {
    int error;
 
    # initialize stack and frame pointers   
    OBJ_INIT(this);
    
    # initialize components
    this->a = 42;
    this->b = OBJ_HEAP_ALLOC( strlen( name ) );
    if( this->b == NULL )
      return -1;
      
    strcpy( this->b, name );
    # ...   
 }


 # Illustration of an ObjX's method
 #
 # it uses tempory memory form ObjX's stack

 int ObjX_generate_name_variants( OBJ_X* this, char* variantsPtrTab[], int *variantsTabSize )
 {
    int len;
 
    # create first name variant
    len = strlen( this->name ); 
    variantsPtrTab[0] = OBJ_STACK_MALLOC( len );
    if( variantsPtrTab[0] == NULL )
      return -1;
      
    strcpy( variantsPtrTab[0], name );
    strlower( variantsPtrTab[0] );
    
    # create second variant name
    ...
    ...
    strupper( variantPtrTab[1] );
    
    *variantsTabSize = 2;
    
    return 0;
 }

 main() 
 {
    #define MAX_VARIANTS   2
    char* variantsPtrTab[ MAX_VARIANTS ];
    
 
    # Allocate memory for object ObjX on system's heap
    
    OBJ_X   *objXPtr = malloc( sizeof( OBJ_X );
    # or on stack
    OBJ_X   objX;
    OBJ_X   objXPtr = &objX;
    
    
    # Initialize object ObjX
    if( ObjX_init( objXPtr, "Test" ) != 0 )
    {
      # error
    }
    
    # Do something with the Object
    # here we invoke a method which requires temporary memory
    # which we take from the Object's stack
    
    # This allocates a new stack frame
    # It remembers the local stack pointer for later restauration
    OBJ_ALLOC_STACK_FRAME( objXPtr );
    
    if( ! ObjX_generate_name_variants( this, variantsPtrTab, MAX_VARIANTS ) )
    {
      # do something with the variants
    }

    if( ! ObjX_generate_more_name_variants( this, variantsPtrTab, MAX_VARIANTS ) )
    {
      # do something with the variants
    }
    
    # This restores the last stack pointer
    OBJ_RELEASE_STACK_FRAME( objXPtr );
  }  
*/

#ifndef _OBJMEM_H
#define _OBJMEM_H

/* === includes ========================================================== */

#include <stdlib.h>
#include <assert.h>


/* === Macros Definitions ================================================ */


/*!
 *  Enable memory check when _DEBUG is defined
 */
#ifdef _DEBUG
  #define _OBJ_MEM_CHK
#endif


/*!
 *  Maximum number of allowed stack frames
 */
#define OBJ_MAX_FRAMES                        10


/*!
 *  In Debug mode each address is remembered
 *  for later access violation check. This
 *  definition defines the number of elements
 *  in the used address tables
 */
#define OBJ_DEBUG_ADDR_TABLE_SIZE             1000


/*
 *  Pre-/Postfix chunk characteristics for
 *  debugging purposes
 */
#define OBJ_PREFIX_LEN                         10
#define OBJ_POSTFIX_LEN                        10
#define OBJ_PREFIX_CHAR                        'X'
#define OBJ_POSTFIX_CHAR                       'Y'


/*
 *  address,size pair for debugging
 */
typedef struct  {
  char*   addr;
  size_t  size;
  } OBJ_MEM_CHK_DESC;


/*
 *  macro for insertion of local memory management
 *  struct components ( heap and stack )
 */
#define _OBJ_DELCARE_LOCAL_HEAP( size )                                   \
    char		objmem[ ( size ) ];                                           \
    char*   heapPtr;                                                      \
    char*   stackPtr;                                                     \
    char*   framePtrTab[ OBJ_MAX_FRAMES ];                                \
    int     framePtrIndex;
    
#ifdef _OBJ_MEM_CHK
  /* debug implementation */
  #define OBJ_DELCARE_LOCAL_HEAP( size )                                  \
    _OBJ_DELCARE_LOCAL_HEAP( size );                                      \
    OBJ_MEM_CHK_DESC    stackDescTable[ OBJ_DEBUG_ADDR_TABLE_SIZE ];      \
    OBJ_MEM_CHK_DESC    heapDescTable[ OBJ_DEBUG_ADDR_TABLE_SIZE ];       \
    int                 stackAddrTableTop;                                \
    int                 heapAddrTableTop;                                 
#else
  /* release implementation */
  #define OBJ_DELCARE_LOCAL_HEAP( size )                                  \
    _OBJ_DELCARE_LOCAL_HEAP( size)

#endif  
    


/*
 *  Protoyp for internal usage, do not instantiate!
 */
typedef struct
{
  OBJ_DELCARE_LOCAL_HEAP( 4 );
} OBJ_PROTOTYP;

/* internal helper for struct member access */
#define OBJ_ACCESS( m )   ( ((OBJ_PROTOTYP *)this)->m )


/*
 *  OBJ_ALLOC_BYTES_FORM_STACK(bytes)
 *  OBJ_ALLOC_BYTES_FORM_STACK(bytes)
 *
 *  Allocate the given number of bytes from heap or stack
 *  These macros should not be invoked from outside because
 *  the memory needs to be aligned in the very most cases.
 *  Use OBJ_HEAP_ALLOC() and OBJ_STACK_ALLOC() instead.
 */
#define OBJ_ALLOC_BYTES_FORM_STACK( bytes )    ( (void *)( ((this->stackPtr-=(bytes)) > this->heapPtr ) ? (this->stackPtr) : (this->stackPtr+=(bytes), NULL) ) )
#define OBJ_ALLOC_BYTES_FORM_HEAP( bytes )     ( (void *)( ((this->heapPtr+=(bytes)) < this->stackPtr ) ? (this->heapPtr-(bytes)) : (this->heapPtr-=(bytes), NULL) ) )


/*
 *  OBJ_ADDR_ALIGN_STACK(size)
 *  OBJ_ADDR_ALIGN_HEAP(size)
 *
 *  Utility function for changing stack / heap pointers to the
 *  next aligned address.
 */
#define OBJ_ADDR_ALIGN_OFFSET(ptr, div)        ( (unsigned long)(ptr) % (div) )

#define OBJ_ADDR_ALIGN_STACK(size)             ( OBJ_ALLOC_BYTES_FORM_STACK( OBJ_ADDR_ALIGN_OFFSET( this->stackPtr, size ) ) )
#define OBJ_ADDR_ALIGN_HEAP(size)              ( OBJ_ALLOC_BYTES_FORM_HEAP( OBJ_ADDR_ALIGN_OFFSET( this->heapPtr, size ) ) )


/*!
 *  OBJ_HEAP_ALLOC( bytes )
 *  OBJ_STACK_ALLOC( bytes )
 *
 *  Most platforms require a pointer alignment to multiple of 4 bytes for
 *  larger datatypes ( float, pointers, etc). These macros align the returned 
 *  memory address for required aligment.
 */
#define _OBJ_HEAP_ALLOC(bytes)                 ( (OBJ_ALLOC_BYTES_FORM_HEAP(bytes) != NULL)  ?  OBJ_ADDR_ALIGN_HEAP(4) : NULL )
#define _OBJ_STACK_ALLOC(bytes)                ( (OBJ_ALLOC_BYTES_FORM_STACK(bytes) != NULL)  ?  OBJ_ADDR_ALIGN_STACK(4) : NULL )

/*
 *    Helper function for initializing memory frame 
 */
char*   _OBJ_init_chk_frame( char* addr, const int size, OBJ_MEM_CHK_DESC descTable[], int tableIndex );

#ifdef _OBJ_MEM_CHK
#define OBJ_STACK_ALLOC(bytes)                                                                            \
  (                                                                                                       \
    ( _OBJ_STACK_ALLOC( ( bytes ) + OBJ_PREFIX_LEN + OBJ_POSTFIX_LEN ) != NULL ) ?                        \
    _OBJ_init_chk_frame( this->stackPtr, bytes, this->stackDescTable, this->stackAddrTableTop++ ) :       \
    NULL                                                                                                  \
  )
#else
#define OBJ_STACK_ALLOC(bytes)  _OBJ_STACK_ALLOC(bytes)
#endif


#ifdef _OBJ_MEM_CHK
#define OBJ_HEAP_ALLOC(bytes)                                                                             \
  (                                                                                                       \
    ( _OBJ_HEAP_ALLOC( ( bytes ) + OBJ_PREFIX_LEN + OBJ_POSTFIX_LEN ) != NULL ) ?                         \
    _OBJ_init_chk_frame( this->heapPtr - ( bytes ) - OBJ_PREFIX_LEN - OBJ_POSTFIX_LEN, bytes, this->heapDescTable,this->heapAddrTableTop++ ) : \
    NULL                                                                                                  \
  )
#else
#define OBJ_HEAP_ALLOC(bytes)  _OBJ_HEAP_ALLOC(bytes)
#endif

/*!
 *  Allocate a new stack frame for a given object for later release 
 */
#define OBJ_ALLOC_STACK_FRAME( this )          { assert( this->framePtrIndex < OBJ_MAX_FRAMES ); this->framePtrTab[ this->framePtrIndex++ ] = this->stackPtr;  }


/*!
 * Release the last stack from the given object 
 */
#define _OBJ_RELEASE_STACK_FRAME( this )        { assert( this->framePtrIndex > 0 ); this->stackPtr = this->framePtrTab[ --this->framePtrIndex ] ;  }

#ifdef _OBJ_MEM_CHK
#define OBJ_RELEASE_STACK_FRAME(this)                                 \
{                                                                     \
    _OBJ_RELEASE_STACK_FRAME(this);                                   \
    this->stackAddrTableTop = this->framePtrIndex;                    \
}
#else
#define OBJ_RELEASE_STACK_FRAME(this)   _OBJ_RELEASE_STACK_FRAME(this)
#endif


/*!
 *  Check the given object's stack frame
 */
#define _CHECK_STACK( this )                                          \
{                                                                     \
  int   i, j, error = 0;                                              \
  char* checked_frame;                                                \
                                                                      \
  for( i=0; i < this->stackAddrTableTop; ++ i )                       \
  {                                                                   \
    checked_frame = this->stackDescTable[i].addr;                     \
                                                                      \
    for( j=0; j < OBJ_PREFIX_LEN; ++j )                               \
    {                                                                 \
      if( checked_frame[j] != OBJ_PREFIX_CHAR )                       \
      {                                                               \
        error |= 0x01;                                                \
        break;                                                        \
      }                                                               \
    }                                                                 \
                                                                      \
    checked_frame += OBJ_PREFIX_LEN + this->stackDescTable[i].size;   \
    for( j=0; j < OBJ_PREFIX_LEN; ++j )                               \
    {                                                                 \
      if( checked_frame[j] != OBJ_POSTFIX_CHAR )                      \
      {                                                               \
        error |= 0x02;                                                \
        break;                                                        \
      }                                                               \
    }                                                                 \
                                                                      \
    if( error & 0x01)                                                 \
      fprintf( stderr, "Prefix access violation error in allocated stack block at %lx\n", (unsigned long) checked_frame + OBJ_PREFIX_LEN );  \
                                                                      \
    if( error & 0x02)                                                 \
      fprintf( stderr, "Postfix access violation error in allocated stack block at %lx\n", (unsigned long) checked_frame + OBJ_PREFIX_LEN );  \
  }                                                                   \
}

/*!
 *  Check the given object's heap
 */
#define _CHECK_HEAP( this )                                           \
{                                                                     \
  int   i, j, error = 0;                                              \
  char* checked_frame;                                                \
                                                                      \
  for( i=0; i < this->heapAddrTableTop; ++ i )                        \
  {                                                                   \
    checked_frame = this->heapDescTable[i].addr;                      \
                                                                      \
    for( j=0; j < OBJ_PREFIX_LEN; ++j )                               \
    {                                                                 \
      if( checked_frame[j] != OBJ_PREFIX_CHAR )                       \
      {                                                               \
        error |= 0x01;                                                \
        break;                                                        \
      }                                                               \
    }                                                                 \
                                                                      \
    checked_frame += OBJ_PREFIX_LEN + this->heapDescTable[i].size;    \
    for( j=0; j < OBJ_PREFIX_LEN; ++j )                               \
    {                                                                 \
      if( checked_frame[j] != OBJ_POSTFIX_CHAR )                      \
      {                                                               \
        error |= 0x02;                                                \
        break;                                                        \
      }                                                               \
    }                                                                 \
                                                                      \
    if( error & 0x01)                                                 \
      fprintf( stderr, "Prefix access violation error in allocated heap block at %lx\n", (unsigned long) checked_frame + OBJ_PREFIX_LEN );  \
                                                                      \
    if( error & 0x02)                                                 \
      fprintf( stderr, "Postfix access violation error in allocated heap block at %lx\n", (unsigned long) checked_frame + OBJ_PREFIX_LEN );  \
  }                                                                   \
}

/*!
 *  Complete memory check for given object
 */
#ifdef _OBJ_MEM_CHK
#define OBJ_CHECK( this )                                             \
{                                                                     \
    _CHECK_HEAP( this );                                              \
    _CHECK_STACK( this );                                             \
}
#else
#define OBJ_CHECK( this )
#endif

/*!
 *  OBJ_INIT(this)
 *
 *  Initializes heap and stack memory for a given object
 *
 *  Function parameters
 *    - this:         pointer to C-structure representing the object
 *
 */
 
#define _OBJ_INIT(this)                                               \
{                                                                     \
  this->heapPtr       = this->objmem;                                 \
  this->stackPtr      = this->objmem + sizeof( this->objmem );        \
  this->framePtrIndex = 0;                                            \
}

#ifdef _OBJ_MEM_CHK
  /* debug implementation */
  #define OBJ_INIT(this)                                              \
  {                                                                   \
    _OBJ_INIT(this);                                                  \
    this->stackAddrTableTop = 0;                                      \
    this->heapAddrTableTop = 0;                                       \
  }
#else
  /* release implementation */
  #define OBJ_INIT(this)                                              \
     _OBJ_INIT(this); 
#endif




#endif /* #ifndef _OBJMEM_H */
