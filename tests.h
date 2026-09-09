#define _GNU_SOURCE
#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define MY_TEST_FRAMEWORK_C (1)
#endif

#include "macros.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((format(printf, 1, 2))) char *aprint(const char *fmt, ...);

#define test_assert(...)                          \
  do {                                            \
    if (!(__VA_ARGS__)) {                         \
      *_result =                                  \
          (test_result){                          \
              aprint("%s", (char *)#__VA_ARGS__), \
              __LINE__ + 1                        \
          };                                      \
      return;                                     \
    }                                             \
  } while (0)

#define test_inteq(a, b)                  \
  do {                                    \
    signed long long _a = a;              \
    signed long long _b = b;              \
    if (_a != _b) {                       \
      *_result = (test_result){           \
          aprint("%lld != %lld", _a, _b), \
          __LINE__ + 1                    \
      };                                  \
      return;                             \
    }                                     \
  } while (0)

#define test_streq(a, b)              \
  do {                                \
    char *_a = a;                     \
    char *_b = b;                     \
    if (strcmp(_a, _b)) {             \
      *_result = (test_result){       \
          aprint("%s != %s", _a, _b), \
          __LINE__ + 1                \
      };                              \
      return;                         \
    }                                 \
  } while (0)

#define test_fpeq(a, b)         \
  do {                          \
    let _a = fp(a);             \
    let _b = fp(b);             \
    if (!fptr_eq(_a, _b)) {     \
      *_result = (test_result){ \
          aprint(               \
              "%.*s != %.*s",   \
              (int)_a.len,      \
              _a.ptr,           \
              (int)_b.len,      \
              _b.ptr            \
          ),                    \
          __LINE__ + 1          \
      };                        \
      return;                   \
    }                           \
  } while (0)
#if !defined MY_TEST_FRAMEWORK_H && !defined MY_TEST_FRAMEWORK_C
  #define MY_TEST_FRAMEWORK_H (1)
  #include "allocator.h"
  #include "macros.h"
typedef struct test_result {
  char *check;
  size_t result;
  char profile;
} test_result;
  #define test_fn(name)                            \
    [[maybe_unused]] static inline void ID_CONCAT( \
        ID_CONCAT(                                 \
            testing_function__, name               \
        ),                                         \
        __COUNTER__                                \
    )(test_result * _result, allocfn allocator)
#elif defined MY_TEST_FRAMEWORK_C && MY_TEST_FRAMEWORK_C == (1)
  #undef MY_TEST_FRAMEWORK_C
  #define MY_TEST_FRAMEWORK_C (2)

typedef struct test_result {
  char *check;
  size_t result;
  char profile;
} test_result;
  #include "allocator.h"
  #include "macros.h"

struct testNode {
  c8 *filename;
  c8 *testname;
  fnptrof((test_result *, allocfn), void) fn;
  struct testNode *next;
}
    *testList = nullptr;

  #define test_fn(name)                     \
    void name(test_result *, allocfn);      \
    [[gnu::constructor]] static void        \
    name##testfunctoin##_register(void) {   \
      static struct testNode thisNode =     \
          (typeof(thisNode)){               \
              .filename = (char *)__FILE__, \
              .testname = (char *)#name,    \
              .fn = name,                   \
          };                                \
      if (!testList) {                      \
        testList = &thisNode;               \
        return;                             \
      }                                     \
      var_ n = testList;                    \
      while (n->next)                       \
        n = n->next;                        \
      n->next = &thisNode;                  \
    }                                       \
    void name(test_result *_result, allocfn allocator)

  #include "stdio.h"
  #define test_RESET "\x1b[0m"
  #define test_RED "\x1b[31m"
  #define test_GREEN "\x1b[32m"

