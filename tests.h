#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define MY_TEST_FRAMEWORK_C (1)
#endif

#include "macros.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((format(printf, 1, 2))) char *aprint(const char *fmt, ...);

#define test_assert(...)            \
  do {                              \
    if (!(__VA_ARGS__)) {           \
      *_result =                    \
          (test_result){            \
              (char *)#__VA_ARGS__, \
              __LINE__ + 1          \
          };                        \
      return;                       \
    }                               \
  } while (0)

#define test_inteq(a, b)                \
  do {                                  \
    ptrdiff_t _a = a;                   \
    ptrdiff_t _b = b;                   \
    if (_a != _b) {                     \
      *_result = (test_result){         \
          aprint("%td != %td", _a, _b), \
          __LINE__ + 1                  \
      };                                \
      return;                           \
    }                                   \
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
} test_result;
  #define test_fn(name)              \
    [[maybe_unused]] void ID_CONCAT( \
        ID_CONCAT(                   \
            testing_function__, name \
        ),                           \
        __COUNTER__                  \
    )(test_result * _result, AllocatorV allocator)
#elif defined MY_TEST_FRAMEWORK_C && MY_TEST_FRAMEWORK_C == (1)
  #undef MY_TEST_FRAMEWORK_C
  #define MY_TEST_FRAMEWORK_C (2)

typedef struct test_result {
  char *check;
  size_t result;
} test_result;
  #include "allocator.h"
  #include "macros.h"

struct testNode {
  c8 *filename;
  c8 *testname;
  fnptrof((test_result *, AllocatorV), void) fn;
  struct testNode *next;
}
    *testList = nullptr;

  #define test_fn(name)                     \
    void name(test_result *, AllocatorV);   \
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
    void name(test_result *_result, AllocatorV allocator)
/*
test_fn(always_pass) {
  var_ memory = &aCreate(allocator, int, 5);
  aFree(allocator, memory, sizeof(*memory));
}
test_fn(always_fail) { test_assert(false); }
test_fn(always_leak) {
  aCreate(allocator, int);
}
*/

  #include "stdio.h"
  #define test_RESET "\x1b[0m"
  #define test_RED "\x1b[31m"
  #define test_GREEN "\x1b[32m"
  #include "allocators/debugallocator.h"
  #include "print.h"
void onalloc(allocationType *t) {
  printf("\t%p %zu -> %p %zu : %zu %s\n", t->iptr, t->insize, t->optr, t->outsize, t->trace.ln, t->trace.fn);
}

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
int main(void) {
  usize count = 0;
  usize pass = 0;
  while (testList) {
    AllocatorV testAlloc = debugAllocator(
            .allocator = stdAlloc,
    // .log = stdout,
  #if defined(LOG_ALLOCATIONS)
            .on_call = onalloc
  #endif
    );
    count++;
    var_ result = (test_result){};
    testList->fn(&result, testAlloc);
    int leaked = debugAllocatorDeInit(testAlloc);
    printf(
        "[%s%s] %s",
        result.result
            ? test_RED "FAIL" test_RESET
            : test_GREEN "PASS" test_RESET,
        leaked ? test_RED ",LEAK" test_RESET : "",
        testList->testname
    );
    if (result.result) printf(
        "\n"
        "\tline\t:%zu\n"
        "\tfile\t:%s\n"
        "\tcond\t:(%s)\n",
        result.result - 1,
        testList->filename,
        result.check
    );
    else printf("\n");
    fflush(stdout);
    pass += !(result.result) && !leaked;
    testList = testList->next;
  }
  printf("%zu tests out of %zu passed", pass, count);
}
  #if !defined __cplusplus && __STDC_VERSION__ >= 202400L
    #include "funct.h" // excluded from  include all for of c23
  #endif
  #define WHEELS_INCLUDE_ALL
  #include "wheels.h"
#endif
