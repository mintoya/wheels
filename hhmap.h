#include "allocator.h"
#include "assertMessage.h"
#include <assert.h>
#if !defined(HMAP_H)
  #define HMAP_H (1)
typedef struct HMap HMap;
  #include "fptr.h"
  #include "sList.h"
/**
 * Creates a new hash map
 * @param kSize size of key type
 * @param vSize size of value type
 * @param allocator allocator
 * @param metaSize number of buckets
 *     each bucket is a dynamic array
 * @param maxHash maximum hash size, only used if metaSize is zero
 *     mutually exclusive with metaSize, both will be used to bodulo the hash of the item
 * @return pointer to new HMap or NULL on failure
 */
HMap *HMap_new(
    u32 kSize,
    u32 vSize,
    AllocatorV allocator,
    usize metaSize,
    usize maxHash
);
/**
 * Creates a new hash, with same keys and vals as another hash map
 * key and val size are trimmed or padded with 0's
 * will replace last
 * `@param last last map
 * `@param kSize size of key type
 * `@param allocator allocator
 * `@param vSize size of value type
 * `@param metaSize number of buckets
 */
void HMap_manage(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize, u32 maxHash);
/**
 * free's hm
 * @param hm map
 */
void HMap_free(HMap *hm);
/**
 * get pointer to key from pointer to val
 * @param hm map
 * @param key pointer to key
 * @return pointer to value, null if not found
 */
void *HMap_get(const HMap *hm, const void *key);
/**
 * set pointer to key from pointer to val
 * @param hm map
 * @param key pointer to key
 * @param val pointer to value
 */
void HMap_set(HMap *map, const void *key, const void *val);
/**
 * @param hm map
 * @param bucket bucket index
 * @return length of bucket[bucket]
 */
u32 HMap_getBucketSize(const HMap *hm, u32 bucket);
/**
 * @param hm map
 * @return bucket count
 */
u32 HMap_getMetaSize(const HMap *);
u32 HMap_getHLen(const HMap *map);
/**
 * helper for looping through all values
 * @param hm map
 * @param bucket bucket index
 * @param index index inside bucket
 * @return pointer to key, get the val by adding your padding
 */
void *HMap_getCoord(const HMap *hm, u32 bucket, u32 index);
/**
 * @param hm map
 * @return total keys and vals
 */
u32 HMap_count(const HMap *map);
/**
 * deletes all keys and values
 * does not free memory
 * @param hm map
 */
void HMap_clear(HMap *map);
// extern inline void *HMap_getKey(const HMap *map, u32 n);
// extern inline void *HMap_getVal(const HMap *map, u32 n);
usize HMap_footprint(const HMap *map);
u8 HMap_load(const HMap *map);
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
 * @param hm map
 * @param key fat pointer to key
 * @param val pointer to key
 */
void HMap_fset(HMap *map, const fptr key, void *val);
/**
 * @param hm map
 * @param key fat pointer to key
 * @param val pointer to key
 *     not set if not found
 */
bool HMap_fget(HMap *map, const fptr key, void *val);
/**
 * like fset but just returns the pointer
 * @param hm map
 * @param key fat pointer to key
 * @return val pointer to val
 *     null
 */
void *HMap_fget_ns(HMap *map, const fptr key);

/**
 * @param vv pointer to HMap pointer
 */
static inline void HMap_cleanup_handler(void *vv) {
  HMap **v = (HMap **)vv;
  if (v && *v) {
    HMap_free(*v);
    *v = nullptr;
  }
}
struct HMap_inner_item {
  void *key, *val;
  u8 *flag;
};

struct HMap_inner_item HMap_get_inner_zero(const HMap *map, usize idx);

  #ifdef __cplusplus
    #include <type_traits>
