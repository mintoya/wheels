#include <string.h>
#if !defined(SHMAP_H)
  #define SHMAP_H (1)
  #include "fptr.h"
  #include "hhmap.h"
  #include "macros.h"
  #include "mylist.h"
  #include "mytypes.h"
  #include "sList.h"
  #include "stringList.h"
  #include <stddef.h>
struct double_idx {
  usize kidx, vidx; // kidx in stringlist, vidx in buckets[fptr_hash(f)]
};
typedef struct string_HMap {
  stringList strings[1];

  struct {
    sList_header *values;
    usize vwidth;
  };

  usize n_buckets;
  struct double_idx *buckets[];
} sHmap;
static inline struct double_idx *sHmap_find(const sHmap *sh, fptr f) {
  umax hash = fptr_hash(f) % sh->n_buckets;
  AllocatorV allocator = sh->strings->allocator;

  var_ list_ptr = &sh->buckets[hash];
  if (!*list_ptr)
    return NULL;

  foreach (var_ entry, span(*list_ptr, msList_len(*list_ptr))) {
    fptr c = stringList_get(sh->strings, entry->kidx);
    if (fptr_eq(f, c))
      return entry;
  }
  return NULL;
}
static inline void *sHmap_set(sHmap *sh, const fptr key, void *val_ptr) {
  umax hash = fptr_hash(key) % sh->n_buckets;
  AllocatorV allocator = sh->strings->allocator;

  struct double_idx **list_ptr = &sh->buckets[hash];
  if (!*list_ptr)
    *list_ptr = msList_init(allocator, struct double_idx);

  foreach (var_ entry, span(*list_ptr, msList_len(*list_ptr))) {
    if (fptr_eq(key, stringList_get(sh->strings, entry->kidx))) {
      if (val_ptr) {
        // sList_getRef(sh->values, sh->vwidth, entry->vidx);
        memcpy(sh->values->buf + (entry->vidx * sh->vwidth), val_ptr, sh->vwidth);
        return sh->values->buf + (entry->vidx * sh->vwidth);
      } else {
        stringList_set(sh->strings, entry->kidx, nullFptr);
        return NULL;
      }
    }
  }

  if (!val_ptr)
    return NULL;
  usize s_idx = stringList_len(sh->strings);
  stringList_push(sh->strings, key);

  usize v_idx = sh->values->length;
  sh->values = sList_append(allocator, sh->values, sh->vwidth, val_ptr);

  struct double_idx entry = {.kidx = s_idx, .vidx = v_idx};
  msList_push(allocator, *list_ptr, entry);
  return sList_getRef(sh->values, sh->vwidth, sh->values->length - 1);
}
static inline void *sHmap_set_cs(sHmap *sh, const char *key, void *val_ptr) {
  return sHmap_set(sh, fptr_CS((void *)key), val_ptr);
}
static inline isize sHmap_get(const sHmap *sh, const fptr k, usize v_width) {
  umax hash = fptr_hash(k);
  usize b_idx = hash % sh->n_buckets;

  var_ list_ptr = &sh->buckets[b_idx];
  if (!*list_ptr)
    return -1;

  foreach (var_ entry, span(*list_ptr, msList_len(*list_ptr)))
    if (fptr_eq(stringList_get(sh->strings, entry->kidx), k))
      return entry->vidx;
  return -1;
}
static inline isize sHmap_get_cs(const sHmap *sh, const char *key, usize v_width) {
  return sHmap_get(sh, fptr_CS((void *)key), v_width);
}
static inline sHmap *shMap_new(AllocatorV allocator, usize size, usize buckets) {
  sHmap *res = (typeof(res))aAlloc(
      allocator, sizeof(sHmap) + (buckets * sizeof(struct double_idx *))
  );
  *res = (sHmap){
      .strings = {stringList_newVal(allocator, 1024)},
      .values = sList_new(allocator, 8, size),
      .vwidth = size,
      .n_buckets = buckets,
  };
  for (usize i = 0; i < buckets; i++)
    res->buckets[i] = NULL;
  return res;
}
static inline void shMap_free(sHmap *map) {
  AllocatorV allocator = map->strings->allocator;
  for (usize i = 0; i < map->n_buckets; i++)
    if (map->buckets[i])
      msList_deInit(allocator, map->buckets[i]);
  aFree(allocator, map->values, map->vwidth * map->values->capacity);
  stringList_free_data(map->strings[0]);
  aFree(allocator, map, sizeof(sHmap) + (map->n_buckets * sizeof(struct double_idx *)));
}
static inline usize sHmap_footprint(const sHmap *map) {
  usize res = stringList_footprint(map->strings);
  foreach (var_ list, span(map->buckets, map->n_buckets))
    if (*list)
      res += sizeof(*msList_vla(*list));
  return res;
}
static inline usize sHmap_countCollisions(const sHmap *map) {
  usize res = 0;
  foreach (var_ listp, span(map->buckets, map->n_buckets)) {
    var_ list = *listp;
    if (list) {
      var_ l = msList_len(list);
      res += l ? l - 1 : 0;
    }
  }
  return res;
}
static inline AllocatorV sHmap_allocator(const sHmap *map) {
  return map->strings->allocator;
}

