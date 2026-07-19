#if !defined MY_OXMAP_H
  #define MY_OXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "fptr.h"
  #include "macros.h"
  #include "mytypes.h"
  #include "sList.h"

typedef struct oxmap {
  AllocatorV allocator;
  const u32 ksize, vsize;
  const fnptrof((const void *, const void *), i8) cmp;
  sList_header *keys;
  sList_header *vals;
} oxmap;

oxmap *oxmap_new(AllocatorV allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp);
void oxmap_free(oxmap *map);
// valptr from keyptr
void *oxmap_key_val(const oxmap *map, const void *key);
// keyptr from valptr
void *oxmap_val_key(const oxmap *map, const void *val);
void *oxmap_set(oxmap *map, const void *key, const void *val);
void *oxmap_get(const oxmap *map, const void *key);
void oxmap_clear(oxmap *map);

  #define moxmap(K, V) ptrof(fnptrof((oxmap *, K *), V))
  #define moxmap_vt(map) typeof((*map)((oxmap *)0, nullptr))
  #define moxmap_tox(map) ((void)sizeof(typeof((*map)((oxmap *)0, nullptr))), (oxmap *)map)
  #define moxmap_init(allocator, K, V, ...) (moxmap(K, V)) oxmap_new(allocator, sizeof(K), sizeof(V), VA_SWITCH(nullptr, __VA_ARGS__))
  #define moxmap_deinit(map) oxmap_free(moxmap_tox(map))
  #define moxmap_set(map, key, val) ((moxmap_vt(map) *)({                       \
    var_ _k = key;                                                              \
    var_ _v = val;                                                              \
    (void)sizeof(({ typeof(&_v) _r = (typeof((*map)((oxmap *)0, &_k)) *)0; })); \
    oxmap_set(moxmap_tox(map), &_k, &_v);                                       \
  }))
  #define moxmap_get(map, key) ((moxmap_vt(map) *)({    \
    var_ _k = key;                                      \
    (void)sizeof((typeof((*map)((oxmap *)0, &_k)) *)0); \
    oxmap_get(moxmap_tox(map), &_k);                    \
  }))

  #define moxmap_rem(map, key)                            \
    ({                                                    \
      var_ _k = key;                                      \
      (void)sizeof((typeof((*map)((oxmap *)0, &_k)) *)0); \
      oxmap_set(moxmap_tox(map), &_k, nullptr);           \
    })
// {iter
  #define FOREACH_oxmap_init(map_ptr) ( \
      struct {                          \
        typeof(map_ptr) _m;             \
        size_t _idx;                    \
        struct {                        \
          void *key;                    \
          void *val;                    \
        } _val[0];                      \
      },                                \
      ({                                \
        var_ _map_eval = map_ptr;       \
        (typeof(_foreach_._foreach_)){  \
            ._m = _map_eval,            \
            ._idx = 0,                  \
        };                              \
      })                                \
  )
  #define FOREACH_oxmap_increase(is) (is._idx++)
  #define FOREACH_oxmap_valid(is) \
    (is._idx < ((oxmap *)is._m)->keys->length)
  #define FOREACH_oxmap_cast(is)                                                            \
    ((typeof(is._val[0])){                                                                  \
        .key = (void *)(((oxmap *)is._m)->keys->buf + (is._idx * ((oxmap *)is._m)->ksize)), \
        .val = (void *)(((oxmap *)is._m)->vals->buf + (is._idx * ((oxmap *)is._m)->vsize)), \
    })

  #define FOREACH_oxmap_iter    \
    (                           \
        FOREACH_oxmap_init,     \
        FOREACH_oxmap_increase, \
        FOREACH_oxmap_valid,    \
        FOREACH_oxmap_cast)

  #define FOREACH_moxmap_init(map_ptr, K, V) ( \
      struct {                                 \
        typeof(map_ptr) _m;                    \
        size_t _idx;                           \
        struct {                               \
          K key;                               \
          V *val;                              \
        } _val[0];                             \
      },                                       \
      ({                                       \
        var_ _map_eval = map_ptr;              \
        _Static_assert(                        \
            types_eq(                          \
                typeof(_map_eval),             \
                moxmap(K, V)                   \
            ),                                 \
            "wrong map iterator type"          \
        );                                     \
        (typeof(_foreach_._foreach_)){         \
            ._m = _map_eval,                   \
            ._idx = 0,                         \
        };                                     \
      })                                       \
  )
  #define FOREACH_moxmap_cast(is)                                                                            \
    ((typeof(is._val[0])){                                                                                   \
        .key = *(typeof(is._val->key) *)(((oxmap *)is._m)->keys->buf + (is._idx * ((oxmap *)is._m)->ksize)), \
        .val = (typeof(is._val->val))(((oxmap *)is._m)->vals->buf + (is._idx * ((oxmap *)is._m)->vsize)),    \
    })
  #define FOREACH_moxmap_iter   \
    (                           \
        FOREACH_moxmap_init,    \
        FOREACH_oxmap_increase, \
        FOREACH_oxmap_valid,    \
        FOREACH_moxmap_cast)
