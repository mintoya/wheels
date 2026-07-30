/*[[[cog
import cog
headers = [
  ("CBA_ALLOCATOR",       "cballocator.h",          True),
  ("TSA_ALLOCATOR",       "tsaAllocator.h",         False),
  ("ARENA_ALLOCATOR",     "arenaAllocator.h",       True),
  ("FBA_ALLOCATOR",       "fbaAllocator.h",         True),
  ("MY_DEBUG_ALLOCATOR",  "debugallocator.h",       True),
  ("FBA_FALLBACK",        "fbafallbackAllocator.h", True),
]

for prefix, header, all_flag in headers:
    cog.outl(f"#if ((defined {prefix}_H) || ({"defined(WHEELS_INCLUDE_ALL)" if all_flag else "0"}))\\\n && !defined {prefix}_C")
    cog.outl(f"  #define {prefix}_C (1)")
    cog.outl(f'  #include "{header}"')
    cog.outl(f"  _Static_assert({prefix}_C == 2 , \"header should define itself as  included\");")
    cog.outl(f"#endif")
]]]*/
#if ((defined CBA_ALLOCATOR_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined CBA_ALLOCATOR_C
  #define CBA_ALLOCATOR_C (1)
  #include "cballocator.h"
  _Static_assert(CBA_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined TSA_ALLOCATOR_H) || (0))\
 && !defined TSA_ALLOCATOR_C
  #define TSA_ALLOCATOR_C (1)
  #include "tsaAllocator.h"
  _Static_assert(TSA_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined ARENA_ALLOCATOR_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined ARENA_ALLOCATOR_C
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
  _Static_assert(ARENA_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined FBA_ALLOCATOR_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined FBA_ALLOCATOR_C
  #define FBA_ALLOCATOR_C (1)
  #include "fbaAllocator.h"
  _Static_assert(FBA_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined MY_DEBUG_ALLOCATOR_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined MY_DEBUG_ALLOCATOR_C
  #define MY_DEBUG_ALLOCATOR_C (1)
  #include "debugallocator.h"
  _Static_assert(MY_DEBUG_ALLOCATOR_C == 2 , "header should define itself as  included");
#endif
#if ((defined FBA_FALLBACK_H) || (defined(WHEELS_INCLUDE_ALL)))\
 && !defined FBA_FALLBACK_C
  #define FBA_FALLBACK_C (1)
  #include "fbafallbackAllocator.h"
  _Static_assert(FBA_FALLBACK_C == 2 , "header should define itself as  included");
#endif
//[[[end]]]
