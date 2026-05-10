#include "sList.h"
#if !defined(MY_THREAD_MACORS_H)
  #define MY_THREAD_MACORS_H (1)
  #include "allocator.h"
  #include "macros.h"
  #include <stdatomic.h>
  #include <threads.h>

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

typedef struct basic_closure_t {
  void *arg;
  void (*fn)(void *); // Or fnptr_((void *), void) depending on your typedefs
} basic_closure_t;

  // 1. Define the closure structure and execution function
  #define closurefn(name, in, ...)                               \
    typedef struct name##_args_struct {                          \
      thrdfn_structItems in                                      \
    } name##_args;                                               \
    void name##_real(thrdfn_argItems in) { __VA_ARGS__ }         \
    void name(void *_void_arg_ptr) {                             \
      name##_args *argstruct = (typeof(argstruct))_void_arg_ptr; \
      name##_real(thrdfn_argItemsT in);                          \
    }

  // 2. Instantiate the argument payload and return the generic basic_closure_t
  #define closurefn_call(alloc, fname, argss) ({              \
    fname##_args *_f_args = aCreate(alloc, typeof(*_f_args)); \
    _Static_assert(                                           \
        types_eq(                                             \
            fnptr_(                                           \
                (APPLY_N_C(                                   \
                    typeof, remove_paren argss                \
                )),                                           \
                void                                          \
            ),                                                \
            typeof(&fname##_real)                             \
        ),                                                    \
        "closurefn called with incorrect arguments"           \
    );                                                        \
    *_f_args = (typeof(*_f_args)){remove_paren argss};        \
    (basic_closure_t){                                        \
        .arg = _f_args,                                       \
        .fn = (void (*)(void *))fname                         \
    };                                                        \
  })
typedef struct tpoolNode_t {
  basic_closure_t task;
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

static bool tpool_doSingle(mutex(tpool, mutex_recursive) * pool) {
  tpoolNode_t *task = NULL;
  AllocatorV allocator = NULL;

  mutex_critical (var_ poolData, mutex_lock, (*pool)) {
    if (poolData->tasks.first) {
      task = poolData->tasks.first;
      poolData->tasks.first = task->next;

      if (!poolData->tasks.first)
        poolData->tasks.last = NULL;
      allocator = poolData->tasks.allocator;
    }
  } else return false;

  if (task) {
    var_ fn = task->task;
    if (fn.fn) fn.fn(fn.arg);
    return true;
  }

  return false;
}
thrdfn(tpool_worker, ((mutex(tpool, mutex_recursive) *, pool)), int, {
  while (1) {
    if (tpool_doSingle(pool))
      continue;

    // empty que
    bool time_to_exit = false;
    mutex_critical (var_ poolData, mutex_lock, (*pool)) {
      while (!poolData->tasks.first && !poolData->shutdown) {
        cnd_wait(&poolData->wake_cnd, pool->mtx);
      }
      if (poolData->shutdown && !poolData->tasks.first) {
        time_to_exit = true;
      }
    } else thrd_exit(5);

    if (time_to_exit) break;
  }
  return 1;
});

  #define poolfn(name, in, out, ...)                               \
    typedef struct {                                               \
      struct {                                                     \
        thrdfn_structItems in                                      \
      } args;                                                      \
      out result;                                                  \
      _Atomic(bool) done[1];                                       \
    } *name##_future;                                              \
    out name##_real(thrdfn_argItems in) { __VA_ARGS__ }            \
    void name(void *_thrd_item_ptr) {                              \
      name##_future argstruct = (typeof(argstruct))_thrd_item_ptr; \
      argstruct->result = name##_real(thrdfn_argItemsT in);        \
      argstruct->done[0] = true;                                   \
    }

/**
 */
static inline void _tpool_enqueue(mutex(tpool, mutex_recursive) * pool, tpoolNode_t *nodeMem, basic_closure_t fn) {
  nodeMem->task = fn;
  nodeMem->next = NULL;

  mutex_critical (var_ pooldata, mutex_lock, (*pool)) {
    if (pooldata->tasks.last)
      pooldata->tasks.last->next = nodeMem;
    else pooldata->tasks.first = nodeMem;
    pooldata->tasks.last = nodeMem;
    cnd_signal(&pooldata->wake_cnd);
  } else unreachable();
}
static inline void _tpool_wait_loop(mutex(tpool, mutex_recursive) * pool, _Atomic(bool) *done_flag) {
  while (!atomic_load(done_flag))
    if (!tpool_doSingle(pool))
      thrd_yield();
}

  #define poolfn_call(pool, fname, argss) ({                        \
    typedef struct {                                                \
      typeof(*(fname##_future)NULL) future[1];                      \
      tpoolNode_t node[1];                                          \
    } pool_handle_object;                                           \
    var_ alloc = tpool_allocator(pool);                             \
    var_ pitem = aCreate(alloc, pool_handle_object);                \
    var_ f = pitem->future;                                         \
                                                                    \
    _Static_assert(                                                 \
        types_eq(                                                   \
            fnptr_(                                                 \
                (APPLY_N_C(                                         \
                    typeof, remove_paren argss                      \
                )),                                                 \
                typeof(f->result)                                   \
            ),                                                      \
            typeof(&fname##_real)                                   \
        ),                                                          \
        "poolfn not called with enough args"                        \
    );                                                              \
                                                                    \
    f->args = (typeof(f->args)){remove_paren argss};                \
    f->done[0] = false;                                             \
    _tpool_enqueue(pool, pitem->node, (basic_closure_t){f, fname}); \
    f;                                                              \
  })

  #define poolfn_await(pool, f) ({                 \
    var_ alloc = tpool_allocator(pool);            \
    typedef struct {                               \
      typeof(*(f)) future[1];                      \
      tpoolNode_t node[1];                         \
    } pool_handle_object;                          \
    typeof(f) f_r = f;                             \
                                                   \
    _tpool_wait_loop(pool, f_r->done);             \
                                                   \
    typeof(f_r->result) res = f_r->result;         \
    aFree(alloc, f_r, sizeof(pool_handle_object)); \
    res;                                           \
  })

static mutex(tpool, mutex_recursive) *
    tpool_init(AllocatorV alloc) {
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

static AllocatorV tpool_allocator(mutex(tpool, mutex_recursive) * pool);
static void tpool_deInit(mutex(tpool, mutex_recursive) * pool) {
  typeof(((tpool *)NULL)->workers) workers = NULL;
  AllocatorV alloc = tpool_allocator(pool);
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
  aFree(alloc, pool, sizeof(*pool));
}

static void tpool_addWorkers(mutex(tpool, mutex_recursive) * pool, usize count) {
  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    var_ list = poolc->workers;
    var_ alloc = mList_allocator(list);

    foreach (var_ i, range(0, count))
      mList_push(list, thrdfn_call(alloc, tpool_worker, (pool)));
  } else unreachable();
}
static AllocatorV tpool_allocator(mutex(tpool, mutex_recursive) * pool) {
  // changing allocators for objects is illegal anyway
  return mList_allocator(pool->data.workers);
}

#endif
