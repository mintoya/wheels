#if !defined(HMAP_H)
  #define HMAP_H (1)

typedef struct HMap HMap;
  #include "allocator.h"
  #include "assertMessage.h"
  #include "fptr.h"
  #include "mytypes.h"
  #include "sList.h"

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
    AllocatorV allocator,
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
void *HMap_getCoord(const HMap *hm, u32 index);
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

umax HMap_hash(const fptr str);
u32 HMap_getMetaSize(const HMap *map);
//
// iterator
//

struct HMapIterator_struct {
  const HMap *map;
  const void *current;
  u32 element_idx;
};
bool HMapIterator_valid(const struct HMapIterator_struct *it);
void HMapIterator_next(struct HMapIterator_struct *it);
struct HMapIterator_struct HMapIterator(const HMap *map);

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

  #define mHmap(Ta, Tb) ptrof(fnptrof((HMap *, Ta), Tb))

  #define mHmap_scoped(Ta, Tb) [[gnu::cleanup(HMap_cleanup_handler)]] mHmap(Ta, Tb)

  #define mHmap_init_length(allocator, keytype, valtype, maxHash, ...) ( \
      (mHmap(keytype, valtype))HMap_new(                                 \
          ({                                                             \
            struct T {                                                   \
              keytype a;                                                 \
              valtype b;                                                 \
            };                                                           \
            offsetof(struct T, b);                                       \
          }),                                                            \
          sizeof(valtype),                                               \
          allocator,                                                     \
          ((void)ASSERT_EXPR(maxHash != 0, ""), maxHash)                 \
      )                                                                  \
  )
  // optional bucket count argument
  #define mHmap_init(allocator, keytype, valtype, ...) \
    mHmap_init_length(allocator, keytype, valtype __VA_OPT__(, __VA_ARGS__), 32)
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

//{iterator

  #define FOREACH_HMap_init(map) ( \
      typeof(HMapIterator(map)),   \
      HMapIterator(map)            \
  )
  #define FOREACH_HMap_increase(is) HMapIterator_next(&is)
  #define FOREACH_HMap_valid(is) HMapIterator_valid(&is)
  #define FOREACH_HMap_cast(is) \
    (is.current)

  #define FOREACH_HMap_iter    \
    (                          \
        FOREACH_HMap_init,     \
        FOREACH_HMap_increase, \
        FOREACH_HMap_valid,    \
        FOREACH_HMap_cast)

  #define FOREACH_mHmap_init(map, keytype) (                  \
      struct {                                                \
        struct HMapIterator_struct _iter[1];                  \
        struct {                                              \
          keytype key;                                        \
          typeof((*map)(nullptr, (*(keytype *)nullptr))) val; \
        } item_type[0];                                       \
      },                                                      \
      {{HMapIterator((HMap *)map)}}                           \
  )
  #define FOREACH_mHmap_increase(is) HMapIterator_next(is._iter)
  #define FOREACH_mHmap_valid(is) HMapIterator_valid(is._iter)
  #define FOREACH_mHmap_cast(is) \
    ((typeof(is.item_type[0]) *)is._iter->current)

  #define FOREACH_mHmap_iter    \
    (                           \
        FOREACH_mHmap_init,     \
        FOREACH_mHmap_increase, \
        FOREACH_mHmap_valid,    \
        FOREACH_mHmap_cast)