template <typename Ta, typename Tb>
using mHmap_t = Tb (**)(HMap *, Ta);
    #define mHmap(Ta, Tb) mHmap_t<Ta, Tb>
  #else
    #define mHmap(Ta, Tb) typeof(typeof(Tb(**)(HMap *, Ta)))
  #endif

  #define mHmap_scoped(Ta, Tb) [[gnu::cleanup(HMap_cleanup_handler)]] mHmap(Ta, Tb)

  #define mHmap_init_length(allocator, keytype, valtype, bucketcount, maxHash, ...) ( \
      (mHmap(keytype, valtype))HMap_new(                                              \
          ({                                                                          \
            struct T {                                                                \
              keytype a;                                                              \
              valtype b;                                                              \
            };                                                                        \
            offsetof(struct T, b);                                                    \
          }),                                                                         \
          sizeof(valtype),                                                            \
          allocator,                                                                  \
          bucketcount, maxHash                                                        \
      )                                                                               \
  )
  // optional bucket count argument
  #define mHmap_init(allocator, keytype, valtype, ...) \
    mHmap_init_length(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32, 32)
  #define mHmap_deinit(map) ({ HMap_free((HMap *)map); })

  #define mHmap_set(map, key, val)                       \
    do {                                                 \
      typeof(key) _k = (key);                            \
      typeof((*map)(nullptr, key)) _v = (val);           \
      ASSERT_EXPR(                                       \
          types_eq(                                      \
              mHmap(typeof(_k), typeof(_v)), typeof(map) \
          ),                                             \
          ""                                             \
      );                                                 \
      HMap_fset(                                         \
          (HMap *)map,                                   \
          (fptr){sizeof(_k), (u8 *)&_k}, &_v             \
      );                                                 \
    } while (0)

  #define mHmap_foreach(map, keyType, keyDec, valType, valDec, ...)       \
    ASSERT_EXPR(                                                          \
        types_eq(                                                         \
            mHmap(typeof(keyType), typeof(valType)),                      \
            typeof(map)                                                   \
        ),                                                                \
        ""                                                                \
    );                                                                    \
    const usize _meta_count = HMap_getMetaSize((HMap *)map);              \
    if (_meta_count) {                                                    \
      for (each_RANGE(usize, _i, 0, _meta_count)) {                       \
        const usize _meta_subcount = HMap_getBucketSize((HMap *)map, _i); \
        for (each_RANGE(usize, _j, 0, _meta_subcount)) {                  \
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
      }                                                                   \
    } else {                                                              \
      for (each_RANGE(usize, _i, 0, HMap_getHLen((HMap *)map))) {         \
        var_ _local_inner = HMap_get_inner_zero((HMap *)map, _i);         \
        if (_local_inner.flag[0] != 1)                                    \
          continue;                                                       \
        const keyType keyDec = *(const keyType *)_local_inner.key;        \
        valType valDec = *(valType *)_local_inner.val;                    \
        {                                                                 \
          __VA_ARGS__                                                     \
        }                                                                 \
      }                                                                   \
    }

  #define mHmap_rem(map, key)                                  \
    do {                                                       \
      typeof(key) _k = (key);                                  \
      ASSERT_EXPR(                                             \
          types_eq(                                            \
              mHmap(typeof(_k), typeof((*map)(nullptr, key))), \
              typeof(map)                                      \
          ),                                                   \
          ""                                                   \
      );                                                       \
      HMap_fset(                                               \
          (HMap *)map,                                         \
          (fptr){                                              \
              sizeof(_k),                                      \
              (u8 *)&_k,                                       \
          },                                                   \
          nullptr                                              \
      );                                                       \
    } while (0)

  #define mHmap_get(map, key)                                   \
    ({                                                          \
      ASSERT_EXPR(                                              \
          types_eq(                                             \
              mHmap(typeof(key), typeof((*map)(nullptr, key))), \
              typeof(map)                                       \
          ),                                                    \
          ""                                                    \
      );                                                        \
      (typeof((*map)(nullptr, key)) *)                          \
          HMap_fget_ns(                                         \
              (HMap *)map,                                      \
              (fptr){sizeof(key), (u8 *)REF(typeof(key), key)}  \
          );                                                    \
    })

  #define mHmapGetOrSet(sh, key, val) ({ \
    mHmap_get(sh, key)                   \
        ?: ({ mHmap_set(sh, key, val); mHmap_get(sh, key); });                        \
  })
  #define mHmap_clear(map) HMap_clear((HMap *)map)
  #define HMap_scoped [[gnu::cleanup(HMap_cleanup_handler)]] HMap

  #if defined(MAKE_TEST_FN)
    #include "macros.h"
