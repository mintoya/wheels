#include <string.h>
#if !defined(HHMAP_H)
  #define HHMAP_H (1)
typedef struct HHMap HHMap;
  #include "fptr.h"
  #include "my-list.h"

HHMap *HHMap_new(usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize);
// crops or expands (with 0's) the keys and vals
// if any argument is 0, it will assume you want the same one of that on the last ptr
void HHMap_transform(HHMap **last, usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize);
void HHMap_free(HHMap *hm);
void *HHMap_get(const HHMap *, const void *);
void HHMap_set(HHMap *map, const void *key, const void *val);
u32 HHMap_getMetaSize(const HHMap *);
u32 HHMap_count(const HHMap *map);
u8 *HHMap_getKeyBuffer(const HHMap *map);
extern inline void *HHMap_getKey(const HHMap *map, u32 n);
extern inline void *HHMap_getVal(const HHMap *map, u32 n);
usize HHMap_footprint(const HHMap *map);
u32 HHMap_countCollisions(const HHMap *map);
usize HHMap_getKeySize(const HHMap *map);
usize HHMap_getValSize(const HHMap *map);
typedef struct HHMap_both {
  void *key;
  void *val;
} HHMap_both;
HHMap_both HHMap_getBoth(HHMap *map, const void *key);

// get key store it in val
// false if key doesnt exist
bool HHMap_getSet(HHMap *map, const void *key, void *val);
void HHMap_fset(HHMap *map, const fptr key, void *val);

static inline void HHMap_cleanup_handler(HHMap **v) {
  if (v && *v) {
    HHMap_free(*v);
    *v = NULL;
  }
}
  #define HHMap_setM(HhMap, key, val)                       \
    ({                                                      \
      typeof(key) kv = (key);                               \
      typeof(val) vv = (val);                               \
      assertMessage(HHMap_getKeySize(HhMap) == sizeof(kv)); \
      assertMessage(HHMap_getValSize(HhMap) == sizeof(vv)); \
      HHMap_set(HhMap, &kv, &vv);                           \
    })
  #define HHMap_getM(HhMap, type, key, elseBlock) \
    ({                                            \
      typeof(key) k = key;                        \
      type rv;                                    \
      void *ptr = HHMap_get(HhMap, &k);           \
      if (ptr) {                                  \
        memcpy(&rv, ptr, sizeof(rv));             \
      } else                                      \
        elseBlock                                 \
            rv;                                   \
    })
  #define HHMap_scoped [[gnu::cleanup(HHMap_cleanup_handler)]] HHMap

#endif // HHMAP_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #pragma once
  #define HHMAP_C (1)
#endif
#ifdef HHMAP_C
  #include "fptr.h"
typedef struct {
  u16 length;
  u16 capacity;
} HHMap_LesserList;

typedef struct HHMap {
  const My_allocator *allocator;
  usize metaSize;
  usize keysize;
  usize valsize;
  HHMap_LesserList *lists;
  void **listHeads;
} HHMap;

static inline umax HHMap_hash(const fptr str) {
  umax hash = 5381;
  size_t s = 0;
  for (; s < str.width; s++)
    hash = ((hash << 5) + hash) + (str.ptr[s]);

  return hash;
}

HHMap *HHMap_new(usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize) {
  assertMessage(kSize && vSize && metaSize && allocator);
  HHMap *hm = (HHMap *)aAlloc(
      allocator,
      sizeof(HHMap) +
          metaSize * sizeof(HHMap_LesserList) +
          metaSize * sizeof(void *) +
          2 * (kSize + vSize)
  );

  HHMap_LesserList *lists = (HHMap_LesserList *)((u8 *)hm + sizeof(HHMap));
  void **listHeads = (void **)((u8 *)lists + metaSize * sizeof(HHMap_LesserList));

  *hm = (HHMap){
      .allocator = allocator,
      .metaSize = metaSize,
      .keysize = kSize,
      .valsize = vSize,
      .lists = lists,
      .listHeads = listHeads,
  };

  memset(hm->listHeads, 0, metaSize * sizeof(void *));
  memset(hm->lists, 0, metaSize * sizeof(HHMap_LesserList));
  return hm;
}

void HHMap_transform(HHMap **last, usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize) {
  if (!last || !*last) {
    return;
  }

  HHMap *oldMap = *last;
  if (kSize == 0)
    kSize = oldMap->keysize;
  if (vSize == 0)
    vSize = oldMap->valsize;
  if (allocator == NULL)
    allocator = oldMap->allocator;
  if (metaSize == 0)
    metaSize = oldMap->metaSize;
  HHMap *newMap = HHMap_new(kSize, vSize, allocator, metaSize);
  u8 *tempKey = (u8 *)aAlloc(allocator, kSize);
  u8 *tempVal = (u8 *)aAlloc(allocator, vSize);
  u32 totalCount = HHMap_count(oldMap);
  for (u32 n = 0; n < totalCount; n++) {
    void *oldKey = HHMap_getKey(oldMap, n);
    void *oldVal = HHMap_getVal(oldMap, n);

    if (!oldKey || !oldVal) {
      continue;
    }
    memset(tempKey, 0, kSize);
    usize keyCopySize = (oldMap->keysize < kSize) ? oldMap->keysize : kSize;
    memcpy(tempKey, oldKey, keyCopySize);

    memset(tempVal, 0, vSize);
    usize valCopySize = (oldMap->valsize < vSize) ? oldMap->valsize : vSize;
    memcpy(tempVal, oldVal, valCopySize);

    HHMap_set(newMap, tempKey, tempVal);
  }

  aFree(allocator, tempKey);
  aFree(allocator, tempVal);
  HHMap_free(oldMap);

  *last = newMap;
}