//}

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
  #define mHmap_setManaged(map, load, growth_factor, key, value) ({ \
    if_unlikely (HMap_load((HMap *)map) >= load) {                  \
      HMap_manage(                                                  \
          (HMap **)&map,                                            \
          nullptr,                                                  \
          HMap_getMetaSize((HMap *)map) *                           \
              growth_factor                                         \
      );                                                            \
    };                                                              \
    mHmap_set(map, key, value);                                     \
  })

  #include "macros.h"
  #include "tests.h"
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
  foreach (var_ it, mHmap_iter(map, int)) {
    array[it->key] = 1;
    if (it->val != it->key * 2)
      return 1;
  }
  foreach (var_ i, vla(array))
    if (!i)
      return 1;
  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * i);

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
  foreach (var_ v, mHmap_iter(map, int))
    bcount++;
  usize ccount = 0;
  foreach (var_ v, HMap_iter((HMap *)map))
    ccount++;

  if (acount != bcount || acount != ccount)
    return 1;

  mHmap_clear(map);
  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (v)
      return 2;
  }

  return 0;
}
test_fn(HMap_open_test) {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 500);
  return HMap_test_structure(map);
}
test_fn(HMap_open_test_loaded) {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 1);
  return HMap_test_structure(map);
}
test_fn(HMap_transform_open_test) {
  mHmap(int, int) map = mHmap_init(allocator, int, int);
  var_ mapp = &map;
  defer { mHmap_deinit(*mapp); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 3);

  HMap_manage((HMap **)&map, allocator, 128);

  for (int i = 0, *v; (v = mHmap_get(map, i), i < 100); i++)
    if (!v || *v != i * 3)
      return 1;

  return 0;
}

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

  usize maxHash;
  usize count;
  msList(u8) flags;
  sList_header *data;
} HMap;
u32 HMap_getMetaSize(const HMap *map) { return map->maxHash; }
usize HMap_getKeySize(const HMap *map) { return map->keysize; }
usize HMap_getValSize(const HMap *map) { return map->valsize; }
u32 HMap_getHLen(const HMap *map) { return msList_len(map->flags); }
void *HMap_getCoord(const HMap *map, u32 index) {
  sList_header *ll = map->data;
  if (!ll || index >= ll->length)
    return nullptr;
  return (u8 *)ll->buf + (map->valsize + map->keysize) * index;
}

