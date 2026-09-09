#include "limits.h"
#include "wheels/assertMessage.h"
#include "wheels/macros.h"
#include "wheels/mytypes.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  const char *error_code;
  const char *file;
  usize line;
  const char *extra_info;
} error_t;
#define ENABLE_ERROR_IN_NORMAL_FUNCTION
#if defined(ENABLE_ERROR_IN_NORMAL_FUNCTION)
static const void *ERROR_VARIABLE_LOCAL_DECLARED_BY_MACRO__ = 0;
#endif
#define errable(...) (error_t * ERROR_VARIABLE_LOCAL_DECLARED_BY_MACRO__ __VA_OPT__(, ) __VA_ARGS__)
#define error_call_args(...) __VA_OPT__(, ) __VA_ARGS__
#define call_error(fn, args) ({                                        \
  static thread_local error_t this_error_ = {};                        \
  let this_result_ = fn(&this_error_ error_call_args(REM_PAREN args)); \
  (struct {typeof(this_result_)result ; error_t error; }){                                                        \
      this_result_,                                                    \
      this_error_,                                                     \
  };                                                                   \
})
__attribute__((noreturn)) inline void error_panic(error_t e) {
  fprintf(stderr, "error\t:%s\n", e.error_code);
  fprintf(stderr, "\tfile\t:%s\n", e.file);
  fprintf(stderr, "\tline\t:%zu\n", e.line);
  if (e.extra_info) fprintf(stderr, "\tdata\t:%s\n", e.extra_info);
  assertMessage(false);
}
inline void error_mask(error_t *out, error_t e) {
  if (out) return (void)(*out = e);
  else return error_panic(e);
}
#define pass_error(result, fullcode) return (                                                 \
    _Generic(                                                                                 \
        ERROR_VARIABLE_LOCAL_DECLARED_BY_MACRO__,                                             \
        error_t *: error_mask((error_t *)ERROR_VARIABLE_LOCAL_DECLARED_BY_MACRO__, fullcode), \
        default: error_panic(fullcode)                                                        \
    ),                                                                                        \
    result                                                                                    \
)
#define return_error(result, code) ({ \
  pass_error(                         \
      result,                         \
      ((error_t){                     \
          .error_code = code,         \
          .file = __FILE__,           \
          .line = __LINE__,           \
      })                              \
  );                                  \
})
#define try_error(default_return, fn, args) ({  \
  let _try_val = call_error(fn, args);          \
  if_unlikely (_try_val.error.error_code)       \
    pass_error(default_return, _try_val.error); \
  _try_val.result;                              \
})
#define catch_error_dcl(name) let name = e_.error;
#define catch_error(errorn, ...)        \
  ({                                    \
    let e_ = errorn;                    \
    if_unlikely (e_.error.error_code) { \
      catch_error_dcl __VA_ARGS__       \
    }                                   \
    e_.result;                          \
  })

// if allocators were ported to be errable
typedef const struct allocfns_errable *allocfn_errable;
typedef const struct allocfns_errable {
  typeof(void *errable(allocfn_errable, void *, usize, usize, const char *, const uint)) *const fn;
} *allocfn_errable;

void *stdAllocatorFunction_errable errable(
    allocfn_errable,
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
  switch (((!!from) << 1) | ((!!to) << 0)) {
    case ALLOC: {
      let res = malloc(to);
      if (!res) return_error((void *)0, "MALLOC_FAILED");
      else return res;
    } break;
    case FREE: {
      if (!op) return_error((void *)0, "MALLOC_FREE_NULL");
      else return (free(op), nullptr);
    } break;
    case RESIZE: {
      if (!op) return_error((void *)0, "MALLOC_RESIZE_NULL");
      let res = realloc(op, to);
      if (!res) return_error((void *)0, "MALLOC_FAILED");
      return res;
    } break;
    default:
      return_error((void *)0, "MALLOC_INVALID_ARGS");
      break;
  }
}
struct allocfns_errable stdallocfn_errable[1] = {stdAllocatorFunction_errable};
