#if !defined MY_OXMAP_H
  #define MY_OXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "mytypes.h"
  #include "sList.h"

typedef struct oxmap {
  allocfn allocator;
  const u32 ksize, vsize;
  const fnptrof((const void *, const void *), i8) cmp;
  sList_header *keys;
  sList_header *vals;
} oxmap;

oxmap *oxmap_new(allocfn allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp);
void oxmap_free(oxmap *map);
// valptr from keyptr
void *oxmap_key_val(const oxmap *map, const void *key);
// keyptr from valptr
void *oxmap_val_key(const oxmap *map, const void *val);
void *oxmap_set(oxmap *map, const void *key, const void *val);
void *oxmap_get(const oxmap *map, const void *key);
void oxmap_clear(oxmap *map);

void oxmap_newm(allocfn allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp, oxmap mem[1]);
void oxmap_freem(oxmap map);
  #define moxmap(K, V) ptrof(fnptrof((oxmap *, K *), V))
  #define moxmap_vt(map) typeof((*map)((oxmap *)0, nullptr))
  #define moxmap_tox(map) ((void)sizeof(typeof((*map)((oxmap *)0, nullptr))), (oxmap *)map)
  #define moxmap_init(allocator, K, V, ...) (moxmap(K, V)) oxmap_new(allocator, sizeof(K), sizeof(V), VA_SWITCH(nullptr, __VA_ARGS__))
  #define moxmap_deinit(map) oxmap_free(moxmap_tox(map))
  #define moxmap_set(map, key, val) ((moxmap_vt(map) *)({ \
    let _k = key;                                         \
    let _v = val;                                         \
    (void)sizeof(({ typeof(&_v) _r = (typeof((*map)((oxmap *)0, &_k)) *)0;0; }));                                  \
    oxmap_set(moxmap_tox(map), &_k, &_v);                 \
  }))
  #define moxmap_get(map, key) ((moxmap_vt(map) *)({    \
    let _k = key;                                       \
    (void)sizeof((typeof((*map)((oxmap *)0, &_k)) *)0); \
    oxmap_get(moxmap_tox(map), &_k);                    \
  }))
  #define moxmap_rem(map, key)                            \
    ({                                                    \
      let _k = key;                                       \
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
        let _map_eval = map_ptr;        \
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
        let _map_eval = map_ptr;               \
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
  let ai = *(int *)a;
  let bi = *(int *)b;
  return ai > bi ? -1 : bi > ai ? 1
                                : 0;
}
test_fn(oxmap_basic) {
  let map = oxmap_new(allocator, sizeof(int), sizeof(int), test_icmp);
  defer { oxmap_free(map); };

  foreach (let i, range(0, 100))
    oxmap_set(map, REF(i), REF(i * i));
  foreach (let i, range(0, 100)) {
    let g = oxmap_get(map, REF(i));
    test_assert(g);
    test_assert(*(int *)g == i * i);
  }

  let ints = acreate(allocator, i8[100]);
  defer { adestroy(allocator, ints); };

  foreach (let it, oxmap_iter(map)) {
    let k = *(int *)it.key;
    let v = *(int *)it.val;
    test_assert(v == k * k);
    (*ints)[k] = 1;
  }

  foreach (let i, vlap(ints))
    test_assert(i == 1);
}
test_fn(oxmap_basic_nosort) {
  let map = oxmap_new(allocator, sizeof(int), sizeof(int), nullptr);
  defer { oxmap_free(map); };

  foreach (let i, range(0, 100))
    oxmap_set(map, REF(i), REF(i * i));
  foreach (let i, range(0, 100)) {
    let g = oxmap_get(map, REF(i));
    test_assert(g);
    test_assert(*(int *)g == i * i);
  }

  let ints = acreate(allocator, i8[100]);
  defer { adestroy(allocator, ints); };

  foreach (let it, oxmap_iter(map)) {
    let k = *(int *)it.key;
    let v = *(int *)it.val;
    test_assert(v == k * k);
    (*ints)[k] = 1;
  }

  foreach (let i, vlap(ints))
    test_assert(i == 1);
}
test_fn(oxmap_macros) {
  let map = moxmap_init(allocator, int, int, test_icmp);
  defer { moxmap_deinit(map); };
  foreach (let i, range(0, 100))
    moxmap_set(map, i, i * i);
  foreach (let i, range(0, 100)) {
    let g = moxmap_get(map, i);
    test_assert(g);
    test_assert(*g == i * i);
  }

  let ints = acreate(allocator, i8[100]);
  defer { adestroy(allocator, ints); };

  foreach (let it, moxmap_iter(map, int, int)) {
    test_assert(*it.val == it.key * it.key);
    (*ints)[it.key] = 1;
  }

  foreach (let i, vlap(ints))
    test_assert(i == 1);
}
#endif
#if (defined MY_OXMAP_C && MY_OXMAP_C == 1) || (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef MY_OXMAP_C
  #define MY_OXMAP_C (2)
void oxmap_newm(allocfn allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp, oxmap mem[1]) {
  assertMessage(ksize || vsize);
  mcpy(
      *mem,
      ((oxmap){
          .allocator = allocator,
          .ksize = ksize,
          .vsize = vsize,
          .cmp = cmp,
          .keys = sList_new(allocator, 2, ksize),
          .vals = sList_new(allocator, 2, vsize),
      })
  );
}
oxmap *oxmap_new(allocfn allocator, u32 ksize, u32 vsize, itypeof(oxmap, cmp) cmp) {
  let r = acreate(allocator, oxmap);
  oxmap_newm(allocator, ksize, vsize, cmp, r);
  return r;
}
void oxmap_freem(oxmap map) {
  let allocator = map.allocator;
  sList_free(allocator, map.keys, map.ksize);
  sList_free(allocator, map.vals, map.vsize);
}
void oxmap_free(oxmap *map) {
  let allocator = map->allocator;
  oxmap_freem(*map);
  adestroy(allocator, map);
}
void *oxmap_key_val(const oxmap *map, const void *key) {
  return (((u8 *)key - (u8 *)map->keys->buf) / map->ksize * map->vsize) + map->vals->buf;
}
void *oxmap_val_key(const oxmap *map, const void *val) {
  return (((u8 *)val - (u8 *)map->vals->buf) / map->vsize * map->ksize) + map->keys->buf;
}
void *oxmap_set(oxmap *map, const void *key, const void *val) {
  if (!key) return nullptr;
  let pos = bbsearch(key, map->keys->buf, map->keys->length, map->ksize, map->cmp);
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
  let pos = bbsearch(key, map->keys->buf, map->keys->length, map->ksize, map->cmp);
  if (pos.f) return oxmap_key_val(map, pos.p);
  return nullptr;
}
void oxmap_clear(oxmap *map) { map->keys->length = (map->vals->length = 0); }
#endif
