#pragma once
#include "allocator.h"
#include "assertMessage.h"
#include "fbaAllocator.h"
#include "macros.h"
#include "mytypes.h"
#include "stdio.h"
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

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
  static int(testname)(AllocatorV allocator) { __VA_ARGS__ }          \
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
  longjmp(test_jump_env, 1);
}

int main(void) {
  signal(SIGSEGV, segv_handler);
  signal(SIGABRT, segv_handler); // sent by my assert depending on #defines
  signal(SIGILL, segv_handler);  // sent by my assert depending on #defines
  usize failCount = 0;

  foreach (var_ test, vla(*VLAP(testList, testCount))) {
    current_test = test.name;
    printf("test %s: {\n", test.name);
    fflush(stdout);
    if (!setjmp(test_jump_env)) {
      int res = test.function(stdAlloc);
      printf("}\n");
      bool failed = false;
      if (res) {
        printf(ASSERTMESSAGE_PRINTRED "test %s failed with %i\n" ASSERTMESSAGE_PRINTRESET, test.name, res);
        fflush(stdout);
        failed = 1;
      }
      failCount += failed;
    } else {
      failCount++;
    }
  }
  printf("%lu tests failed out of %lu", failCount, testCount);
  aFree(stdAlloc, testList, testCount * sizeof(*testList));
}
#define WHEELS_INCLUDE_ALL (1)
#include "wheels.h"
