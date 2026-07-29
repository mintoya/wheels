#include "assertMessage.h"
#include "macros.h"
#include "mytypes.h"
#include "stdio.h"
#include <stdlib.h>
typedef const struct allocfn *allocfn;
typedef const struct allocfn {
  const fnptrof(
      (allocfn, void *, usize, usize, const char *, const uint),
      void *
  ) fn;
} *allocfn;
void *stdAllocatorFunction(
    allocfn,
    void *op,
    usize from,
    usize to,
    const char *fname,
    const uint ln
) {
#define pptr(ptr) ({let _p = ptr; printf("%p", _p);_p; })
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;

  switch (((!!from) << 1) | ((!!to) << 0)) {
    case ALLOC:
      return (assertMessage(!op, "allocation called pointer"), pptr(malloc(to)));
    case FREE:
      return (assertMessage(op, "allocator does not return null"), free(op), pptr(nullptr));
    case RESIZE:;
      return (assertMessage(op, "allocator does not return null"), pptr(realloc(op, to)));
    default:
      assertMessage(false, "invalid call from %s line %u: (%p , %zu , %zu)", fname, ln, op, from, to);
  }
#undef pptr
}
static const struct allocfn stdallocfn[1] = {{stdAllocatorFunction}};
#define vcallargs(it, ...) (it __VA_OPT__(, ) __VA_ARGS__)
#define vcall(it, name, args) (it->name vcallargs(it, REM_PAREN args))

#define create(alloc, T) \
  ((ptrof(T))vcall((alloc), fn, (nullptr, 0, sizeof(T), __FILE__, __LINE__)))

#define destroy(alloc, ptr) \
  ((void)vcall((alloc), fn, ((ptr), sizeof(*(ptr)), 0, __FILE__, __LINE__)))

#define resize(alloc, ptr, T) \
  ((ptrof(T))vcall((alloc), fn, ((ptr), sizeof(*(ptr)), sizeof(T), __FILE__, __LINE__)))

#define create_extra(alloc, ptr, T, extra) \
  ((ptrof(T))vcall((alloc), fn, ((ptr), 0, sizeof(T) + (extra), __FILE__, __LINE__)))

#include "tests.h"
test_fn(lifecycle) {
  destroy(stdallocfn, resize(stdallocfn, create(stdallocfn, int[5]), int[2]));
}
