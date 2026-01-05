#include "allocator.h"
#include "my-list.h"
#include <string.h>
#if !defined(HLMAP_H)
  #define HLMAP_H (1)
typedef struct HLMap {
  size_t metaSize;
  List klist;
  List vlist;
  List ulist;
} HLMap;
  #include "fptr.h"
  #include "my-list.h"

/**
 * @brief create a new HHMap
 *
 * if `k` and `v` dont have compatible alignment,
 * HLMap_fset can compensage if adequate padding is added
 * to (`kSize`)
 *
 * @tparam k key type.
 * @tparam v val type.
 * @param kSize at least `sizeof(k)`.
 * @param vSize `sizeof(v)`.
 * @param buckets buckets used in map.
 */
HLMap *HLMap_new(usize kSize, usize vSize, AllocatorV allocator, u32 metaSize);
// crops or expands (with 0's) the keys and vals
// if any argument is 0, it will assume you want the same one of that on the last ptr
void HLMap_transform(HLMap **last, usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize);
void HLMap_reload(HLMap **last, f32 load);
void HLMap_free(HLMap *hm);
void *HLMap_get(const HLMap *, const void *);
void HLMap_set(HLMap *map, const void *key, const void *val);
u32 HLMap_getMetaSize(const HLMap *);
u32 HLMap_count(const HLMap *map);
void HLMap_clear(HLMap *map);
u8 *HLMap_getKeyBuffer(const HLMap *map);
extern inline void *HLMap_getKey(const HLMap *map, u32 n);
extern inline void *HLMap_getVal(const HLMap *map, u32 n);
usize HLMap_footprint(const HLMap *map);
// u32 HLMap_countCollisions(const HLMap *map);
usize HLMap_getKeySize(const HLMap *map);
usize HLMap_getValSize(const HLMap *map);
// resizes all buckets, can do this before lots of insertions
// void HLMap_fatten(const HHMap *map, usize bucketLength);
typedef struct HLMap_both {
  void *key;
  void *val;
} HLMap_both;
HLMap_both HLMap_getBoth(HLMap *map, const void *key);

// get key store it in val
// false if key doesnt exist
void HLMap_fset(HLMap *map, const fptr key, void *val);
void *HLMap_fget(HLMap *map, const fptr key);
bool HLMap_getSet(HLMap *map, const void *key, void *val);

static inline void HLMap_cleanup_handler(HLMap **v) {
  if (v && *v) {
    HLMap_free(*v);
    *v = NULL;
  }
}
  #define HLMap_setM(HhMap, key, val)                       \
    ({                                                      \
      typeof(key) kv = (key);                               \
      typeof(val) vv = (val);                               \
      assertMessage(HLMap_getKeySize(HhMap) == sizeof(kv)); \
      assertMessage(HLMap_getValSize(HhMap) == sizeof(vv)); \
      HLMap_set(HhMap, &kv, &vv);                           \
    })
  #define HLMap_getM(HhMap, type, key, elseBlock) \
    ({                                            \
      typeof(key) k = key;                        \
      type rv;                                    \
      void *ptr = HLMap_get(HhMap, &k);           \
      if (ptr) {                                  \
        memcpy(&rv, ptr, sizeof(rv));             \
      } else                                      \
        elseBlock                                 \
            rv;                                   \
    })
  #define HLMap_scoped [[gnu::cleanup(HLMap_cleanup_handler)]] HLMap

#endif // HLMap_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #pragma once
  #define HLMAP_C (1)
#endif
#ifdef HLMAP_C
  #include "fptr.h"

