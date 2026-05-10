#include "../arenaAllocator.h"
#include "../print.h"
#include "../shmap.h"
#include "../thrd_macros.h"
#include "../tu_macros.h"
#include <__stddef_unreachable.h>
#include <stddefer.h>

tu_def(
    (DynData, u8),
    (Num, i64),
    (Text, cstr),
    (List_, msList(i64)),
);

// clang-format off
poolfn(process_variant, ((DynData, data)), i64, {
  var_ sum = 0ll;

  tu_match(
    data,
    of(Num, n, {
      sum = n * 10;
    }),
    of(Text, t, {
      sum = strlen(t);
    }),
    of(List_, l, {
      foreach (var_ item, vla(*msList_vla(l))) { sum += item; }
    }),
    otherwise( sum = -1; )
  );

  return sum;
});
// clang-format on
#include "../tsaAllocator.h"
int main(void) {
  var_ arena = arena_new_ext(stdAlloc, 4096);
  defer { arena_cleanup(arena); };

  var_ pool = mutex_initW(tpool, mutex_recursive, (tpool){});
  defer { mutex_deInit(pool); };

  TSA_State tss[1];
  TSA_init(arena_new_ext(stdAlloc, 1024), tss);
  defer { TSA_deinit(tss); };
  AllocatorV tsa = tss->allocator;

  mutex_critical (var_ p, mutex_lock, pool) {
    p->tasks.allocator = tsa;
  } else unreachable();

  var_ workers = msList_init(arena, tpool_worker_future);
  foreach (var_ i, range(0, 4))
    msList_push(arena, workers, thrdfn_call(arena, tpool_worker, (&pool)));

  var_ config = msHmap_init(arena, DynData);
  defer { msHmap_deinit(config); };

  var_ sub_list = msList_init(arena, i64);
  msList_pushArr(arena, sub_list, ((i64[]){10, 20, 30}));

  msHmap_set(config, "retries", (DynData)tu_of(Num, 5));
  msHmap_set(config, "host", (DynData)tu_of(Text, "localhost"));
  msHmap_set(config, "ports", (DynData)tu_of(List_, sub_list));

  var_ futures = msList_init(arena, process_variant_future);
  char *keys[] = {"retries", "host", "ports", "missing"};

  foreach (var_ key, vla(keys)) {
    var_ val = msHmap_GetOrSet(config, key, ((DynData)tu_of(Num, 0)));
    msList_push(arena, futures, poolfn_call(arena, (&pool), process_variant, (*val)));
  }

  var_ total = 0ll;
  foreach (var_ f, vla(*msList_vla(futures)))
    total += poolfn_await(arena, f);

  mutex_critical (var_ mt, mutex_lock, pool) {
    println("shutting down");
    mt->shutdown = true;
    cnd_broadcast(&mt->wake_cnd);
  } else unreachable();

  println("joining threads");
  foreach (var_ f, vla(*msList_vla(workers)))
    thrdfn_await(f);

  println("Async Map-Reduce Result: {}", (usize)total);
  return 0;
}
#include "../wheels.h"