inline int HMap_test_structure(mHmap(int, int) map) {
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 2);

  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (!v || *v != i * 2)
      return 1;
  }
  int array[100] = {};
  mHmap_foreach(map, int, i, int, i2, {
    array[i] = 1;
    if (i2 != i * 2)
      return 1;
  });
  for_each_((var_ i, VLAP(array, 100)), {
    if (!i)
      return 1;
  });
  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * i);

  printf("map load : %i \n", (int)HMap_load((HMap *)map));

  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (!v || *v != i * i)
      return 1;
  }

  for (int i = 0; i < 100; i++)
    if (i % 2)
      mHmap_rem(map, i);
  for (int i = 0; i < 100; i++)
    if (!((!!mHmap_get(map, i)) ^ i % 2))
      return 1;

  mHmap_clear(map);
  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (v)
      return 1;
  }

  return 0;
}
MAKE_TEST_FN(HMap_basic_test, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 500, 0);
  return HMap_test_structure(map);
});
MAKE_TEST_FN(HMap_open_test, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 0, 500);
  return HMap_test_structure(map);
});
MAKE_TEST_FN(HMap_basic_test_loaded, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 1, 0);
  return HMap_test_structure(map);
});
MAKE_TEST_FN(HMap_open_test_loaded, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 0, 1);
  return HMap_test_structure(map);
});
MAKE_TEST_FN(HMap_transform_basic_test, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 8);
  var_ mapp = &map;
  defer { mHmap_deinit(*mapp); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 3);

  HMap_manage((HMap **)&map, sizeof(int), sizeof(int), allocator, 128, 0);

  for (int i = 0, *v; (v = mHmap_get(map, i), i < 100); i++)
    if (!v || *v != i * 3)
      return 1;

  return 0;
});
MAKE_TEST_FN(HMap_transform_open_test, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 0);
  var_ mapp = &map;
  defer { mHmap_deinit(*mapp); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 3);

  HMap_manage((HMap **)&map, sizeof(int), sizeof(int), allocator, 0, 128);

  for (int i = 0, *v; (v = mHmap_get(map, i), i < 100); i++)
    if (!v || *v != i * 3)
      return 1;

  return 0;
});
  #endif

#endif // HMap_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define HMAP_C (1)
#endif

#if defined(HMAP_C)
  #include <string.h>

typedef struct HMap {
  AllocatorV allocator;
  u32 keysize;
  u32 valsize;
  usize maxHash; // only used for open mode
  usize metaSize;
  // usize
  sList_header *storage[/*metasize*/];
} HMap;
usize HMap_getKeySize(const HMap *map) { return map->keysize; }
usize HMap_getValSize(const HMap *map) { return map->valsize; }
u32 HMap_getMetaSize(const HMap *map) { return map->metaSize; }
u32 HMap_getHLen(const HMap *map) { return map->storage[1]->length; }
inline u32 HMap_getBucketSize(const HMap *map, u32 idx) { return map->storage[idx][0].length; }
inline void *HMap_getCoord(const HMap *map, u32 bucket, u32 index) {
  if (bucket > map->metaSize)
    return nullptr;
  var_ ll = map->storage[bucket];
  if (index > ll->length)
    return nullptr;
  return (
      (u8 *)(map->storage[bucket]->buf) +
      (map->valsize + map->keysize) * index
  );
}

static inline umax HMap_hash(const fptr str) {
  umax hash = 5381;

  if (str.len == sizeof(umax)) {
    memcpy(&hash, str.ptr, sizeof(hash));
  } else {
    usize i = 0;
    for (; i + sizeof(umax) <= str.len; i += sizeof(umax)) {
      u64 part;
      memcpy(&part, str.ptr + i, sizeof(part));

      hash ^= part;
      hash *= 0xff51afd7ed558ccdULL;
      hash ^= hash >> 33;
    }

    for (; i < str.len; i++) {
      hash ^= str.ptr[i];
      hash *= 0x100000001b3ULL;
    }
  }

  hash ^= hash >> 33;
  hash *= 0xff51afd7ed558ccdULL;
  hash ^= hash >> 33;

  return hash;
}

