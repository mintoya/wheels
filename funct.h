#if !defined(MY_THREAD_MACORS_H)
  #define MY_THREAD_MACORS_H (1)

//
// c23 only
//

  #include "allocator.h"
  #include "macros.h"
  #include "mylist.h"
  #include "mytypes.h"
  #include "stdatomic.h"
  #include <threads.h>

//
// mutex
//

  #define mutex_plain mutex_plain
  #define mutex_timed mutex_timed
  #define mutex_recursive mutex_recursive
  #define mutex_recursive_timed mutex_recursive_timed

typedef struct {
  int mutex_plain;
} mutex_plain;
typedef struct {
  int mutex_timed;
} mutex_timed;
typedef struct {
  int mutex_recursive;
} mutex_recursive;
typedef struct {
  int mutex_recursive_timed;
} mutex_recursive_timed;
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

  #define mutex_timedlock(mutex, time) (       \
      (void)sizeof((mutex).kind->mutex_timed), \
      mtx_timedlock((mutex).mtx, time)         \
  )
  #define mutex_trylock(mutex) (mtx_trylock((mutex).mtx))
  #define mutex_lock(mutex) (mtx_lock((mutex).mtx))

  #define mutex_unlock(mutex) (mtx_unlock((mutex).mtx))

  #define mutex_critical(decl, lockfn, mutex, ...)                      \
    for (struct { int err; int keep; } _mc = {lockfn(mutex __VA_OPT__(, __VA_ARGS__)), 1}; \
         _mc.keep || (_mc.err ? 0 : (mutex_unlock(mutex), 0));          \
         _mc.keep = 0)                                                  \
      if (!_mc.err)                                                     \
        for (decl = &mutex.data; _mc.keep; _mc.keep = 0)
//
// function wrappers
//

  #define typelist_tuple_types_helper(...) APPLY_N_C(TUPLE_EXPAND_A, __VA_ARGS__)
  #define typelist_tuple_types(typelist) typelist_tuple_types_helper typelist

  #define typelist_tuple_arg(in) TUPLE_EXPAND_A(in) TUPLE_EXPAND_B(in)
  #define typelist_tuple_args_helper(...) APPLY_N_C(typelist_tuple_arg, __VA_ARGS__)
  #define typelist_tuple_args(typelist) typelist_tuple_args_helper typelist

  #define deffunctoin_struct_item(tuple) TUPLE_EXPAND_A(tuple) TUPLE_EXPAND_B(tuple);
  #define deffunctoin_struct_items(...) APPLY_N(deffunctoin_struct_item, __VA_ARGS__)

  #define deffunction_struct(in, out) \
    {                                 \
      struct {                        \
        deffunctoin_struct_items in   \
      } args;                         \
      out result;                     \
    }
  #define decfunction(name, in, out)                                            \
    typedef struct name##_struct_t deffunction_struct(in, out) name##_struct_t; \
    void name##_wrapper(void *);                                                \
    out name(typelist_tuple_types(in))

  #define SELECT_2(a, b, ...) b
// #define SELECT_2(a, b, ...) (a, b, ...)

  #define deffunction_return_if_nothing_nothing_t ~, return nothing_v;
  #define deffunction_return_if_nothing_w(...) SELECT_2(__VA_ARGS__, (void)(0);)
  #define deffunction_return_if_nothing(out_t) deffunction_return_if_nothing_w(deffunction_return_if_nothing_##out_t)

  #define deffunction_extract_item(tuple) ins->args.TUPLE_EXPAND_B(tuple)
  #define deffunction_extract_items(...) APPLY_N_C(deffunction_extract_item, __VA_ARGS__)
  #define deffunction(name, in, out, ...)                                                     \
    typedef struct name##_struct_t deffunction_struct(in, out) name##_struct_t;               \
    out name(typelist_tuple_args(in)) { __VA_ARGS__ deffunction_return_if_nothing(out) } /**/ \
    void name##_wrapper(void *inn) {                                                          \
      name##_struct_t *ins = (typeof(ins))inn;                                                \
      ins->result = name(deffunction_extract_items in);                                       \
    }

