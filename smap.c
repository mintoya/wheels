#include "allocator.h"
#include "allocators/arenaAllocator.h"
#include "allocators/debugallocator.h"
#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include "sList.h"
#include "shmap.h"
#include "time.h"
#include "wheels.h"

// layout
// length | u8(la) |(padding)| u8(vallen)
// must garuntee val starts a a max aligned address
typedef struct {
  usize la : sizeof(usize) * 8 - 1;
  usize sentinel : 1;
  u8 buffer[];
} *mapitem;
typedef struct {
  usize count;
  usize vallen;
  usize mhash;
  msList(mapitem) array;
  AllocatorV allocator;
} stringmap;

stringmap *stringmap_new(AllocatorV allocator, usize vlen, usize mhash) {
  var_ arena = arena_new_ext(allocator, 128);
  var_ list = msList_init(arena, mapitem, mhash);
  msList_pushVla(arena, list, VLAP((mapitem *)nullptr, mhash));
  return aValue(
      arena,
      ((stringmap){
          .vallen = vlen,
          .mhash = mhash,
          .array = list,
          .allocator = arena,
      })
  );
}
void stringmap_free(stringmap *map) { arena_cleanup(map->allocator); }

void *stringmap_set(stringmap *map, fptr key, void *value) {
  assertMessage(key.ptr && key.len);
  mapitem ptr = nullptr;
  void *place = nullptr;
  usize size =
      sizeof(ptrstype(mapitem)) +
      alignof(myAlign) +
      key.len +
      map->vallen;

  if (value) {
    map->count++;
    ptr = aAlloc(map->allocator, size);
    ptr->la = key.len;
    ptr->sentinel = 0;
    place = (u8 *)lineup((uptr)(ptr->buffer) + key.len, alignof(myAlign));
    memcpy(place, value, map->vallen);
    memcpy(ptr->buffer, key.ptr, key.len);
  }

  var_ idx = fptr_hash(key) % map->mhash;
  var_ first_tombstone = (usize)-1;

  for (; idx < msList_len(map->array); idx++) {
    if (!map->array[idx]) break;

    if (map->array[idx]->sentinel) {
      if (first_tombstone == (usize)-1) first_tombstone = idx;
      continue;
    }

    fptr k = (fptr){map->array[idx]->la, map->array[idx]->buffer};
    if (fptr_isNull(k)) break;
    if (fptr_eq(k, key)) break;
  }

  if (!value) {
    if (idx < msList_len(map->array) && map->array[idx]) {
      map->array[idx]->sentinel = 1;
      map->count--;
    }
    return nullptr;
  }

  if (first_tombstone != (usize)-1 && (idx == msList_len(map->array) || !map->array[idx]))
    idx = first_tombstone;

  if (idx == msList_len(map->array))
    msList_push(map->allocator, map->array, nullptr);

  if (idx < msList_len(map->array))
    if (map->array[idx]) (map->count--, aFree(map->allocator, map->array[idx], size));

  map->array[idx] = ptr;
  return place;
}
void *stringmap_get(stringmap *map, fptr key) {
  usize size =
      sizeof(ptrstype(mapitem)) +
      alignof(myAlign) +
      key.len +
      map->vallen;

  var_ idx = fptr_hash(key) % map->mhash;
  for (; idx < msList_len(map->array); idx++) {
    if (!map->array[idx]) return nullptr;
    if (map->array[idx]->sentinel) continue;

    fptr k = (fptr){map->array[idx]->la, map->array[idx]->buffer};
    if (fptr_isNull(k)) break;
    if (fptr_eq(k, key)) break;
  }

  if (idx < msList_len(map->array) && map->array[idx] && !map->array[idx]->sentinel)
    return (u8 *)lineup((uptr)(map->array[idx]->buffer) + key.len, alignof(myAlign));
  return nullptr;
}

void *stringmap_setCs(stringmap *map, char *key, void *value) {
  return stringmap_set(map, fp(key), value);
}
void *stringmap_getCs(stringmap *map, char *key) { return stringmap_get(map, fp(key)); }
static inline void stringmap_cleanup_handler(void *vv) {
  stringmap **v = (stringmap **)vv;
  if (v && *v) {
    stringmap_free(*v);
    *v = nullptr;
  }
}

#define mSmap(Tval) typeof(typeof(Tval(**)(stringmap *, fptr)))

#define mSmap_scoped(Tval) [[gnu::cleanup(stringmap_cleanup_handler)]] mSmap(Tval)

#define mSmap_init(allocator, valtype, mhash) \
  ((mSmap(valtype))stringmap_new(allocator, sizeof(valtype), mhash))

#define mSmap_deinit(map) stringmap_free((stringmap *)map)

#define mSmap_set(map, key, val)                                                                                              \
  do {                                                                                                                        \
    typeof((*map)(nullptr, (fptr){})) _v = (val);                                                                             \
    _Generic((key), char *: stringmap_setCs, const char *: stringmap_setCs, fptr: stringmap_set)((stringmap *)map, key, &_v); \
  } while (0)

#define mSmap_get(map, key) \
  ((typeof((*map)(nullptr, (fptr){})) *)_Generic((key), char *: stringmap_getCs, const char *: stringmap_getCs, fptr: stringmap_get)((stringmap *)map, key))

#define mSmap_rem(map, key) \
  _Generic((key), char *: stringmap_setCs, const char *: stringmap_setCs, fptr: stringmap_set)((stringmap *)map, key, nullptr)

#define ITERS 50000

int main(void) {
  println("=== mSmap (Arena Allocation) ===");
  {
    var_ local = debugAllocator(.allocator = stdAlloc);
    defer {
      debugAllocatorDeInit(local);
    };

    mSmap_scoped(int) m1 = mSmap_init(local, int, ITERS);

    clock_t start = clock();

    for (int i = 0; i < ITERS; i++)
      mSmap_set(m1, ((fptr){sizeof(i), (u8 *)&i}), i);

    for (int i = 0; i < ITERS; i++) {
      var_ val = mSmap_get(m1, ((fptr){sizeof(i), (u8 *)&i}));
      if (!val) println("mSmap missing key: {}", i);
    }

    clock_t end = clock();
    println("mSmap Time : {f128} ms", (f128)(end - start) / CLOCKS_PER_SEC * 1000.0);
    println("{dbga-stats}", debugAllocator_stats(local));
  }

  println("\n=== msHmap (Packed Strings) ===");
  {
    var_ local = debugAllocator(.allocator = stdAlloc);
    defer {
      debugAllocatorDeInit(local);
    };
    // var_ local_arena = arena_new_ext(local, 1024);
    // defer {arena_cleanup(local_arena);};
    // msHmap(int) m2 = msHmap_init(local_arena, int, ITERS);
    msHmap(int) m2 = msHmap_init(local, int, ITERS);
    defer { msHmap_deinit(m2); };

    clock_t start = clock();

    for (int i = 0; i < ITERS; i++)
      msHmap_set(m2, ((fptr){sizeof(i), (u8 *)&i}), i);

    for (int i = 0; i < ITERS; i++) {
      var_ val = msHmap_get(m2, ((fptr){sizeof(i), (u8 *)&i}));
      if (!val) println("mSmap missing key: {}", i);
    }

    clock_t end = clock();
    println("msHmap Time : {f128} ms", (f128)(end - start) / CLOCKS_PER_SEC * 1000.0);
    println("{dbga-stats}", debugAllocator_stats(local));
  }

  return 0;
}
