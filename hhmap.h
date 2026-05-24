#if !defined(HMAP_H)
  #define HMAP_H (1)
typedef struct HMap HMap;
  #include "allocator.h"
  #include "assertMessage.h"
  #include "fptr.h"
  #include "mytypes.h"
  #include "sList.h"
  #include <assert.h>
/**
 * Creates a new hash map
 * @param kSize size of key type
 * @param vSize size of value type
 * @param allocator allocator
 * @param metaSize number of buckets
 *     each bucket is a dynamic array
 * @param maxHash maximum hash size, only used if metaSize is zero
 *     mutually exclusive with metaSize, both will be used to mod
 *     the hash of the item
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
 * @param last last map
 * @param kSize size of key type
 * @param allocator allocator
 * @param vSize size of value type
 * @param metaSize number of buckets
 */
void HMap_manage(
    HMap **last,
    usize kSize,
    usize vSize,
    AllocatorV allocator,
    u32 metaSize,
    u32 maxHash
);
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
void *HMap_set(HMap *map, const void *key, const void *val);
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
void *HMap_fset(HMap *map, const fptr key, void *val);
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

//
// iterator
//

struct HMapIterator_struct_inner {
  const HMap *map;
  const void *current;
  u32 bucket_idx;
  u32 element_idx;
};
bool HMapIterator_valid(const struct HMapIterator_struct_inner *it);
void HMapIterator_next(struct HMapIterator_struct_inner *it);
struct HMapIterator_struct {
  struct HMapIterator_struct_inner state[1];
  typeof(&HMapIterator_valid) valid;
  typeof(&HMapIterator_next) next;
};
extern inline struct HMapIterator_struct HMapIterator(const HMap *map);

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

template <typename K, typename V>
struct mHmap {
  HMap *ptr;
  using KeyType = K;
  using ValType = V;
  struct Item {
    const K key;
    V val;
  };

  static inline mHmap init_length(AllocatorV allocator, u32 bucketcount, u32 maxHash) {
    return {HMap_new(offsetof(Item, val), sizeof(V), allocator, bucketcount, maxHash)};
  }

  inline void deinit() {
    if (ptr) {
      HMap_free(ptr);
      ptr = nullptr;
    }
  }

  inline void set(const K key, const V val) {
    HMap_fset(ptr, fptr{sizeof(K), (u8 *)&key}, (void *)&val);
  }

  inline V *get(const K key) const {
    return (V *)(HMap_fget_ns(ptr, fptr{sizeof(K), (u8 *)&key}));
  }

  inline void rem(const K &key) {
    K _k = key;
    HMap_fset(ptr, fptr{sizeof(K), (u8 *)&_k}, nullptr);
  }

  inline V *getOrSet(const K &key, const V &val) {
    V *res = get(key);
    if (!res) {
      set(key, val);
      res = get(key);
    }
    return res;
  }

  inline void clear() {
    HMap_clear(ptr);
  }
};

template <typename K, typename V>
static inline void mHmap_cleanup_handler(mHmap<K, V> *map) {
  if (map) map->deinit();
}

    #define mHmap(Ta, Tb) mHmap<Ta, Tb>
    #define mHmap_scoped(Ta, Tb) [[gnu::cleanup(mHmap_cleanup_handler<Ta, Tb>)]] mHmap<Ta, Tb>

    #define mHmap_init_length(allocator, keytype, valtype, bucketcount, maxHash, ...) \
      mHmap<keytype, valtype>::init_length(allocator, bucketcount, maxHash)

    #define mHmap_init(allocator, keytype, valtype, ...) \
      mHmap_init_length(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32, 32)

    #define mHmap_deinit(map) (map).deinit()
    #define mHmap_set(map, key, val) (map).set(key, val)
    #define mHmap_rem(map, key) (map).rem(key)
    #define mHmap_get(map, key) (map).get(key)
    #define mHmap_GetOrSet(map, key, val) (map).getOrSet(key, val)
    #define mHmap_clear(map) (map).clear()

    #define mHmap_iterator(map, ...) \
      HMapIterator((map).ptr), typeof(decltype(map)::Item *)

  #else
    #define mHmap(Ta, Tb) typeof(typeof(Tb(**)(HMap *, Ta)))

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
            bucketcount,                                                                \
            maxHash                                                                     \
        )                                                                               \
    )
    // optional bucket count argument
    #define mHmap_init(allocator, keytype, valtype, ...) \
      mHmap_init_length(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32, 32)
    #define mHmap_deinit(map) ({ HMap_free((HMap *)map); })

    #define mHmap_set(map, key, val)                       \
      do {                                                 \
        typeof(key) _k = (typeof(_k))(key);                \
        typeof((*map)(nullptr, key)) _v = (val);           \
        ASSERT_EXPR(                                       \
            types_eq(                                      \
                mHmap(typeof(_k), typeof(_v)), typeof(map) \
            ),                                             \
            ""                                             \
        );                                                 \
        HMap_fset(                                         \
            (HMap *)map,                                   \
            (fptr){sizeof(_k), (u8 *)&_k},                 \
            &_v                                            \
        );                                                 \
      } while (0)

    #define mHmap_iterator(map, keyType) \
      HMapIterator((HMap *)map),         \
          typeof(({  struct {                            \
          keyType key;                                 \
          typeof((*map)((HMap *)NULL, (keyType){})) val; \
        } _;  _; })) *

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

    #define mHmap_GetOrSet(sh, key, val) ({ \
      mHmap_get(sh, key)                    \
          ?: ({ mHmap_set(sh, key, val); mHmap_get(sh, key); });                         \
    })
    #define mHmap_clear(map) HMap_clear((HMap *)map)
  #endif

  // #include "tests.c"
  #if defined(MAKE_TEST_FN)
    #include "macros.h"
