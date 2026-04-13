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
  sList_header *values;
  usize vwidth;
  usize num_buckets;
  struct double_idx *buckets[];
} sHmap;
static inline struct double_idx *sHmap_find(const sHmap *sh, fptr f) {
  umax hash = fptr_hash(f) % sh->num_buckets;
  AllocatorV allocator = sh->strings->allocator;

  var_ list_ptr = &sh->buckets[hash];
  if (!*list_ptr)
    return NULL;

  for_each_P((var_ entry, msList_vla(*list_ptr)), {
    fptr c = stringList_get(sh->strings, entry->kidx);
    if (fptr_eq(f, c))
      return entry;
  });
  return NULL;
}
static inline void sHmap_set(sHmap *sh, const fptr key, void *val_ptr) {
  umax hash = fptr_hash(key) % sh->num_buckets;
  AllocatorV allocator = sh->strings->allocator;

  struct double_idx **list_ptr = &sh->buckets[hash];
  if (!*list_ptr)
    *list_ptr = msList_init(allocator, struct double_idx);

  if (msList_len(*list_ptr))
    for_each_P((var_ entry, msList_vla(*list_ptr)), {
      if (fptr_eq(key, stringList_get(sh->strings, entry->kidx)))
        return val_ptr
                   ? (void)memcpy(sh->values->buf + (entry->vidx * sh->vwidth), val_ptr, sh->vwidth)
                   : (void)stringList_set(sh->strings, entry->kidx, nullFptr);
    });

  if (!val_ptr)
    return;
  usize s_idx = stringList_len(sh->strings);
  stringList_push(sh->strings, key);

  usize v_idx = sh->values->length;
  sh->values = sList_append(allocator, sh->values, sh->vwidth, val_ptr);

  struct double_idx entry = {.kidx = s_idx, .vidx = v_idx};
  msList_push(allocator, *list_ptr, entry);
}
static inline void sHmap_set_cs(sHmap *sh, const char *key, void *val_ptr) { sHmap_set(sh, fptr_CS((void *)key), val_ptr); }
static inline isize sHmap_get(const sHmap *sh, const fptr k, usize v_width) {
  umax hash = fptr_hash(k);
  usize b_idx = hash % sh->num_buckets;

  var_ list_ptr = &sh->buckets[b_idx];
  if (!*list_ptr)
    return -1;

  for_each_((var_ entry, msList_vla(*list_ptr)), {
    if (fptr_eq(stringList_get(sh->strings, entry.kidx), k))
      return entry.vidx;
  });
  return -1;
}
static inline isize sHmap_get_cs(const sHmap *sh, const char *key, usize v_width) {
  return sHmap_get(sh, fptr_CS((void *)key), v_width);
}
static inline sHmap *shMap_new(AllocatorV allocator, usize size, usize buckets) {
  sHmap *res = (typeof(res))aAlloc(allocator, sizeof(sHmap) + (buckets * sizeof(struct double_idx *)));
  *res = (sHmap){
      .strings = {stringList_newVal(allocator, 1024)},
      .values = sList_new(allocator, 8, size),
      .vwidth = size,
      .num_buckets = buckets,
  };
  for (usize i = 0; i < buckets; i++)
    res->buckets[i] = NULL;
  return res;
}
static inline void shMap_free(sHmap *map) {
  AllocatorV allocator = map->strings->allocator;
  for (usize i = 0; i < map->num_buckets; i++)
    if (map->buckets[i])
      msList_deInit(allocator, map->buckets[i]);
  aFree(allocator, map->values, map->vwidth * map->values->capacity);
  stringList_free_data(map->strings[0]);
  aFree(allocator, map, sizeof(sHmap) + (map->num_buckets * sizeof(struct double_idx *)));
}
static inline usize sHmap_footprint(const sHmap *map) {
  usize res = stringList_footprint(map->strings);
  for_each_((var_ list, VLAP(map->buckets, map->num_buckets)), {
    if (list)
      res += sizeof(*msList_vla(list));
  });
  return res;
}
static inline usize sHmap_countCollisions(const sHmap *map) {
  usize res = 0;
  for_each_((var_ list, VLAP(map->buckets, map->num_buckets)), {
    if (list) {
      var_ l = msList_len(list);
      res += l ? l - 1 : 0;
    }
  });
  return res;
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

  #define msHmap_deinit(sh) \
    shMap_free((sHmap *)sh)
  #define msHmap_set(sh, key, val)   \
    do {                             \
      msHmap_iType(sh) _v = (val);   \
      _Generic(                      \
          (key),                     \
          fptr: sHmap_set,           \
          char *: sHmap_set_cs,      \
          const char *: sHmap_set_cs \
      )((sHmap *)sh, key, &_v);      \
    } while (0)
  #define msHmap_rem(sh, key)        \
    do {                             \
      _Generic(                      \
          (key),                     \
          fptr: sHmap_set,           \
          char *: sHmap_set_cs,      \
          const char *: sHmap_set_cs \
      )((sHmap *)sh, key, NULL);     \
    } while (0)

  #define msHmap_get(sh, key) (typeof(msHmap_iType(sh) *))({                               \
    isize _idx = _Generic(                                                                 \
        (key),                                                                             \
        fptr: sHmap_get,                                                                   \
        char *: sHmap_get_cs,                                                              \
        const char *: sHmap_get_cs                                                         \
    )((sHmap *)sh, key, sizeof(msHmap_iType(sh)));                                         \
    _idx < 0 ? NULL : sList_getRef(((sHmap *)sh)->values, sizeof(msHmap_iType(sh)), _idx); \
  })
  #define msHmapGetOrSet(sh, key, val) ({                            \
    var_ *temp_ = msHmap_get(sh, key);                               \
    temp_ ? temp_ : (msHmap_set(sh, key, val), msHmap_get(sh, key)); \
  })

  #if defined(MAKE_TEST_FN)
MAKE_TEST_FN(test_shmap_generic_values, {
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
});
MAKE_TEST_FN(test_shmap_struct_values, {
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
});
  #endif
#endif
