#if !defined(OMAP_H)
  #define OMAP_H (1)
  #include "stringList.h"
typedef struct {
  stringList *keys;
  stringList *vals;
} OMap;
OMap *OMap_new(AllocatorV, usize initSize);
void OMap_free(void *);
fptr OMap_set(OMap *map, fptr key, fptr val);
fptr OMap_get(OMap *map, fptr key);
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define OMAP_C (1)
#endif

#if defined(OMAP_C)
inline AllocatorV OMap_allocator(OMap *omap) {
  return stringList_allocator(omap->keys);
}
OMap *OMap_new(AllocatorV allocator, usize initSize) {
  OMap *res = aCreate(allocator, OMap);
  initSize = initSize ?: 2;
  res->keys = stringList_new(allocator, initSize / 2);
  res->vals = stringList_new(allocator, initSize / 2);
  return res;
}
void OMap_free(void *omapPtr) {
  OMap *omap = (OMap *)omapPtr;
  AllocatorV allocator = OMap_allocator(omap);
  stringList_free(omap->keys);
  stringList_free(omap->vals);
  aFree(allocator, omap);
}
fptr OMap_set(OMap *map, fptr key, fptr val);
fptr OMap_get(OMap *map, fptr key);
#endif