typedef struct thread_info {
  thrd_t thread[1];
  int status[1];
} thread_info;

  #define decfunction_thrd(name, in, out)                                 \
    decfunction(name, ((thread_info, threadid), REM_PAREN in), out); /**/ \
    int name##_spawn(name##_struct_t *ins);

  #define deffunction_thrd(name, in, out, ...)                                        \
    deffunction(name, ((thread_info, threadid), REM_PAREN in), out, __VA_ARGS__) /**/ \
        int                                                                           \
        name##_thrd_wrapper(void *inn) {                                              \
      name##_wrapper(inn);                                                            \
      return 0;                                                                       \
    }                                                                                 \
    int name##_spawn(name##_struct_t *ins) {                                          \
      return thrd_create(ins->args.threadid.thread, name##_thrd_wrapper, ins);        \
    }

  #define defunction_argsStruct(name, argss) (     \
      (void)sizeof(name argss),                    \
      (name##_struct_t){.args = {REM_PAREN argss}} \
  )

//
// function pool
//

typedef struct basic_closure_t {
  void *arg;
  fnptr_((void *), void) fn;
} basic_closure_t;
typedef struct tpoolNode_t {
  struct {
    basic_closure_t task;
    _Atomic(bool) done[1];
  } task;
  struct tpoolNode_t *next;
} tpoolNode_t;
typedef struct tpoolList {
  struct tpoolNode_t *first, *last;
} tpoolList;
typedef struct tpool {
  tpoolList tasks;
  cnd_t wake_cnd;
  bool shutdown;
  mList(struct tpool_worker_struct_t *) workers;
  // i will store the allocator in there
} tpool;
typedef mutex(tpool, mutex_recursive) * tpool_single_t;
AllocatorV tpool_allocator(tpool_single_t pool);
decfunction_thrd(tpool_worker, ((tpool_single_t, pool)), nothing_t);
static bool tpool_doSingle(tpool_single_t pool);
tpoolNode_t *_tpool_queup(tpool_single_t pool, basic_closure_t fn);
void _tpool_wait_loop(tpool_single_t pool, _Atomic(bool) *done_flag);
tpool_single_t tpool_init(AllocatorV alloc);
void tpool_deInit(tpool_single_t pool);
void tpool_addWorkers(tpool_single_t pool, usize count);

  #define defunction_call(name, argss) ({           \
    var_ _ins = defunction_argsStruct(name, argss); \
    name##_wrapper(&_ins);                          \
    _ins.result;                                    \
  })
  #define thrdfunction_call(alloc, name, argss) ({                   \
    name##_struct_t *_structdata = aCreate(                          \
        alloc,                                                       \
        typeof(*_structdata)                                         \
    );                                                               \
    _structdata->args = (typeof(_structdata->args)){                 \
        _structdata->args.threadid,                                  \
        REM_PAREN argss,                                             \
    };                                                               \
    (void)sizeof(name(_structdata->args.threadid, REM_PAREN argss)); \
    var_ k = name##_spawn(_structdata);                              \
    _structdata;                                                     \
  })
  #define thrdfunction_await(alloc, future) ({                                  \
    typeof(future) _future = future;                                            \
    thrd_join(_future->args.threadid.thread[0], _future->args.threadid.status); \
    defer { aFree(alloc, _future, sizeof(*_future)); };                         \
    struct {                                                                    \
      typeof(_future->result) result;                                           \
      int status;                                                               \
    } _r = {                                                                    \
        _future->result,                                                        \
        _future->args.threadid.status[0],                                       \
    };                                                                          \
    _r;                                                                         \
  })
  #define poolfunction_call(pool, func, argss) ({                                           \
    var_ args = aCreate(tpool_allocator(pool), typeof(defunction_argsStruct(func, argss))); \
    *args = defunction_argsStruct(func, argss);                                             \
    var_ future = _tpool_queup(pool, (basic_closure_t){args, func##_wrapper});              \
    struct {                                                                                \
      typeof(future) future;                                                                \
      typeof(*args) argsType[0];                                                            \
    } _r = {future};                                                                        \
    _r;                                                                                     \
  })
  #define poolfunction_call_type(pool, func, type, argss) ({                                \
    var_ args = aCreate(tpool_allocator(pool), typeof(defunction_argsStruct(func, argss))); \
    *args = defunction_argsStruct(func, argss);                                             \
    var_ future = _tpool_queup(pool, (basic_closure_t){args, func##_wrapper});              \
    type _r = {future};                                                                     \
    _r;                                                                                     \
  })
  #define poolfunction_await(pool, futuree) ({                                        \
    var_ _future = futuree;                                                           \
    defer { aFree(tpool_allocator(pool), _future.future, sizeof(*_future.future)); }; \
    defer {                                                                           \
      aFree(                                                                          \
          tpool_allocator(pool),                                                      \
          _future.future->task.task.arg,                                              \
          sizeof(_future.argsType[0])                                                 \
      );                                                                              \
    };                                                                                \
    _tpool_wait_loop(pool, _future.future->task.done);                                \
    var_ _r = (typeof(_future.argsType[0]) *)_future.future->task.task.arg;           \
    _r->result;                                                                       \
  })

