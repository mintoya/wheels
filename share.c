#include "assertMessage.h"
#include "macros.h"
#include "mylist.h"
#include "mytypes.h"
#include <stdatomic.h>
#include <string.h>

typedef struct shared_ptr shared_ptr;
struct shared_ptr {
  _Atomic(usize) refcount[1];
  fnptrof((void *), void) destruct;
  _Alignas(myAlign) u8(ptr)[];
};

shared_ptr *sharedptr_new(const void *ptr, usize size, itypeof(shared_ptr, destruct) dst) {
  var_ res = (shared_ptr *)aAlloc(stdAlloc, sizeof(shared_ptr) + size);
  atomic_init(res->refcount, 1);
  res->destruct = dst;
  memcpy(res->ptr, ptr, size);
  return res;
}
void sharedptr_drop(shared_ptr *sp, usize size) {
  assertMessage(sp && sp->refcount[0]);
  if (atomic_fetch_sub(sp->refcount, 1) == 1) {
    sp->destruct((void *)sp->ptr);
    aFree(stdAlloc, sp, size + sizeof(shared_ptr));
  }
}
void sharedptr_take(shared_ptr *sp) { atomic_fetch_add(sp->refcount, 1); }

// Dummy tags to create type incompatibility
struct tag_shared; // Held locally
struct tag_owned;  // Passed to functions that take ownership (and will drop)
struct tag_borrow; // Passed to functions that just read (will not drop)

#define shared(T) ptrof(fnptrof((struct tag_shared *), T))
#define owned(T) ptrof(fnptrof((struct tag_owned *), T))
#define borrowed(T) ptrof(fnptrof((struct tag_borrow *), T))

#define shared_init(init, dt)                                      \
  (shared(typeof(init)))({                                         \
    var_ _sr = init;                                               \
    (void)sizeof(dt(&_sr));                                        \
    sharedptr_new(&_sr, sizeof(_sr), (fnptrof((void *), void))dt); \
  })

#define shared_drop(shared_var)            \
  ({                                       \
    if (shared_var) {                      \
      sharedptr_drop(                      \
          (shared_ptr *)shared_var,        \
          sizeof((*shared_var)((void *)0)) \
      );                                   \
    }                                      \
  })

#define shared_scope(DECL, SRC)                                                       \
  for (                                                                               \
      int _shared_done =                                                              \
          (assertMessage(SRC, "pointer is moved"),                                    \
           shared_take(SRC),                                                          \
           0);                                                                        \
      !_shared_done;                                                                  \
      (shared_drop(SRC), _shared_done = 1))                                           \
    for (DECL = (typeof((*SRC)((struct tag_shared *)0)) *)((shared_ptr *)(SRC))->ptr; \
         !_shared_done;                                                               \
         _shared_done = 1)

#define shared_move(VAR)                \
  ((owned(typeof((*VAR)((void *)0))))({ \
    var_ _tmp = (VAR);                  \
    (VAR) = 0;                          \
    _tmp;                               \
  }))

#define shared_borrow(VAR) \
  ((borrowed(typeof((*VAR)((void *)0))))(VAR))
#define shared_take(VAR) \
  sharedptr_take((shared_ptr *)VAR)

void deInitInt(int *i) { printf("%i deinit\n", *(int *)i); }
void dropthing(owned(int) i) {
  printf("entered dropthing\n");
  defer { printf("exited dropthing\n"); };
  shared_drop(i);
}

int main() {
  shared(int) x = shared_init(1, deInitInt);
  defer { shared_drop(x); };
  dropthing(shared_move(x));

  shared_scope(int *xp, x)
      printf("xp is : %i\n", xp ? *xp : -999);
}
