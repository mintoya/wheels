#include "allocator.h"
#include "assertMessage.h"
#if !defined(HMAP_H)
  #define HMAP_H (1)
typedef struct HMap HMap;
  #include "fptr.h"
/**
 * Creates a new hash map
 * @param kSize size of key type
 * @param vSize size of value type
 * @param allocator allocator
 * @param metaSize number of buckets
 *     each bucket is a dynamic array
 * @return pointer to new HMap or NULL on failure
 */
HMap *HMap_new(
    u32 kSize,
    u32 vSize,
    AllocatorV allocator,
    usize metaSize
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
void HMap_transform(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize);
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
/**
 * used by fset and fget
 * @return aligned memory big enough for key
 */
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

  #ifdef __cplusplus
    #include <type_traits>
template <typename Ta, typename Tb>
using mHmap_t = Tb (**)(HMap *, Ta);
    #define mHmap(Ta, Tb) mHmap_t<Ta, Tb>
  // #define equaltypes_mHmap(T1, T2) std::is_same<T1, T2>::value
  #else
    #define mHmap(Ta, Tb) typeof(typeof(Tb(**)(HMap *, Ta)))
  #endif
  #define equaltypes_mHmap(T1, T2) \
    (_Generic((typeof(T2) *)nullptr, typeof(T1) *: true, default: false))

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
          (fptr){sizeof(_k), (u8 *)&_k}, &_v             \
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

  #define mHmap_rem(map, key)                                  \
    do {                                                       \
      typeof(key) _k = (key);                                  \
      static_assert(                                           \
          equaltypes_mHmap(                                    \
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
              (fptr){sizeof(key), (u8 *)REF(typeof(key), key)}  \
          );                                                    \
    })
  #define mHmap_clear(map) HMap_clear((HMap *)map)
  #define HMap_scoped [[gnu::cleanup(HMap_cleanup_handler)]] HMap

  #if defined(MAKE_TEST_FN)
    #include "macros.h"
MAKE_TEST_FN(HMap_basic_set_get_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int);
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 100; i++)
    mHmap_set(map, i, i * 2);

  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (!v || *v != i * 2)
      return 1;
  }

  return 0;
});
MAKE_TEST_FN(HMap_overwrite_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int);
  defer { mHmap_deinit(map); };

  mHmap_set(map, 5, 10);
  mHmap_set(map, 5, 99);

  int *v = mHmap_get(map, 5);
  if (!v || *v != 99)
    return 1;

  return 0;
});
MAKE_TEST_FN(HMap_remove_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int);
  defer { mHmap_deinit(map); };

  mHmap_set(map, 7, 77);
  mHmap_rem(map, 7);

  if (mHmap_get(map, 7) != NULL)
    return 1;

  return 0;
});
MAKE_TEST_FN(HMap_count_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 8);
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 200; i++) {
    mHmap_set(map, i, i);
  }

  if (HMap_count((HMap *)map) != 200)
    return 1;

  // collisions should exist with small bucket count
  if (HMap_countCollisions((HMap *)map) == 0)
    return 1;

  return 0;
});
MAKE_TEST_FN(HMap_clear_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int);
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 100; i++) {
    mHmap_set(map, i, i);
  }

  mHmap_clear(map);

  if (HMap_count((HMap *)map) != 0)
    return 1;

  for (int i = 0; i < 100; i++) {
    if (mHmap_get(map, i) != NULL)
      return 1;
  }

  return 0;
});
MAKE_TEST_FN(HMap_transform_macro, {
  mHmap(int, int) map = mHmap_init(allocator, int, int, 8);
  defer { mHmap_deinit(map); };

  for (int i = 0; i < 100; i++) {
    mHmap_set(map, i, i * 3);
  }

  HMap_transform((HMap **)&map, sizeof(int), sizeof(int), allocator, 128);

  for (int i = 0; i < 100; i++) {
    int *v = mHmap_get(map, i);
    if (!v || *v != i * 3)
      return 1;
  }

  return 0;
});

  #endif
#endif // HMap_H

#if defined(HMAP_C) && HMAP_C == (1)
  #define HMAP_C (2)
  #include "src/hhmap.c"
#endif // HMap_C