void HHMap_free(HHMap *hm) {
  const My_allocator *allocator = hm->allocator;
  const u32 msize = hm->metaSize;
  for (int i = 0; i < msize; i++)
    if (hm->listHeads[i])
      aFree(allocator, hm->listHeads[i]);
  aFree(allocator, hm);
}

void HHMap_set(HHMap *map, const void *key, const void *val) {
  assertMessage(map);
  if (!key)
    return;
  u32 lindex = (u32)(HHMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  HHMap_LesserList *hll = map->lists + lindex;
  void *lh = map->listHeads[lindex];
  List generated;
  if (!lh) {
    lh = aAlloc(map->allocator, map->keysize + map->valsize);
    generated = (List){
        .width = map->keysize + map->valsize,
        .length = 0,
        .size = 1,
        .head = (u8 *)lh,
        .allocator = map->allocator,
    };
  } else
    generated = (List){
        .width = map->keysize + map->valsize,
        .length = hll->length,
        .size = hll->capacity,
        .head = (u8 *)lh,
        .allocator = map->allocator,
    };
  u32 listindex = 0;
  for (; listindex < generated.length; listindex++)
    if (!memcmp(List_getRef(&(generated), listindex), key, map->keysize))
      break;
  if (listindex == generated.length)
    List_append(&(generated), NULL);
  u8 *place = (u8 *)List_getRef(&(generated), listindex);
  memcpy(place, key, map->keysize);
  if (val)
    memcpy(place + map->keysize, val, map->valsize);
  else
    memset(place + map->keysize, 0, map->valsize);

  *hll = (HHMap_LesserList){
      .length = (u16)generated.length,
      .capacity = (u16)generated.size,
  };
  map->listHeads[lindex] = (void *)generated.head;
}

void *HHMap_get(const HHMap *map, const void *key) {
  if (!key) {
    return NULL;
  }
  u32 lindex = (u32)(HHMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  void *lh = map->listHeads[lindex];
  if (!lh)
    return NULL;
  HHMap_LesserList *hll = map->lists + lindex;
  List generated = (List){
      .width = map->keysize + map->valsize,
      .length = hll->length,
      .size = hll->capacity,
      .head = (u8 *)lh,
      .allocator = map->allocator,
  };
  u32 listindex = 0;
  for (; listindex < generated.length; listindex++)
    if (!memcmp(List_getRef(&(generated), listindex), key, map->keysize))
      break;
  if (listindex == generated.length)
    return NULL;
  u8 *place = (u8 *)List_getRef(&(generated), listindex);
  return place + map->keysize;
}

usize HHMap_getKeySize(const HHMap *map) { return map->keysize; }
usize HHMap_getValSize(const HHMap *map) { return map->valsize; }
u32 HHMap_getMetaSize(const HHMap *map) { return map->metaSize; }

u8 *HHMap_getKeyBuffer(const HHMap *map) {
  u32 metasize = map->metaSize;
  return ((u8 *)map + sizeof(HHMap) + metasize * sizeof(HHMap_LesserList) + metasize * sizeof(void *));
}

u8 *HHMap_getValBuffer(const HHMap *map) {
  u32 metasize = map->metaSize;
  return ((u8 *)map + sizeof(HHMap) + metasize * sizeof(HHMap_LesserList) + metasize * sizeof(void *) + map->keysize);
}

inline void *HHMap_getKey(const HHMap *map, u32 n) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize && i <= n;) {
    if (i + map->lists[lindex].length > n) {
      usize s = map->keysize + map->valsize;
      usize i2 = n - i;
      return (u8 *)map->listHeads[lindex] + s * i2;
    }
    i += map->lists[lindex].length;
    lindex++;
  }
  return NULL;
}

inline void *HHMap_getVal(const HHMap *map, u32 n) {
  u8 *place = (u8 *)HHMap_getKey(map, n);
  if (place)
    return place + map->keysize;
  return NULL;
}

u32 HHMap_count(const HHMap *map) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    i += map->lists[lindex].length;
    lindex++;
  }
  return i;
}

u32 HHMap_countCollisions(const HHMap *map) {
  u32 collisions = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    if (map->lists[lindex].length)
      collisions += map->lists[lindex].length - 1;
    lindex++;
  }
  return collisions;
}

usize HHMap_footprint(const HHMap *map) {
  usize res = 0;
  for (u32 lindex = 0; lindex < map->metaSize; lindex++) {
    if (map->lists[lindex].length)
      res += (map->keysize + map->valsize) * map->lists[lindex].capacity;
  }
  res += sizeof(HHMap);
  res += map->keysize + map->valsize;
  res += map->metaSize * sizeof(HHMap_LesserList) * 2;
  res += map->metaSize * sizeof(void *) * 2;
  return res;
}
bool HHMap_getSet(HHMap *map, const void *key, void *val) {
  assertMessage(map && key && val);

  void *result = HHMap_get(map, key);
  if (result) {
    memcpy(val, result, map->valsize);
    return true;
  }
  return false;
}
void HHMap_fset(HHMap *map, const fptr key, void *val) {
  assertMessage(key.width <= HHMap_getKeySize(map));
  u8 *nname = ((u8 *)map) +
              sizeof(HHMap) +
              map->metaSize * (sizeof(HHMap_LesserList) + sizeof(void *)) +
              map->keysize + map->valsize;
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HHMap_getKeySize(map) - key.width);
  return HHMap_set(map, nname, val);
}
HHMap_both HHMap_getBoth(HHMap *map, const void *key) {
  u8 *place = (u8 *)HHMap_get(map, key);
  if (!place)
    return (HHMap_both){NULL, NULL};
  return (HHMap_both){place - map->valsize, place};
}

#endif // HHMAP_C