// }
i8 test_icmp(const void *a, const void *b) {
  var_ ai = *(int *)a;
  var_ bi = *(int *)b;
  return ai > bi ? -1 : bi > ai ? 1
                                : 0;
}
test_fn(oxmap_basic) {
  var_ map = oxmap_new(allocator, sizeof(int), sizeof(int), test_icmp);
  defer { oxmap_free(map); };

  foreach (var_ i, range(0, 100))
    oxmap_set(map, REF(i), REF(i * i));
  foreach (var_ i, range(0, 100)) {
    var_ g = oxmap_get(map, REF(i));
    test_assert(g);
    test_assert(*(int *)g == i * i);
  }

  var_ ints = &aCreate(allocator, i8, 100);
  defer { aFree(allocator, ints, sizeof(*ints)); };

  foreach (var_ it, oxmap_iter(map)) {
    var_ k = *(int *)it.key;
    var_ v = *(int *)it.val;
    test_assert(v == k * k);
    (*ints)[k] = 1;
  }

  foreach (var_ i, vlap(ints))
    test_assert(i == 1);

  test_pass();
}
test_fn(oxmap_basic_nosort) {
  var_ map = oxmap_new(allocator, sizeof(int), sizeof(int), nullptr);
  defer { oxmap_free(map); };

  foreach (var_ i, range(0, 100))
    oxmap_set(map, REF(i), REF(i * i));
  foreach (var_ i, range(0, 100)) {
    var_ g = oxmap_get(map, REF(i));
    test_assert(g);
    test_assert(*(int *)g == i * i);
  }

  var_ ints = &aCreate(allocator, i8, 100);
  defer { aFree(allocator, ints, sizeof(*ints)); };

  foreach (var_ it, oxmap_iter(map)) {
    var_ k = *(int *)it.key;
    var_ v = *(int *)it.val;
    test_assert(v == k * k);
    (*ints)[k] = 1;
  }

  foreach (var_ i, vlap(ints))
    test_assert(i == 1);

  test_pass();
}
test_fn(oxmap_macros) {
  var_ map = moxmap_init(allocator, int, int, test_icmp);
  defer { moxmap_deinit(map); };
  foreach (var_ i, range(0, 100))
    moxmap_set(map, i, i * i);
  foreach (var_ i, range(0, 100)) {
    var_ g = moxmap_get(map, i);
    test_assert(g);
    test_assert(*g == i * i);
  }

  var_ ints = &aCreate(allocator, i8, 100);
  defer { aFree(allocator, ints, sizeof(*ints)); };

  foreach (var_ it, moxmap_iter(map, int, int)) {
    test_assert(*it.val == it.key * it.key);
    (*ints)[it.key] = 1;
  }

  foreach (var_ i, vlap(ints))
    test_assert(i == 1);
  test_pass();
}
#endif
#if (defined MY_OXMAP_C && MY_OXMAP_C == 1) || (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define MY_OXMAP_C (2)
oxmap *oxmap_new(AllocatorV allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp) {
  assertMessage(ksize || vsize);
  var_ res = aCreate(allocator, oxmap);
  mcpy(
      *res,
      ((oxmap){
          .allocator = allocator,
          .cmp = cmp,
          .ksize = ksize,
          .vsize = vsize,
          .keys = sList_new(allocator, 2, ksize),
          .vals = sList_new(allocator, 2, vsize),
      })
  );
  return res;
}
void oxmap_free(oxmap *map) {
  var_ allocator = map->allocator;
  sList_free(allocator, map->keys, map->ksize);
  sList_free(allocator, map->vals, map->vsize);
  aFree(allocator, map, sizeof(*map));
}
struct bbs_result oxmap_base_search(const oxmap *map, const void *key) {
  if (map->cmp) return bbsearch(key, map->keys->buf, map->keys->length, map->ksize, map->cmp);

  usize size = map->ksize;
  usize nmemb = map->keys->length;
  var_ base = (const u8 *)map->keys->buf;

  for (usize lim = nmemb; lim; lim /= 2) {
    var_ p = base + (lim / 2) * size;
    var_ cmp = fptr_cmp(((fptr){size, (u8 *)key}), ((fptr){size, (u8 *)p}));
    if (!cmp)
      return (struct bbs_result){(void *)p, true};
    if (cmp > 0) {
      base = (const u8 *)p + size;
      lim--;
    }
  }
  return (struct bbs_result){(void *)base, false};
}
void *oxmap_key_val(const oxmap *map, const void *key) {
  return (((u8 *)key - (u8 *)map->keys->buf) / map->ksize * map->vsize) + map->vals->buf;
}
void *oxmap_val_key(const oxmap *map, const void *val) {
  return (((u8 *)val - (u8 *)map->vals->buf) / map->vsize * map->ksize) + map->keys->buf;
}
void *oxmap_set(oxmap *map, const void *key, const void *val) {
  if (!key) return nullptr;
  if (map->keys->length * 4 > map->keys->capacity * 3) {
    var_ ns = map->keys->length * 2;
    map->keys = sList_realloc(map->allocator, map->keys, map->ksize, ns);
    map->vals = sList_realloc(map->allocator, map->vals, map->vsize, ns);
  }
  var_ pos = oxmap_base_search(map, key);
  usize idx = ((u8 *)pos.p - map->keys->buf) / map->ksize;

  if (val) {
    if (pos.f) return memcpy(oxmap_key_val(map, pos.p), val, map->vsize);
    else {
      map->keys = sList_insert(map->allocator, map->keys, map->ksize, idx, key);
      map->vals = sList_insert(map->allocator, map->vals, map->vsize, idx, val);
      return map->vals->buf + (idx * map->vsize);
    }
  } else {
    if (pos.f) {
      sList_remove(map->keys, map->ksize, idx);
      sList_remove(map->vals, map->vsize, idx);
    }
    return nullptr;
  }
}
void *oxmap_get(const oxmap *map, const void *key) {
  if (!key) return nullptr;
  var_ pos = oxmap_base_search(map, key);
  if (pos.f) return oxmap_key_val(map, pos.p);
  return nullptr;
}
void oxmap_clear(oxmap *map) { map->keys->length = (map->vals->length = 0); }
#endif
