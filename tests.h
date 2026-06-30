#if defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0
  #define MY_TEST_FRAMEWORK_C (1)
#endif

#if !defined MY_TEST_FRAMEWORK_H && !defined MY_TEST_FRAMEWORK_C
  #include "allocator.h"
  #include "macros.h"
  #define MY_TEST_FRAMEWORK_H (1)
  #define test_fn(name)                  \
    [[maybe_unused]] int ID_CONCAT(      \
        ID_CONCAT(                       \
            testing_function__, __LINE__ \
        ),                               \
        __COUNTER__                      \
    )(AllocatorV allocator)
#elif defined MY_TEST_FRAMEWORK_C && MY_TEST_FRAMEWORK_C == (1)
  #undef MY_TEST_FRAMEWORK_C
  #define MY_TEST_FRAMEWORK_C (2)
  #include "allocator.h"
  #include "macros.h"

struct testNode {
  c8 *testname;
  fnptrof((AllocatorV), int) fn;
  struct testNode *next;
} *testList = nullptr;

  #define test_fn(name)                   \
    int name(AllocatorV);                 \
    [[gnu::constructor]] static void      \
    name##testfunctoin##_register(void) { \
      static struct testNode thisNode =   \
          (typeof(thisNode)){             \
              .testname = (char *)#name,  \
              .fn = name,                 \
          };                              \
      thisNode.next = testList;           \
      testList = &thisNode;               \
    }                                     \
    int name(AllocatorV allocator)

test_fn(always_pass) {
  var_ memory = &aCreate(allocator, int, 5);
  aFree(allocator, memory, sizeof(*memory));
  return 0;
}
test_fn(always_fail) { return 1; }
test_fn(always_leak) {
  aCreate(allocator, int);
  return 0;
}

  #include "stdio.h"
  #define test_RESET "\x1b[0m"
  #define test_RED "\x1b[31m"
  #define test_GREEN "\x1b[32m"
  #include "allocators/debugallocator.h"
void onalloc(allocationType *t) {
  printf("\t%p %zu -> %p %zu : %zu %s\n", t->iptr, t->insize, t->optr, t->outsize, t->trace.ln, t->trace.fn);
}

int main(void) {
  usize count = 0;
  usize pass = 0;
  while (testList) {
    AllocatorV testAlloc = debugAllocator(
            .allocator = stdAlloc,
  #if defined(LOG_ALOCATIONS)
            .on_call = onalloc
  #endif
    );
    count++;
    var_ result = testList->fn(testAlloc);
    int leaked = debugAllocatorDeInit(testAlloc);
    printf(
        "[%s%s] %s\t: %i\n",
        result
            ? test_RED "FAIL" test_RESET
            : test_GREEN "PASS" test_RESET,
        leaked ? test_RED ",LEAK" test_RESET : "",
        testList->testname,
        result
    );
    fflush(stdout);
    pass += !result && !leaked;
    testList = testList->next;
  }
  printf("%zu tests out of %zu passed", pass, count);
}
  #include "funct.h" // excluded from  include all for of c23
  #define WHEELS_INCLUDE_ALL
  #include "wheels.h"
#endif
