// impl order matters

#ifdef HMAP_ALLOCATOR_H
  #define HMAP_ALLOCATOR_C
  #include "hmap_arena.h"
#endif

#ifdef KMLM_H
  #define KMLM_C
  #include "kmlM.h"
#endif

#ifdef MY_PRINTER_H
  #define MY_PRINTER_C
  #include "print.h"
#endif

#ifdef HMAP_H
  #define HMAP_C
  #include "hmap.h"
#endif

#ifdef OMAP_H
  #define OMAP_C
  #include "omap.h"
#endif

#ifdef HHMAP_H
  #define HHMAP_C
  #include "hhmap.h"
#endif

#ifdef STRING_LIST_H
  #define STRING_LIST_C
  #include "stringList.h"
#endif

#ifdef MY_LIST_H
  #define MY_LIST_C
  #include "my-list.h"
#endif

#ifdef ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_C
  #include "arenaAllocator.h"
#endif

#ifdef MY_ALLOCATOR_H
  #define MY_ALLOCATOR_C
  #include "allocator.h"
#endif
