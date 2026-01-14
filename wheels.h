// impl order matters

#ifdef MY_PRINTER_H
  #define MY_PRINTER_C (1)
  #include "print.h"
#endif

// TODO
// ordered map, based on stringlist
// dynamic key and value sizes
// index based linked list
#ifdef OMAP_H
  #define OMAP_C (1)
  #include "omap.h"
#endif

#ifdef HHMAP_H
  #define HHMAP_C (1)
  #include "hhmap.h"
#endif

// TODO
#ifdef STRING_LIST_H
  #define STRING_LIST_C (1)
  #include "stringList.h"
#endif

#ifdef VASON_PARSER_H
  #define VASON_PARSER_C (1)
  #include "vason.h"
#endif

#ifdef MY_LIST_H
  #define MY_LIST_C (1)
  #include "my-list.h"
#endif

#ifdef ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
#endif

#ifdef MY_ALLOCATOR_H
  #define MY_ALLOCATOR_C (1)
  #include "allocator.h"
#endif

#ifdef FPTR_H
  #define FPTR_C (1)
  #include "fptr.h"
#endif

#ifdef ASSERTMESSAGE_H
  #define ASSERTMESSAGE_C (1)
  #include "assertMessage.h"
#endif