static inline umax HLMap_hash(const fptr str) {
  umax hash = 5381;
  if (str.width % sizeof(u64)) {
    for (usize i = 0; i < str.width; i++) {
      hash ^= hash >> 3;
      hash = hash * 65;
      hash ^= (str.ptr[i]);
    }
  } else {
    assertMessage(
        !((uintptr_t)str.ptr % alignof(u64)),
        "change hash for unaligned pointers\n"
        "or use hmap"
    );
    for (usize i = 0; i < str.width; i += sizeof(u64)) {
      hash ^= hash >> 33;
      hash *= 0xff51afd7ed558ccd;
      hash ^= *(u64 *)(str.ptr + i);
    }
  }
  return hash;
}
HLMap *HLMap_new(usize kSize, usize vSize, AllocatorV allocator, u32 metaSize) {
  assertMessage(kSize && vSize && metaSize && allocator);

  usize totalSize =
      sizeof(HLMap) +
      2 * alignof(max_align_t) +
      2 * (kSize + vSize);
  HLMap *hm = (HLMap *)aAlloc(allocator, totalSize);

  *hm = (HLMap){.metaSize = metaSize};
  List_makeNew(allocator, &hm->vlist, vSize, metaSize);
  List_makeNew(allocator, &hm->klist, kSize, metaSize);
  List_makeNew(allocator, &hm->ulist, sizeof(u8), metaSize);
  return hm;
}
u8 *HLMap_getKeyBuffer(const HLMap *map) {
  return (u8 *)(lineup((uintptr_t)map + sizeof(HLMap), alignof(max_align_t)));
}

u8 *HLMap_getValBuffer(const HLMap *map) {
  return (u8 *)HLMap_getKeyBuffer(map) + map->klist.width;
}
void HLMap_free(HLMap *hm) {
  const My_allocator *allocator = hm->klist.allocator;
  aFree(allocator, hm->klist.head);
  aFree(allocator, hm->vlist.head);
  aFree(allocator, hm->ulist.head);
  aFree(allocator, hm);
}

void HLMap_transform(HLMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize) {
  assertMessage(last && *last);

  HLMap *oldMap = *last;
  if (!kSize)
    kSize = oldMap->klist.width;
  if (!vSize)
    kSize = oldMap->vlist.width;
  if (!allocator)
    allocator = oldMap->ulist.allocator;
  if (!metaSize)
    metaSize = oldMap->metaSize;
  HLMap *newMap = HLMap_new(kSize, vSize, allocator, metaSize);
  u8 *tempKey;
  u8 *tempVal;
  if (newMap->klist.width > oldMap->klist.width)
    tempKey = HLMap_getKeyBuffer(newMap);
  else
    tempKey = HLMap_getKeyBuffer(oldMap);
  if (newMap->vlist.width > oldMap->vlist.width)
    tempVal = HLMap_getValBuffer(newMap);
  else
    tempVal = HLMap_getValBuffer(oldMap);
  u32 totalCount = HLMap_count(oldMap);
  for (u32 n = 0; n < totalCount; n++) {
    void *oldKey = HLMap_getKey(oldMap, n);
    void *oldVal = HLMap_getVal(oldMap, n);

    if (!oldKey || !oldVal) {
      continue;
    }
    memset(tempKey, 0, kSize);
    usize keyCopySize = (oldMap->klist.width < kSize) ? oldMap->klist.width : kSize;
    memcpy(tempKey, oldKey, keyCopySize);

    memset(tempVal, 0, vSize);
    usize valCopySize = (oldMap->vlist.width < vSize) ? oldMap->vlist.width : vSize;
    memcpy(tempVal, oldVal, valCopySize);

    HLMap_set(newMap, tempKey, tempVal);
  }

  HLMap_free(oldMap);

  *last = newMap;
}
void HLMap_set(HLMap *map, const void *key, const void *val) {
  if (!key)
    return;
  u32 lindex = (u32)(HLMap_hash(fptr_fromPL(key, map->klist.width)) % map->metaSize);
  if (map->ulist.length <= lindex) {
    u32 nl = lindex - map->klist.length + 1;
    List_appendFromArr(&map->klist, NULL, nl);
    List_appendFromArr(&map->vlist, NULL, nl);
    List_appendFromArr(&map->ulist, NULL, nl);
  }
  u32 l = map->ulist.length;
  for (u32 i = lindex; i <= l; i++) {
    u8 *exists = (u8 *)List_getRef(&map->ulist, i);
    if (!exists || !*exists) {
      if (val) {
        List_set(&map->klist, i, key);
        List_set(&map->vlist, i, val);
        List_set(&map->ulist, i, REF(u8, 1));
      }
      return;
    } else if (!memcmp(List_getRef(&map->klist, i), key, map->klist.width)) {
      if (val) {
        List_set(&map->vlist, i, val);
      } else {
        List_set(&map->ulist, i, NULL);
      }
      return;
    }
  }
  assertMessage(false, "unreachable");
}
void *HLMap_get(const HLMap *map, const void *key) {
  if (!key)
    return NULL;
  u32 lindex = (u32)(HLMap_hash(fptr_fromPL(key, map->klist.width)) % map->metaSize);
  for (u32 i = lindex; i < map->ulist.length; i++) {
    u8 *exists = (u8 *)List_getRef(&map->ulist, i);
    if (!exists || !*exists) {
      return NULL;
    } else if (!memcmp(List_getRef(&map->klist, i), key, HLMap_getKeySize(map))) {
      return List_getRef(&map->vlist, i);
    }
  }
  return NULL;
}

