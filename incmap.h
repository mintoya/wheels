#if __INCLUDE_LEVEL__ == 0
  #pragma GCC warning "using example mapconfig"
// comparison ismeant to be ordered, but currently it does not sort the collisoins at all
  #define mapconfig iimap, int, int, ((k) * 31 ^ 0x1000), (!(a == b))
#endif

#include "allocator.h"
#include "macros.h"
#include "mytypes.h"

#ifndef mapconfig
  #error "mapconfig must be defined as: typename , KeyT, ValT , hash(T k), comparison(T a , T b)"
#endif

#define _MAP_GET_K(name, k, v, ...) k
#define _MAP_GET_V(name, k, v, ...) v
#define _MAP_EVAL_K(tuple) _MAP_GET_K(tuple)
#define _MAP_EVAL_V(tuple) _MAP_GET_V(tuple)

#define MAP_K _MAP_EVAL_K(mapconfig)
#define MAP_V _MAP_EVAL_V(mapconfig)

#define MAP_HASHASHI(name, k, v, ...) VA_SWITCH(0 __VA_OPT__(, 1))
#define MAP_HASHASH(__VA_ARGS__) MAP_HASHASHI(__VA_ARGS__)
#define MAP_HASCMPII(name, k, v, h, ...) VA_SWITCH(0 __VA_OPT__(, 1))
#define MAP_HASCMPI(name, k, v, ...) VA_SWITCH(0 __VA_OPT__(, MAP_HASCMPII(name, k, v, __VA_ARGS__)))
#define MAP_HASCMP(__VA_ARGS__) MAP_HASHASHI(__VA_ARGS__)

#define mapnameii(name, ...) name
#define mapnamei(...) mapnameii(__VA_ARGS__)
#define mapname mapnamei(mapconfig)

#if MAP_HASHASH(mapconfig)
  #define MAP_GETCMPI(name, k, v, h, cmp) cmp
  #define MAP_GETCMP(__VA_ARGS__) MAP_GETCMPI(__VA_ARGS__)
  #define mapcmp(_a, _b) ({let a = _a;let b = _b;  MAP_GETCMP(mapconfig)  ; })
#else
  #define mapcmp(a, b) MAP_FN(default_cmp)(a, b)
#endif
#if MAP_HASCMP(mapconfig)
  #define MAP_GETHASHI(name, k, v, hash, ...) hash
  #define MAP_GETHASH(__VA_ARGS__) MAP_GETHASHI(__VA_ARGS__)
  #define maphash(_k) ({let k = _k;  MAP_GETHASH(mapconfig)  ; })
#else
  #define maphash(a) MAP_FN(default_hash)(a)
#endif

#define _MAP_CAT_(a, b) a##b
#define _MAP_CAT(a, b) _MAP_CAT_(a, b)
#define MAP_FN(fn) _MAP_CAT(mapname, _MAP_CAT(_, fn))

// Isolate all types and constants per map instance
#define hxint _MAP_CAT(mapname, _hxint)
#define HXEMPTY _MAP_CAT(mapname, __HXEMPTY)
#define HXOCC _MAP_CAT(mapname, __HXOCC)
#define mxflag _MAP_CAT(mapname, _mxflag)
#define flagshift _MAP_CAT(mapname, _flagshift)
#define isHXOCCUPIED _MAP_CAT(mapname, _isHXOCCUPIED)
#define isHXEMPTY _MAP_CAT(mapname, _isHXEMPTY)
#define HXHASHBITS _MAP_CAT(mapname, _HXHASHBITS)

typedef u64 hxint;
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

typedef struct mapname {
  allocfn allocator;
  usize count;
  int capbit;
  u64 *__restrict flags;
  MAP_K *__restrict keys;
  MAP_V *__restrict vals;
} mapname;

static inline i8 MAP_FN(default_cmp)(MAP_K a, MAP_K b) {
  return memcmp(&a, &b, sizeof(MAP_K));
}

