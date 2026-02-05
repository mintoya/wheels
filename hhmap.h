#include "allocator.h"
#include "assertMessage.h"
#include <string.h>
#if !defined(HMAP_H)
  #define HMAP_H (1)
typedef struct HMap HMap;
  #include "fptr.h"
  #include "my-list.h"
/**
 * Creates a new hash map
 * `@param` **kSize** size of key type
 * `@param` **vSize** size of value type
 * `@param` **allocator** allocator
 * `@param` **metaSize** number of buckets
 *     each bucket is a dynamic array
 * `@return` pointer to new HMap or NULL on failure
 */
HMap *HMap_new(
    u32 kSize,
    u32 vSize,
    AllocatorV allocator,
    u32 metaSize
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
void HMap_transform(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize);
/**
 * free's **hm**
 * `@param` **hm** map
 */
void HMap_free(HMap *hm);
/**
 * get pointer to key from pointer to val
 * `@param` **hm** map
 * `@param` **key** pointer to key
 * `@return` pointer to value, null if not found
 */
void *HMap_get(const HMap *hm, const void *key);
/**
 * set pointer to key from pointer to val
 * `@param` **hm** map
 * `@param` **key** pointer to key
 * `@param` **val** pointer to value
 */
void HMap_set(HMap *map, const void *key, const void *val);
/**
 * `@param` **hm** map
 * `@param` **bucket** bucket index
 * `@return` length of bucket[**bucket**]
 */
u32 HMap_getBucketSize(const HMap *hm, u32 bucket);
/**
 * `@param` **hm** map
 * `@return` bucket count
 */
u32 HMap_getMetaSize(const HMap *);
/**
 * helper for looping through all values
 * `@param` **hm** map
 * `@param` **bucket** bucket index
 * `@param` **index** index inside bucket
 * `@return` pointer to key, get the val by adding your padding
 */
void *HMap_getCoord(const HMap *hm, u32 bucket, u32 index);
/**
 * `@param` **hm** map
 * `@return` total keys and vals
 */
u32 HMap_count(const HMap *map);
/**
 * deletes all keys and values
 * does not free memory
 * `@param` **hm** map
 */
void HMap_clear(HMap *map);
/**
 * used by fset and fget
 * `@return` aligned memory big enough for key
 */
[[gnu::pure, gnu::assume_aligned(alignof(max_align_t))]]
u8 *HMap_getKeyBuffer(const HMap *map);
extern inline void *HMap_getKey(const HMap *map, u32 n);
extern inline void *HMap_getVal(const HMap *map, u32 n);
usize HMap_footprint(const HMap *map);
u32 HMap_countCollisions(const HMap *map);
usize HMap_getKeySize(const HMap *map);
usize HMap_getValSize(const HMap *map);
// resizes all buckets, can do this before lots of insertions
// void HMap_fatten(const HMap *map, usize bucketLength);
typedef struct HMap_both {
  void *key;
  void *val;
} HMap_both;
HMap_both HMap_getBoth(HMap *map, const void *key);

/**
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@param` **val** pointer to key
 */
void HMap_fset(HMap *map, const fptr key, void *val);
/**
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@param` **val** pointer to key
 *     not set if not found
 */
bool HMap_fget(HMap *map, const fptr key, void *val);
/**
 * like fset but just returns the pointer
 * `@param` **hm** map
 * `@param` **key** fat pointer to key
 * `@return` **val** pointer to val
 *     null
 */
void *HMap_fget_ns(HMap *map, const fptr key);

/**
 * `@param` **vv** pointer to HMap pointer
 */
static inline void HMap_cleanup_handler(void *vv) {
  HMap **v = (HMap **)vv;
  if (v && *v) {
    HMap_free(*v);
    *v = nullptr;
  }
}

  #ifdef __cplusplus
    #include <type_traits>
template <typename Ta, typename Tb>
using mHmap_t = Tb (**)(HMap *, Ta);
    #define mHmap(Ta, Tb) mHmap_t<Ta, Tb>
    #define equaltypes_mHmap(T1, T2) std::is_same<T1, T2>::value
  #else
    #define mHmap(Ta, Tb) typeof(typeof(Tb(**)(HMap *, Ta)))
    #define equaltypes_mHmap(T1, T2) \
      (_Generic((typeof(T2) *)nullptr, typeof(T1) *: true, default: false))
  #endif

  #define mHmap_scoped(Ta, Tb) [[gnu::cleanup(HMap_cleanup_handler)]] mHmap(Ta, Tb)

  #define mHmap_init_length(allocator, keytype, valtype, bucketcount, ...) ( \
      (mHmap(keytype, valtype))HMap_new(                                     \
          ({                                                                 \
            struct T {                                                       \
              keytype a;                                                     \
              valtype b;                                                     \
            };                                                               \
            offsetof(struct T, b);                                           \
          }),                                                                \
          sizeof(valtype),                                                   \
          allocator,                                                         \
          bucketcount                                                        \
      )                                                                      \
  )
  // optional bucket count argument
  #define mHmap_init(allocator, keytype, valtype, ...) \
    mHmap_init_length(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32)
  #define mHmap_deinit(map) ({ HMap_free((HMap *)map); })

  #define mHmap_set(map, key, val)                       \
    do {                                                 \
      typeof(key) _k = (key);                            \
      typeof((*map)(nullptr, key)) _v = (val);           \
      static_assert(                                     \
          equaltypes_mHmap(                              \
              mHmap(typeof(_k), typeof(_v)), typeof(map) \
          ),                                             \
          ""                                             \
      );                                                 \
      HMap_fset(                                         \
          (HMap *)map,                                   \
          fptr_fromTypeDef(_k),                          \
          &_v                                            \
      );                                                 \
    } while (0)

  #define mHmap_foreach(map, keyType, keyDec, valType, valDec, ...)     \
    static_assert(                                                      \
        equaltypes_mHmap(                                               \
            mHmap(typeof(keyType), typeof(valType)), typeof(map)        \
        ),                                                              \
        ""                                                              \
    );                                                                  \
    const usize _meta_count = HMap_getMetaSize((HMap *)map);            \
    for (usize _i = 0; _i < _meta_count; _i++) {                        \
      const usize _meta_subcount = HMap_getBucketSize((HMap *)map, _i); \
      for (usize _j = 0; _j < _meta_subcount; _j++) {                   \
        struct {                                                        \
          const keyType a;                                              \
          valType b;                                                    \
        } *_element;                                                    \
        _element = (typeof(_element))                                   \
            HMap_getCoord(                                              \
                (HMap *)map,                                            \
                _i, _j                                                  \
            );                                                          \
        const keyType keyDec = _element->a;                             \
        valType valDec = _element->b;                                   \
        {                                                               \
          __VA_ARGS__                                                   \
        }                                                               \
      }                                                                 \
    }

  #define mHmap_rem(map, key)                                   \
    do {                                                        \
      static_assert(                                            \
          equaltypes_mHmap(                                     \
              mHmap(typeof(key), typeof((*map)(nullptr, key))), \
              typeof(map)                                       \
          ),                                                    \
          ""                                                    \
      );                                                        \
      HMap_fset(                                                \
          (HMap *)map,                                          \
          fptr_fromTypeDef(key),                                \
          nullptr                                               \
      );                                                        \
    } while (0)

  #define mHmap_get(map, key)                                   \
    ({                                                          \
      static_assert(                                            \
          equaltypes_mHmap(                                     \
              mHmap(typeof(key), typeof((*map)(nullptr, key))), \
              typeof(map)                                       \
          ),                                                    \
          ""                                                    \
      );                                                        \
      (typeof((*map)(nullptr, key)) *)                          \
          HMap_fget_ns(                                         \
              (HMap *)map,                                      \
              fptr_fromTypeDef(key)                             \
          );                                                    \
    })
  #define HMap_scoped [[gnu::cleanup(HMap_cleanup_handler)]] HMap