__attribute__((format(printf, 1, 2))) char *aprint(const char *fmt, ...) {
  let l = (va_list){};
  let l2 = (va_list){};
  va_start(l, fmt);
  va_copy(l2, l);

  let len = vsnprintf(0, 0, fmt, l);
  va_end(l);

  let res = (char *)malloc((len + 1) * sizeof(char));

  vsnprintf(res, len + 1, fmt, l2);
  va_end(l2);

  return res;
}
  #include "allocators/debugallocator.h"
test_result runtest(typeof(testList) test) {
  allocfn testAlloc = debugAllocator(.allocator = stdAlloc);
  defer { debugAllocatorDeInit(testAlloc); };

  var_ result = (test_result){};
  test->fn(&result, testAlloc);
  let leaked = 0;
  if (result.profile) {
    let stats = debugAllocator_stats(testAlloc);
    printf("{" test_RED "prof" test_RESET "}");
    _dbga_stats_printer(
        (fptr){sizeof(stats), (u8 *)&stats},
        (printerfunction_context){fileprint, stdout, {}, stdAlloc}
    );
    fileprint("\n", stdout, 1, 1);
  }
  foreach (let location, vtable(debugallocator_iterator, testAlloc)) {
    leaked++;
    printf(
        "\t(" test_RED "leak" test_RESET ") %zu bytes\n"
        "\t\tfile:%s\n"
        "\t\tline:%zu\n",
        location.trace.size,
        location.trace.fn,
        location.trace.ln
    );
  }
  if (result.result) printf(
      "\n"
      "\tline\t:%zu\n"
      "\tfile\t:%s\n"
      "\tcond\t:(%s)\n",
      result.result - 1,
      test->filename,
      result.check
  );
  if (result.check) free(result.check);
  fflush(stdout);
  result.result += !!leaked;
  return result;
}
test_result runtest_named(const char *test) {
  let curr = testList;
  while (curr && strcmp(curr->testname, test))
    curr = curr->next;
  assertMessage(curr);
  return runtest(curr);
}
  // #define TESTS_SUBPROCESSES (1) // run all tests in their own subprocess
  #if (defined(TESTS_SUBPROCESSES) && (TESTS_SUBPROCESSES == 1))
    #include "deps/subprocess.h/subprocess.h"
  #endif
int main(int nargs, char **args) {
  if (nargs == 2)
    return (runtest_named(args[1]).result);
  usize count = 0;
  usize pass = 0;

  #if (defined(TESTS_SUBPROCESSES) && (TESTS_SUBPROCESSES == 1))
  while (testList) {
    count++;
    int status = 0;
    struct subprocess_s sub;
    subprocess_create((const char *const[]){args[0], testList->testname, nullptr}, 0, &sub);
    subprocess_join(&sub, &status);

    typeof(char[1024]) buf = {};
    int count = 0;

    while ((count = subprocess_read_stdout(&sub, buf, sizeof(buf))))
      fwrite(buf, sizeof(buf[0]), count, stdout);
    while ((count = subprocess_read_stderr(&sub, buf, sizeof(buf))))
      fwrite(buf, sizeof(buf[0]), count, stderr);

    fflush(stdout);
    fflush(stderr);

    subprocess_destroy(&sub);

    printf(
        "[%s] %s\t%i\n",
        status
            ? test_RED "FAIL" test_RESET
            : test_GREEN "PASS" test_RESET,
        testList->testname,
        status
    );
    pass += !status;
    testList = testList->next;
  }
  #else
  while (testList) {
    count++;
    int status = runtest(testList).result;
    printf(
        "[%s] %s\n",
        status
            ? test_RED "FAIL" test_RESET
            : test_GREEN "PASS" test_RESET,
        testList->testname
    );
    pass += !status;
    testList = testList->next;
  }
  #endif
  printf("%zu tests out of %zu passed", pass, count);
  return 0;
}
  #if !defined __cplusplus && __STDC_VERSION__ >= 202400L
    #include "funct.h" // excluded from  include all for of c23
  #endif
  #define WHEELS_INCLUDE_ALL
  #include "wheels.h"
#endif