usize HLMap_getKeySize(const HLMap *map) { return map->klist.width; }
usize HLMap_getValSize(const HLMap *map) { return map->vlist.width; }
u32 HLMap_getMetaSize(const HLMap *map) { return map->metaSize; }

inline void *HLMap_getKey(const HLMap *map, u32 n) {
  u8 *exists = (u8 *)List_getRef(&map->ulist, n);
  if (!exists)
    return NULL;
  if (!*exists)
    return NULL;
  return List_getRef(&map->klist, n);
}

inline void *HLMap_getVal(const HLMap *map, u32 n) {
  u8 *exists = (u8 *)List_getRef(&map->ulist, n);
  if (!exists)
    return NULL;
  if (!*exists)
    return NULL;
  return List_getRef(&map->vlist, n);
}

u32 HLMap_count(const HLMap *map) { return List_length(&map->ulist); }

// u32 HLMap_countCollisions(const HLMap *map) {
//   u32 collisions = 0;
//   u32 lindex = 0;
//   for (; lindex < map->metaSize;) {
//     if (map->lists[lindex].length)
//       collisions += map->lists[lindex].length - 1;
//     lindex++;
//   }
//   return collisions;
// }

bool HLMap_getSet(HLMap *map, const void *key, void *val) {
  assertMessage(key && val);

  void *result = HLMap_get(map, key);
  if (result) {
    memcpy(val, result, map->vlist.width);
    return true;
  }
  return false;
}
usize HLMap_footprint(const HLMap *map) {
  return List_headArea(&map->klist) + List_headArea(&map->vlist) + List_headArea(&map->ulist);
}
void HLMap_fset(HLMap *map, const fptr key, void *val) {
  usize els = map->klist.width + map->vlist.width;
  assertMessage(key.width <= HLMap_getKeySize(map));

  u8 *nname = (u8 *)lineup((uptr)HLMap_getKeyBuffer(map) + map->vlist.width + map->klist.width, alignof(max_align_t));

  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HLMap_getKeySize(map) - key.width);
  return HLMap_set(map, nname, val);
}
void *HLMap_fget(HLMap *map, const fptr key) {
  assertMessage(key.width <= HLMap_getKeySize(map));

  u8 *nname = (u8 *)lineup((uptr)HLMap_getKeyBuffer(map) + map->vlist.width + map->klist.width, alignof(max_align_t));

  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HLMap_getKeySize(map) - key.width);
  return HLMap_get(map, nname);
}
HLMap_both HLMap_getBoth(HLMap *map, const void *key) {
  u8 *place = (u8 *)HLMap_get(map, key);
  if (!place)
    return (HLMap_both){NULL, NULL};
  u32 pos = ((uptr)place - (uptr)map->vlist.head) / map->vlist.width;
  return (HLMap_both){List_getRef(&map->klist, pos), place};
}

void HLMap_clear(HLMap *map) {
  for (u32 i = 0; i < map->metaSize; i++) {
    map->ulist.length = 0;
    map->vlist.length = 0;
    map->klist.length = 0;
  }
}

void HLMap_reload(HLMap **last, f32 load) {
  assertMessage(last && *last);
  HLMap *map = *last;
  u32 len = 0;
  for (u32 i = 0; i < map->ulist.length; i++)
    len += (*(u8 *)List_getRef(&map->ulist, i)) ? 1 : 0;
  HLMap_transform(
      last,
      (*last)->klist.width,
      (*last)->vlist.width,
      (*last)->vlist.allocator,
      len * load
  );
}
#endif // HLMAP_C
