#include "allocator.h"
#include "assertMessage.h"
#include <string.h>
#if !defined(HHMAP_H)
  #define HHMAP_H (1)
typedef struct HHMap HHMap;
  #include "fptr.h"
  #include "my-list.h"
/**
 * Creates a new hash map
 * `@param` **kSize** size of key type
 * `@param` **vSize** size of value type
 * `@param` **allocator** allocator
 * `@param` **metaSize** number of buckets
 *     each bucket is a dynamic array
 * `@return` pointer to new HHMap or NULL on failure
 */
HHMap *HHMap_new(
    usize kSize,          //< size of key type
    usize vSize,          //< size of value type
    AllocatorV allocator, //< allocator
    u32 metaSize          //< number of buckets
);
/**
 * Creates a new hash, with same keys and vals as another hash map
 * key and val size are trimmed or padded with 0's
 * will replace **last**
 * `@param` **last** last map
 * `@param` **kSize** size of key type
 * `@param` **allocator** allocator
 * `@param` **vSize** size of value type
 * `@param` **metaSize** number of buckets
 */
void HHMap_transform(HHMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize);
/**
 * free's **hm**
 * `@param` **hm** map
 */
void HHMap_free(HHMap *hm);
/**
 * get pointer to key from pointer to val
 * `@param` **hm** map
 * `@param` **key** pointer to key
 * `@return` pointer to value, null if not found
 */
void *HHMap_get(const HHMap *hm, const void *key);
/**
 * set pointer to key from pointer to val
 * `@param` **hm** map
 * `@param` **key** pointer to key
 * `@param` **val** pointer to value
 */
void HHMap_set(HHMap *map, const void *key, const void *val);
/**
 * `@param` **hm** map
 * `@param` **bucket** bucket index
 * `@return` length of bucket[**bucket**]
 */
extern inline u32 HHMap_getBucketSize(const HHMap *hm, u32 bucket);
/**
 * `@param` **hm** map
 * `@return` bucket count
 */
u32 HHMap_getMetaSize(const HHMap *);
/**
 * helper for looping through all values
 * `@param` **hm** map
 * `@param` **bucket** bucket index
 * `@param` **index** index inside bucket
 * `@return` pointer to key, get the val by adding your padding
 */
extern inline void *HHMap_getCoord(const HHMap *hm, u32 bucket, u32 index);
/**
 * `@param` **hm** map
 * `@return` total keys and vals
 */
u32 HHMap_count(const HHMap *map);
/**
 * deletes all keys and values
 * does not free memory
 * `@param` **hm** map
 */
void HHMap_clear(HHMap *map);
/**
 * used by fset and fget
 * `@return` aligned memory big enough for key
 */
[[gnu::pure, gnu::assume_aligned(alignof(max_align_t))]]
u8 *HHMap_getKeyBuffer(const HHMap *map);
extern inline void *HHMap_getKey(const HHMap *map, u32 n);
extern inline void *HHMap_getVal(const HHMap *map, u32 n);
usize HHMap_footprint(const HHMap *map);
u32 HHMap_countCollisions(const HHMap *map);
usize HHMap_getKeySize(const HHMap *map);
usize HHMap_getValSize(const HHMap *map);
// resizes all buckets, can do this before lots of insertions
// void HHMap_fatten(const HHMap *map, usize bucketLength);
typedef struct HHMap_both {
  void *key;
  void *val;
} HHMap_both;
HHMap_both HHMap_getBoth(HHMap *map, const void *key);

/**
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@param` **val** pointer to key
 */
void HHMap_fset(HHMap *map, const fptr key, void *val);
/**
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@param` **val** pointer to key
 *     not set if not found
 */
bool HHMap_fget(HHMap *map, const fptr key, void *val);
/**
 * like fset but just returns the pointer
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@param` **val** pointer to key
 *     null set if not found
 */
void *HHMap_fget_ns(HHMap *map, const fptr key);

/**
 * `@param` **vv** pointer to HHMap pointer
 */
