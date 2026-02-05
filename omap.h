#include "my-list.h"
#include "types.h"
#if !defined(OMAP_H)
  #define OMAP_H (1)
  #include "stringList.h"
typedef struct {
  stringList data;
} OMap;
OMap *OMap_new(AllocatorV, usize initSize);
void OMap_free(void *);
fptr OMap_set(OMap *map, fptr key, fptr val);
fptr OMap_get(OMap *map, fptr key);
usize OMap_footprint(OMap* map);
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define OMAP_C (1)
#endif

#if defined(OMAP_C)
static inline AllocatorV OMap_allocator(OMap *omap) {
  return stringList_allocator((stringList *)omap);
}
OMap *OMap_new(AllocatorV allocator, usize initSize) {
  return (OMap *)stringList_new(allocator, initSize);
}
void OMap_free(void *omapPtr) {
  stringList_free((stringList *)omapPtr);
}
struct OSearch_T {
  bool found;
  List_index_t i;
};
struct OSearch_T OMap_search(OMap *map, fptr key) {
  stringList *sl = (stringList *)map;
  List_index_t t = mList_len(sl->ulist) / 2;
  List_index_t b = 0;
  while (t > b) {
    List_index_t m = (t + b) / 2;
    int cmp = fptr_cmp(key, stringList_get(sl, m * 2));
    if (!cmp)
      return (struct OSearch_T){.found = true, .i = m * 2};
    else if (cmp > 0)
      b = m + 1;
    else
      t = m;
  }
  return (struct OSearch_T){.found = false, .i = b * 2};
}
fptr OMap_set(OMap *map, fptr key, fptr val) {
  stringList *sl = (stringList *)map;
  struct OSearch_T place = OMap_search(map, key);
  fptr res;
  if (place.found) {
    res = stringList_set(sl, place.i + 1, val);
  } else {
    stringList_insert(sl, place.i, key);
    res = stringList_insert(sl, place.i + 1, val);
  }
  return res;
}
fptr OMap_get(OMap *map, fptr key) {
  stringList *sl = (stringList *)map;
  struct OSearch_T place = OMap_search(map, key);
  if (place.found)
    return stringList_get(sl, place.i + 1);
  return nullFptr;
}
usize OMap_footprint(OMap* map){
  return stringList_footprint((stringList*)map);
}
#endif