HMap *HMap_new(u32 kSize, u32 vSize, AllocatorV allocator, usize metaSize, usize maxHash) {
  assertMessage(kSize && vSize && allocator);
  if (!metaSize) {
    assertMessage(maxHash);
    usize totalSize = sizeof(HMap) + 3 * sizeof(sList_header);
    HMap *hm = (typeof(hm))aAlloc(allocator, totalSize);
    *hm = (HMap){
        .allocator = allocator,
        .keysize = kSize,
        .valsize = vSize,
        .maxHash = maxHash,
        .metaSize = 0,
    };
    hm->storage[0] = sList_new(allocator, hm->maxHash, kSize + vSize);
    hm->storage[1] = sList_new(allocator, hm->maxHash, sizeof(u8));

    hm->storage[0] = sList_appendFromArr(allocator, hm->storage[0], kSize + vSize, NULL, hm->maxHash);
    hm->storage[1] = sList_appendFromArr(allocator, hm->storage[1], sizeof(u8), NULL, hm->maxHash);
    return hm;
  } else {
    usize totalSize = sizeof(HMap) + metaSize * sizeof(sList_header);
    HMap *hm = (typeof(hm))aAlloc(allocator, totalSize);
    *hm = (HMap){
        .allocator = allocator,
        .keysize = kSize,
        .valsize = vSize,
        .metaSize = metaSize,
    };
    for_each_P((sList_header * *v, VLAP(hm->storage, hm->metaSize)), {
      *v = sList_new(allocator, 2, kSize + vSize);
    });
    return hm;
  }
}

struct HMap_inner_item HMap_get_inner_zero(const HMap *map, usize idx) {
  assertMessage(idx < map->storage[0]->length);
  struct HMap_inner_item res = {};
  res.flag = ((u8 *)(map->storage[1]->buf)) + idx;
  res.key = ((u8 *)(map->storage[0]->buf)) + idx * (map->keysize + map->valsize);
  res.val = (void *)((u8 *)res.key + map->keysize);
  return res;
}

void HMap_free(HMap *hm) {
  AllocatorV allocator = hm->allocator;
  const u32 msize = hm->metaSize ?: 2;
  for_each_((var_ s, VLAP(hm->storage, msize)), {
    aFree(allocator, s);
  });
  aFree(allocator, hm);
}

__attribute__((const, always_inline)) void *LesserList_getref(const usize elw, const sList_header *hll, u32 idx) {
  return (u8 *)(hll->buf) + idx * (elw);
}

void HMap_manage(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize, u32 maxHash) {
  assertMessage(last && *last);
  assertMessage(metaSize || maxHash);

  HMap *oldMap = *last;
  if (kSize == 0)
    kSize = oldMap->keysize;
  if (vSize == 0)
    vSize = oldMap->valsize;
  if (allocator == nullptr)
    allocator = oldMap->allocator;
  HMap *newMap = HMap_new(kSize, vSize, allocator, metaSize, maxHash);

  var_ tembBuf = aCreate(allocator, u8, newMap->keysize + newMap->valsize);
  defer { aFree(allocator, tembBuf); };

  var_ tempKey = tembBuf;
  var_ tempVal = tembBuf + newMap->keysize;

  usize keyCopySize = (oldMap->keysize < kSize) ? oldMap->keysize : kSize;
  usize valCopySize = (oldMap->valsize < vSize) ? oldMap->valsize : vSize;

  if (!oldMap->metaSize) {
    for (usize i = 0; i < oldMap->storage[0]->length; i++) {
      u8 flag = ((u8 *)oldMap->storage[1]->buf)[i];
      if (flag != 1)
        continue; // skip empty (0) and deleted (2)
      struct HMap_inner_item it = HMap_get_inner_zero(oldMap, i);
      memset(tempKey, 0, kSize);
      memcpy(tempKey, it.key, keyCopySize);
      memset(tempVal, 0, vSize);
      memcpy(tempVal, it.val, valCopySize);
      HMap_set(newMap, tempKey, tempVal);
    }
  } else {
    for (usize i = 0; i < oldMap->metaSize; i++) {
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
  }

  HMap_free(oldMap);
  *last = newMap;
}

void HMap_set_normal(HMap *map, const void *key, const void *val) {
  const usize elw = map->keysize + map->valsize;
  if (!key)
    return;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  sList_header **list = map->storage + lindex;
  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < list[0]->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, list[0], listindex);
    if (!memcmp(place, key, map->keysize))
      break;
  }
  if (listindex == list[0]->length) {
    list[0] = sList_append(map->allocator, list[0], elw, NULL);
    place = (u8 *)LesserList_getref(elw, list[0], list[0]->length - 1);
  }
  memcpy(place, key, map->keysize);
  val ? (void)memcpy(place + map->keysize, val, map->valsize)
      : (void)sList_remove(list[0], elw, listindex);
}
void *HMap_get_normal(const HMap *map, const void *key) {
  if (!key)
    return nullptr;

  const usize elw = map->keysize + map->valsize;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  sList_header *const *hll = &(map->storage[lindex]);

  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < hll[0]->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, *hll, listindex);
    if (!memcmp(place, key, map->keysize)) {
      break;
    }
  }
  if (listindex == hll[0]->length)
    return nullptr;
  return place + map->keysize;
}

