#if defined(MY_WHEEELS_H)
  #error "wheels should be included once"
#endif
#define MY_WHEEELS_H

#ifdef MY_PRINTER_H
  #define MY_PRINTER_C (1)
  #include "print.h"
#endif
#undef MY_PRINTER_C

#ifdef STRING_LIST_H
  #define STRING_LIST_C (1)
  #include "stringList.h"
#endif
#undef STRING_LIST_C

#ifdef VASON_PARSER_H
  #define VASON_PARSER_C (1)
  #include "vason_arr.h"
#endif
#undef VASON_PARSER_C

#ifdef MY_DEBUG_ALLOCATOR_H
  #define MY_DEBUG_ALLOCATOR_C (1)
  #include "debugallocator.h"
#endif
#undef MY_DEBUG_ALLOCATOR_C

#ifdef OMAP_H
  #define OMAP_C (1)
  #include "omap.h"
#endif
#undef OMAP_C

#ifdef HMAP_H
  #define HMAP_C (1)
  #include "hhmap.h"
#endif
#undef HMAP_C

#ifdef MY_LIST_H
  #define MY_LIST_C (1)
  #include "my-list.h"
#endif
#undef MY_LIST_C

#ifdef ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
#endif
#undef ARENA_ALLOCATOR_C

#ifdef MY_ALLOCATOR_H
  #define MY_ALLOCATOR_C (1)
  #include "allocator.h"
#endif
#undef MY_ALLOCATOR_C

// #ifdef FPTR_H
//   #define FPTR_C (1)
//   #include "fptr.h"
// #endif
//   #undef FPTR_C

#ifdef ASSERTMESSAGE_H
  #define ASSERTMESSAGE_C (1)
  #include "assertMessage.h"
#endif
#undef ASSERTMESSAGE_C