#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_THREAD_MACORS_C (1)
#endif

#if defined(MY_THREAD_MACORS_C)

static bool tpool_doSingle(tpool_single_t pool) {
  tpoolNode_t *task = NULL;
  mutex_critical (var_ poolData, mutex_lock, (*pool)) {
    if (poolData->tasks.first) {
      task = poolData->tasks.first;
      poolData->tasks.first = task->next;

      if (!poolData->tasks.first)
        poolData->tasks.last = NULL;
    }
  } else return false;

  if (task) {
    var_ fn = task->task.task;
    if (fn.fn) fn.fn(fn.arg);
    atomic_store(task->task.done, true);
    return true;
  }
  return false;
}
deffunction_thrd(tpool_worker, ((tpool_single_t, pool)), nothing_t, {
  while (1) {
    if (tpool_doSingle(pool)) continue;
    bool to_exit = false;
    mutex_critical (var_ poolData, mutex_lock, (*pool)) {
      while (!poolData->tasks.first && !poolData->shutdown)
        cnd_wait(&poolData->wake_cnd, pool->mtx);
      if (poolData->shutdown && !poolData->tasks.first)
        to_exit = true;
    } else thrd_exit(5);
    if (to_exit) thrd_exit(5);
  }
});
tpoolNode_t *_tpool_queup(tpool_single_t pool, basic_closure_t fn) {
  tpoolNode_t *node = aCreate(tpool_allocator(pool), tpoolNode_t);
  *node = (typeof(*node)){.task = {fn, {false}}, .next = NULL};

  mutex_critical (var_ pooldata, mutex_lock, (*pool)) {
    if (pooldata->tasks.last)
      pooldata->tasks.last->next = node;
    else pooldata->tasks.first = node;
    pooldata->tasks.last = node;
    cnd_signal(&pooldata->wake_cnd);
  } else unreachable();
  return node;
}
void _tpool_wait_loop(tpool_single_t pool, _Atomic(bool) *done_flag) {
  while (!atomic_load(done_flag))
    if (!tpool_doSingle(pool))
      thrd_yield();
}
tpool_single_t tpool_init(AllocatorV alloc) {
  typedef typeof(struct tpool_worker_future_struct *) worker_t;

  tpool_single_t pool = aCreate(alloc, typeof(*pool));
  pool->data = (tpool){
      .shutdown = false,
      .workers = mList_init(alloc, tpool_worker_struct_t *),
  };
  cnd_init(&pool->data.wake_cnd);
  mutex_init((*pool));
  return pool;
}
AllocatorV tpool_allocator(tpool_single_t pool) {
  // changing allocators for objects is illegal anyway
  // mlist pointer is also stable, not constant vause it needs to be mutated
  return mList_allocator(pool->data.workers);
}
void tpool_deInit(tpool_single_t pool) {
  typeof(((tpool *)NULL)->workers) workers = NULL;
  AllocatorV alloc = tpool_allocator(pool);
  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    workers = poolc->workers;
    poolc->shutdown = true;
    cnd_broadcast(&poolc->wake_cnd);
  } else unreachable();

  foreach (var_ f, vla(*mList_vla(workers)))
    thrdfunction_await(alloc, f);

  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    cnd_destroy(&poolc->wake_cnd);
    mList_deInit(poolc->workers);
  } else unreachable();

  mutex_deInit((*pool));
  aFree(alloc, pool, sizeof(*pool));
}
void tpool_addWorkers(tpool_single_t pool, usize count) {
  mutex_critical (var_ poolc, mutex_lock, (*pool)) {
    var_ list = poolc->workers;
    var_ alloc = mList_allocator(list);

    foreach (var_ i, range(0, count))
      mList_push(list, thrdfunction_call(alloc, tpool_worker, (pool)));
  } else unreachable();
}

#endif
