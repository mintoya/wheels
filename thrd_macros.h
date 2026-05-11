#if !defined(MY_THREAD_MACORS_H)
  #define MY_THREAD_MACORS_H (1)
  #include "allocator.h"
  #include "macros.h"
  #include <stdatomic.h>
  #include <threads.h>

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
    out name(thrdfn_argItems in) { __VA_ARGS__ }                   \
    int name##_wrap(void *_thrd_item_ptr) {                        \
      name##_future argstruct = (typeof(argstruct))_thrd_item_ptr; \
      argstruct->result = name(thrdfn_argItemsT in);               \
      return 1;                                                    \
    }

  #define thrdfn_call(alloc, fname, argss) ({       \
    fname##_future f = aCreate(alloc, typeof(*f));  \
    (void)sizeof(fname argss);                      \
    f->args = (typeof(f->args)){REM_PAREN argss};   \
    f->allocator = alloc;                           \
    thrd_create(f->thread_handle, fname##_wrap, f); \
    f;                                              \
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

typedef struct basic_closure_t {
  void *arg;
  void (*fn)(void *); // Or fnptr_((void *), void) depending on your typedefs
} basic_closure_t;

  // forcing args as type + name , so (int , i)
  #define fdec_type(tup) TUPLE_EXPAND_A(tup)
  #define fdec_name(tup) TUPLE_EXPAND_B(tup)

  #define fdec_params_helper(tup) fdec_type(tup) fdec_name(tup)
  #define fdec_params(...) APPLY_N_C(fdec_params_helper, __VA_ARGS__)

  #define fdec_struct_elem(e) fdec_type(e) fdec_name(e);
  #define fdec_struct_elems(...) APPLY_N(fdec_struct_elem, __VA_ARGS__)
  #define closure_fn_return_nothing ~, return (nothing){};
  #define closure_fn_return_void ~, ,

  #define SELECT_SECOND_HELPER(a, b, ...) b
  #define SELECT_SECOND(...) SELECT_SECOND_HELPER(__VA_ARGS__)
  #define SELECT_THIRD_HELPER(...) (__VA_ARGS__)
  // #define SELECT_THIRD_HELPER(a, b, c, ...) c
  #define SELECT_THIRD(...) SELECT_THIRD_HELPER(__VA_ARGS__)

  #define closure_fn_return_if_nothing(type) SELECT_SECOND(MACRO_EXPAND(closure_fn_return_##type), )

  #define fdec(name, in, out, ...) \
    out name(fdec_params in) { __VA_ARGS__ closure_fn_return_if_nothing(out) }

  #define closure_fn_structitem(a) args.arguments.fdec_name(a)
  #define closure_fn_structitems(...) APPLY_N_C(closure_fn_structitem, __VA_ARGS__)

  #define closure_fn_resultdef(type)

  #define closure_fn(name, in, out, ...)             \
    fdec(name, in, out, __VA_ARGS__);                \
    typedef struct name##_fnStruct {                 \
      struct {                                       \
        fdec_struct_elems in                         \
      } arguments;                                   \
      out result;                                    \
    } name##_fnStruct;                               \
    void name##_wrap(void *ptr) {                    \
      name##_fnStruct args = *(typeof(args) *)ptr;   \
      args.result = name(closure_fn_structitems in); \
      ((typeof(args) *)ptr)[0] = args;               \
    }
  #define closure_fn_genArgs(closure, args) ({                    \
    closure##_fnStruct _args = (typeof(_args)){{REM_PAREN args}}; \
    (void)sizeof(closure args);                                   \
    _args;                                                        \
  })
  #define closure_fn_call(closure, args) ({         \
    var_ _args = closure_fn_genArgs(closure, args); \
    closure##_wrap((void *)&_args);                 \
    _args.result;                                   \
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
    out name(thrdfn_argItems in) { __VA_ARGS__ }                   \
    void name##_wrap(void *_thrd_item_ptr) {                       \
      name##_future argstruct = (typeof(argstruct))_thrd_item_ptr; \
      argstruct->result = name(thrdfn_argItemsT in);               \
      atomic_store(argstruct->done, true);                      \
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

  #define poolfn_call(pool, fname, argss) ({                               \
    typedef struct {                                                       \
      typeof(*(fname##_future)NULL) future[1];                             \
      tpoolNode_t node[1];                                                 \
    } pool_handle_object;                                                  \
    var_ alloc = tpool_allocator(pool);                                    \
    var_ pitem = aCreate(alloc, pool_handle_object);                       \
    var_ f = pitem->future;                                                \
    (void)sizeof(fname argss);                                             \
    f->args = (typeof(f->args)){REM_PAREN argss};                          \
    f->done[0] = false;                                                    \
    _tpool_enqueue(pool, pitem->node, (basic_closure_t){f, fname##_wrap}); \
    f;                                                                     \
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
