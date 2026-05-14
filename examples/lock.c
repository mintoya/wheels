#include "../arenaAllocator.h"
#include "../print.h"
#include "../thrd.h"
#include <stdatomic.h>
#include <stddefer.h>
#include <threads.h>


// 1. The Dependent Sub-Task
// This just simulates some work that a parent task needs done.
poolfn(inner_task, ((int, id)), int, {
  println("inner task thread : {}", thrd_current());
  println("  -> Inner task {} is executing!", id);
  thrd_sleep(&(struct timespec){1}, NULL);
  return id * 10;
});

poolfn(outer_task, ((AllocatorV, alloc), (mutex(tpool, mutex_recursive) *, pool), (int, id)), int, {
  println(
      "outer task thread : {}\n"
      "[Worker] Outer task {} started.",
      thrd_current(),
      id
  );

  var_ inner_f = poolfn_call(pool, inner_task, (id));

  println("[Worker] Outer task {} is now awaiting its inner task...", id);

  var_ res = poolfn_await(pool, inner_f);

  println("[Worker] Outer task {} completed with result: {}", id, res);
  return res;
});
#include "../debugallocator.h"
int main(void) {
  println("main thread : {}", thrd_current());
  var_ allocator = debugAllocator(.allocator = stdAlloc, .log = stdout);
  defer { debugAllocatorDeInit(allocator); };

  var_ pool = tpool_init(allocator);
  tpool_addWorkers(pool, 2);
  defer { tpool_deInit(pool); };

  var_ futures = msList_init(allocator, outer_task_future);
  defer { msList_deInit(allocator, futures); };

  foreach (var_ i, range(0, 6)) {
    var_ v = poolfn_call(pool, outer_task, (allocator, pool, i));
    msList_push(allocator, futures, v);
  }

  println("[Main] Awaiting all outer tasks to finish...");
  var_ total = 0;
  foreach (var_ f, vla(*msList_vla(futures))) {
    total += poolfn_await(pool, f);
  }
  println("[Main] SUCCESS! All tasks finished. Total: {}", total);
  return 0;
}
#include "../wheels.h"