static inline void HHMap_cleanup_handler(void *vv) {
  HHMap **v = vv;
  if (v && *v) {
    HHMap_free(*v);
    *v = NULL;
  }
}
  // makes sure key has enough padding to directly derefarance
  // hmap_get
  #define calign_second(Ta, Tb) \
    (offsetof(struct { Ta a; Tb b; }, b))

  #define mHmap(Ta, Tb) typeof(Tb (*)(Ta))
  #define mHmap_scoped(Ta, Tb) [[gnu::cleanup(HHMap_cleanup_handler)]] mHmap(Ta, Tb)

  #define HMAP_INIT_HELPER(allocator, keytype, valtype, bucketcount, ...) ({ \
    (mHmap(keytype, valtype)) HHMap_new(                                     \
        calign_second(keytype, valtype),                                     \
        sizeof(valtype),                                                     \
        allocator, bucketcount                                               \
    );                                                                       \
  })
  // optional bucket count argument
  #define mHmap_init(allocator, keytype, valtype, ...) \
    HMAP_INIT_HELPER(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32)
  #define mHmap_free(map) ({ HHMap_free((HHMap *)map); })

  #define mHmap_set(map, key, val)                         \
    ({                                                     \
      static_assert(                                       \
          __builtin_types_compatible_p(                    \
              mHmap(typeof(key), typeof(val)), typeof(map) \
          )                                                \
      );                                                   \
      HHMap_fset(                                          \
          (HHMap *)map,                                    \
          fptr_fromTypeDef(key),                           \
          (typeof(map(key))[1]){(val)}                     \
      );                                                   \
    })

  #define mHmap_get(map, key)                       \
    ({                                              \
      static_assert(                                \
          __builtin_types_compatible_p(             \
              mHmap(typeof(key), typeof(map(key))), \
              typeof(map)                           \
          )                                         \
      );                                            \
      (typeof(map(key)) *)                          \
          HHMap_fget_ns(                            \
              (HHMap *)map,                         \
              fptr_fromTypeDef(key)                 \
          );                                        \
    })
  #define HHMap_scoped [[gnu::cleanup(HHMap_cleanup_handler)]] HHMap

#endif // HHMAP_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
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
HHMap *HHMap_new(usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize) {
  assertMessage(kSize && vSize && metaSize && allocator);

  usize totalSize =
      sizeof(HHMap) +
      sizeof(HHMap_LesserList) * metaSize +
      sizeof(void **) * metaSize +
      alignof(max_align_t) * 2 +
      (kSize + vSize);
  HHMap *hm = (HHMap *)aAlloc(allocator, totalSize);

  HHMap_LesserList *lists = (HHMap_LesserList *)lineup(
      (uintptr_t)((u8 *)hm + sizeof(HHMap)),
      alignof(max_align_t)
  );
  void **listHeads = (void **)(lineup(
      (uintptr_t)lists + metaSize * sizeof(HHMap_LesserList),
      alignof(max_align_t)
  ));

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
[[gnu::pure, gnu::assume_aligned(alignof(max_align_t))]]
u8 *HHMap_getKeyBuffer(const HHMap *map) {
  return (
      (u8 *)lineup(
          (uptr)map->listHeads +
              map->metaSize * sizeof(*map->listHeads),
          alignof(max_align_t)
      )
  );
}

[[gnu::pure]]
/**
 * used by fset and fget
 * `@return` aligned memory big enough for value
 */
u8 *HHMap_getValBuffer(const HHMap *map) {
  return (u8 *)lineup(
      (uptr)(u8 *)HHMap_getKeyBuffer(map) + map->keysize, alignof(max_align_t)
  );
}
void HHMap_free(HHMap *hm) {
  const My_allocator *allocator = hm->allocator;
  const u32 msize = hm->metaSize;
  for (int i = 0; i < msize; i++)
    if (hm->listHeads[i])
      aFree(allocator, hm->listHeads[i]);
  aFree(allocator, hm);
}

void HHMap_transform(HHMap **last, usize kSize, usize vSize, const My_allocator *allocator, u32 metaSize) {
  assertMessage(last && *last);

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

  u8 *tempKey = HHMap_getKeyBuffer(newMap);
  u8 *tempVal = HHMap_getValBuffer(newMap);

  usize keyCopySize = (oldMap->keysize < kSize) ? oldMap->keysize : kSize;
  usize valCopySize = (oldMap->valsize < vSize) ? oldMap->valsize : vSize;

  if (newMap->keysize < oldMap->keysize)
    tempKey = HHMap_getKeyBuffer(oldMap);
  if (newMap->valsize < oldMap->valsize)
    tempVal = HHMap_getValBuffer(oldMap);

  for (auto i = 0; i < HHMap_getMetaSize(oldMap); i++) {
    for (auto j = 0; j < HHMap_getBucketSize(oldMap, i); j++) {
      u8 *oldKey = HHMap_getCoord(oldMap, i, j);
      u8 *oldVal = oldKey + oldMap->keysize;
      if (!oldKey)
        continue;
      memset(tempKey, 0, kSize);
      memcpy(tempKey, oldKey, keyCopySize);
      memset(tempVal, 0, vSize);
      memcpy(tempVal, oldVal, valCopySize);

      HHMap_set(newMap, tempKey, tempVal);
    }
  }

  HHMap_free(oldMap);

  *last = newMap;
}
[[gnu::always_inline]] static inline void *LesserList_getref(usize elw, HHMap_LesserList *hll, void *head, u32 idx) {
  return (u8 *)head + idx * (elw);
}
static inline void LesserList_appendGarbage(usize elw, HHMap_LesserList *hll, void **headptr, AllocatorV allocator) {
  if (!hll->capacity) {
    *headptr = aAlloc(allocator, elw);
    hll->capacity = 1;
  }
  if (hll->capacity < hll->length + 1) {
    *headptr = aRealloc(allocator, *headptr, hll->capacity * 2 * elw);
    hll->capacity *= 2;
  }
  hll->length++;
}
void HHMap_set(HHMap *map, const void *key, const void *val) {
  usize elw = map->keysize + map->valsize;
  if (!key)
    return;
  u32 lindex = (u32)(HHMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  HHMap_LesserList *hll = map->lists + lindex;
  void *lh = map->listHeads[lindex];
  u32 listindex = 0;
  u8 *place = NULL;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, lh, listindex);
    if (!memcmp(place, key, map->keysize))
      break;
  }
  if (listindex == hll->length) {
    LesserList_appendGarbage(elw, hll, &lh, map->allocator);
    place = (u8 *)LesserList_getref(elw, hll, lh, hll->length - 1);
  }
  memcpy(place, key, map->keysize);
  if (val)
    memcpy(place + map->keysize, val, map->valsize);
  else {
    memmove(place, place + elw, (hll->length - listindex - 1) * elw);
    hll->length--;
  }
  map->listHeads[lindex] = lh;
}

