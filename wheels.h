#if defined(MY_WHEELS_H)
  #error "wheels should be included once"
#endif
#define MY_WHEELS_H
/*[[[cog
import cog
headers = [
  ("MY_BIGINT" , "bigint.h",True),
  ("MY_PRINTER" , "print.h",True),
  ("STRING_LIST" , "stringList.h",True),
  ("VASON_BUILDER" , "vason_tree.h",True),
  ("VASON_PARSER" , "vason_arr.h",True),
  ("MY_THREAD_MACROS" , "funct.h",False),
  ("OMAP" , "omap.h",True),
  ("MY_HXMAP" , "hxmap.h",True),
  ("SXMAP" , "smap.h",True),
  ("SINGLE_ALLOCATOR" , "allocator.h",True),
  ("MY_OXMAP" , "oxmap.h",True),
  ("MY_SEGMENTTLIST" , "sglist.h",True),
  ("MY_LIST" , "mylist.h",True),
  ("MY_TRACE" , "trace.h",True),
  ("ASSERTMESSAGE" , "assertMessage.h",True),
]

for prefix, header, all_flag in headers:
    cog.outl(f"#if ((defined {prefix}_H) || ({"defined(WHEELS_INCLUDE_ALL)" if all_flag else "0"}))\\\n && !defined {prefix}_C")
    cog.outl(f"  #define {prefix}_C (1)")
    cog.outl(f'  #include "{header}"')
    cog.outl(f"  _Static_assert({prefix}_C == 2 , \"header should define itself as  included\");")
    cog.outl(f"#endif")

]]]*/
#if ((defined MY_BIGINT_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_BIGINT_C
  #define MY_BIGINT_C (1)
  #include "bigint.h"
  _Static_assert(MY_BIGINT_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_PRINTER_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_PRINTER_C
  #define MY_PRINTER_C (1)
  #include "print.h"
  _Static_assert(MY_PRINTER_C == 2 , "header should define itself as  included");
#endif
#if ((defined STRING_LIST_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined STRING_LIST_C
  #define STRING_LIST_C (1)
  #include "stringList.h"
  _Static_assert(STRING_LIST_C == 2 , "header should define itself as  included");
#endif
#if ((defined VASON_BUILDER_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined VASON_BUILDER_C
  #define VASON_BUILDER_C (1)
  #include "vason_tree.h"
  _Static_assert(VASON_BUILDER_C == 2 , "header should define itself as  included");
#endif
#if ((defined VASON_PARSER_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined VASON_PARSER_C
  #define VASON_PARSER_C (1)
  #include "vason_arr.h"
  _Static_assert(VASON_PARSER_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_THREAD_MACROS_H) || (0))\
 && !defined MY_THREAD_MACROS_C
  #define MY_THREAD_MACROS_C (1)
  #include "funct.h"
  _Static_assert(MY_THREAD_MACROS_C == 2 , "header should define itself as  included");
#endif
#if ((defined OMAP_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined OMAP_C
  #define OMAP_C (1)
  #include "omap.h"
  _Static_assert(OMAP_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_HXMAP_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_HXMAP_C
  #define MY_HXMAP_C (1)
  #include "hxmap.h"
  _Static_assert(MY_HXMAP_C == 2 , "header should define itself as  included");
#endif
#if ((defined SXMAP_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined SXMAP_C
  #define SXMAP_C (1)
  #include "smap.h"
  _Static_assert(SXMAP_C == 2 , "header should define itself as  included");
#endif
#if ((defined SINGLE_ALLOCATOR_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined SINGLE_ALLOCATOR_C
  #define SINGLE_ALLOCATOR_C (1)
  #include "allocator.h"
  _Static_assert(SINGLE_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_OXMAP_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_OXMAP_C
  #define MY_OXMAP_C (1)
  #include "oxmap.h"
  _Static_assert(MY_OXMAP_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_SEGMENTTLIST_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_SEGMENTTLIST_C
  #define MY_SEGMENTTLIST_C (1)
  #include "sglist.h"
  _Static_assert(MY_SEGMENTTLIST_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_LIST_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_LIST_C
  #define MY_LIST_C (1)
  #include "mylist.h"
  _Static_assert(MY_LIST_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_TRACE_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_TRACE_C
  #define MY_TRACE_C (1)
  #include "trace.h"
  _Static_assert(MY_TRACE_C == 2 , "header should define itself as  included");
#endif
#if ((defined ASSERTMESSAGE_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined ASSERTMESSAGE_C
  #define ASSERTMESSAGE_C (1)
  #include "assertMessage.h"
  _Static_assert(ASSERTMESSAGE_C == 2 , "header should define itself as  included");
#endif
//[[[end]]]
#include "allocators/wheels.h"
