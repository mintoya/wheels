#if !defined MY_HXMAP_H
  #define MY_HXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"

typedef enum : u64 {
  EMPTY = 0,
  OCCUPIED = 1,
  LEFT = 2,
} mflag;
typedef struct hxmap {
  AllocatorV allocator;
  const u32 ksize, vsize;
  usize count, cap;
  const fnptrof((const void *), u64) hfn;
  const fnptrof((const void *, const void *), i8) cmp;
  struct {
    u64 ohash : sizeof(u64) * 8 - 2;
    mflag flag : 2;
  } *flags;
  u8 *keys;
  u8 *vals;
} hxmap;
hxmap *hxmap_new(
    AllocatorV allocator,
    usize ksize,
    usize vsize,
    usize cap,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
);
void hxmap_free(hxmap *map);
void *hxmap_set(
    hxmap *m,
    void *key,
    void *val
);
void *hxmap_get(
    const hxmap *m,
    void *key
);
void hxmap_manage(
    hxmap *map,
    i8 scale
);
void *hxmap_val_key(
    const hxmap *map,
    void *val
);

  #define mxMap(K, V) ptrof(fnptrof((hxmap *, ptrof(K)), V))
  #define mxMap_valType(map) typeof((*map)(((hxmap *)0), nullptr))
  #define mxMap_defaults(...) VA_SWITCH_REMP((0, 0, 0)__VA_OPT__(, (__VA_ARGS__)))
  #define mxMap_init(allocator, K, V, ...) (mxMap(K, V)) hxmap_new(allocator, sizeof(K), sizeof(V), mxMap_defaults(__VA_ARGS__))
  #define mxMap_set(map, key, val) ({                                  \
    var_ _k = key;                                                     \
    var_ _v = val;                                                     \
    ASSERT_EXPR(types_eq(typeof(map), mxMap(typeof(_k), typeof(_v)))); \
    (ptrof(mxMap_valType(map))) hxmap_set((hxmap *)map, &_k, &_v);     \
  })
  #define mxMap_rem(map, key) ({                                               \
    var_ _k = key;                                                             \
    ASSERT_EXPR(types_eq(typeof(map), mxMap(typeof(_k), mxMap_valType(map)))); \
    (ptrof(mxMap_valType(map))) hxmap_set((hxmap *)map, &_k, nullptr);         \
  })
  #define mxMap_get(map, key) ({                                               \
    var_ _k = key;                                                             \
    ASSERT_EXPR(types_eq(typeof(map), mxMap(typeof(_k), mxMap_valType(map)))); \
    (ptrof(mxMap_valType(map))) hxmap_get((hxmap *)map, &_k);                  \
  })
  #define mxMap_deinit(map) hxmap_free(((void)(sizeof(typeof(mxMap_valType(map)))), (hxmap *)map))

//{hxmap(map)

  #define FOREACH_hxmap_init(map_ptr) ( \
      struct {                          \
        typeof(map_ptr) _m;             \
        size_t _idx;                    \
      },                                \
      ({                                \
        var_ _map_eval = map_ptr;       \
        (typeof(_foreach_._foreach_)){  \
            ._m = _map_eval,            \
            ._idx = 0,                  \
        };                              \
      })                                \
  )
  #define FOREACH_hxmap_increase(is) (is._idx++)
  #define FOREACH_hxmap_valid(is)                                                                  \
    ({                                                                                             \
      while (is._idx < ((hxmap *)is._m)->cap && ((hxmap *)is._m)->flags[is._idx].flag != OCCUPIED) \
        is._idx++;                                                                                 \
      is._idx < ((hxmap *)is._m)->cap;                                                             \
    })
  #define FOREACH_hxmap_cast(is)                                             \
    ((struct { void *key, *val; }){                                          \
        .key = ((hxmap *)is._m)->keys + (is._idx * ((hxmap *)is._m)->ksize), \
        .val = ((hxmap *)is._m)->vals + (is._idx * ((hxmap *)is._m)->vsize), \
    })

  #define FOREACH_hxmap_iter    \
    (                           \
        FOREACH_hxmap_init,     \
        FOREACH_hxmap_increase, \
        FOREACH_hxmap_valid,    \
        FOREACH_hxmap_cast)
  //}
  #include "tests.h"
