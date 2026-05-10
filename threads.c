#include "allocator.h"
#include "macros.h"
#include <threads.h>

//
// mutex
//

// clang-format off
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
#define mutex_deInit(mutex) mtx_destroy((mutex).mtx)
#define mutex_lock(mutex) (mtx_lock((mutex).mtx))
#define mutex_timedlock(mutex, time) (           \
    (void)sizeof((mutex).kind->mutex_recursive), \
    mtx_timedlock((mutex).mtx, time)             \
)
#define mutex_trylock(mutex) (mtx_trylock((mutex).mtx))
#define mutex_unlock(mutex) (mtx_unlock((mutex).mtx))

// helper to make functions compatible with thrd_create
#define thrdfn_structItem(type_name) TUPLE_EXPAND_A(type_name) TUPLE_EXPAND_B(type_name);
#define thrdfn_structItems(...) APPLY_N(thrdfn_structItem, __VA_ARGS__)

#define thrdfn_argItem(type_name) TUPLE_EXPAND_A(type_name) TUPLE_EXPAND_B(type_name)
#define thrdfn_argItems(...) APPLY_N_C(thrdfn_argItem, __VA_ARGS__)

#define thrdfn_argItemT(type_name) argstruct->args.TUPLE_EXPAND_B(type_name)
#define thrdfn_argItemsT(...) APPLY_N_C(thrdfn_argItemT, __VA_ARGS__)

#define thrdfn(name, in, out, ...)                                              \
  typedef struct {                                                              \
    thrd_t thread_handle[1];                                                    \
    AllocatorV allocator;                                                       \
    int thread_status[1];                                                       \
    struct {                                                                    \
      thrdfn_structItems in                                                     \
    } args;                                                                     \
    out result;                                                                 \
  } *name##_future;                                                             \
  out name##_real(thrdfn_argItems in, AllocatorV allocator) { __VA_ARGS__ }     \
  int name(void *_thrd_item_ptr) {                                              \
    name##_future argstruct = (typeof(argstruct))_thrd_item_ptr;                \
    argstruct->result = name##_real(thrdfn_argItemsT in, argstruct->allocator); \
    return 1;                                                                   \
  }

#define remove_paren(...) __VA_ARGS__

#define thrdfn_call(alloc, fname, argss) ({        \
  fname##_future f = aCreate(alloc, typeof(*f));   \
  f->args = (typeof(f->args)){remove_paren argss}; \
  f->allocator = alloc;                            \
  thrd_create(f->thread_handle, fname, f);         \
  f;                                               \
})

#define thrdfn_await(allocator, f) ({                   \
  typeof(f) f_r = f;                                    \
  defer { aFree(allocator, f_r, sizeof(*f_r)); };       \
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
#include "print.h"
// example
thrdfn(mtxassig, ((mutex(int, mutex_plain) *, a), (int, b)), int, {
  println("hello from thread : {}", thrd_current());
  if (mutex_trylock(*a) != thrd_success)
    thrd_exit(5);
  a->data = b;
  mutex_unlock(*a);
  return b;
});
int main(void) {
  mutex(int, mutex_plain) j;
  mutex_init(j);
  defer { mutex_deInit(j); };
  var_ asi = thrdfn_await(stdAlloc, thrdfn_call(stdAlloc, mtxassig, (&j, 6)));
  println("thread finished with result : {} , mutex value : {} , status : {}", asi.result, j.data, asi.status);

  mutex_lock(j);
  var_ asi2 = thrdfn_await(stdAlloc, thrdfn_call(stdAlloc, mtxassig, (&j, 6)));
  println("thread finished with result : {} , mutex value : {} , status : {}", asi2.result, j.data, asi2.status);
}
