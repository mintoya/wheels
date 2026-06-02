#include "../allocators/arenaAllocator.h"
#include "../funct.h"
#include "../print.h"
#include <stdatomic.h>
#include <stddefer.h>

#include "../thread_help.h"

// 1. The Dependent Sub-Task
// This just simulates some work that a parent task needs done.
decfunction(inner_task, ((int, id)), int);
deffunction(inner_task, ((int, id)), int, {
  println("inner task thread : {}", thrd_current());
  println("  -> Inner task {} is executing!", id);
  thrd_sleep(&(struct timespec){1}, NULL);
  return id * 10;
});
typedef struct mtxtpoolmutex_recursive *poolptr;
decfunction(outer_task, ((AllocatorV, alloc), (poolptr, pool), (int, id)), int);
deffunction(outer_task, ((AllocatorV, alloc), (poolptr, pool), (int, id)), int, {
  println(
      "outer task thread : {}\n"
      "[Worker] Outer task {} started.",
      thrd_current(),
      id
  );

  var_ inner_f = poolfunction_call(pool, inner_task, (id));

  println("[Worker] Outer task {} is now awaiting its inner task...", id);

  var_ res = poolfunction_await(pool, inner_f);

  println("[Worker] Outer task {} completed with result: {}", id, res);
  return res;
});

#include "../allocators/debugallocator.h"

int main(void) {
  println("main thread : {}", thrd_current());
  var_ allocator = debugAllocator(.allocator = stdAlloc, .log = stdout);
  defer { debugAllocatorDeInit(allocator); };

  var_ pool = tpool_init(allocator);
  tpool_addWorkers(pool, 2);
  defer { tpool_deInit(pool); };

  var_ futures = msList_init(allocator, typeof(poolfunction_call(pool, outer_task, (allocator, pool, 0))));
  defer { msList_deInit(allocator, futures); };

  foreach (var_ i, range(0, 6)) {
    var_ v = poolfunction_call_type(pool, outer_task, typeof(*futures), (allocator, pool, i));
    msList_push(allocator, futures, v);
  }

  println("[Main] Awaiting all outer tasks to finish...");
  var_ total = 0;
  foreach (var_ f, vla(*msList_vla(futures))) {
    total += poolfunction_await(pool, f);
  }
  println("[Main] SUCCESS! All tasks finished. Total: {}", total);
  return 0;
}
#include "../wheels.h"
