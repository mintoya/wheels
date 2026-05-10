#include "sList.h"
#if !defined(MY_THREAD_MACORS_H)
  #define MY_THREAD_MACORS_H (1)
  #include "allocator.h"
  #include "macros.h"
  #include <stdatomic.h>
  #include <threads.h>
  #include <time.h>

  #define remove_paren(...) __VA_ARGS__

//
// mutex
//

// clang-format off

#define mutex_plain mutex_plain
#define mutex_timed mutex_timed
#define mutex_recursive mutex_recursive
#define mutex_recursive_timed mutex_recursive_timed

typedef struct {int mutex_plain;}mutex_plain;
typedef struct {int mutex_timed;}mutex_timed;
typedef struct {int mutex_recursive;}mutex_recursive;
typedef struct {int mutex_recursive_timed;}mutex_recursive_timed;
#define mutex_toEnum(name) _Generic(                 \
    (typeof(name)){},                                \
    mutex_plain: mtx_plain,                          \
    mutex_timed: mtx_timed,                          \
    mutex_recursive: mtx_recursive,                  \
    mutex_recursive_timed: mtx_recursive | mtx_timed \
)
// clang-format on

  #define mutex(T, mtx_T)         \
    /*valid mtx_T values:     */  \
    /*  mutex_plain           */  \
    /*  mutex_recursive       */  \
    /*  mutex_timed           */  \
    /*  mutex_recursive_timed */  \
    typeof(struct mtx##T##mtx_T { \
      mtx_t mtx[1];               \
      mtx_T kind[0];              \
      T data;                     \
    })

  #define mutex_init(mutex) \
    (mtx_init(mutex.mtx, mutex_toEnum((mutex).kind[0])))
  #define mutex_initW(T, k, init_val) ({ \
    mutex(T, k) res;                     \
    res.data = init_val;                 \
    mutex_init(res);                     \
    res;                                 \
  })
  #define mutex_deInit(mutex) mtx_destroy((mutex).mtx)

  #define mutex_timedlock_(mutex, time) (      \
      (void)sizeof((mutex).kind->mutex_timed), \
      mtx_timedlock((mutex).mtx, time)         \
  )
  #define mutex_trylock_(mutex) (mtx_trylock((mutex).mtx))
  #define mutex_lock_(mutex) (mtx_lock((mutex).mtx))

  #define mutex_timedlock mutex_timedlock_
  #define mutex_trylock mutex_trylock_
  #define mutex_lock mutex_lock_

  #define mutex_unlock(mutex) (mtx_unlock((mutex).mtx))

  #define mutex_critical(decl, lockfn, mutex, ...)                      \
    for (struct { int err; int keep; } _mc = {lockfn(mutex __VA_OPT__(, __VA_ARGS__)), 1}; \
         _mc.keep || (_mc.err ? 0 : (mutex_unlock(mutex), 0));          \
         _mc.keep = 0)                                                  \
      if (!_mc.err)                                                     \
        for (decl = &mutex.data; _mc.keep; _mc.keep = 0)

  // helper to make functions compatible with thrd_create
  #define thrdfn_structItem(type_name) TUPLE_EXPAND_A(type_name) TUPLE_EXPAND_B(type_name);
  #define thrdfn_structItems(...) APPLY_N(thrdfn_structItem, __VA_ARGS__)

  #define thrdfn_argItem(type_name) TUPLE_EXPAND_A(type_name) TUPLE_EXPAND_B(type_name)
  #define thrdfn_argItems(...) APPLY_N_C(thrdfn_argItem, __VA_ARGS__)

  #define thrdfn_argItemT(type_name) argstruct->args.TUPLE_EXPAND_B(type_name)
  #define thrdfn_argItemsT(...) APPLY_N_C(thrdfn_argItemT, __VA_ARGS__)

  #define thrdfn(name, in, out, ...)                               \
    typedef struct name##_future_struct {                          \
      thrd_t thread_handle[1];                                     \
      AllocatorV allocator;                                        \
      int thread_status[1];                                        \
      struct {                                                     \
        thrdfn_structItems in                                      \
      } args;                                                      \
      out result;                                                  \
    } *name##_future;                                              \
    out name##_real(thrdfn_argItems in) { __VA_ARGS__ }            \
    int name(void *_thrd_item_ptr) {                               \
      name##_future argstruct = (typeof(argstruct))_thrd_item_ptr; \
      argstruct->result = name##_real(thrdfn_argItemsT in);        \
      return 1;                                                    \
    }

  #define thrdfn_call(alloc, fname, argss) ({        \
    fname##_future f = aCreate(alloc, typeof(*f));   \
    _Static_assert(                                  \
        types_eq(                                    \
            fnptr_(                                  \
                (APPLY_N(                            \
                    typeof, remove_paren argss       \
                )),                                  \
                typeof(f->result)                    \
            ),                                       \
            typeof(&fname##_real)                    \
        ),                                           \
        "thrdfn not called with enough args"         \
    );                                               \
    f->args = (typeof(f->args)){remove_paren argss}; \
    f->allocator = alloc;                            \
    thrd_create(f->thread_handle, fname, f);         \
    f;                                               \
  })

  #define thrdfn_await(f) ({                              \
    typeof(f) f_r = f;                                    \
    defer { aFree(f_r->allocator, f_r, sizeof(*f_r)); };  \
    thrd_join(f_r->thread_handle[0], f_r->thread_status); \
                                                          \
    struct {                                              \
      typeof(f_r->result) result;                         \
      int status;                                         \
    } res = (typeof(res)){                                \
        f_r->result,                                      \
        f_r->thread_status[0]                             \
    };                                                    \
    res;                                                  \
  })

  #include "mylist.h"
  #include "print.h"

