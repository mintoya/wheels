#pragma once
#include "allocator.h"
#include "assertMessage.h"
#include "fbaAllocator.h"
#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
#include "stdio.h"
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

typedef int (*testerFunction)(AllocatorV); // required to clean up after itself
struct testItem {
  testerFunction function;
  c8 *name;
};
usize testCount;
struct testItem *testList = nullptr;
void testPush_a() { testCount++; }
void testPush_b(testerFunction t, c8 *name) {
  testList[testCount++] = (struct testItem){t, name};
}

[[gnu::constructor(302)]] void initTestList() {
  testList = aCreate(stdAlloc, struct testItem, testCount);
  testCount = 0;
}
#define MAKE_TEST_CONCAT_(a, b) a##b
#define MAKE_TEST_CONCAT(a, b) MAKE_TEST_CONCAT_(a, b)
#define MAKE_TEST_FN(testname, ...)                                   \
  int(testname)(AllocatorV allocator) { __VA_ARGS__ }                 \
  [[gnu::constructor(301)]] void                                      \
  MAKE_TEST_CONCAT(test_register_a_, __COUNTER__)() { testPush_a(); } \
  [[gnu::constructor(304)]] void                                      \
  MAKE_TEST_CONCAT(test_register_b_, __COUNTER__)() { testPush_b(testname, (c8 *)#testname); }

static char *current_test = 0;

static jmp_buf test_jump_env;

void segv_handler(int sig) {
  fflush(stdout);
  printf(
      ASSERTMESSAGE_PRINTRED
      "\ntest  %s threw an exception\n" ASSERTMESSAGE_PRINTRESET,
      current_test
  );
  fflush(stdout);
  signal(sig, segv_handler);
  siglongjmp(test_jump_env, 1);
}

MAKE_TEST_FN(hello, { printf("hello test\n");return 0; });
MAKE_TEST_FN(allocator_test, {
  int *i = aCreate(allocator, int, 5);
  defer { aFree(allocator, i); };
  int *i2 = aCreate(allocator, int, 8);
  defer { aFree(allocator, i2); };
  _fba_print(allocator);
  for (int ii = 0; ii < 5; ii++)
    i[ii] = ii * ii;
  for (int ii = 0; ii < 5; ii++)
    if (i[ii] != ii * ii)
      return 1;
  return 0;
});
// MAKE_TEST_FN(segFault, {
//   int i = *(int *)NULL;
// });

int main(void) {
  signal(SIGSEGV, segv_handler);
  signal(SIGABRT, segv_handler);
  signal(SIGILL, segv_handler);
  AllocatorV allocator = FBA_static(1 << 20);
  usize failCount = 0;
  foreach (struct testItem test, VLAP(testList, testCount)) {
    current_test = test.name;
    printf("test %s: {\n", test.name);
    fflush(stdout);
    if (!sigsetjmp(test_jump_env, 1)) {
      int res = test.function(allocator);
      printf("}\n");
      bool failed = false;
      if (res) {
        printf(ASSERTMESSAGE_PRINTRED "test %s failed\n" ASSERTMESSAGE_PRINTRESET, test.name);
        fflush(stdout);
        failed = 1;
      }
      if (FBA_current(allocator)) {
        printf(ASSERTMESSAGE_PRINTORANGE "test %s leaked\n", test.name);
        printf("memory layout : {\n");
        _fba_print(allocator);
        printf("}\n" ASSERTMESSAGE_PRINTRESET);
        fflush(stdout);
        failed = 1;
      }
      failCount += failed;
    } else {
      failCount++;
    }
    FBA_reset(allocator);
  }
  printf("%lu tests failed out of %lu", failCount, testCount);
}
#define WHEELS_INCLUDE_ALL (1)
#include "wheels.h"
