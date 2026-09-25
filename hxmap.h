#if !defined MY_HXMAP_H
  #define MY_HXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"

typedef struct {
  fnptrof((void *, const void *, const void *), cmpres) fn;
  void *arg;
} hxmap_cmp;
typedef struct {
  fnptrof((void *, const void *), u64) fn;
  void *arg;
} hxmap_hsh;
typedef struct hxmap {
  allocfn allocator;
  const u32 ksize, vsize;
  usize count;
  usize cap;
  const hxmap_hsh hfn;
  const hxmap_cmp cmp;
  u8 *__restrict ctrl;
  u8 *__restrict keys;
  u8 *__restrict vals;
} hxmap;
typedef u64 hxint;

  #define HXCTRL_OCC (0x80)
  #define HXCTRL_BITS (0x7f)

[[gnu::pure]] static inline bool hxmap_occupied(const hxmap *map, usize i) {
  return (map->ctrl[i] & HXCTRL_OCC) != 0;
}
[[gnu::pure]] static inline u8 hxmap_tag(u64 h) { return (u8)(HXCTRL_OCC | (h & HXCTRL_BITS)); }
[[gnu::pure]] static inline usize hxmap_next(const hxmap *map, usize i) {
  return i + 1 == map->cap ? 0 : i + 1;
}
// maps a 64 bit hash onto [0, cap) for any cap, using the high bits
[[gnu::pure]] static inline usize hxmap_slot(const hxmap *map, u64 h) {
  #if defined __SIZEOF_INT128__
  return (usize)(((__uint128_t)h * (__uint128_t)map->cap) >> 64);
  #else
  return (usize)((((h >> 32) * (u64)map->cap)) >> 32);
  #endif
}
[[gnu::pure]] static inline void *hxmap_key_at(const hxmap *map, usize i) {
  return map->keys + (i * map->ksize);
}
[[gnu::pure]] static inline void *hxmap_val_at(const hxmap *map, usize i) {
  return map->vals + (i * map->vsize);
}

void hxmap_newm(
    allocfn allocator,
    usize ksize,
    usize vsize,
    int power,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn,
    hxmap map[1]
);
hxmap *hxmap_new(
    allocfn allocator,
    usize ksize,
    usize vsize,
    int power,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
);
void hxmap_freem(hxmap map);
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
void hxmap_clear(hxmap *map);

  #define mxmap(K, V) ptrof(fnptrof((hxmap *, ptrof(K)), V))
  #define mxmap_valType(map) typeof((*map)(((hxmap *)0), nullptr))
  #define mxmap_defaults(...) VA_SWITCH_REMP((3, ((hxmap_hsh){}), ((hxmap_cmp){}))__VA_OPT__(, (__VA_ARGS__)))
  #define mxmap_init(allocator, K, V, ...) (mxmap(K, V)) hxmap_new(allocator, sizeof(K), sizeof(V), mxmap_defaults(__VA_ARGS__))
  #define mxmap_set(map, key, val) ({                                  \
    let _k = key;                                                      \
    let _v = val;                                                      \
    ASSERT_EXPR(types_eq(typeof(map), mxmap(typeof(_k), typeof(_v)))); \
    (ptrof(mxmap_valType(map))) hxmap_set((hxmap *)map, &_k, &_v);     \
  })
  #define mxmap_rem(map, key) ({                                               \
    let _k = key;                                                              \
    ASSERT_EXPR(types_eq(typeof(map), mxmap(typeof(_k), mxmap_valType(map)))); \
    (ptrof(mxmap_valType(map))) hxmap_set((hxmap *)map, &_k, nullptr);         \
  })
  #define mxmap_get(map, key) ({                                               \
    let _k = key;                                                              \
    ASSERT_EXPR(types_eq(typeof(map), mxmap(typeof(_k), mxmap_valType(map)))); \
    (ptrof(mxmap_valType(map))) hxmap_get((hxmap *)map, &_k);                  \
  })
  #define mxmap_deinit(map) hxmap_free(((void)(sizeof(typeof(mxmap_valType(map)))), (hxmap *)map))