typedef struct tpoolNode_t {
  struct {
    void *arg;
    void (*fn)(void *);
  } task;
  struct tpoolNode_t *next;
} tpoolNode_t;
typedef struct tpoolList {
  AllocatorV allocator;
  struct tpoolNode_t *first, *last;
} tpoolList;

typedef struct {
  tpoolList tasks;
  cnd_t wake_cnd;
  bool shutdown;
  mList(struct tpool_worker_future_struct *) workers;
  // i will store the allocator in there
} tpool;

thrdfn(tpool_worker, ((mutex(tpool, mutex_recursive) *, pool)), int, {
  tpoolNode_t *task = NULL;
  AllocatorV allocator = NULL;
  while (1) {
    bool time_to_exit = false;
    mutex_critical (var_ poolData, mutex_lock, (*pool)) {
      allocator = allocator ?: poolData->tasks.allocator;

      while (!poolData->tasks.first && !poolData->shutdown)
        cnd_wait(&poolData->wake_cnd, pool->mtx);

      if (poolData->shutdown && !poolData->tasks.first) {
        time_to_exit = true;
      } else {
        task = poolData->tasks.first;
        poolData->tasks.first = task->next;

        if (!poolData->tasks.first) {
          poolData->tasks.last = NULL;
        }
      }
    } else thrd_exit(5);

    if (time_to_exit) break;

    var_ fn = task->task;
    if (fn.fn) fn.fn(fn.arg);

    if (!allocator) thrd_exit(5);
    aFree(allocator, task, sizeof(*task));
  }
  return 1;
});

  #define poolfn(name, in, out, ...)                               \
    typedef struct {                                               \
      struct {                                                     \
        thrdfn_structItems in                                      \
      } args;                                                      \
      out result;                                                  \
      _Atomic(bool) done;                                          \
    } *name##_future;                                              \
    out name##_real(thrdfn_argItems in) { __VA_ARGS__ }            \
    void name(void *_thrd_item_ptr) {                              \
      name##_future argstruct = (typeof(argstruct))_thrd_item_ptr; \
      argstruct->result = name##_real(thrdfn_argItemsT in);        \
      argstruct->done = true;                                      \
    }

  #define poolfn_call(alloc, pool, fname, argss) ({                         \
    fname##_future f = aCreate(alloc, typeof(*f));                          \
    _Static_assert(                                                         \
        types_eq(                                                           \
            fnptr_(                                                         \
                (APPLY_N(                                                   \
                    typeof, remove_paren argss                              \
                )),                                                         \
                typeof(f->result)                                           \
            ),                                                              \
            typeof(&fname##_real)                                           \
        ),                                                                  \
        "poolfn not called with enough args"                                \
    );                                                                      \
    f->args = (typeof(f->args)){remove_paren argss};                        \
    f->done = false;                                                        \
                                                                            \
    tpoolNode_t *node = aCreate(pool->data.tasks.allocator, typeof(*node)); \
    node->task.fn = (void (*)(void *))fname;                                \
    node->task.arg = f;                                                     \
    node->next = NULL;                                                      \
                                                                            \
    mutex_critical (var_ poolData, mutex_lock, (*pool)) {                   \
      if (poolData->tasks.last) {                                           \
        poolData->tasks.last->next = node;                                  \
      } else {                                                              \
        poolData->tasks.first = node;                                       \
      }                                                                     \
      poolData->tasks.last = node;                                          \
      cnd_signal(&poolData->wake_cnd);                                      \
    } else unreachable();                                                   \
                                                                            \
    f;                                                                      \
  })
  #define poolfn_await(allocator, f) ({    \
    typeof(f) f_r = f;                     \
    while (!f_r->done) {                   \
      thrd_yield();                        \
    }                                      \
    typeof(f_r->result) res = f_r->result; \
    aFree(allocator, f_r, sizeof(*f_r));   \
    res;                                   \
  })

