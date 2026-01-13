// impl order matters

#ifdef HMAP_ALLOCATOR_H
  #define HMAP_ALLOCATOR_C (1)
  #include "hmap_arena.h"
#endif

#ifdef KMLM_H
  #define KMLM_C (1)
  #include "kmlM.h"
#endif

#ifdef MY_PRINTER_H
  #define MY_PRINTER_C (1)
  #include "print.h"
#endif

// hash map, based on stringlist
// dynamic key and value sizes
// index based linked list
#ifdef HMAP_H
  #define HMAP_C (1)
  #include "hmap.h"
#endif

// ordered map, based on stringlist
// dynamic key and value sizes
// index based linked list
#ifdef OMAP_H
  #define OMAP_C (1)
  #include "omap.h"
#endif

// hashmap with separate chaining
#ifdef HHMAP_H
  #define HHMAP_C (1)
  #include "hhmap.h"
#endif

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
