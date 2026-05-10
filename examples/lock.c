#include "../arenaAllocator.h"
#include "../print.h"
#include "../thrd_macros.h"
#include <stddefer.h>
#include <threads.h>

// 1. The Dependent Sub-Task
// This just simulates some work that a parent task needs done.
poolfn(inner_task, ((int, id)), int, {
  println("inner task thread : {}", thrd_current());
  println("  -> Inner task {} is executing!", id);
  thrd_sleep(&(struct timespec){1}, NULL); // Sleep 0.5s
  return id * 10;
});

// 2. The Blocking Parent Task
// This task hogs a worker thread, spawns a sub-task, and synchronously waits for it.
poolfn(outer_task, ((AllocatorV, alloc), (mutex(tpool, mutex_recursive) *, pool), (int, id)), int, {
  println("outer task thread : {}", thrd_current());
  println("[Worker] Outer task {} started.", id);

  // Submit the dependent sub-task back into the SAME pool
  var_ inner_f = poolfn_call(alloc, pool, inner_task, (id));

  println("[Worker] Outer task {} is now awaiting its inner task...", id);

  // THE TRAP: We block this worker waiting for inner_f to finish.
  var_ res = poolfn_await(alloc, pool, inner_f); // (Or `poolfn_await(pool, inner_f)` if you applied the work-stealing fix)

  println("[Worker] Outer task {} completed with result: {}", id, res);
  return res;
});

int main(void) {
  println("main thread : {}", thrd_current());
  var_ arena = arena_new_ext(stdAlloc, 1024); // 1MB to avoid OOMs
  defer { arena_cleanup(arena); };

  // Initialize a pool with exactly 5 workers
  var_ pool = tpool_init(arena); // (Assuming you applied the heap-allocation fix)
  tpool_addWorkers(pool, 5);
  defer { tpool_deInit(pool); };

  var_ futures = msList_init(arena, outer_task_future);

  println("[Main] Submitting 5 outer tasks to the 2-worker pool...");

  // Submit EXACTLY as many blocking tasks as there are workers (5)
  foreach (var_ i, range(0, 5)) {
    var_ v = poolfn_call(arena, pool, outer_task, (arena, pool, i));
    msList_push(arena, futures, v);
  }

  println("[Main] Awaiting all outer tasks to finish...");
  var_ total = 0;
  foreach (var_ f, vla(*msList_vla(futures))) {
    total += poolfn_await(arena, pool, f);
  }

  println("[Main] SUCCESS! All tasks finished. Total: {}", total);
  return 0;
}
#include "../wheels.h"