static mutex(tpool, mutex_recursive) * tpool_init(AllocatorV alloc) {
  typedef typeof(struct tpool_worker_future_struct *) worker_t;

  var_ pool = aCreate(alloc, mutex(tpool, mutex_recursive));
  pool->data = (tpool){
      .tasks.allocator = alloc,
      .shutdown = false,
      .workers = mList_init(alloc, worker_t),
  };
  cnd_init(&pool->data.wake_cnd);
  mutex_init((*pool));
  return pool;
}

static void tpool_deInit(mutex(tpool, mutex_recursive) * pool) {
  typeof(((tpool *)NULL)->workers) workers = NULL;

  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    workers = poolc->workers;
    poolc->shutdown = true;
    cnd_broadcast(&poolc->wake_cnd);
  } else unreachable();

  foreach (var_ f, vla(*mList_vla(workers)))
    thrdfn_await(f);

  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    cnd_destroy(&poolc->wake_cnd);
    mList_deInit(poolc->workers);
  } else unreachable();

  mutex_deInit((*pool));
  // Note: aFree(pool) is needed here depending on your lifecycle design
}

static void tpool_addWorkers(mutex(tpool, mutex_recursive) * pool, usize count) {
  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    var_ list = poolc->workers;
    var_ alloc = mList_allocator(list);

    foreach (var_ i, range(0, count))
      mList_push(list, thrdfn_call(alloc, tpool_worker, (pool)));
  } else unreachable();
}

// example
// poolfn(waitp, ((int, seconds)), int, {
//   foreach (var_ i, range(0, seconds)) {
//     thrd_sleep(&(struct timespec){1}, NULL);
//     println("poolwait : {}", i);
//   }
//   return 1;
// });
// #include "arenaAllocator.h"
// #if !defined POOL_COUNT
//   #define POOL_COUNT (1)
// #endif
// int main(void) {
//
//   var_ pool = mutex_initW(tpool, mutex_plain, (tpool){});
//   defer { mutex_deInit(pool); };
//
//   var_ poolAlloc = arena_new_ext(stdAlloc, 1024);
//   defer { arena_cleanup(poolAlloc); };
//
//   mutex_critical (var_ poolptr, mutex_lock, pool) {
//     poolptr->tasks.allocator = poolAlloc;
//   } else unreachable();
//
//   foreach (var_ i, range(0, POOL_COUNT))
//     thrdfn_call(stdAlloc, tpool_worker, (&pool));
//   var_ waita = poolfn_call(stdAlloc, (&pool), waitp, (6));
//   var_ waitb = poolfn_call(stdAlloc, (&pool), waitp, (8));
//   poolfn_await(stdAlloc, waita);
//   poolfn_await(stdAlloc, waitb);
// }
// #include "wheels.h"
#endif