test_fn(hxmap_tests) {
  var_ map = mxMap_init(allocator, u32, u64);

  u32 k1 = 42;
  u64 v1 = 100;
  mxMap_set(map, k1, v1);

  var_ r1 = mxMap_get(map, k1);
  test_assert(r1);
  test_assert(*r1 == 100);
  test_assert(((hxmap *)map)->count == 1);

  u64 v2 = 200;
  mxMap_set(map, k1, v2);
  var_ r2 = mxMap_get(map, k1);
  test_assert(r2);
  test_assert(*r2 == 200);
  test_assert(((hxmap *)map)->count == 1);

  u32 k2 = 99;
  var_ r3 = mxMap_get(map, k2);
  test_assert(!r3);

  mxMap_rem(map, k1);
  var_ r4 = mxMap_get(map, k1);
  test_assert(!r4);
  test_assert(((hxmap *)map)->count == 0);

  for (u32 i = 0; i < 1000; i++) {
    u64 val = i * 10;
    mxMap_set(map, i, val);
  }

  test_assert(((hxmap *)map)->count == 1000);
  test_assert(((hxmap *)map)->cap > 1000);

  foreach (u32 i, range(0, 1000)) {
    var_ r = mxMap_get(map, i);
    test_assert(r);
    test_assert(*r == i * 10);
  }

  mxMap_deinit(map);

  test_pass();
}
#endif

#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define MY_HXMAP_C (1)
#endif

#if defined MY_HXMAP_C
hxmap *hxmap_new(
    AllocatorV allocator,
    usize ksize,
    usize vsize,
    usize cap,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
) {
  cap = cap ?: 8;
  assertMessage(allocator);
  assertMessage(ksize);
  assertMessage(vsize);
  var_ res = ((hxmap){
      .allocator = allocator,
      .ksize = (u32)ksize,
      .vsize = (u32)vsize,
      .count = 0,
      .cap = cap,
      .hfn = hashfn,
      .cmp = cmpfn,
      .flags = aCreate(allocator, ptrstype(itypeof(hxmap, flags)), cap),
      .keys = aCreate(allocator, u8, cap * ksize),
      .vals = aCreate(allocator, u8, cap * vsize),
  });
  var_ resp = aCreate(allocator, typeof(res));
  memcpy(resp, &res, sizeof(res));
  return resp;
}
void hxmap_free(hxmap *map) {
  var_ allocator = map->allocator;
  aFree(allocator, map->flags, sizeof(*map->flags) * map->cap);
  aFree(allocator, map->keys, map->ksize * map->cap);
  aFree(allocator, map->vals, map->vsize * map->cap);
  aFree(allocator, map, sizeof(*map));
}
static inline i8 hxmap_base_cmp(const hxmap *m, const void *a, const void *b) {
  if (m->cmp) return m->cmp(a, b);
  int mc = memcmp(a, b, m->ksize);
  return mc < 0 ? -1 : mc > 0 ? 1
                              : 0;
}
static inline u64 hxmap_base_hash(const hxmap *m, const void *a) {
  if (m->hfn) return m->hfn(a);
  u8(*bytes)[m->ksize] = (typeof(bytes))a;
  switch (sizeof(*bytes)) {
    case sizeof(u64):
      return *(u64 *)bytes;
    case sizeof(u32):
      return *(u32 *)bytes;
    case sizeof(u16):
      return *(u16 *)bytes;
    case sizeof(u8):
      return *(u8 *)bytes;
    default: {
      u64 hash = 0xcbf29ce484222325ULL;
      foreach (var_ b, vla(*bytes)) {
        hash ^= b;
        hash *= 0x100000001b3ULL;
      }
      return hash;
    } break;
  }
}

