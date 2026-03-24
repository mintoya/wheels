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

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define OMAP_C (1)
#endif

#if defined(OMAP_C)
static inline AllocatorV OMap_allocator(OMap *omap) {
  return omap->data->allocator;
}
OMap *OMap_new(AllocatorV allocator, usize initSize) {
  return (OMap *)stringList_new(allocator, initSize);
}
void OMap_free(void *omapPtr) { stringList_free((stringList *)omapPtr); }
struct OSearch_T {
  bool found;
  usize i;
};
struct OSearch_T OMap_search(OMap *map, fptr key) {
  usize t = stringList_len(map->data) / 2;
  usize b = 0;
  while (t > b) {
    usize m = (t + b) / 2;
    int cmp = fptr_cmp(key, stringList_get(map->data, m * 2));
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
  struct OSearch_T place = OMap_search(map, key);
  fptr res = nullFptr;
  if (place.found) {
    if (!val.len) {
      stringList_remove(map->data, place.i);
      stringList_remove(map->data, place.i);
    } else
      res = stringList_set(map->data, place.i + 1, val);
  } else {
    stringList_insert(map->data, place.i, key);
    res = stringList_insert(map->data, place.i + 1, val);
  }
  return res;
}
fptr OMap_get(OMap *map, fptr key) {
  struct OSearch_T place = OMap_search(map, key);
  if (place.found)
    return stringList_get(map->data, place.i + 1);
  return nullFptr;
}
usize OMap_footprint(OMap *map) {
  return stringList_footprint((stringList *)map);
}

usize OMap_len(OMap *map) { return stringList_len(map->data) / 2; }
struct OMap_both OMap_getN(OMap *map, usize i) {
  if (i < OMap_len(map)) {
    return (struct OMap_both){
        stringList_get(map->data, i * 2),
        stringList_get(map->data, i * 2 + 1),
    };
  }
  return (struct OMap_both){
      nullFptr,
      nullFptr,
  };
}
#endif