//{hxmap(map)

  #define FOREACH_hxmap_init(map_ptr) ( \
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
  #define FOREACH_hxmap_increase(is) (is._idx++)
  #define FOREACH_hxmap_valid(is)                                                         \
    ({                                                                                    \
      while (is._idx < ((hxmap *)is._m)->cap && !hxmap_occupied((hxmap *)is._m, is._idx)) \
        is._idx++;                                                                        \
      is._idx < ((hxmap *)is._m)->cap;                                                    \
    })
  #define FOREACH_hxmap_cast(is)                      \
    ((typeof(is._val[0])){                            \
        .key = hxmap_key_at((hxmap *)is._m, is._idx), \
        .val = hxmap_val_at((hxmap *)is._m, is._idx), \
    })

  #define FOREACH_hxmap_iter    \
    (                           \
        FOREACH_hxmap_init,     \
        FOREACH_hxmap_increase, \
        FOREACH_hxmap_valid,    \
        FOREACH_hxmap_cast)

  #define FOREACH_mxmap_init(map_ptr, keytype, valtype) ( \
      struct {                                            \
        typeof(map_ptr) _m;                               \
        size_t _idx;                                      \
        struct {                                          \
          keytype key;                                    \
          valtype *val;                                   \
        } _val[0];                                        \
      },                                                  \
      ({                                                  \
        let _map_eval = map_ptr;                          \
        _Static_assert(                                   \
            types_eq(                                     \
                typeof(_map_eval),                        \
                typeof(mxmap(keytype, valtype))           \
            ),                                            \
            "wrong types passed to iterator"              \
        );                                                \
        (typeof(_foreach_._foreach_)){                    \
            ._m = _map_eval,                              \
            ._idx = 0,                                    \
        };                                                \
      })                                                  \
  )
  #define FOREACH_mxmap_cast(is)                                               \
    ((typeof(is._val[0])){                                                     \
        .key = *(typeof(is._val->key) *)hxmap_key_at((hxmap *)is._m, is._idx), \
        .val = (typeof(is._val->val))hxmap_val_at((hxmap *)is._m, is._idx),    \
    })
  #define FOREACH_mxmap_iter    \
    (                           \
        FOREACH_mxmap_init,     \
        FOREACH_hxmap_increase, \
        FOREACH_hxmap_valid,    \
        FOREACH_mxmap_cast)
  //}
  #include "tests.h"
test_fn(hxmap_tests) {
  let map = mxmap_init(allocator, u32, hxint);

  u32 k1 = 42;
  hxint v1 = 100;
  mxmap_set(map, k1, v1);

  let r1 = mxmap_get(map, k1);
  test_assert(r1);
  test_assert(*r1 == 100);
  test_assert(((hxmap *)map)->count == 1);

  hxint v2 = 200;
  mxmap_set(map, k1, v2);
  let r2 = mxmap_get(map, k1);
  test_assert(r2);
  test_assert(*r2 == 200);
  test_assert(((hxmap *)map)->count == 1);

  u32 k2 = 99;
  let r3 = mxmap_get(map, k2);
  test_assert(!r3);

  mxmap_rem(map, k1);
  test_assert(!mxmap_get(map, k1));

  test_assert(((hxmap *)map)->count == 0);

  for (u32 i = 0; i < 1000; i++) {
    hxint val = i * 10;
    mxmap_set(map, i, val);
  }

  test_assert(((hxmap *)map)->count == 1000);

  let ps = acreate(allocator, hxint[1000]);
  defer { adestroy(allocator, ps); };

  foreach (let item, mxmap_iter(map, u32, hxint))
    ps[0][item.key] = 1;
  foreach (let x, vlap(ps))
    test_assert(x);

  foreach (u32 i, range(0, 1000)) {
    let r = mxmap_get(map, i);
    test_assert(r);
    test_assert(*r == i * 10);
  }

  mxmap_deinit(map);
}
// every key inserted then removed must leave a table that still finds the rest,
// with the table shrinking back down instead of holding the high water mark
test_fn(hxmap_shrink_tests) {
  let map = mxmap_init(allocator, u64, u64);
  foreach (u64 i, range(0, 20000))
    mxmap_set(map, i, i + 1);
  let grown = ((hxmap *)map)->cap;
  test_assert(((hxmap *)map)->count == 20000);
  foreach (u64 i, range(0, 20000))
    mxmap_rem(map, i);
  test_assert(((hxmap *)map)->count == 0);
  test_assert(((hxmap *)map)->cap < grown);

  // still usable, and nothing survives a reshuffle that should not
  foreach (u64 i, range(0, 2000))
    mxmap_set(map, i, i + 7);
  foreach (u64 i, range(0, 2000))
    if (i % 2) mxmap_rem(map, i);
  foreach (u64 i, range(0, 2000)) {
    let r = mxmap_get(map, i);
    if (i % 2) test_assert(!r);
    else test_assert(r && *r == i + 7);
  }
  test_assert(((hxmap *)map)->count == 1000);
  mxmap_deinit(map);
}
#endif

