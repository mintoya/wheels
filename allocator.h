#include <stdio.h>
#if !defined SINGLE_ALLOCATOR_H
  #define SINGLE_ALLOCATOR_H (1)
  #include "macros.h"
  #include "mytypes.h"
  #include <stdlib.h>
  #include <string.h>

void *malloc(size_t);
void *realloc(void *, size_t);
void free(void *);

typedef const struct allocfns *allocfn;
typedef const struct allocfns {
  const fnptrof(
      (allocfn, void *, usize, usize, const char *, const uint),
      void *
  ) fn;
} *allocfn;

  #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 202400L
    #define gnu_const__ [[gnu::const]]
  #else
    #define gnu_const__
  #endif

gnu_const__ static inline uptr lineup(uptr u, usize a) { return (((u + (a - 1)) / a) * a); }
gnu_const__ static inline uptr alloc_align(uptr u) { return lineup(u, alignof(myAlign)); }

  #define vcallargs(it, ...) (it __VA_OPT__(, ) __VA_ARGS__)
  #define vcall(it, name, args) (it->name vcallargs(it, REM_PAREN args))

  #define acreate(alloc, T)                           \
    ({                                                \
      ptrof(T) _result = (typeof(_result))vcall(      \
          (alloc),                                    \
          fn,                                         \
          (                                           \
              nullptr,                                \
              0,                                      \
              alloc_align(sizeof(*_result)),          \
              __FILE__,                               \
              __LINE__                                \
          )                                           \
      );                                              \
      __builtin_memset(_result, 0, sizeof(*_result)); \
      _result;                                        \
    })
  #define avalue(alloc, val) ({                \
    let _r = acreate(alloc, typeof(val));      \
    let _m = val;                              \
    (typeof(_m) *)memcpy(_r, &_m, sizeof(_m)); \
  })

  #define adestroy(alloc, ptr) \
    ((void)vcall((alloc), fn, ((ptr), alloc_align(sizeof(*(ptr))), 0, __FILE__, __LINE__)))

  #define aresize(alloc, ptr, T) \
    ((ptrof(T))vcall((alloc), fn, ((ptr), alloc_align(sizeof(*(ptr))), sizeof(T), __FILE__, __LINE__)))

  #define acreate_extra(alloc, T, extra) \
    ((ptrof(T))vcall((alloc), fn, (0, 0, alloc_align(sizeof(T) + (extra)), __FILE__, __LINE__)))

// base allocator
void *stdAllocatorFunction(
    allocfn,
    void *op,
    usize from,
    usize to,
    const char *fname,
    const uint ln
);
static const struct allocfns stdAlloc[1] = {{stdAllocatorFunction}};
#endif

#if (defined SINGLE_ALLOCATOR_C && SINGLE_ALLOCATOR_C == 1) || (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)
  #undef SINGLE_ALLOCATOR_C
  #define SINGLE_ALLOCATOR_C (2)
//   #include "tests.h"
// test_fn(lifecycle) {
//   adestroy(stdAlloc, aresize(stdAlloc, acreate(stdAlloc, int[5]), int[2]));
// }
// #define STD_PRINT_DEBUG
  #if defined STD_PRINT_DEBUG
    #define pptr(ptr) ({let _p = ptr; printf("%p", _p);_p; })
  #else
    #define pptr(ptr) ({let _p = ptr; _p; })
  #endif
void *stdAllocatorFunction(
    allocfn,
    void *op,
    usize from,
    usize to,
    const char *fname,
    const uint ln
) {
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;
  #if defined STD_PRINT_DEBUG
  printf("%s %u (%p %zu %zu ", fname, ln, op, from, to);
  defer { printf(")\n"); };
  #endif

  switch (((!!from) << 1) | ((!!to) << 0)) {
    case ALLOC:
      return (assert(!op && "allocation called pointer"), pptr(P$(malloc(to), (assert($ && "malloc null"), $))));
    case FREE:
      return (assert(op && "allocator does not return null"), free(op), pptr(nullptr));
    case RESIZE:;
      return (assert(op && "allocator does not return null"), pptr(P$(realloc(op, to), (assert($ && "realloc null"), $))));
    default:
      assert(printf("invalid call from %s line %u: (%p , %zu , %zu)", fname, ln, op, from, to) && false);
  }
  __builtin_trap();
}
  #undef pptr
#endif
