#if !defined MY_OXMAP_H
  #define MY_OXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "fptr.h"
  #include "macros.h"
  #include "mytypes.h"
  #include "sList.h"

typedef struct oxmap {
  AllocatorV allocator;
  const u32 ksize, vsize;
  const fnptrof((const void *, const void *), i8) cmp;
  sList_header *keys;
  sList_header *vals;
} oxmap;
oxmap *oxmap_new(AllocatorV allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp) {
  var_ res = aCreate(allocator, oxmap);
  mcpy(
      *res,
      ((oxmap){
          .allocator = allocator,
          .cmp = cmp,
          .ksize = ksize,
          .vsize = vsize,
          .keys = sList_new(allocator, 2, ksize),
          .vals = sList_new(allocator, 2, vsize),
      })
  );
  return res;
}
void oxmap_free(oxmap *map) {
  var_ allocator = map->allocator;
  sList_free(allocator, map->keys, map->ksize);
  sList_free(allocator, map->vals, map->vsize);
  aFree(allocator, map, sizeof(*map));
}
struct bbs_result oxmap_base_search(const oxmap *map, const void *key) {
  if (map->cmp) return bbsearch(key, map->keys, map->keys->length, map->ksize, map->cmp);

  usize size = map->ksize;
  usize nmemb = map->keys->length / size;
  const char *base = (const char *)map->keys;

  for (usize lim = nmemb; lim; lim /= 2) {
    var_ p = base + (lim >> 1) * size;
    var_ cmp = fptr_cmp(((fptr){size, (u8 *)key}), ((fptr){size, (u8 *)p}));
    if (!cmp)
      return (struct bbs_result){(void *)p, true};
    if (cmp > 0) {
      base = (const char *)p + size;
      lim--;
    }
  }
  return (struct bbs_result){(void *)base, false};
}
void *oxmap_key_val(const oxmap *map, const void *key) {
  return (((u8 *)key - (u8 *)map->keys->buf) / map->ksize * map->vsize) + map->vals;
}
void *oxmap_set(oxmap *map, const void *key, const void *val) {
  if (!key) return nullptr;
  var_ pos = oxmap_base_search(map, key);
  usize idx = ((u8 *)pos.p - map->keys->buf) / map->ksize;

  if (val) {
    if (pos.f) return memcpy(oxmap_key_val(map, pos.p), val, map->vsize);
    else {
      map->keys = sList_insert(map->allocator, map->keys, map->ksize, idx, key);
      map->vals = sList_insert(map->allocator, map->vals, map->vsize, idx, val);
      return map->vals->buf + (idx * map->vsize);
    }
  } else {
    if (pos.f) {
      sList_remove(map->keys, map->ksize, idx);
      sList_remove(map->vals, map->vsize, idx);
    }
    return nullptr;
  }
}
void *oxmap_get(const oxmap *map, const void *key) {
  if (!key) return nullptr;
  var_ pos = oxmap_base_search(map, key);
  if (pos.f) return oxmap_key_val(map, pos.p);
  return nullptr;
}
#endif
