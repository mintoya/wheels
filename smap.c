#include "allocators/arenaAllocator.h"
#include "allocators/debugallocator.h"
#include "fptr.h"
#include "hxmap.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include <string.h>
typedef struct {
  hxmap map[1];
  AllocatorV stringArena;
} sxmap;

u64 hashfptr(const void *a) { return fptr_hash(*(fptr *)a); }
i8 cmpfptr(const void *a, const void *b) { return fptr_cmp(*(fptr *)a, *(fptr *)b); }
sxmap *smap_new(AllocatorV allocator, u32 vsize, usize cap, usize arenaSize) {
  var_ res = aCreate(allocator, sxmap);
  var_ m =
      hxmap_new(
          allocator,
          sizeof(fptr),
          vsize,
          cap,
          hashfptr,
          cmpfptr
      );
  defer { aFree(allocator, m, sizeof(*m)); };
  memcpy(
      res->map, m, sizeof(hxmap)
  );
  res->stringArena = arena_new_ext(allocator, arenaSize);
  return res;
}
void smap_free(sxmap *map) {
  var_ allocator = map->map->allocator;
  arena_cleanup(map->stringArena);
  aFree(allocator, map->map->flags, sizeof(*map->map->flags) * map->map->cap);
  aFree(allocator, map->map->keys, map->map->ksize * map->map->cap);
  aFree(allocator, map->map->vals, map->map->vsize * map->map->cap);
  aFree(allocator, map, sizeof(*map));
}
void *smap_set(sxmap *map, fptr k, void *b) {
  if (!k.len) return nullptr;
  if (!b) return hxmap_set(map->map, &k, nullptr);
  var_ copy = P$(
      hxmap_get(map->map, &k),
      ({
        $
            ? *(fptr *)hxmap_val_key(map->map, $)
            : (fptr){k.len, memcpy(aCreate(map->stringArena, u8, k.len), k.ptr, k.len)};
      })
  );
  return hxmap_set(map->map, &copy, b);
}
void *smap_get(sxmap *map, fptr k) { return hxmap_get(map->map, &k); }
// {sxmap(map)
#define FOREACH_sxmap_cast(is)                                                        \
  ((struct {fptr key;void *val }){                                                                      \
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

test_fn(smap_tests) {
  var_ map = smap_new(allocator, sizeof(int), 8, 1024);
  defer { smap_free(map); };
  foreach (int i, range(0, 50)) {
    var_ str = snprint(allocator, "integer {}", i);
    defer { slice_free(allocator, str); };
    smap_set(map, bitcast(fptr, str), &i);
  }
  foreach (int i, range(0, 50)) {
    var_ str = snprint(allocator, "integer {}", i);
    defer { slice_free(allocator, str); };
    assert(*(int *)smap_get(map, bitcast(fptr, str)) == i);
    if (i % 2) smap_set(map, bitcast(fptr, str), nullptr);
  }
  foreach (int i, range(0, 50)) {
    var_ str = snprint(allocator, "integer {}", i);
    defer { slice_free(allocator, str); };
    if (!(i % 2))
      test_assert(!!!!*(int *)smap_get(map, bitcast(fptr, str)) == i);
    else
      test_assert(!!!!smap_get(map, bitcast(fptr, str)));
  }

  foreach (var_ item, sxmap_iter(map)) {
    var_ k = item.key;
    var_ v = *(int *)item.val;
    println("{slice(c8)} -> {}", k, v);
  }
  test_pass();
}
#include "wheels.h"
