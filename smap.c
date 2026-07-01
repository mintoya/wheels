#include "allocators/arenaAllocator.h"
#include "allocators/debugallocator.h"
#include "fptr.h"
#include "hxmap.h"
#include "macros.h"
#include "print.h"
#include <time.h>
typedef struct {
  hxmap map[1];
} smap;
i8 fptr_cmpfn(const void *a, const void *b) {
  return fptr_cmp(*(const fptr *)a, *(const fptr *)b);
}
u64 fptr_hashfn(const void *f) { return fptr_hash(*(fptr *)f); }

static inline fptr smap_alloc_key(AllocatorV arena, const fptr key) {
  var_ new_ptr = (u8 *)aAlloc(arena, key.len);
  if (new_ptr)
    memcpy(new_ptr, key.ptr, key.len);
  return (fptr){key.len, new_ptr};
}

static inline fptr smap_pass_key(fptr key) { return key; }

// Natively typed, preserving the fptr signature
#define smap(V) mxMap(fptr, V)

// Create the arena and immediately use it to initialize the mxMap
#define smap_init(alloc, V, ...) ({                                                \
  var_ _arena = arena_new_ext(alloc, 4096);                                        \
  mxMap_init(_arena, fptr, V, VA_SWITCH(8, __VA_ARGS__), fptr_hashfn, fptr_cmpfn); \
})

// Pull the arena right back out of the map for string allocation
#define smap_set(map, key, val) ({ \
  mxMap_set(map, key, val);        \
})

#define smap_get(map, key) ({ \
  mxMap_get(map, key);        \
})

#define smap_rem(map, key) ({                                                                                    \
  var_ _k = _Generic((key), fptr: smap_pass_key, char *: smap_pass_key_cs, const char *: smap_pass_key_cs)(key); \
  mxMap_rem(map, _k);                                                                                            \
})

#define smap_deinit(map)                     \
  do {                                       \
    var_ _arena = ((hxmap *)map)->allocator; \
    mxMap_deinit(map);                       \
    arena_cleanup(_arena);                   \
  } while (0)

#define ITERS 10 * 1000
int main(void) {

  char *strs[ITERS];
  for (var_ ptr = (char **)&strs[0]; ptr < (char **)&strs[ITERS]; ptr++)
    *ptr = (char *)snprint(stdAlloc, "integer {}", (usize)(ptr - (char **)strs)).ptr;
  var_ allocator = debugAllocator(.allocator = stdAlloc);
  var_ c = clock();
  {
    defer {
      var_ diff = clock() - c;
      println("shmap time : {u32}", diff);
      println("{dbga-stats}", debugAllocator_stats(allocator));
      debugAllocatorDeInit(allocator);
      allocator = debugAllocator(.allocator = stdAlloc);
    };
    msHmap(int) imap = msHmap_init(allocator, int, 16);
    defer { msHmap_deinit(imap); };
    foreach (var_ i, range(0, ITERS))
      msHmap_set(imap, fp(strs[i]), i);
    foreach (var_ i, range(0, ITERS))
      assert(*msHmap_get(imap, fp(strs[i])) == i);
  }
  c = clock();
  {
    defer {
      var_ diff = clock() - c;
      println("sxmap time : {u32}", diff);
      println("{dbga-stats}", debugAllocator_stats(allocator));
      debugAllocatorDeInit(allocator);
      allocator = debugAllocator(.allocator = stdAlloc);
    };
    smap(int) imap = smap_init(allocator, int, 16);
    defer { smap_deinit(imap); };
    foreach (var_ i, range(0, ITERS))
      assert(*smap_set(imap, fp(strs[i]), i) == i);
    foreach (var_ i, range(0, ITERS))
      assert(*smap_get(imap, fp(strs[i])) == i);
  }
}
#include "wheels.h"