//
// iterator
//

struct sHmapIterator_struct {
  const sHmap *map;
  const struct double_idx *current;
  u32 bucket_idx;
  u32 element_idx;
};

static inline struct sHmapIterator_struct sHmapIterator(const sHmap *map);

static bool sHmapIterator_valid(const struct sHmapIterator_struct *it) {
  return it->bucket_idx < it->map->n_buckets &&
         it->map->buckets[it->bucket_idx] != NULL &&
         it->element_idx < msList_len(it->map->buckets[it->bucket_idx]);
}
static void sHmapIterator_next(struct sHmapIterator_struct *it) {
  it->element_idx++;

  while (it->bucket_idx < it->map->n_buckets) {
    usize current_bucket_len = it->map->buckets[it->bucket_idx]
                                   ? msList_len(it->map->buckets[it->bucket_idx])
                                   : 0;

    if (it->element_idx < current_bucket_len)
      break;

    it->bucket_idx++;
    it->element_idx = 0;
  }

  if (it->bucket_idx < it->map->n_buckets)
    it->current = &it->map->buckets[it->bucket_idx][it->element_idx];
  else
    it->current = NULL;
}
static inline struct sHmapIterator_struct sHmapIterator(const sHmap *map) {
  struct sHmapIterator_struct it = {
      .map = map,
      .current = NULL,
      .bucket_idx = 0,
      .element_idx = 0,
  };

  if (!map || !map->n_buckets)
    return it;

  while (it.bucket_idx < map->n_buckets) {
    usize current_bucket_len =
        map->buckets[it.bucket_idx]
            ? msList_len(map->buckets[it.bucket_idx])
            : 0;

    if (current_bucket_len > 0) {
      break;
    }
    it.bucket_idx++;
  }

  if (it.bucket_idx < map->n_buckets) {
    it.current =
        &map->buckets[it.bucket_idx][it.element_idx];
  }

  return it;
}

  #ifdef __cplusplus
template <typename T>
using msHmap_t = T (**)(sHmap *);
    #define msHmap(T) msHmap_t<T>
  #else
    #define msHmap(T) typeof(T(**)(sHmap *))
  #endif
  #define msHmap_iType(sh) typeof((*sh)(NULL))
  #define msHmap_init(allocator, T, ...) \
    (msHmap(T)) shMap_new(allocator, sizeof(T), VA_SWITCH(8, __VA_ARGS__))

  #define msHmap_allocator(map) (sHmap_allocator((sHmap *)map))
  #define msHmap_deinit(sh) shMap_free((sHmap *)sh)
  #define msHmap_set(sh, key, val)                                                                                                   \
    ({                                                                                                                               \
      msHmap_iType(sh) _v = (val);                                                                                                   \
      (msHmap_iType(sh) *)_Generic((key), fptr: sHmap_set, char *: sHmap_set_cs, const char *: sHmap_set_cs)((sHmap *)sh, key, &_v); \
    })
  #define msHmap_rem(sh, key)                                                                                     \
    do {                                                                                                          \
      _Generic((key), fptr: sHmap_set, char *: sHmap_set_cs, const char *: sHmap_set_cs)((sHmap *)sh, key, NULL); \
    } while (0)

  #define msHmap_get(sh, key)                                                                                                                      \
    (typeof(msHmap_iType(sh) *))({                                                                                                                 \
      isize _idx = _Generic((key), fptr: sHmap_get, char *: sHmap_get_cs, const char *: sHmap_get_cs)((sHmap *)sh, key, sizeof(msHmap_iType(sh))); \
      _idx < 0                                                                                                                                     \
          ? NULL                                                                                                                                   \
          : sList_getRef(((sHmap *)sh)->values, sizeof(msHmap_iType(sh)), _idx);                                                                   \
    })
  #define msHmap_GetOrSet(sh, key, val)                                \
    ({                                                                 \
      var_ *temp_ = msHmap_get(sh, key);                               \
      temp_ ? temp_ : (msHmap_set(sh, key, val), msHmap_get(sh, key)); \
    })

