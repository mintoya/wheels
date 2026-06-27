#if defined(MY_WHEELS_H)
  #error "wheels should be included once"
#endif
#define MY_WHEELS_H

#if defined(MY_BIGINT_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_BIGINT_C (1)
  #include "bigint.h"
#endif
#undef MY_BIGINT_C

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

// #if defined(MY_THREAD_MACORS_H) || defined(WHEELS_INCLUDE_ALL)
#if defined(MY_THREAD_MACORS_H)
  #define MY_THREAD_MACORS_C (1)
  #include "funct.h"
#endif
#undef MY_THREAD_MACORS_C

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

#include "allocators/wheels.h"
#include "smallstreams/wheels.h"

#if defined(MY_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_ALLOCATOR_C (1)
  #include "allocator.h"
#endif
#undef MY_ALLOCATOR_C

#if defined(ASSERTMESSAGE_H) || defined(WHEELS_INCLUDE_ALL)
  #define ASSERTMESSAGE_C (1)
  #include "assertMessage.h"
#endif
#undef ASSERTMESSAGE_C
