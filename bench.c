#include "hxmap.h"
#include "macros.h"
#include "ts_int.h"
#include <stdio.h>
#define ihash(k) ((k) * 31 ^ 0x1000)
#define icmp(a, b) !(a == b)
u64 hint(const void *i) { return ihash(*(int *)i); }
i8 cint(const void *a, const void *b) { return icmp(*(int *)a, *(int *)b); }

void perfectset(usize count) {
  let map = &aCreate(
      stdAlloc,
      typeof(struct pmap {
        bool occupied : 1;
        u64 value : 63;
      }),
      count
  );
  defer { aDestroy(stdAlloc, *map); };
  foreach (let i, range(0, count)) {
    (*map)[i].occupied = 1;
    (*map)[i].value = i * i;
  }
  foreach (let i, range(0, count)) {
    assert((*map)[i].occupied && (*map)[i].value == i * i);
  }
}
void perfectreset(usize count) {
  let map = &aCreate(
      stdAlloc,
      typeof(struct pmap {
        bool occupied : 1;
        u64 value : 63;
      }),
      count
  );
  defer { aDestroy(stdAlloc, *map); };
  foreach (let i, range(0, count)) {
    (*map)[i].occupied = 1;
    (*map)[i].value = i * i;
  }
  foreach (let i, range(0, count)) {
    assert((*map)[i].occupied && (*map)[i].value == i * i);
  }
  foreach (let i, range(0, count))
    if (i & 1)
      (*map)[i].occupied = 0;
  foreach (let i, range(0, count)) {
    if (i & 1) assert(!(*map)[i].occupied);
    else assert((*map)[i].occupied && (*map)[i].value == i * i);
  }
}
void hxset(usize count) {
  usize bits = 0;
  while (1 << bits < count)
    bits++;
  let map = mxmap_init(stdAlloc, int, int, ++bits, hint, cint);
  defer { mxmap_deinit(map); };
  foreach (let i, range(0, count))
    mxmap_set(map, i, i * i);
  foreach (let i, range(0, count)) {
    let x = mxmap_get(map, i);
    assert(x && *x == i * i);
  }
}
void hxsetreset(usize count) {
  usize bits = 0;
  while (1 << bits < count)
    bits++;
  let map = mxmap_init(stdAlloc, int, int, ++bits, hint, cint);
  defer { mxmap_deinit(map); };
  foreach (let i, range(0, count))
    mxmap_set(map, i, i * i);
  foreach (let i, range(0, count)) {
    let x = mxmap_get(map, i);
    assert(x && *x == i * i);
  }
  foreach (let i, range(0, count))
    if (i & 1)
      mxmap_rem(map, i);
  foreach (let i, range(0, count)) {
    let x = mxmap_get(map, i);
    if (i & 1) assert(!x);
    else assert(x && *x == i * i);
  }
}
#define mapconfig iimap, int, int, (ihash(k)), (icmp(a, b))
#include "incmap.h"
void directset(usize count) {
  usize bits = 0;
  while (1 << bits < count)
    bits++;
  let map = iimap_new(stdAlloc, ++bits);
  defer { iimap_free(map); };
  foreach (let i, range(0, count))
    iimap_set(map, i, i * i);
  foreach (let i, range(0, count)) {
    let x = iimap_get(map, i);
    assert(x && *x == i * i);
  }
}
void directreset(usize count) {
  usize bits = 0;
  while (1 << bits < count)
    bits++;
  let map = iimap_new(stdAlloc, ++bits);
  defer { iimap_free(map); };
  foreach (let i, range(0, count))
    iimap_set(map, i, i * i);
  foreach (let i, range(0, count)) {
    let x = iimap_get(map, i);
    assert(x && *x == i * i);
  }
  foreach (let i, range(0, count))
    if (i & 1)
      iimap_rem(map, i);
  foreach (let i, range(0, count)) {
    let x = iimap_get(map, i);
    if (i & 1) assert(!x);
    else assert(x && *x == i * i);
  }
}

ts_int bench(fnptrof((usize), void) fn, usize n) {
  foreach (let _, range(0, 100))
    fn(100);

  let tim = now();
  foreach (let _, range(0, 1000))
    fn(n);
  return (now() - tim) / 1000;
}
void testwith(usize c) {
  println("count : {}", c);
  println("perfect setting took  \t{ts_int}", bench(perfectset, c));
  println("perfect resetting took\t{ts_int}", bench(perfectreset, c));
  println();
  println("setting took          \t{ts_int}", bench(hxset, c));
  println("resetting took        \t{ts_int}", bench(hxsetreset, c));
  println("inc setting took      \t{ts_int}", bench(directset, c));
  println("inc resetting took    \t{ts_int}", bench(directreset, c));
  println("========================================================");
}
int main(void) {
  testwith(20000);
  testwith(1000);
  testwith(100);
}
#include "wheels.h"
