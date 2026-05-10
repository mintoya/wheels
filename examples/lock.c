#include "../arenaAllocator.h"
#include "../print.h"
#include "../thrd_macros.h"
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

// 2. The Blocking Parent Task
// This task hogs a worker thread, spawns a sub-task, and synchronously waits for it.
poolfn(outer_task, ((AllocatorV, alloc), (mutex(tpool, mutex_recursive) *, pool), (int, id)), int, {
  println("outer task thread : {}", thrd_current());
  println("[Worker] Outer task {} started.", id);

  // Submit the dependent sub-task back into the SAME pool
  var_ inner_f = poolfn_call(pool, inner_task, (id));

  println("[Worker] Outer task {} is now awaiting its inner task...", id);

  var_ res = poolfn_await(pool, inner_f);

  println("[Worker] Outer task {} completed with result: {}", id, res);
  return res;
});

int main(void) {
  println("main thread : {}", thrd_current());
  var_ arena = arena_new_ext(stdAlloc, 1024);
  defer { arena_cleanup(arena); };

  var_ pool = tpool_init(arena);
  tpool_addWorkers(pool, 2);
  defer { tpool_deInit(pool); };

  var_ futures = msList_init(arena, outer_task_future);

  println("[Main] Submitting 5 outer tasks to the 2-worker pool...");

  foreach (var_ i, range(0, 5)) {
    var_ v = poolfn_call(pool, outer_task, (arena, pool, i));
    msList_push(arena, futures, v);
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