#if (defined MY_HXMAP_C && MY_HXMAP_C == 1) || \
    defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #undef MY_HXMAP_C
  #define MY_HXMAP_C (2)
void hxmap_newm(
    allocfn allocator,
    usize ksize,
    usize vsize,
    int power,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn,
    hxmap map[1]
) {
  usize cap = (usize)1 << (power > 0 ? (usize)power : 3);
  assertMessage(allocator);
  assertMessage(ksize);
  assertMessage(vsize);
  let rs = ((hxmap){
      .allocator = allocator,
      .ksize = (u32)ksize,
      .vsize = (u32)vsize,
      .count = 0,
      .cap = cap,
      .hfn = hashfn,
      .cmp = cmpfn,
      .ctrl = *acreate(allocator, u8[cap]),
      .keys = *acreate(allocator, u8[ksize * cap]),
      .vals = *acreate(allocator, u8[vsize * cap]),
  });
  memcpy((void *)map, &rs, sizeof(rs));
}
hxmap *hxmap_new(
    allocfn allocator,
    usize ksize,
    usize vsize,
    int power,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
) {
  let map = (hxmap){};
  hxmap_newm(allocator, ksize, vsize, power, hashfn, cmpfn, &map);
  return avalue(allocator, map);
}
void hxmap_freem(hxmap map) {
  let allocator = map.allocator;
  usize cap = map.cap;
  adestroy(allocator, (u8(*)[cap])map.ctrl);
  adestroy(allocator, (u8(*)[map.ksize * cap]) map.keys);
  adestroy(allocator, (u8(*)[map.vsize * cap]) map.vals);
}
void hxmap_free(hxmap *map) {
  let allocator = map->allocator;
  hxmap_freem(*map);
  adestroy(allocator, map);
}
static inline cmpres hxmap_base_cmp(const hxmap *m, const void *a, const void *b) {
  if (m->cmp.fn) return m->cmp.fn(m->cmp.arg, a, b);
  return cmp_memcmp(a, b, m->ksize);
}
// the finalizer, so that every hash - custom ones included - spreads over the
// full 64 bits (hxmap_slot reads the high bits)
[[gnu::pure]] static inline u64 hxmap_mix(u64 x) {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}
static inline hxint hxmap_base_hash(const hxmap *m, const void *a) {
  if (m->hfn.fn) return hxmap_mix(m->hfn.fn(m->hfn.arg, a));
  u8(*bytes)[m->ksize] = (typeof(bytes))a;
  switch (sizeof(*bytes)) {
    case sizeof(u64): {
      return hxmap_mix(*(u64 *)bytes);
    }
    case sizeof(u32): {
      return hxmap_mix(*(u32 *)bytes);
    }
    case sizeof(u16):
      return hxmap_mix(*(u16 *)bytes);
    case sizeof(u8):
      return hxmap_mix(*(u8 *)bytes);
    default: {
      hxint hash = 0xcbf29ce484222325ULL;
      foreach (let b, vla(*bytes)) {
        hash ^= b;
        hash *= 0x100000001b3ULL;
      }
      return hxmap_mix(hash);
    }
  }
}
void hxmap_resize(hxmap *map, usize newcap);
[[gnu::pure]] static inline usize hxmap_nextcap(const hxmap *map) {
  return map->cap < 512 ? map->cap * 2 : map->cap + (map->cap / 2) + 1;
}
static inline void hxmap_place(hxmap *map, const void *key, const void *val) {
  let h = hxmap_base_hash(map, key);
  usize idx = hxmap_slot(map, h);
  usize n = map->cap;
  while (n && map->ctrl[idx]) {
    idx = hxmap_next(map, idx);
    n--;
  }
  if_unlikely (!n) return hxmap_resize(map, hxmap_nextcap(map)), hxmap_place(map, key, val);
  map->ctrl[idx] = hxmap_tag(h);
  memcpy(map->keys + (idx * map->ksize), key, map->ksize);
  memcpy(map->vals + (idx * map->vsize), val, map->vsize);
  map->count++;
}
void hxmap_resize(hxmap *map, usize newcap) {
  if (newcap < 8) newcap = 8;
  // never resize into a table that cannot hold what is already there
  usize least = map->count + (map->count / 2) + 8;
  if (newcap < least) newcap = least;
  if (newcap == map->cap) return;
  let allocator = map->allocator;
  usize ocap = map->cap;
  u8 *oc = map->ctrl, *ok = map->keys, *ov = map->vals;

  map->cap = newcap;
  map->count = 0;
  map->ctrl = *acreate(allocator, u8[newcap]);
  map->keys = *acreate(allocator, u8[map->ksize * newcap]);
  map->vals = *acreate(allocator, u8[map->vsize * newcap]);

  foreach (usize i, range(0, ocap))
    if (oc[i] & HXCTRL_OCC) hxmap_place(map, ok + (i * map->ksize), ov + (i * map->vsize));

  adestroy(allocator, (u8(*)[ocap])oc);
  adestroy(allocator, (u8(*)[map->ksize * ocap]) ok);
  adestroy(allocator, (u8(*)[map->vsize * ocap]) ov);
}
void hxmap_manage(hxmap *map, i8 scale) {
  if (!scale) return;
  hxmap_resize(map, scale > 0 ? (map->cap << scale) : (map->cap >> -scale));
}
void *hxmap_get(const hxmap *m, void *key) {
  assertMessage(key);
  if_unlikely (!m->cap) return nullptr;
  let h = hxmap_base_hash(m, key);
  let tag = hxmap_tag(h);
  usize n = m->cap;
  for (usize idx = hxmap_slot(m, h); n; idx = hxmap_next(m, idx), n--) {
    let c = m->ctrl[idx];
    if (!c) return nullptr;
    if (c == tag && !hxmap_base_cmp(m, m->keys + (idx * m->ksize), key))
      return m->vals + (idx * m->vsize);
  }
  return nullptr;
}
void *hxmap_set(
    hxmap *m,
    void *key,
    void *val
) {
  if (!key) return nullptr;
  if_unlikely (!m->cap) hxmap_resize(m, 8);
  if_unlikely ((m->count + 1) * 4 >= m->cap * 3) hxmap_resize(m, hxmap_nextcap(m));

  let h = hxmap_base_hash(m, key);
  let tag = hxmap_tag(h);
  usize n = m->cap, idx = hxmap_slot(m, h);

  for (; n; idx = hxmap_next(m, idx), n--) {
    if (!m->ctrl[idx]) break;
    if (m->ctrl[idx] == tag && !hxmap_base_cmp(m, m->keys + (idx * m->ksize), key)) break;
  }
  if_unlikely (n == 0) { // no empty slot: grow and retry
    hxmap_resize(m, hxmap_nextcap(m));
    return hxmap_set(m, key, val);
  }

  if (!val) {
    if (m->ctrl[idx] & HXCTRL_OCC) {
      m->count--;
      m->ctrl[idx] = 0;

      usize i = idx;
      usize j = hxmap_next(m, i);
      while (m->ctrl[j]) {
        // the tables no longer store the full hash, so the home slot of the
        // occupant has to be recomputed to know if it can move back
        usize home = hxmap_slot(m, hxmap_base_hash(m, m->keys + (j * m->ksize)));
        usize dist_i = i >= home ? i - home : i + m->cap - home;
        usize dist_j = j >= home ? j - home : j + m->cap - home;
        if (dist_i < dist_j) {
          m->ctrl[i] = m->ctrl[j];
          memcpy(m->keys + (i * m->ksize), m->keys + (j * m->ksize), m->ksize);
          memcpy(m->vals + (i * m->vsize), m->vals + (j * m->vsize), m->vsize);
          m->ctrl[j] = 0;
          i = j;
        }
        j = hxmap_next(m, j);
      }
      // give the memory back once the table is mostly empty
      if_unlikely (m->cap > 64 && m->count * 8 < m->cap) hxmap_resize(m, m->cap / 2);
    }
    return nullptr;
  }

  m->count += !(m->ctrl[idx] & HXCTRL_OCC);
  m->ctrl[idx] = tag;
  memcpy(m->keys + (idx * m->ksize), key, m->ksize);
  return memcpy(m->vals + (idx * m->vsize), val, m->vsize);
}

void *hxmap_val_key(
    const hxmap *map,
    void *val
) {
  usize idx = ((u8 *)val - map->vals) / map->vsize;
  return map->keys + (idx * map->ksize);
}
void hxmap_clear(hxmap *map) {
  memset(map->ctrl, 0, map->cap);
  map->count = 0;
}
#endif
