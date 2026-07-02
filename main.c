#include "macros.h"
#include <stdcountof.h>

/* 1. Helper to merge `val` and `(type, func)` into `(val, type, func)` */
#define REMOVE_PARENS(...) __VA_ARGS__
#define ADD_ARG(add, ...) (add, REMOVE_PARENS __VA_ARGS__)

/* 2. Coerce unevaluated branches to satisfy the type checker.
 *(type*)0 creates a valid lvalue of `type` without evaluating. */
#define CT_COERCE(val, type) \
  _Generic((val), type: (val), default: *(typeof(type) *)0)

/* 3. Receives the unpacked arguments and formats the branch */
#define CT_CASE_UW(val, type, mac, ...) \
  type:                                 \
  mac(CT_COERCE(val, type))

/* 4. The proxy that forces __VA_ARGS__ to expand into separate arguments */
#define CT_CASE_INVOKE(...) CT_CASE_UW __VA_ARGS__

/* 5. Combines the captured `val` and the tuple, then unpacks them */
#define CT_CASE(val, tuple) \
  CT_CASE_INVOKE(ADD_ARG(val, tuple))

/* 6. The primary conversion interface */
#define convert_type(value, ...) \
  _Generic((value), APPLY_N_WITH_C(CT_CASE, value, __VA_ARGS__))

// --- Example Usage ---
#include "fptr.h"
#include "mytypes.h"
#include "stdio.h"
sliceDef(int);
int main(void) {
#define printarrint(x)                        \
  ({                                          \
    printf("[");                              \
    foreach (var_ i, span(*x, countof(*x))) { \
      printf("%i,", *i);                      \
    }                                         \
    printf("]");                              \
  })
#define printarrc8(x)                         \
  ({                                          \
    printf("[");                              \
    foreach (var_ i, span(*x, countof(*x))) { \
      printf("%c,", *i);                      \
    }                                         \
    printf("]");                              \
  })
#define printslicec8(x)                    \
  do {                                     \
    printf("[");                           \
    foreach (var_ i, span(x.ptr, x.len)) { \
      printf("%c,", *i);                   \
    }                                      \
    printf("]");                           \
  } while (0)
#define printsliceint(x)                   \
  do {                                     \
    printf("[");                           \
    foreach (var_ i, span(x.ptr, x.len)) { \
      printf("%i,", *i);                   \
    }                                      \
    printf("]");                           \
  } while (0)

  c8 c8arr[2] = {'a', 0};
  int iarr[] = {1, 2, 3, 4, 5};
  slice(c8) c8slice = (slice(c8)){sizeof(c8arr), c8arr};
  slice(int) islice = (slice(int)){countof(iarr), iarr};
#define printit(x)                                            \
  convert_type(                                               \
      &x,                                                     \
      (typeof(int (*)[sizeof(x) / sizeof(*x)]), printarrint), \
      (typeof(c8(*)[sizeof(x) / sizeof(*x)]), printarrc8),    \
  )
#define isarray(a) _Generic(&(a), typeof(a)[]: 1, default: 0)

  int integer = 5;
  isarray(integer);
  isarray(c8arr);

  printit(iarr);
  printit(c8arr);
  printit(REF(c8slice));
#define countofa(a) (sizeof(a) / sizeof(typeof(a)[1]))
  countofa(integer);
  countofa(iarr);
  var_ p = fp(fp(""));

  return 0;
}
