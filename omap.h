#include "my-list.h"
#include "types.h"
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
static inline AllocatorV OMap_allocator(OMap *omap) {
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
struct OSearch_T {
  bool found;
  List_index_t i;
};
struct OSearch_T OMap_search(OMap *map, fptr key) {
  List_index_t b = 0;
  List_index_t t = mList_len(map->keys);
  while (b < t) {
    List_index_t m = (b + t) / 2;
    int cmp = fptr_cmp(key, stringList_get(map->keys, m));
    if (cmp == 0) {
      return (struct OSearch_T){.found = 1, .i = m};
    } else if (cmp > 0) {
      t = m;
    } else {
      b = m + 1;
    }
  }
  return (struct OSearch_T){.found = 0, .i = b};
}
fptr OMap_set(OMap *map, fptr key, fptr val) {
  struct OSearch_T place = OMap_search(map, key);
  fptr res;
  if (place.found) {
    stringList_set(map->keys, place.i, key);
    res = stringList_set(map->vals, place.i, val);
  } else {
    stringList_insert(map->keys, place.i, key);
    res = stringList_insert(map->vals, place.i, val);
  }
  return res;
}
fptr OMap_get(OMap *map, fptr key) {
  struct OSearch_T place = OMap_search(map, key);
  if (place.found)
    return stringList_get(map->vals, place.i);
  return nullFptr;
}
#endif
