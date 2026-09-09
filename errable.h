#if !defined(MY_ERRORS_H)
  #define MY_ERRORS_H (1)
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"
  #include <stdio.h>

typedef struct {
  const char *err_code;
  const char *file;
  usize line;
  const char *extra_info;
} err_t;

  // #define ENABLE_ERROR_IN_NORMAL_FUNCTION
  #if defined(ENABLE_ERROR_IN_NORMAL_FUNCTION)
static const void *err_VARIABLE_LOCAL_DECLARED_BY_MACRO__ = 0;
  #endif

typedef int errable_void;

  #define errs(...) (err_t * err_VARIABLE_LOCAL_DECLARED_BY_MACRO__ __VA_OPT__(, ) __VA_ARGS__)

  #define err_call_args(...) __VA_OPT__(, ) __VA_ARGS__

  #define call_err(fn, args) ({                                                   \
    err_t this_err_ = {};                                                         \
    let this_result_ = _Generic(                                                  \
        (typeof(fn(&this_err_ err_call_args(REM_PAREN args))) *)0,                \
        void *: (fn(&this_err_ err_call_args(REM_PAREN args)), (errable_void){}), \
        default: fn(&this_err_ err_call_args(REM_PAREN args))                     \
    );                                                                            \
    (struct {typeof(this_result_)result ; err_t err; }){                                                                 \
        this_result_,                                                             \
        this_err_,                                                                \
    };                                                                            \
  })

__attribute__((noreturn)) static inline void err_panic(err_t e) {
  fprintf(stderr, "err\t:%s\n", e.err_code);
  fprintf(stderr, "\tfile\t:%s\n", e.file);
  fprintf(stderr, "\tline\t:%zu\n", e.line);
  if (e.extra_info) fprintf(stderr, "\tdata\t:%s\n", e.extra_info);
  assertMessage(false);
}

static inline void err_mask(err_t *out, err_t e) {
  if (out) return (void)(*out = e);
  else return err_panic(e);
}

  #define pass_err(fullcode, ...)                                                       \
    ({                                                                                  \
      _Generic(                                                                         \
          err_VARIABLE_LOCAL_DECLARED_BY_MACRO__,                                       \
          err_t *: err_mask((err_t *)err_VARIABLE_LOCAL_DECLARED_BY_MACRO__, fullcode), \
          default: err_panic(fullcode)                                                  \
      );                                                                                \
      _Pragma("GCC diagnostic push");                                                   \
      _Pragma("GCC diagnostic ignored \"-Wreturn-type\"");                              \
      _Pragma("GCC diagnostic ignored \"-Wreturn-mismatch\"");                          \
      return __VA_ARGS__;                                                               \
      _Pragma("GCC diagnostic pop");                                                    \
    })

  #define return_err(code, ...) ({ \
    pass_err(                      \
        ((err_t){                  \
            .err_code = code,      \
            .file = __FILE__,      \
            .line = __LINE__,      \
        }) __VA_OPT__(, )          \
            __VA_ARGS__            \
    );                             \
  })
  #define return_err_extras(code, extra_infos, ...) ({ \
    pass_err(                                          \
        ((err_t){                                      \
            .err_code = code,                          \
            .file = __FILE__,                          \
            .line = __LINE__,                          \
            .extra_info = extra_infos,                 \
        }) __VA_OPT__(, )                              \
            __VA_ARGS__                                \
    );                                                 \
  })

  #define try_err(fn, args, ...) ({     \
    let _try_val = call_err(fn, args);  \
    if_unlikely (_try_val.err.err_code) \
      pass_err(                         \
          _try_val.err __VA_OPT__(, )   \
              __VA_ARGS__               \
      );                                \
    _try_val.result;                    \
  })

  #define catch_err_bind(err_name, res_name) \
    let err_name = e_.err;                   \
    let res_name = e_.result;

  #define catch_err(errn, bind_tuple, block) \
    ({                                       \
      let e_ = (errn);                       \
      typeof(e_.result) _catch_res;          \
      if_unlikely (e_.err.err_code) {        \
        catch_err_bind bind_tuple            \
            _catch_res = block;              \
      } else {                               \
        _catch_res = e_.result;              \
      }                                      \
      _catch_res;                            \
    })

  #define catch_errcall(call_tuple, bind_tuple, block) \
    catch_err(call_err call_tuple, bind_tuple, block)

// tests
  #include "tests.h"
static inline int test_divide errs(int a, int b) {
  if (b == 0) return_err("DIV_BY_ZERO");
  return a / b;
}

static inline int test_bubble errs(int a, int b) {
  int res = try_err(test_divide, (a, b), 0);
  return res * 2;
}

static inline void test_void_err errs(int a) {
  if (a < 0) return_err("NEGATIVE_VOID");
}

test_fn(errable_success) {
  bool caught = false;
  int res = catch_errcall((test_divide, (10, 2)), (e, _), (caught = true, 0));
  test_assert(!caught);
  test_assert(res == 5);
}

test_fn(errable_catch_error) {
  bool caught = false;
  catch_errcall(
      (test_divide, (10, 0)), (e, _), ({
        test_assert(!strcmp(e.err_code, "DIV_BY_ZERO"));
        test_assert(e.line > 0);
        test_assert(e.file != NULL);
        caught = true;
      })
  );
  test_assert(caught);
}

test_fn(errable_try_bubble) {
  bool caught = false;
  int res = catch_errcall(
      (test_bubble, (10, 0)), (e, _), ({
        test_assert(!strcmp(e.err_code, "DIV_BY_ZERO"));
        caught = true;
      })
  );
  test_assert(caught);
  int res_success = catch_errcall((test_bubble, (10, 2)), (e, _), (err_panic(e), 0));
  test_assert(res_success == 10);
}

test_fn(errable_call_err_raw) {
  var_ success_res = call_err(test_divide, (10, 2));
  test_assert(!success_res.err.err_code);
  test_assert(success_res.result == 5);

  var_ fail_res = call_err(test_divide, (10, 0));
  test_assert(fail_res.err.err_code);
  test_assert(!strcmp(fail_res.err.err_code, "DIV_BY_ZERO"));
}

test_fn(errable_void_return) {
  bool caught = false;
  catch_errcall(
      (test_void_err, (-1)), (e, _), ({
        test_assert(!strcmp(e.err_code, "NEGATIVE_VOID"));
        caught = true;
        _;
      })
  );
  test_assert(caught);

  bool success_caught = false;
  catch_errcall((test_void_err, (1)), (e, _), success_caught = true);
  test_assert(!success_caught);
}
#endif
#if defined MY_ERRORS_C && MY_ERRORS_C == 1
  #define MY_ERRORS_C (2)
#endif
