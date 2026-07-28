#if !defined SXMAP_H
  #define SXMAP_H (1)
  #include "allocators/arenaAllocator.h"
  #include "fptr.h"
  #include "hxmap.h"
  #include "macros.h"
  #include "mytypes.h"
  #include "oxmap.h"
typedef struct {
  union {
    hxmap hmap[1];
    oxmap omap[1];
  };
  AllocatorV stringArena;
} sxmap;

sxmap *smap_new(AllocatorV allocator, u32 vsize, usize cap, usize arenaSize);
void *smap_set(sxmap *map, fptr k, void *b);
void smap_free(sxmap *map);
void *smap_get(sxmap *map, fptr k);
  #define msxmap(V) ptrof(fnptrof((sxmap), V))
  #define msxmap_iType(map) typeof((*map)((sxmap){0}))
  #define msxmap_defaults(...) VA_SWITCH_REMP((8, 1024)__VA_OPT__(, (__VA_ARGS__)))
  #define msxmap_init(allocator, V, ...) (msxmap(V)) smap_new(allocator, sizeof(V), msxmap_defaults(__VA_ARGS__))
  #define msxmap_deinit(map) ((void)sizeof(msxmap_iType(map)), smap_free((sxmap *)map))

  #define msxmap_set(map, k, v) (ptrof(msxmap_iType(map)))(smap_set((sxmap *)map, fp(k), (void *)REF(msxmap_iType(map), v)))
  #define msxmap_rem(map, k) ((void)smap_set((sxmap *)map, fp(k), nullptr))
  #define msxmap_get(map, k) (ptrof(msxmap_iType(map))) smap_get((sxmap *)map, fp(k))
msxmap(int) j;
  // {sxmap(map)
  #define FOREACH_sxmap_cast(is)                                                        \
    ((struct {fptr key;void *val; }){                                                                      \
        .key = *(fptr *)(((hxmap *)is._m)->keys + (is._idx * ((hxmap *)is._m)->ksize)), \
        .val = ((hxmap *)is._m)->vals + (is._idx * ((hxmap *)is._m)->vsize),            \
    })

  #define FOREACH_sxmap_iter    \
    (                           \
        FOREACH_hxmap_init,     \
        FOREACH_hxmap_increase, \
        FOREACH_hxmap_valid,    \
        FOREACH_sxmap_cast)
  //}
  // {msxmap(map)
  #define FOREACH_msxmap_cast(is)                                                                          \
    ((struct {fptr key; ptrof(msxmap_iType(is._m)) val; }){                                                                                         \
        .key = *(fptr *)(((hxmap *)is._m)->keys + (is._idx * ((hxmap *)is._m)->ksize)),                    \
        .val = (ptrof(msxmap_iType(is._m)))(((hxmap *)is._m)->vals + (is._idx * ((hxmap *)is._m)->vsize)), \
    })

  #define FOREACH_msxmap_iter   \
    (                           \
        FOREACH_hxmap_init,     \
        FOREACH_hxmap_increase, \
        FOREACH_hxmap_valid,    \
        FOREACH_msxmap_cast)
//}
  #include "tests.h"
test_fn(smap_test) {
  var_ map = msxmap_init(allocator, int);
  defer { msxmap_deinit(map); };
  char buffer[sizeof("integer ") + 10];
  foreach (int i, range(0, 50)) {
    var_ str = ((fptr){(usize)snprintf(buffer, sizeof(buffer), "integer %i", i), (u8 *)buffer});
    msxmap_set(map, str, i);
  }
  foreach (int i, range(0, 50)) {
    var_ str = ((fptr){(usize)snprintf(buffer, sizeof(buffer), "integer %i", i), (u8 *)buffer});
    var_ m = msxmap_get(map, str);
    test_assert(*m == i);
    if (i % 2) msxmap_rem(map, str);
  }
  foreach (int i, range(0, 50)) {
    var_ str = ((fptr){(usize)snprintf(buffer, sizeof(buffer), "integer %i", i), (u8 *)buffer});
    if (!(i % 2)) test_assert(*msxmap_get(map, str) == i);
    else test_assert(!msxmap_get(map, str));
  }
}

u64 hashfptr(const void *a);
i8 cmpfptr(const void *a, const void *b);
#endif

#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define SXMAP_C (1)
#endif

#if defined SXMAP_C
u64 hashfptr(const void *a) { return fptr_hash(*(fptr *)a); }
i8 cmpfptr(const void *a, const void *b) { return fptr_cmp(*(fptr *)a, *(fptr *)b); }
sxmap *smap_new(AllocatorV allocator, u32 vsize, usize cap, usize arenaSize) {
  var_ res = aCreate(allocator, sxmap);
  hxmap_newm(allocator, sizeof(fptr), vsize, cap, hashfptr, cmpfptr, res->hmap);
  // oxmap_newm(allocator, sizeof(fptr), vsize, cmpfptr, res->omap);
  res->stringArena = arena_new_ext(allocator, arenaSize);
  return res;
}
void smap_free(sxmap *map) {
  var_ allocator = map->hmap->allocator;
  arena_cleanup(map->stringArena);
  hxmap_freem(map->hmap[0]);
  aDestroy(allocator, map);
}
void *smap_set(sxmap *map, fptr k, void *b) {
  if (!k.len) return nullptr;
  if (!b) return hxmap_set(map->hmap, &k, nullptr);
  var_ copy = P$(
      hxmap_get(map->hmap, &k),
      ({
        $
            ? *(fptr *)hxmap_val_key(map->hmap, $)
            : (fptr){k.len, (u8 *)memcpy(aCreate(map->stringArena, u8, k.len), k.ptr, k.len)};
      })
  );
  return hxmap_set(map->hmap, &copy, b);
}
void *smap_get(sxmap *map, fptr k) { return hxmap_get(map->hmap, &k); }
#endif
