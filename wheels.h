#if defined(MY_WHEEELS_H)
  #error "wheels should be included once"
#endif
#define MY_WHEEELS_H

#if defined(MY_BIGINT_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_BIGINT_C (1)
  #include "bigint.h"
#endif
#undef MY_BIGINT_H

#if defined(MY_PRINTER_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_PRINTER_C (1)
  #include "print.h"
#endif
#undef MY_PRINTER_C

#if defined(SHMAP_H) || defined(WHEELS_INCLUDE_ALL)
  #define SHMAP_C (1)
  #include "shmap.h"
#endif
#undef SHMAP_C

#if defined(STRING_LIST_H) || defined(WHEELS_INCLUDE_ALL)
  #define STRING_LIST_C (1)
  #include "stringList.h"
#endif
#undef STRING_LIST_C

#if defined(VASON_BUILDER_H) || defined(WHEELS_INCLUDE_ALL)
  #define VASON_BUILDER_C (1)
  #include "vason_tree.h"
#endif
#undef VASON_BUILDER_H

#if defined(VASON_PARSER_H) || defined(WHEELS_INCLUDE_ALL)
  #define VASON_PARSER_C (1)
  #include "vason_arr.h"
#endif
#undef VASON_PARSER_C

#if defined(MY_DEBUG_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_DEBUG_ALLOCATOR_C (1)
  #include "debugallocator.h"
#endif
#undef MY_DEBUG_ALLOCATOR_C

#if defined(OMAP_H) || defined(WHEELS_INCLUDE_ALL)
  #define OMAP_C (1)
  #include "omap.h"
#endif
#undef OMAP_C

#if defined(HMAP_H) || defined(WHEELS_INCLUDE_ALL)
  #define HMAP_C (1)
  #include "hhmap.h"
#endif
#undef HMAP_C

#if defined(MY_LIST_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_LIST_C (1)
  #include "mylist.h"
#endif
#undef MY_LIST_C

#if defined(ARENA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
#endif
#undef ARENA_ALLOCATOR_C

#if defined(FBA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define FBA_ALLOCATOR_C (1)
  #include "fbaAllocator.h"
#endif
#undef FBA_ALLOCATOR_H

#if defined(MY_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_ALLOCATOR_C (1)
  #include "allocator.h"
#endif
#undef MY_ALLOCATOR_C

// (  #ined )ifdef FPTR_H || defined (WHEELS_INCLUDE_ALL)
//   #define FPTR_C (1)
//   #include "fptr.h"
// #endif
//   #undef FPTR_C

#if defined(ASSERTMESSAGE_H) || defined(WHEELS_INCLUDE_ALL)
  #define ASSERTMESSAGE_C (1)
  #include "assertMessage.h"
#endif
#undef ASSERTMESSAGE_C
