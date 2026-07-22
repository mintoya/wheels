#if !defined MY_HXMAP_H
  #define MY_HXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"

typedef enum : u64 {
  HXEMPTY = 0,
  HXOCC = 1,
} mxflag;

[[gnu::pure]] static inline bool isHXOCCUPIED(u64 x) {
  constexpr u64 ebits = (u64)1 << 63;
  return x & ebits;
}
[[gnu::pure]] static inline bool isHXEMPTY(u64 x) { return !isHXOCCUPIED(x); }
[[gnu::pure]] static inline u64 HXHASHBITS(u64 x) {
  constexpr u64 ebits = ((u64)1 << 63) - 1;
  return x & ebits;
}

typedef struct hxmap {
  AllocatorV allocator;
  const u32 ksize, vsize;
  usize count;
  int capbit;
  const fnptrof((const void *), u64) hfn;
  const fnptrof((const void *, const void *), i8) cmp;
  u64 *__restrict flags;
  u8 *__restrict keys;
  u8 *__restrict vals;
} hxmap;
void hxmap_newm(
    AllocatorV allocator,
    usize ksize,
    usize vsize,
    int power,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn,
    hxmap map[1]
);
hxmap *hxmap_new(
    AllocatorV allocator,
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

  #define mxmap(K, V) ptrof(fnptrof((hxmap *, ptrof(K)), V))
  #define mxmap_valType(map) typeof((*map)(((hxmap *)0), nullptr))
  #define mxmap_defaults(...) VA_SWITCH_REMP((3, 0, 0)__VA_OPT__(, (__VA_ARGS__)))
  #define mxmap_init(allocator, K, V, ...) (mxmap(K, V)) hxmap_new(allocator, sizeof(K), sizeof(V), mxmap_defaults(__VA_ARGS__))
  #define mxmap_set(map, key, val) ({                                  \
    var_ _k = key;                                                     \
    var_ _v = val;                                                     \
    ASSERT_EXPR(types_eq(typeof(map), mxmap(typeof(_k), typeof(_v)))); \
    (ptrof(mxmap_valType(map))) hxmap_set((hxmap *)map, &_k, &_v);     \
  })
  #define mxmap_rem(map, key) ({                                               \
    var_ _k = key;                                                             \
    ASSERT_EXPR(types_eq(typeof(map), mxmap(typeof(_k), mxmap_valType(map)))); \
    (ptrof(mxmap_valType(map))) hxmap_set((hxmap *)map, &_k, nullptr);         \
  })
  #define mxmap_get(map, key) ({                                               \
    var_ _k = key;                                                             \
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
        var_ _map_eval = map_ptr;       \
        (typeof(_foreach_._foreach_)){  \
            ._m = _map_eval,            \
            ._idx = 0,                  \
        };                              \
      })                                \
  )
  #define FOREACH_hxmap_increase(is) (is._idx++)
  #define FOREACH_hxmap_valid(is)                                                                                 \
    ({                                                                                                            \
      while (is._idx < ((usize)1 << ((hxmap *)is._m)->capbit) && !isHXOCCUPIED(((hxmap *)is._m)->flags[is._idx])) \
        is._idx++;                                                                                                \
      is._idx < ((usize)1 << ((hxmap *)is._m)->capbit);                                                           \
    })
  #define FOREACH_hxmap_cast(is)                                                       \
    ((typeof(is._val[0])){                                                             \
        .key = (void *)(((hxmap *)is._m)->keys + (is._idx * ((hxmap *)is._m)->ksize)), \
        .val = (void *)(((hxmap *)is._m)->vals + (is._idx * ((hxmap *)is._m)->vsize)), \
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
        var_ _map_eval = map_ptr;                         \
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
  #define FOREACH_mxmap_cast(is)                                                                        \
    ((typeof(is._val[0])){                                                                              \
        .key = *(typeof(is._val->key) *)(((hxmap *)is._m)->keys + (is._idx * ((hxmap *)is._m)->ksize)), \
        .val = (typeof(is._val->val))(((hxmap *)is._m)->vals + (is._idx * ((hxmap *)is._m)->vsize)),    \
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
  var_ map = mxmap_init(allocator, u32, u64);

  u32 k1 = 42;
  u64 v1 = 100;
  mxmap_set(map, k1, v1);

  var_ r1 = mxmap_get(map, k1);
  test_assert(r1);
  test_assert(*r1 == 100);
  test_assert(((hxmap *)map)->count == 1);

  u64 v2 = 200;
  mxmap_set(map, k1, v2);
  var_ r2 = mxmap_get(map, k1);
  test_assert(r2);
  test_assert(*r2 == 200);
  test_assert(((hxmap *)map)->count == 1);

  u32 k2 = 99;
  var_ r3 = mxmap_get(map, k2);
  test_assert(!r3);

  mxmap_rem(map, k1);
  test_assert(!mxmap_get(map, k1));

  test_assert(((hxmap *)map)->count == 0);

  for (u32 i = 0; i < 1000; i++) {
    u64 val = i * 10;
    mxmap_set(map, i, val);
  }

  test_assert(((hxmap *)map)->count == 1000);

  var_ ps = &aCreate(allocator, u64, 1000);
  defer { aFree(allocator, ps, sizeof(*ps)); };

  foreach (var_ item, mxmap_iter(map, u32, u64))
    ps[0][item.key] = 1;
  foreach (var_ x, vlap(ps))
    test_assert(x);

  foreach (u32 i, range(0, 1000)) {
    var_ r = mxmap_get(map, i);
    test_assert(r);
    test_assert(*r == i * 10);
  }

  mxmap_deinit(map);
}
#endif

#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define MY_HXMAP_C (1)
#endif

#if defined MY_HXMAP_C && MY_HXMAP_C == 1
void hxmap_newm(
    AllocatorV allocator,
    usize ksize,
    usize vsize,
    int capbit,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn,
    hxmap map[1]
) {
  capbit = capbit ?: 3;
  usize cap = (usize)1 << capbit;
  assertMessage(allocator);
  assertMessage(ksize);
  assertMessage(vsize);
  var_ rs = ((hxmap){
      .allocator = allocator,
      .ksize = (u32)ksize,
      .vsize = (u32)vsize,
      .count = 0,
      .capbit = capbit,
      .hfn = hashfn,
      .cmp = cmpfn,
      .flags = aCreate(allocator, ptrstype(itypeof(hxmap, flags)), cap),
      .keys = aCreate(allocator, u8, cap * ksize),
      .vals = aCreate(allocator, u8, cap * vsize),
  });
  mcpy(*map, rs);
}
hxmap *hxmap_new(
    AllocatorV allocator,
    usize ksize,
    usize vsize,
    int capbit,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
) {
  var_ map = (hxmap){};
  hxmap_newm(allocator, ksize, vsize, capbit, hashfn, cmpfn, &map);
  return aValue(allocator, map);
}
void hxmap_freem(hxmap map) {
  var_ allocator = map.allocator;
  usize cap = (usize)1 << map.capbit;
  aFree(allocator, map.flags, sizeof(*map.flags) * cap);
  aFree(allocator, map.keys, map.ksize * cap);
  aFree(allocator, map.vals, map.vsize * cap);
}
void hxmap_free(hxmap *map) {
  var_ allocator = map->allocator;
  hxmap_freem(*map);
  aDestroy(allocator, map);
}
static inline i8 hxmap_base_cmp(const hxmap *m, const void *a, const void *b) {
  if (m->cmp) return m->cmp(a, b);
  return memcmp(a, b, m->ksize);
}
static inline u64 hxmap_base_hash(const hxmap *m, const void *a) {
  if (m->hfn) return m->hfn(a);
  u8(*bytes)[m->ksize] = (typeof(bytes))a;
  switch (sizeof(*bytes)) {
    case sizeof(u64): {
      u64 x = *(u64 *)bytes;
      x ^= x >> 30;
      x *= 0xbf58476d1ce4e5b9ULL;
      x ^= x >> 27;
      x *= 0x94d049bb133111ebULL;
      x ^= x >> 31;
      return x;
    }
    case sizeof(u32): {
      u32 x = *(u32 *)bytes;
      x ^= x >> 16;
      x *= 0x85ebca6b;
      x ^= x >> 13;
      x *= 0xc2b2ae35;
      x ^= x >> 16;
      return x;
    }
    case sizeof(u16):
      return *(u16 *)bytes * 0x85ebca6b;
    case sizeof(u8):
      return *(u8 *)bytes * 0x85ebca6b;
    default: {
      u64 hash = 0xcbf29ce484222325ULL;
      foreach (var_ b, vla(*bytes)) {
        hash ^= b;
        hash *= 0x100000001b3ULL;
      }
      return hash;
    }
  }
}
void hxmap_manage(
    hxmap *map,
    i8 scale
) {

  assert(scale > 0);
  var_ ocb = map->capbit;
  var_ ncb = map->capbit + scale;
  var_ oc = (usize)1 << ocb;
  var_ nc = (usize)1 << ncb;

  usize newcount = 0;
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

  map->capbit = ncb;
  map->vals = nv;
  map->keys = nk;
  map->flags = nf;

  var_ ks = map->ksize;
  var_ vs = map->vsize;

  foreach (usize i, range(0, oc))
    if (isHXOCCUPIED(of[i])) {
      newcount++;
      u64 hx = HXHASHBITS(of[i]);
      var_ idx = hx % nc;

      while (isHXOCCUPIED(nf[idx])) {
        idx++;
        if (idx >= nc) idx = 0;
      }

      nf[idx] = ((u64)HXOCC << 63) | hx;
      memcpy(nk + (ks * idx), ok + (ks * i), ks);
      memcpy(nv + (vs * idx), ov + (vs * i), vs);
    }
  map->count = newcount;
}

void *hxmap_get(const hxmap *m, void *key) {
  assertMessage(key);
  var_ hx = HXHASHBITS(hxmap_base_hash(m, key));
  var_ mask = ((usize)1 << m->capbit) - 1;

  for (var_ idx = hx & mask; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        isHXOCCUPIED(m->flags[idx]) &&
        HXHASHBITS(m->flags[idx]) == hx &&
        !hxmap_base_cmp(m, m->keys + (m->ksize * idx), key)
    ) return m->vals + (m->vsize * idx);
  }
  return nullptr;
}
void *hxmap_set(
    hxmap *m,
    void *key,
    void *val
) {
  if (!key) return nullptr;
  usize cap = (usize)1 << (m->capbit);
  if_unlikely ((m->count + 1) * 4 >= cap * 3) hxmap_manage(m, 1);
  cap = (usize)1 << (m->capbit);

  var_ hx = HXHASHBITS(hxmap_base_hash(m, key));
  var_ mask = cap - 1;
  var_ idx = hx & mask;

  for (; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        HXHASHBITS(m->flags[idx]) == hx &&
        !hxmap_base_cmp(m, m->keys + (m->ksize * idx), key)
    ) break;
  }

  const var_ flag = m->flags[idx];
  if (!val) {
    if (isHXOCCUPIED(flag)) {
      m->count--;
      m->flags[idx] = HXEMPTY;

      var_ i = idx;
      var_ j = (i + 1) & mask;

      for (; !isHXEMPTY(m->flags[j]); j = (j + 1) & mask) {
        var_ k = HXHASHBITS(m->flags[j]) & mask;

        var_ dist_i = (i - k) & mask;
        var_ dist_j = (j - k) & mask;

        if (dist_i < dist_j) {
          m->flags[i] = m->flags[j];
          memcpy(m->keys + (m->ksize * i), m->keys + (m->ksize * j), m->ksize);
          memcpy(m->vals + (m->vsize * i), m->vals + (m->vsize * j), m->vsize);
          m->flags[j] = HXEMPTY;
          i = j;
        }
      }
    }
    return nullptr;
  }

  m->count += isHXEMPTY(flag);
  m->flags[idx] = hx | (HXOCC << 63);
  memcpy(m->keys + (m->ksize * idx), key, m->ksize);
  return memcpy(m->vals + (m->vsize * idx), val, m->vsize);
}

void *hxmap_val_key(
    const hxmap *map,
    void *val
) {
  usize idx = ((u8 *)val - map->vals) / map->vsize;
  return map->keys + (idx * map->ksize);
}
#endif