void *HHMap_get(const HHMap *map, const void *key) {
  if (!key) {
    return NULL;
  }
  u32 lindex = (u32)(HHMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  usize elw = map->keysize + map->valsize;
  void *lh = map->listHeads[lindex];
  if (!lh)
    return NULL;
  HHMap_LesserList *hll = map->lists + lindex;
  u32 listindex = 0;
  u8 *place = NULL;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, lh, listindex);
    if (!memcmp(place, key, map->keysize)) {
      break;
    }
  }
  if (listindex == hll->length)
    return NULL;
  return place + map->keysize;
}

usize HHMap_getKeySize(const HHMap *map) { return map->keysize; }
usize HHMap_getValSize(const HHMap *map) { return map->valsize; }
u32 HHMap_getMetaSize(const HHMap *map) { return map->metaSize; }
inline u32 HHMap_getBucketSize(const HHMap *map, u32 idx) { return map->lists[idx].length; }

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
  assertMessage(key && val);

  void *result = HHMap_get(map, key);
  if (result) {
    memcpy(val, result, map->valsize);
    return true;
  }
  return false;
}
void HHMap_fset(HHMap *map, const fptr key, void *val) {
  assertMessage(key.width <= HHMap_getKeySize(map));
  u8 *nname = ((u8 *)map->listHeads + map->metaSize * sizeof(void **));
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HHMap_getKeySize(map) - key.width);
  return HHMap_set(map, nname, val);
}
bool HHMap_fget(HHMap *map, const fptr key, void *val) {
  assertMessage(key.width <= HHMap_getKeySize(map));
  u8 *nname = ((u8 *)map->listHeads + map->metaSize * sizeof(void **));
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HHMap_getKeySize(map) - key.width);
  void *res = HHMap_get(map, nname);
  if (!res)
    return false;
  memcpy(val, res, map->valsize);
  return true;
}
void *HHMap_fget_ns(HHMap *map, const fptr key) {
  assertMessage(key.width <= HHMap_getKeySize(map));
  u8 *nname = ((u8 *)map->listHeads + map->metaSize * sizeof(void **));
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HHMap_getKeySize(map) - key.width);
  return HHMap_get(map, nname);
}
HHMap_both HHMap_getBoth(HHMap *map, const void *key) {
  u8 *place = (u8 *)HHMap_get(map, key);
  if (!place)
    return (HHMap_both){NULL, NULL};
  return (HHMap_both){place - map->valsize, place};
}

void HHMap_clear(HHMap *map) {
  for (u32 i = 0; i < map->metaSize; i++)
    map->lists[i].length = 0;
}

inline void *HHMap_getCoord(const HHMap *map, u32 bucket, u32 index) {
  if (bucket > map->metaSize)
    return NULL;
  HHMap_LesserList ll = map->lists[bucket];
  if (index > ll.length)
    return NULL;
  return (
      (u8 *)(map->listHeads[bucket]) +
      (map->valsize + map->keysize) * index
  );
}

#endif // HHMAP_C
