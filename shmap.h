#if !defined(SHMAP_H)
  #define SHMAP_H (1)
  #include "fptr.h"
  #include "hhmap.h"
  #include "mylist.h"
  #include "mytypes.h"
  #include "sList.h"
  #include "stringList.h"
  #include <stddef.h>
struct double_idx {
  usize kidx, vidx;
};
typedef struct string_HMap {
  stringList *strings;
  sList_header *values;
  usize vwidth;
  mHmap(umax, struct double_idx *) map;
} sHmap;

static inline void sHmap_set(sHmap *sh, const fptr key, void *val_ptr) {
  umax hash = fptr_hash(key);
  AllocatorV allocator = sh->strings->allocator;

  struct double_idx **list_ptr = mHmap_get(sh->map, hash);
  if (!list_ptr) {
    struct double_idx *new_list = msList_init(allocator, struct double_idx);
    mHmap_set(sh->map, hash, new_list);
    list_ptr = mHmap_get(sh->map, hash);
  }

  for (each_VLAP(entry, msList_vla(*list_ptr))) {
    fptr c = stringList_get(sh->strings, entry->kidx);
    if (fptr_eq(key, c)) {
      memcpy(sh->values->buf + (entry->vidx * sh->vwidth), val_ptr, sh->vwidth);
      return;
    }
  }

  usize s_idx = stringList_len(sh->strings);
  stringList_append(sh->strings, key);

  usize v_idx = sh->values->length;
  sh->values = sList_append(allocator, sh->values, sh->vwidth, val_ptr);

  struct double_idx entry = {.kidx = s_idx, .vidx = v_idx};
  msList_push(allocator, *list_ptr, entry);
}

static inline void sHmap_set_cs(sHmap *sh, const char *key, void *val_ptr) { sHmap_set(sh, fptr_CS((void *)key), val_ptr); }
static inline isize sHmap_get(sHmap *sh, const fptr k, usize v_width) {
  umax hash = fptr_hash(k);

  struct double_idx **list_ptr = mHmap_get(sh->map, hash);
  if (!list_ptr || !*list_ptr)
    return -1;

  for (each_VLAP(entry, msList_vla(*list_ptr))) {
    fptr e = stringList_get(sh->strings, entry->kidx);
    if (fptr_eq(e, k))
      return entry->vidx;
  }
  return -1;
}
static inline isize sHmap_get_cs(sHmap *sh, const char *key, usize v_width) {
  return sHmap_get(sh, fptr_CS((void *)key), v_width);
}
static inline sHmap *shMap_new(AllocatorV allocator, usize size) {
  sHmap *res = aCreate(allocator, sHmap);
  *res = (sHmap){
      .strings = stringList_new(allocator, 1024),
      .values = sList_new(allocator, 8, size),
      .vwidth = size,
      .map = mHmap_init(allocator, umax, struct double_idx *),
  };
  return res;
}
static inline void shMap_free(sHmap *map) {
  AllocatorV allocator = map->strings->allocator;
  mHmap_foreach(map->map, umax, hashes, struct double_idx *, list, {
    msList_deInit(allocator, list);
  });
  aFree(allocator, map->values);
  stringList_free(map->strings);
  mHmap_deinit(map->map);
  aFree(allocator, map);
}

  #ifdef __cplusplus
template <typename T>
using msHmap_t = T (**)(sHmap *);
    #define msHmap(T) msHmap_t<T>
  #else
    #define msHmap(T) typeof(T(**)(sHmap *))
  #endif
  #define msHmap_iType(sh) typeof((*sh)(NULL))
  #define msHmap_init(allocator, T) \
    (msHmap(T)) shMap_new(allocator, sizeof(T))

  #define msHmap_deinit(allocator, sh) \
    shMap_free((sHmap *)sh)
  // Internal dispatch for SET
  #define msHmap_set(sh, key, val)   \
    do {                             \
      msHmap_iType(sh) _v = (val);   \
      _Generic(                      \
          (key),                     \
          fptr: sHmap_set,           \
          const fptr: sHmap_set,     \
          char *: sHmap_set_cs,      \
          const char *: sHmap_set_cs \
      )((sHmap *)sh, key, &_v);      \
    } while (0)

  // Internal dispatch for GET
  #define msHmap_get(sh, key) (typeof(msHmap_iType(sh) *))({                               \
    isize _idx = _Generic(                                                                 \
        (key),                                                                             \
        fptr: sHmap_get,                                                                   \
        const fptr: sHmap_get,                                                             \
        char *: sHmap_get_cs,                                                              \
        const char *: sHmap_get_cs                                                         \
    )((sHmap *)sh, key, sizeof(msHmap_iType(sh)));                                         \
    _idx < 0 ? NULL : sList_getRef(((sHmap *)sh)->values, sizeof(msHmap_iType(sh)), _idx); \
  })

  // #include "tests.c"
  #if defined(MAKE_TEST_FN)
MAKE_TEST_FN(test_shmap_generic_values, {
  msHmap(int) sm = msHmap_init(allocator, int);
  defer { msHmap_deinit(allocator, sm); };

  msHmap_set(sm, "age", 25);
  msHmap_set(sm, "score", 100);

  int *age = msHmap_get(sm, "age");
  if (!age || *age != 25)
    return 1;
  msHmap_set(sm, "age", 26);

  if (*msHmap_get(sm, "age") != 26)
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
  defer { msHmap_deinit(allocator, sm); };

  msHmap_set(sm, "player", ((Pos){1.0f, 2.0f}));

  Pos *p = msHmap_get(sm, "player");
  if (!p || p->x != 1.0f || p->y != 2.0f)
    return 1;

  return 0;
});
  #endif
#endif
