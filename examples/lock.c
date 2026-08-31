#include "../allocators/arenaAllocator.h"
#include "../funct.h"
#include "../print.h"
#include <stdatomic.h>

#include "../thread_help.h"

deffunction(inner_task, ((int, id)), int) {
  println("inner task thread : {}", thrd_current());
  println("  -> Inner task {} is executing!", id);
  thrd_sleep(&(struct timespec){1}, NULL);
  return id * 10;
}
deffunction(outer_task, ((allocfn, alloc), (tpool_single_t, pool), (int, id)), int) {
  println(
      "outer task thread : {}\n"
      "[Worker] Outer task {} started.",
      thrd_current(),
      id
  );

  let inner_f = poolfunction_call(pool, inner_task, (id));

  println("[Worker] Outer task {} is now awaiting its inner task...", id);

  let res = poolfunction_await(pool, inner_f);

  println("[Worker] Outer task {} completed with result: {}", id, res);
  return res;
}

#include "../allocators/debugallocator.h"

int main(void) {
  println("main thread : {}", thrd_current());
  let allocator = debugAllocator(.allocator = stdAlloc);
  defer { debugAllocatorDeInit(allocator); };

  let pool = tpool_init(allocator);
  tpool_addWorkers(pool, 2);
  defer { tpool_deInit(pool); };

  let futures = msList_init(allocator, typeof(poolfunction_call(pool, outer_task, (allocator, pool, 0))));
  defer { msList_deInit(allocator, futures); };

  foreach (let i, range(0, 6)) {
    let v = poolfunction_call_type(pool, outer_task, typeof(*futures), (allocator, pool, i));
    msList_push(allocator, futures, v);
  }

  println("[Main] Awaiting all outer tasks to finish...");
  let total = 0;
  foreach (let f, vla(*msList_vla(futures))) {
    total += poolfunction_await(pool, f);
  }
  println("[Main]  tasks finished. Total: {}", total);
  return 0;
}
#include "../wheels.h"