//{iterator

  #define FOREACH_sHmap_init(map) ( \
      typeof(sHmapIterator(map)),   \
      sHmapIterator(map)            \
  )
  #define FOREACH_sHmap_increase(is) sHmapIterator_next(&is)
  #define FOREACH_sHmap_valid(is) sHmapIterator_valid(&is)
  #define FOREACH_sHmap_cast(is) \
    (is.current)

  #define FOREACH_sHmap_iter    \
    (                           \
        FOREACH_sHmap_init,     \
        FOREACH_sHmap_increase, \
        FOREACH_sHmap_valid,    \
        FOREACH_sHmap_cast)

  #define FOREACH_msHmap_init(map) (               \
      struct {                                     \
        typeof(sHmapIterator((sHmap *)map)) _iter; \
        typeof(map) _map;                          \
      },                                           \
      {sHmapIterator((sHmap *)map), map}           \
  )
  #define FOREACH_msHmap_increase(is) sHmapIterator_next(&is._iter)
  #define FOREACH_msHmap_valid(is) sHmapIterator_valid(&is._iter)
  #define FOREACH_msHmap_cast(is)                                                                                      \
    ({                                                                                                                 \
      struct {                                                                                                         \
        fptr key;                                                                                                      \
        msHmap_iType(is._map) val;                                                                                     \
      } item;                                                                                                          \
      item.key = stringList_get(((sHmap *)is._map)->strings, is._iter.current->kidx);                                  \
      item.val = *(typeof(item.val) *)(((sHmap *)is._map)->values->buf + (sizeof(item.val) * is._iter.current->vidx)); \
      item;                                                                                                            \
    })

  #define FOREACH_msHmap_iter    \
    (                            \
        FOREACH_msHmap_init,     \
        FOREACH_msHmap_increase, \
        FOREACH_msHmap_valid,    \
        FOREACH_msHmap_cast)
// var_ k = stringList_get(((sHmap *)sm)->strings, it->kidx);
// var_ v = *(Pos *)(((sHmap *)sm)->values->buf + (sizeof(Pos) * it->vidx));
//}
  #include "tests.h"
test_fn(test_shmap_generic_values) {
  msHmap(int) sm = msHmap_init(allocator, int);
  defer { msHmap_deinit(sm); };

  msHmap_set(sm, "age", 25);
  msHmap_set(sm, "score", 100);

  int *age = msHmap_get(sm, "age");
  if (!age || *age != 25)
    return 1;
  msHmap_set(sm, "age", 26);

  if (*msHmap_get(sm, "age") != 26)
    return 1;
  msHmap_rem(sm, "age");
  if (msHmap_get(sm, "age"))
    return 1;
  if (((sHmap *)sm)->values->length != 2)
    return 1;

  return 0;
}
test_fn(test_shmap_struct_values) {
  typedef struct {
    float x, y;
  } Pos;
  msHmap(Pos) sm = msHmap_init(allocator, Pos);
  defer { msHmap_deinit(sm); };

  msHmap_set(sm, "player", ((Pos){1.0f, 2.0f}));

  Pos *p = msHmap_get(sm, "player");
  if (!p || p->x != 1.0f || p->y != 2.0f)
    return 1;

  return 0;
}
test_fn(test_shmap_iterator) {
  typedef struct {
    int x, y;
  } Pos;
  msHmap(Pos) sm = msHmap_init(allocator, Pos);
  defer { msHmap_deinit(sm); };

  foreach (var_ i, range(0, 10))
    msHmap_set(sm, ((fptr){sizeof(i), (u8 *)&i}), ((Pos){i, i}));
  usize count = 0;
  foreach (var_ it, sHmap_iter((sHmap *)sm)) {
    count++;
    var_ k = stringList_get(((sHmap *)sm)->strings, it->kidx);
    int i = 0;
    memcpy(&i, k.ptr, k.len);
    var_ v = *(Pos *)(((sHmap *)sm)->values->buf + (sizeof(Pos) * it->vidx));
    if (v.x != i) return 2;
    if (v.y != i) return 3;
  }
  if (count != 10) return 1;

  return 0;
}

test_fn(test_shmap_iterator_cast) {
  typedef struct {
    int x, y;
  } Pos;
  msHmap(Pos) sm = msHmap_init(allocator, Pos);
  defer { msHmap_deinit(sm); };

  foreach (var_ i, range(0, 10))
    msHmap_set(sm, ((fptr){sizeof(i), (u8 *)&i}), ((Pos){i, i}));
  usize count = 0;
  foreach (var_ it, msHmap_iter(sm)) {
    count++;
    int i = 0;
    memcpy(&i, it.key.ptr, it.key.len);
    if (it.val.x != i) return 2;
    if (it.val.y != i) return 3;
  }
  if (count != 10) return 1;

  return 0;
}
#endif