static inline int HMap_test_structure(mHmap(int, int) map) {
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 2);

  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (!v || *v != i * 2)
      return 1;
  }
  int array[100] = {};
  foreach (var_ it, iter(mHmap_iterator(map, int))) {
    array[it->key] = 1;
    if (it->val != it->key * 2)
      return 1;
  }
  foreach (var_ i, vla(*VLAP(array, 100))) {
    if (!i)
      return 1;
  }
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

  usize acount = HMap_count((HMap *)map);
  usize bcount = 0;
  foreach (var_ v, iter(mHmap_iterator(map, int)))
    bcount++;
  usize ccount = 0;
  foreach (var_ v, iter(HMapIterator((HMap *)map)))
    ccount++;

  if (acount != bcount || acount != ccount) {
    printf("%i  , %i , %i", acount, bcount, ccount);
    return 1;
  }

  mHmap_clear(map);
  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (v)
      return 2;
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
  usize maxHash;  // only used for 1d mode
  usize metaSize; // only used for 2d mode
  // usize
  sList_header *storage[/*metasize*/];
} HMap;
usize HMap_getKeySize(const HMap *map) { return map->keysize; }
usize HMap_getValSize(const HMap *map) { return map->valsize; }
u32 HMap_getMetaSize(const HMap *map) { return map->metaSize; }
u32 HMap_getHLen(const HMap *map) { return map->storage[1]->length; }
inline u32 HMap_getBucketSize(const HMap *map, u32 idx) { return map->storage[idx][0].length; }
void *HMap_getCoord(const HMap *map, u32 bucket, u32 index) {
  if (map->metaSize) {
    if (bucket >= map->metaSize)
      return nullptr;
    sList_header *ll = map->storage[bucket];
    if (!ll || index >= ll->length) // was index > ll->length
      return nullptr;
    return (u8 *)ll->buf + (map->valsize + map->keysize) * index;
  } else {
    if (bucket != 0)
      return nullptr;
    sList_header *ll = map->storage[0];
    if (!ll || index >= ll->length) // was index > ll->length
      return nullptr;
    return (u8 *)ll->buf + (map->valsize + map->keysize) * index;
  }
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
    usize totalSize = sizeof(HMap) + 2 * sizeof(sList_header);
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
    foreach (var_ s, span(hm->storage, hm->metaSize))
      s[0] = sList_new(allocator, 2, kSize + vSize);
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
  if (!hm->metaSize) {
    usize totalSize = sizeof(HMap) + 2 * sizeof(sList_header);
    defer { aFree(hm->allocator, hm, totalSize); };
    sList_free(hm->allocator, hm->storage[0], hm->keysize + hm->valsize);
    sList_free(hm->allocator, hm->storage[1], sizeof(u8));
  } else {
    usize totalSize = sizeof(HMap) + hm->metaSize * sizeof(sList_header);
    defer { aFree(hm->allocator, hm, totalSize); };
    foreach (var_ v, vla(*VLAP(hm->storage, hm->metaSize))) {
      sList_free(hm->allocator, v, hm->keysize + hm->valsize);
    };
  }
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
  defer { aFree(allocator, tembBuf, newMap->keysize + newMap->valsize); };

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

void *HMap_set_normal(HMap *map, const void *key, const void *val) {
  const usize elw = map->keysize + map->valsize;
  if (!key)
    return nullptr;
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
  return val ? place + map->keysize : nullptr;
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
void *HMap_set_open(HMap *map, const void *key, const void *val) {
  if (!key)
    return nullptr;
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
  return it.val;
}

void *HMap_set(HMap *map, const void *key, const void *val) {
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
    foreach (var_ v, vla(*VLAP(map->storage, map->metaSize))) {
      i += v->length;
    }
  } else {
    foreach (var_ v, vla(*VLAP(map->storage[1]->buf, map->storage[1]->length)))
      i += v == 1;
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
  foreach (var_ v, vla(*VLAP(map->storage, map->metaSize)))
    res += (elementSize)*v->length;
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
void *HMap_fset(HMap *map, const fptr key, void *val) {
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
    foreach (var_ v, vla(*VLAP(map->storage, map->metaSize))) {
      v->length = 0;
    };
  }
}

//
// iterator
//

bool HMapIterator_valid(const struct HMapIterator_struct_inner *it) {
  if (it->map->metaSize) {
    return it->bucket_idx < it->map->metaSize &&
           it->element_idx < it->map->storage[it->bucket_idx]->length;
  } else {
    usize len = HMap_getHLen(it->map); // must match macro’s length
    return it->element_idx < len &&
           ((u8 *)it->map->storage[1]->buf)[it->element_idx] == 1;
  }
}
void HMapIterator_next(struct HMapIterator_struct_inner *it) {
  it->element_idx++;
  if (it->map->metaSize) {
    while (it->bucket_idx < it->map->metaSize &&
           it->element_idx >= it->map->storage[it->bucket_idx]->length) {
      it->bucket_idx++;
      it->element_idx = 0;
    }
  } else {
    usize len = HMap_getHLen(it->map);
    while (it->element_idx < len &&
           ((u8 *)it->map->storage[1]->buf)[it->element_idx] != 1) {
      it->element_idx++;
    }
  }
  it->current = HMap_getCoord(it->map, it->bucket_idx, it->element_idx);
}
inline struct HMapIterator_struct HMapIterator(const HMap *map) {
  struct HMapIterator_struct it = {
      .state = {{
          .map = map,
          .current = NULL,
          .bucket_idx = 0,
          .element_idx = 0,
      }},
      .next = HMapIterator_next,
      .valid = HMapIterator_valid,
  };

  if (map->metaSize) {
    while (it.state[0].bucket_idx < map->metaSize &&
           map->storage[it.state[0].bucket_idx]->length == 0) {
      it.state[0].bucket_idx++;
    }
  } else {
    usize len = HMap_getHLen(map);
    while (it.state[0].element_idx < len &&
           ((u8 *)map->storage[1]->buf)[it.state[0].element_idx] != 1) {
      it.state[0].element_idx++;
    }
  }

  it.state[0].current = HMap_getCoord(map, it.state[0].bucket_idx, it.state[0].element_idx);
  return it;
}

static inline u8 HMap_guess_load(const HMap *map) {
  if (map->metaSize) {
    // 2D Mode: Sample the first two buckets.
    u32 b0 = HMap_getBucketSize(map, 0);
    u32 b1 = map->metaSize > 1 ? HMap_getBucketSize(map, 1) : 0;

    u32 est_load = (b0 + b1) * 50;
    return est_load < 0xff ? est_load : 0xff;
  }

  // 1D Open Addressing Mode: Sample up to the first 32 flags
  u32 cap = map->maxHash;
  if (!cap)
    return 0;

  u32 sample_size = cap < 32 ? cap : 32;
  u32 count = 0;
  u8 *flags = (u8 *)map->storage[1]->buf;

  for (u32 i = 0; i < sample_size; i++) {
    if (flags[i] == 1)
      count++;
  }

  u32 est_load = (count * 100) / sample_size;
  return est_load < 0xff ? est_load : 0xff;
}
void HMap_manage_linear(HMap **last) {
  assertMessage((**last).maxHash);
  HMap *map = *last;
  usize newCap = map->maxHash * 2;
  usize elw = map->keysize + map->valsize;

  // 1. Grab old buffers
  sList_header *oldData = map->storage[0];
  sList_header *oldFlags = map->storage[1];
  usize oldLen = oldData->length;

  // 2. Expand capacity and plug new lists directly into the existing wrapper
  map->maxHash = newCap;
  map->storage[0] = sList_new(map->allocator, newCap, elw);
  map->storage[1] = sList_new(map->allocator, newCap, sizeof(u8));

  map->storage[0] =
      sList_appendFromArr(map->allocator, map->storage[0], elw, NULL, newCap);
  map->storage[1] = sList_appendFromArr(map->allocator, map->storage[1], sizeof(u8), NULL, newCap);

  // 3. Blast the old elements into the new slots
  u8 *old_f = (u8 *)oldFlags->buf;
  u8 *old_d = (u8 *)oldData->buf;

  for (usize i = 0; i < oldLen; i++) {
    if (old_f[i] == 1) {
      u8 *key = old_d + (i * elw);
      u8 *val = key + map->keysize;

      // Since maxHash is updated, this natively slots them into the expanded
      // bounds
      HMap_set_open(map, key, val);
    }
  }

  // 4. Drop the old lists (you can axe these if your allocator is pure
  // arena-based)
  sList_free(map->allocator, oldData, elw);
  sList_free(map->allocator, oldFlags, sizeof(u8));
}
#endif // HMap_C