static inline hxint MAP_FN(default_hash)(MAP_K a) {
  u8(*bytes)[sizeof(MAP_K)] = (typeof(bytes))&a;
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

static inline void MAP_FN(newm)(
    allocfn allocator,
    int capbit,
    mapname map[1]
) {
  capbit = capbit ?: 3;
  usize cap = (usize)1 << capbit;
  assert(allocator);
  let rs = ((mapname){
      .allocator = allocator,
      .count = 0,
      .capbit = capbit,
      .flags = *acreate(allocator, ptrstype(itypeof(mapname, flags))[cap]),
      .keys = *acreate(allocator, MAP_K[cap]),
      .vals = *acreate(allocator, MAP_V[cap]),
  });
  mcpy(*map, rs);
}

static inline mapname *MAP_FN(new)(
    allocfn allocator,
    int capbit
) {
  let map = acreate(allocator, mapname);
  MAP_FN(newm)(allocator, capbit, map);
  return map;
}

static inline void MAP_FN(freem)(mapname map) {
  let allocator = map.allocator;
  adestroy(allocator, map.flags);
  adestroy(allocator, map.keys);
  adestroy(allocator, map.vals);
}

static inline void MAP_FN(free)(mapname *map) {
  let allocator = map->allocator;
  MAP_FN(freem)(*map);
  adestroy(allocator, map);
}

static inline void MAP_FN(manage)(
    mapname *map,
    i8 scale
) {
  assert(scale > 0);
  let ocb = map->capbit;
  let ncb = map->capbit + scale;
  let oc = (usize)1 << ocb;
  let nc = (usize)1 << ncb;

  usize newcount = 0;
  let nv = acreate(map->allocator, MAP_V[nc]);
  let nk = acreate(map->allocator, MAP_K[nc]);
  let nf = acreate(map->allocator, ptrstype(itypeof(mapname, flags))[nc]);

  let ov = map->vals;
  let ok = map->keys;
  let of = map->flags;
  defer {
    adestroy(map->allocator, ov);
    adestroy(map->allocator, ok);
    adestroy(map->allocator, of);
  };

  map->capbit = ncb;
  map->vals = *nv;
  map->keys = *nk;
  map->flags = *nf;

  foreach (usize i, range(0, oc))
    if (isHXOCCUPIED(of[i])) {
      newcount++;
      hxint hx = HXHASHBITS(of[i]);
      let idx = hx % nc;

      while (isHXOCCUPIED((*nf)[idx])) {
        idx++;
        if (idx >= nc) idx = 0;
      }

      (*nf)[idx] = ((hxint)HXOCC << flagshift) | hx;
      (*nk)[idx] = ok[i];
      (*nv)[idx] = ov[i];
    }
  map->count = newcount;
}

static inline MAP_V *MAP_FN(get)(const mapname *m, MAP_K key) {
  let hx = HXHASHBITS(maphash(key));
  let mask = ((usize)1 << m->capbit) - 1;

  for (let idx = hx & mask; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        isHXOCCUPIED(m->flags[idx]) &&
        HXHASHBITS(m->flags[idx]) == hx &&
        !mapcmp(m->keys[idx], key)
    ) return m->vals + idx;
  }
  return nullptr;
}
static inline MAP_V *MAP_FN(set)(
    mapname *m,
    MAP_K key,
    MAP_V val
) {
  usize cap = (usize)1 << (m->capbit);
  if_unlikely ((m->count + 1) * 4 >= cap * 3) MAP_FN(manage)(m, 1);
  cap = (usize)1 << (m->capbit);

  let hx = HXHASHBITS(maphash(key));
  let mask = cap - 1;
  let idx = hx & mask;

  for (; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        HXHASHBITS(m->flags[idx]) == hx &&
        !mapcmp(m->keys[idx], key)
    ) break;
  }

  const let flag = m->flags[idx];

  m->count += isHXEMPTY(flag);
  m->flags[idx] = hx | (HXOCC << flagshift);
  m->keys[idx] = key;
  m->vals[idx] = val;
  return m->vals + idx;
}

static inline MAP_V *MAP_FN(rem)(
    mapname *m,
    MAP_K key
) {
  usize cap = (usize)1 << (m->capbit);

  let hx = HXHASHBITS(maphash(key));
  let mask = cap - 1;
  let idx = hx & mask;

  for (; !isHXEMPTY(m->flags[idx]); idx = (idx + 1) & mask) {
    if (
        HXHASHBITS(m->flags[idx]) == hx &&
        !mapcmp(m->keys[idx], key)
    ) break;
  }

  const let flag = m->flags[idx];
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
        m->keys[i] = m->keys[j];
        m->vals[i] = m->vals[j];
        m->flags[j] = HXEMPTY;
        i = j;
      }
    }
  }
  return nullptr;
}
static inline MAP_K *MAP_FN(val_key)(
    const mapname *map,
    const MAP_V *val
) {
  usize idx = val - map->vals;
  return map->keys + idx;
}
static inline void MAP_FN(clear)(
    const mapname *map
) {
  let count = 1 << map->capbit;
  memset(map->flags, 0, sizeof(typeof (*map->flags)[count]));
}

typedef struct MAP_FN(iter_state) {
  const mapname *map;
  usize current;
} MAP_FN(iter_state);

static inline MAP_FN(iter_state) MAP_FN(iter_init)(const mapname *map) {
  MAP_FN(iter_state)
  it = {.map = map, .current = 0};
  let cap = (usize)1 << it.map->capbit;
  while (it.current < cap && isHXEMPTY(it.map->flags[it.current]))
    it.current++;
  return it;
}

static inline int MAP_FN(iter_valid)(MAP_FN(iter_state) * it) {
  let cap = (usize)1 << it->map->capbit;
  return it->current < cap;
}

static inline void MAP_FN(iter_increase)(MAP_FN(iter_state) * it) {
  let cap = (usize)1 << it->map->capbit;
  it->current++;
  while (it->current < cap && isHXEMPTY(it->map->flags[it->current]))
    it->current++;
}

typedef struct MAP_FN(k_v) {
  MAP_K key;
  MAP_V *val;
} MAP_FN(k_v);
static inline MAP_FN(k_v) MAP_FN(iter_cast)(MAP_FN(iter_state) * it) {
  return (MAP_FN(k_v)){
      .key = it->map->keys[it->current],
      .val = it->map->vals + it->current
  };
}

typedef struct MAP_FN(iterator_t) {
  const typeof(&MAP_FN(iter_init)) init;
  const typeof(&MAP_FN(iter_valid)) valid;
  const typeof(&MAP_FN(iter_increase)) increase;
  const typeof(&MAP_FN(iter_cast)) cast;
} MAP_FN(iterator_t);
static const MAP_FN(iterator_t) MAP_FN(iterator) = {
    &MAP_FN(iter_init),
    &MAP_FN(iter_valid),
    &MAP_FN(iter_increase),
    &MAP_FN(iter_cast),
};

#undef mapconfig
#undef mapname
#undef mapiterator
#undef maphash
#undef mapcmp
#undef MAP_K
#undef MAP_V
#undef MAP_FN
#undef _MAP_CAT
#undef _MAP_CAT_
#undef _MAP_EVAL_K
#undef _MAP_EVAL_V
#undef _MAP_GET_K
#undef _MAP_GET_V
#undef hxint
#undef HXEMPTY
#undef HXOCC
#undef mxflag
#undef flagshift
#undef isHXOCCUPIED
#undef isHXEMPTY
#undef HXHASHBITS