#endif // HMap_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define HMAP_C (1)
#endif
#ifdef HMAP_C
  #include "fptr.h"

typedef struct HMap {
  AllocatorV allocator;
  u32 keysize;
  u32 valsize;
  usize metaSize;
  [[clang::counted_by(metaSize)]] struct {
    u32 length;
    u32 capacity;
    void *ptr;
  } storage[];
} HMap;
typedef typeof((*((HMap *)nullptr)->storage)) HMap_LesserList;

static inline umax HMap_hash(const fptr str) {
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
HMap *HMap_new(u32 kSize, u32 vSize, AllocatorV allocator, u32 metaSize) {
  assertMessage(kSize && vSize && metaSize && allocator);

  usize totalSize =
      sizeof(HMap) +
      metaSize * sizeof(*(((HMap *)nullptr)->storage)) +
      alignof(max_align_t) * 2 +
      (kSize + vSize);
  HMap *hm = (HMap *)aAlloc(allocator, totalSize);
  *hm = (HMap){
      .allocator = allocator,
      .keysize = kSize,
      .valsize = vSize,
      .metaSize = metaSize,
  };
  memset(hm->storage, 0, metaSize * sizeof(*hm->storage));
  return hm;
}
[[gnu::pure, gnu::assume_aligned(alignof(max_align_t))]]
u8 *HMap_getKeyBuffer(const HMap *map) {
  return (
      (u8 *)lineup(
          (uptr)((u8 *)map) + sizeof(*map) + map->metaSize * sizeof(*(map->storage)),
          alignof(max_align_t)
      )
  );
}

[[gnu::pure]]
/**
 * used by fset and fget
 * `@return` aligned memory big enough for value
 */
u8 *HMap_getValBuffer(const HMap *map) {
  return (u8 *)lineup(
      (uptr)(u8 *)HMap_getKeyBuffer(map) + map->keysize, alignof(max_align_t)
  );
}
void HMap_free(HMap *hm) {
  AllocatorV allocator = hm->allocator;
  const u32 msize = hm->metaSize;
  for (int i = 0; i < msize; i++)
    if (hm->storage[i].ptr)
      aFree(allocator, hm->storage[i].ptr);
  aFree(allocator, hm);
}

void HMap_transform(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize) {
  assertMessage(last && *last);

  HMap *oldMap = *last;
  if (kSize == 0)
    kSize = oldMap->keysize;
  if (vSize == 0)
    vSize = oldMap->valsize;
  if (allocator == nullptr)
    allocator = oldMap->allocator;
  if (metaSize == 0)
    metaSize = oldMap->metaSize;
  HMap *newMap = HMap_new(kSize, vSize, allocator, metaSize);

  u8 *tempKey = HMap_getKeyBuffer(newMap);
  u8 *tempVal = HMap_getValBuffer(newMap);

  usize keyCopySize = (oldMap->keysize < kSize) ? oldMap->keysize : kSize;
  usize valCopySize = (oldMap->valsize < vSize) ? oldMap->valsize : vSize;

  if (newMap->keysize < oldMap->keysize)
    tempKey = HMap_getKeyBuffer(oldMap);
  if (newMap->valsize < oldMap->valsize)
    tempVal = HMap_getValBuffer(oldMap);

  for (usize i = 0; i < HMap_getMetaSize(oldMap); i++) {
    for (usize j = 0; j < HMap_getBucketSize(oldMap, i); j++) {
      u8 *oldKey = (u8 *)HMap_getCoord(oldMap, i, j);
      u8 *oldVal = oldKey + oldMap->keysize;
      if (!oldKey)
        continue;
      memset(tempKey, 0, kSize);
      memcpy(tempKey, oldKey, keyCopySize);
      memset(tempVal, 0, vSize);
      memcpy(tempVal, oldVal, valCopySize);

      HMap_set(newMap, tempKey, tempVal);
    }
  }

  HMap_free(oldMap);

  *last = newMap;
}
[[gnu::always_inline]] static void *LesserList_getref(usize elw, const HMap_LesserList *hll, u32 idx) {
  return (u8 *)(hll->ptr) + idx * (elw);
}
[[gnu::always_inline]] static void LesserList_appendGarbage(usize elw, HMap_LesserList *hll, AllocatorV allocator) {
  if (!hll->capacity) {
    hll->ptr = aAlloc(allocator, elw);
    if (allocator->size)
      hll->capacity = allocator->size(allocator, hll->ptr) / elw;
    else
      hll->capacity = 1;
  }
  if (hll->capacity < hll->length + 1) {
    hll->ptr = aRealloc(allocator, hll->ptr, hll->capacity * 2 * elw);
    if (allocator->size)
      hll->capacity = allocator->size(allocator, hll->ptr) / elw;
    else
      hll->capacity *= 2;
  }
  hll->length++;
}
void HMap_set(HMap *map, const void *key, const void *val) {
  usize elw = map->keysize + map->valsize;
  if (!key)
    return;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  HMap_LesserList *hll = &(map->storage[lindex]);
  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, listindex);
    if (!memcmp(place, key, map->keysize))
      break;
  }
  if (listindex == hll->length) {
    LesserList_appendGarbage(elw, hll, map->allocator);
    place = (u8 *)LesserList_getref(elw, hll, hll->length - 1);
  }
  memcpy(place, key, map->keysize);
  if (val)
    memcpy(place + map->keysize, val, map->valsize);
  else {
    memmove(place, place + elw, (hll->length - listindex - 1) * elw);
    hll->length--;
  }
}