umax HMap_hash(const fptr str) {
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

HMap *HMap_new(u32 kSize, u32 vSize, AllocatorV allocator, usize maxHash) {
  assertMessage(kSize && vSize && allocator);
  assertMessage(maxHash);
  HMap *hm = (typeof(hm))aCreate(allocator, HMap);
  *hm = (HMap){
      .allocator = allocator,
      .keysize = kSize,
      .valsize = vSize,
      .maxHash = maxHash,
      .count = 0,
  };
  hm->data = sList_new(allocator, hm->maxHash, kSize + vSize);
  hm->flags = msList_init(allocator, u8, hm->maxHash);

  hm->data = sList_appendFromArr(allocator, hm->data, kSize + vSize, NULL, hm->maxHash);
  msList_pushArr(allocator, hm->flags, *VLAP((u8 *)nullptr, hm->maxHash));
  return hm;
}
void HMap_free(HMap *hm) {
  var_ allocator = hm->allocator;
  defer { aFree(allocator, hm, sizeof(*hm)); };
  sList_free(allocator, hm->data, hm->keysize + hm->valsize);
  msList_deInit(allocator, hm->flags);
}

struct HMap_inner_item HMap_get_inner_zero(const HMap *map, usize idx) {
  assertMessage(idx < map->data->length);
  struct HMap_inner_item res = {};
  res.flag = ((u8 *)(map->flags)) + idx;
  res.key = ((u8 *)(map->data->buf)) + idx * (map->keysize + map->valsize);
  res.val = (void *)((u8 *)res.key + map->keysize);
  return res;
}

__attribute__((const, always_inline)) void *LesserList_getref(const usize elw, const sList_header *hll, u32 idx) {
  return (u8 *)(hll->buf) + idx * (elw);
}

void HMap_manage(HMap **last, AllocatorV allocator, u32 maxHash) {
  assertMessage(last && *last);
  assertMessage(maxHash);

  var_ oldMap = *last;
  var_ kSize = oldMap->keysize;
  var_ vSize = oldMap->valsize;

  if (allocator == nullptr)
    allocator = oldMap->allocator;
  HMap *newMap = HMap_new(kSize, vSize, allocator, maxHash);

  var_ tembBuf = aCreate(allocator, u8, newMap->keysize + newMap->valsize);
  defer { aFree(allocator, tembBuf, newMap->keysize + newMap->valsize); };

  var_ tempKey = tembBuf;
  var_ tempVal = tembBuf + newMap->keysize;

  foreach (var_ i, range(0, oldMap->data->length))
    if (((u8 *)oldMap->flags)[i] == 1) {
      struct HMap_inner_item it = HMap_get_inner_zero(oldMap, i);
      memcpy(tempKey, it.key, kSize);
      memcpy(tempVal, it.val, vSize);
      HMap_set(newMap, tempKey, tempVal);
    }

  HMap_free(oldMap);
  *last = newMap;
}

void *HMap_get(const HMap *map, const void *key) {
  if (!key)
    return nullptr;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->maxHash);
  if (lindex >= map->data->length)
    return nullptr;
  struct HMap_inner_item it = HMap_get_inner_zero(map, lindex);
  while (it.flag[0] && memcmp(it.key, key, map->keysize)) {
    lindex++;
    if (lindex < map->data->length)
      it = HMap_get_inner_zero(map, lindex);
    else
      break;
  }
  if (lindex == map->data->length)
    return nullptr;
  return it.flag[0] == 1 ? it.val : nullptr;
}
void *HMap_set(HMap *map, const void *key, const void *val) {
  if (!key)
    return nullptr;

  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->maxHash);
  struct HMap_inner_item it = HMap_get_inner_zero(map, lindex);
  while (it.flag[0] && memcmp(it.key, key, map->keysize)) {
    lindex++;
    if (lindex < map->data->length)
      it = HMap_get_inner_zero(map, lindex);
    else
      break;
  }
  if (lindex == map->data->length) {
    map->data = sList_append(map->allocator, map->data, map->keysize + map->valsize, NULL);
    msList_push(map->allocator, map->flags, 0);
    it = HMap_get_inner_zero(map, lindex);
  }
  memcpy(it.key, key, map->keysize);
  if (val)
    memcpy(it.val, val, map->valsize);
  u8 before = it.flag[0] == 1;
  memcpy(it.flag, REF(u8, (u8)(val ? 1 : 2)), sizeof(u8));
  u8 after = it.flag[0] == 1;
  if (before && !after) map->count--;
  else if (!before && after) map->count++;
  return it.val;
}

u32 HMap_count(const HMap *map) { return map->count; }
u8 HMap_load(const HMap *map) {
  u32 cap = map->maxHash;
  u32 count = HMap_count(map);
  u32 load = (count * 100) / cap;
  return load < 0xff ? load : 0xff;
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
  memset(map->flags, 0, msList_len(map->flags) * sizeof(u8));
  map->count = 0;
}

//
// iterator
//

bool HMapIterator_valid(const struct HMapIterator_struct *it) {
  usize len = HMap_getHLen(it->map); // must match macro’s length
  return it->element_idx < len &&
         ((u8 *)it->map->flags)[it->element_idx] == 1;
}
void HMapIterator_next(struct HMapIterator_struct *it) {
  it->element_idx++;
  usize len = HMap_getHLen(it->map);
  while (it->element_idx < len &&
         ((u8 *)it->map->flags)[it->element_idx] != 1) {
    it->element_idx++;
  }
  it->current = HMap_getCoord(it->map, it->element_idx);
}
struct HMapIterator_struct HMapIterator(const HMap *map) {
  struct HMapIterator_struct it = {
      .map = map,
      .current = NULL,
      .element_idx = 0,
  };

  usize len = HMap_getHLen(map);
  while (it.element_idx < len &&
         ((u8 *)map->flags)[it.element_idx] != 1) {
    it.element_idx++;
  }

  it.current = HMap_getCoord(map, it.element_idx);
  return it;
}

#endif // HMap_C
