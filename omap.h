#if !defined(OMAP_H)
  #define OMAP_H (1)
  #include "stringList.h"
typedef struct {
  stringList data[1];
} OMap;
OMap *OMap_new(AllocatorV, usize initSize);
void OMap_free(void *);
fptr OMap_set(OMap *map, fptr key, fptr val);
fptr OMap_get(OMap *map, fptr key);
usize OMap_footprint(OMap *map);
usize OMap_len(OMap *map);
struct OMap_both {
  fptr key;
  fptr val;
};
struct OMap_both OMap_getN(OMap *, usize);
#endif

#if defined(OMAP_C) && OMAP_C == (1)
  #define OMAP_C (2)
  #include "src/omap.c"
#endif