void *HMap_get(const HMap *map, const void *key) {
  if (!key) {
    return nullptr;
  }
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  usize elw = map->keysize + map->valsize;
  const HMap_LesserList *hll = &(map->storage[lindex]);

  if (!hll->ptr)
    return nullptr;
  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, listindex);
    if (!memcmp(place, key, map->keysize)) {
      break;
    }
  }
  if (listindex == hll->length)
    return nullptr;
  return place + map->keysize;
}

usize HMap_getKeySize(const HMap *map) { return map->keysize; }
usize HMap_getValSize(const HMap *map) { return map->valsize; }
u32 HMap_getMetaSize(const HMap *map) { return map->metaSize; }
inline u32 HMap_getBucketSize(const HMap *map, u32 idx) { return map->storage[idx].length; }

inline void *HMap_getKey(const HMap *map, u32 n) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize && i <= n;) {
    if (i + map->storage[lindex].length > n) {
      usize s = map->keysize + map->valsize;
      usize i2 = n - i;
      return (u8 *)map->storage[lindex].ptr + s * i2;
    }
    i += map->storage[lindex].length;
    lindex++;
  }
  return nullptr;
}

inline void *HMap_getVal(const HMap *map, u32 n) {
  u8 *place = (u8 *)HMap_getKey(map, n);
  if (place)
    return place + map->keysize;
  return nullptr;
}