void hxmap_manage(
    hxmap *map,
    i8 scale
) {

  assert(scale > 0);
  var_ oc = map->cap;
  var_ nc = map->cap * scale;
  // var_ nc = scale < 0 ? map->cap / (-scale) : map->cap * scale;

  var_ nv = aCreate(map->allocator, u8, map->vsize * nc);
  var_ nk = aCreate(map->allocator, u8, map->ksize * nc);
  var_ nf = aCreate(map->allocator, ptrstype(itypeof(hxmap, flags)), nc);

  var_ ov = map->vals;
  var_ ok = map->keys;
  var_ of = map->flags;
  defer {
    aFree(map->allocator, ov, map->vsize * oc);
    aFree(map->allocator, ok, map->ksize * oc);
    aFree(map->allocator, of, sizeof(*of) * oc);
  };

  map->cap = nc;
  map->vals = nv;
  map->keys = nk;
  map->flags = nf;

  var_ ks = map->ksize;
  var_ vs = map->vsize;

  foreach (usize i, range(0, oc))
    if (of[i].flag == OCCUPIED) {
      u64 hx = of[i].ohash;
      var_ idx = hx % nc;

      while (nf[idx].flag == OCCUPIED) {
        idx++;
        if (idx >= nc) idx = 0;
      }

      nf[idx].flag = OCCUPIED;
      nf[idx].ohash = hx;
      memcpy(nk + (ks * idx), ok + (ks * i), ks);
      memcpy(nv + (vs * idx), ov + (vs * i), vs);
    }
}
void *hxmap_set(
    hxmap *m,
    void *key,
    void *val
) {
  if (!key) return nullptr;

  if_unlikely (m->count * 4 >= m->cap * 3) hxmap_manage(m, 2);

  var_ hx = hxmap_base_hash(m, key) & (~(u64)0 >> 2);
  var_ cap = m->cap;
  var_ ks = m->ksize;
  var_ vs = m->vsize;
  var_ idx = hx % cap;
  var_ target_idx = cap;

  while (m->flags[idx].flag != EMPTY) {
    if (m->flags[idx].flag == LEFT) {
      if (target_idx == cap) target_idx = idx;
    } else if (
        m->flags[idx].flag == OCCUPIED &&
        m->flags[idx].ohash == hx &&
        !hxmap_base_cmp(m, m->keys + (ks * idx), key)
    ) {
      if (val)
        return memcpy(m->vals + (vs * idx), val, vs);
      else {
        m->count--;
        m->flags[idx].flag = LEFT;
        return nullptr;
      }
    }
    idx++;
    idx %= cap;
  }

  if (!val) return nullptr;
  m->count++;

  if (target_idx != cap) idx = target_idx;

  m->flags[idx].flag = OCCUPIED;
  m->flags[idx].ohash = hx;
  memcpy(m->keys + (ks * idx), key, ks);
  return memcpy(m->vals + (vs * idx), val, vs);
}
void *hxmap_get(
    const hxmap *m,
    void *key
) {
  assertMessage(key);

  var_ hx = hxmap_base_hash(m, key) & (~(u64)0 >> 2);
  var_ cap = m->cap;
  var_ ks = m->ksize;
  var_ idx = hx % cap;

  while (m->flags[idx].flag != EMPTY) {
    if (
        m->flags[idx].flag == OCCUPIED &&
        m->flags[idx].ohash == hx &&
        !hxmap_base_cmp(m, m->keys + (ks * idx), key)
    ) return m->vals + (m->vsize * idx);
    idx++;
    idx %= cap;
  }
  return nullptr;
}
void *hxmap_val_key(
    const hxmap *map,
    void *val
) {
  usize idx = ((u8 *)val - map->vals) / map->vsize;
  return map->keys + (idx * map->ksize);
}
#endif