void *HMap_get_open(const HMap *map, const void *key) {
  if (!key)
    return nullptr;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->maxHash);
  if (lindex >= map->storage[0]->length)
    return nullptr;
  struct HMap_inner_item it = HMap_get_inner_zero(map, lindex);
  while (it.flag[0] && memcmp(it.key, key, map->keysize)) {
    lindex++;
    if (lindex < map->storage[0]->length)
      it = HMap_get_inner_zero(map, lindex);
    else
      break;
  }
  if (lindex == map->storage[0]->length)
    return nullptr;
  return it.flag[0] == 1 ? it.val : nullptr;
}
void HMap_set_open(HMap *map, const void *key, const void *val) {
  if (!key)
    return;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->maxHash);
  struct HMap_inner_item it = HMap_get_inner_zero(map, lindex);
  while (it.flag[0] && memcmp(it.key, key, map->keysize)) {
    lindex++;
    if (lindex < map->storage[0]->length)
      it = HMap_get_inner_zero(map, lindex);
    else
      break;
  }
  if (lindex == map->storage[0]->length) {
    map->storage[0] = sList_append(map->allocator, map->storage[0], map->keysize + map->valsize, NULL);
    map->storage[1] = sList_append(map->allocator, map->storage[1], sizeof(u8), NULL);
    it = HMap_get_inner_zero(map, lindex);
  }
  memcpy(it.key, key, map->keysize);
  if (val)
    memcpy(it.val, val, map->valsize);
  memcpy(it.flag, REF(u8, (u8)(val ? 1 : 2)), sizeof(u8));
}

void HMap_set(HMap *map, const void *key, const void *val) {
  if (map->metaSize)
    return HMap_set_normal(map, key, val);
  else
    return HMap_set_open(map, key, val);
}
void *HMap_get(const HMap *map, const void *key) {
  if (map->metaSize)
    return HMap_get_normal(map, key);
  else
    return HMap_get_open(map, key);
}
u32 HMap_count(const HMap *map) {
  u32 i = 0;
  if (map->metaSize) {
    for_each_((var_ v, VLAP(map->storage, map->metaSize)), {
      i += v->length;
    });
  } else {
    for_each_((var_ v, VLAP(map->storage[1]->buf, map->storage[1]->length)), {
      i += v == 1;
    });
  }
  return i;
}
u8 HMap_load(const HMap *map) {
  u32 count = HMap_count(map), cap = map->metaSize ?: map->maxHash;
  u32 load = (count * 100) / cap;
  return load < 0xff ? load : 0xff;
}

usize HMap_footprint(const HMap *map) {
  usize res = 0;
  usize elementSize = map->keysize + map->valsize;

  res += sizeof(HMap);
  res += sizeof(*(map->storage)) * map->metaSize;
  res += elementSize;
  for_each_((var_ v, VLAP(map->storage, map->metaSize)), {
    res += (elementSize)*v->length;
  });
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
  assertMessage(key.len <= HMap_getKeySize(map));
  if (key.len == map->keysize)
    return HMap_set(map, key.ptr, val);
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  return HMap_set(map, nname, val);
}
bool HMap_fget(HMap *map, const fptr key, void *val) {
  assertMessage(key.len <= HMap_getKeySize(map));
  if (key.len == map->keysize)
    return HMap_get(map, key.ptr);
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  void *res = HMap_get(map, nname);
  if (!res)
    return false;
  memcpy(val, res, map->valsize);
  return true;
}
void *HMap_fget_ns(HMap *map, const fptr key) {
  assertMessage(key.len <= HMap_getKeySize(map));
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  return HMap_get(map, nname);
}
HMap_both HMap_getBoth(HMap *map, const void *key) {
  u8 *place = (u8 *)HMap_get(map, key);
  if (!place)
    return (HMap_both){nullptr, nullptr};
  return (HMap_both){place - map->keysize, place};
}

void HMap_clear(HMap *map) {

  if (!map->metaSize) {
    memset(map->storage[1]->buf, 0, map->storage[1]->length * sizeof(u8));
  } else {
    for_each_((var_ v, VLAP(map->storage, map->metaSize)), {
      v->length = 0;
    });
  }
}
#endif // HMap_C