u32 HMap_count(const HMap *map) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    i += map->storage[lindex].length;
    lindex++;
  }
  return i;
}

u32 HMap_countCollisions(const HMap *map) {
  u32 collisions = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    if (map->storage[lindex].length)
      collisions += map->storage[lindex].length - 1;
    lindex++;
  }
  return collisions;
}

usize HMap_footprint(const HMap *map) {
  usize res = 0;
  usize elementSize = map->keysize + map->valsize;

  res += sizeof(HMap);
  res += sizeof(*(map->storage)) * map->metaSize;
  res += elementSize;
  for (u32 lindex = 0; lindex < map->metaSize; lindex++)
    if (map->storage[lindex].length)
      res += (elementSize)*map->storage[lindex].capacity;
  return res;
}
bool HMap_getSet(HMap *map, const void *key, void *val) {
  assertMessage(key && val);

  void *result = HMap_get(map, key);
  if (result) {
    memcpy(val, result, map->valsize);
    return true;
  }
  return false;
}
void HMap_fset(HMap *map, const fptr key, void *val) {
  assertMessage(key.width <= HMap_getKeySize(map));
  u8 *nname = HMap_getKeyBuffer(map);
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HMap_getKeySize(map) - key.width);
  return HMap_set(map, nname, val);
}
bool HMap_fget(HMap *map, const fptr key, void *val) {
  assertMessage(key.width <= HMap_getKeySize(map));
  u8 *nname = HMap_getKeyBuffer(map);
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HMap_getKeySize(map) - key.width);
  void *res = HMap_get(map, nname);
  if (!res)
    return false;
  memcpy(val, res, map->valsize);
  return true;
}
void *HMap_fget_ns(HMap *map, const fptr key) {
  assertMessage(key.width <= HMap_getKeySize(map));
  u8 *nname = HMap_getKeyBuffer(map);
  memcpy(nname, key.ptr, key.width);
  memset(nname + key.width, 0, HMap_getKeySize(map) - key.width);
  return HMap_get(map, nname);
}
HMap_both HMap_getBoth(HMap *map, const void *key) {
  u8 *place = (u8 *)HMap_get(map, key);
  if (!place)
    return (HMap_both){nullptr, nullptr};
  return (HMap_both){place - map->keysize, place};
}

void HMap_clear(HMap *map) {
  for (u32 i = 0; i < map->metaSize; i++)
    map->storage[i].length = 0;
}

inline void *HMap_getCoord(const HMap *map, u32 bucket, u32 index) {
  if (bucket > map->metaSize)
    return nullptr;
  HMap_LesserList ll = map->storage[bucket];
  if (index > ll.length)
    return nullptr;
  return (
      (u8 *)(map->storage[bucket].ptr) +
      (map->valsize + map->keysize) * index
  );
}

#endif // HMap_C
