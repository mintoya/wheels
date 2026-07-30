#if !defined MY_HXMAP_H
  #define MY_HXMAP_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"

typedef struct hxmap {
  allocfn allocator;
  const u32 ksize, vsize;
  usize count;
  int capbit;
  const fnptrof((const void *), u64) hfn;
  const fnptrof((const void *, const void *), i8) cmp;
  u64 *__restrict flags;
  u8 *__restrict keys;
  u8 *__restrict vals;
} hxmap;
typedef ptrstype(itypeof(hxmap, flags)) hxint;
typedef enum : hxint {
  HXEMPTY = 0,
  HXOCC = 1,
} mxflag;
CONST_EXPR int flagshift = sizeof(hxint) * 8 - 1;
[[gnu::pure]] static inline bool isHXOCCUPIED(hxint x) {
  CONST_EXPR hxint ebits = (hxint)1 << flagshift;
  return x & ebits;
}
[[gnu::pure]] static inline bool isHXEMPTY(hxint x) { return !isHXOCCUPIED(x); }
[[gnu::pure]] static inline hxint HXHASHBITS(hxint x) {
  CONST_EXPR hxint ebits = ((hxint)1 << flagshift) - 1;
  return x & ebits;
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
  #define mxmap_defaults(...) VA_SWITCH_REMP((3, 0, 0)__VA_OPT__(, (__VA_ARGS__)))
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
#endif

#if (defined MY_HXMAP_C && MY_HXMAP_C == 1) || \
    defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #undef MY_HXMAP_C
  #define MY_HXMAP_C (2)
void hxmap_newm(
    allocfn allocator,
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
  let rs = ((hxmap){
      .allocator = allocator,
      .ksize = (u32)ksize,
      .vsize = (u32)vsize,
      .count = 0,
      .capbit = capbit,
      .hfn = hashfn,
      .cmp = cmpfn,
      .flags = *acreate(allocator, ptrstype(itypeof(hxmap, flags))[cap]),
      .keys = **acreate(allocator, u8[ksize][cap]),
      .vals = **acreate(allocator, u8[vsize][cap]),
  });
  mcpy(*map, rs);
}
hxmap *hxmap_new(
    allocfn allocator,
    usize ksize,
    usize vsize,
    int capbit,
    itypeof(hxmap, hfn) hashfn,
    itypeof(hxmap, cmp) cmpfn
) {
  let map = (hxmap){};
  hxmap_newm(allocator, ksize, vsize, capbit, hashfn, cmpfn, &map);
  return avalue(allocator, map);
}
void hxmap_freem(hxmap map) {
  let allocator = map.allocator;
  usize cap = (usize)1 << map.capbit;
  adestroy(allocator, (ptrstype(map.flags)(*)[cap])map.flags);
  adestroy(allocator, (u8(*)[map.ksize][cap])map.keys);
  adestroy(allocator, (u8(*)[map.vsize][cap])map.vals);
}
void hxmap_free(hxmap *map) {
  let allocator = map->allocator;
  hxmap_freem(*map);
  adestroy(allocator, map);
}
static inline i8 hxmap_base_cmp(const hxmap *m, const void *a, const void *b) {
  if (m->cmp) return m->cmp(a, b);
  return memcmp(a, b, m->ksize);
}
static inline hxint hxmap_base_hash(const hxmap *m, const void *a) {
  if (m->hfn) return m->hfn(a);
  u8(*bytes)[m->ksize] = (typeof(bytes))a;
  switch (sizeof(*bytes)) {
    case sizeof(u64): {
      let x = *(hxint *)bytes;
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
      hxint hash = 0xcbf29ce484222325ULL;
      foreach (let b, vla(*bytes)) {
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
  let ocb = map->capbit;
  let ncb = map->capbit + scale;
  let oc = (usize)1 << ocb;
  let nc = (usize)1 << ncb;

  usize newcount = 0;
  // let nc = scale < 0 ? map->cap / (-scale) : map->cap * scale;
  let nv = acreate(map->allocator, u8[map->vsize * nc]);
  let nk = acreate(map->allocator, u8[map->ksize * nc]);
  let nf = acreate(map->allocator, ptrstype(itypeof(hxmap, flags))[nc]);

  let ov = map->vals;
  let ok = map->keys;
  let of = map->flags;
  defer {
    adestroy(map->allocator, (u8(*)[map->vsize][oc])ov);
    adestroy(map->allocator, (u8(*)[map->ksize][oc])ok);
    adestroy(map->allocator, (u8(*)[sizeof(*of)][oc])of);
  };

  map->capbit = ncb;
  map->vals = *nv;
  map->keys = *nk;
  map->flags = *nf;

  let ks = map->ksize;
  let vs = map->vsize;

  foreach (usize i, range(0, oc))
    if (isHXOCCUPIED(of[i])) {
      newcount++;
      hxint hx = HXHASHBITS(of[i]);
      let idx = hx & (nc - 1);

      while (isHXOCCUPIED((*nf)[idx])) {
        idx++;
        if (idx >= nc) idx = 0;
      }

      (*nf)[idx] = ((hxint)HXOCC << flagshift) | hx;
      memcpy(map->keys + (ks * idx), ok + (ks * i), ks);
      memcpy(map->vals + (vs * idx), ov + (vs * i), vs);
    }
  map->count = newcount;
}

void *hxmap_get(const hxmap *m, void *key) {
  assertMessage(key);
  let hx = HXHASHBITS(hxmap_base_hash(m, key));
  let mask = ((usize)1 << m->capbit) - 1;

  for (let idx = hx & mask; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
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

  let hx = HXHASHBITS(hxmap_base_hash(m, key));
  let mask = cap - 1;
  let idx = hx & mask;

  for (; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        HXHASHBITS(m->flags[idx]) == hx &&
        !hxmap_base_cmp(m, m->keys + (m->ksize * idx), key)
    ) break;
  }

  const let flag = m->flags[idx];
  if (!val) {
    if (isHXOCCUPIED(flag)) {
      m->count--;
      m->flags[idx] = HXEMPTY;

      let i = idx;
      let j = (i + 1) & mask;

      for (; !isHXEMPTY(m->flags[j]); j = (j + 1) & mask) {
        let k = HXHASHBITS(m->flags[j]) & mask;

        let dist_i = (i - k) & mask;
        let dist_j = (j - k) & mask;

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
  m->flags[idx] = hx | (HXOCC << flagshift);
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
void hxmap_clear(hxmap *map) {
  memset(map->flags, 0, sizeof(ptrstype(map->flags)[1 << map->capbit]));
}
#endif
