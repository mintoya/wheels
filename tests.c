#pragma once
#include "allocator.h"
#include "allocators/debugallocator.h"
#include "assertMessage.h"
#include "macros.h"
#include "mytypes.h"
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

void onalloc(allocationType *t) {
  printf("\t%p %zu -> %p %zu : %zu %s\n", t->iptr, t->insize, t->optr, t->outsize, t->trace.ln, t->trace.fn);
}
int main(void) {
  signal(SIGSEGV, segv_handler);
  signal(SIGABRT, segv_handler); // sent by my assert depending on #defines
  signal(SIGILL, segv_handler);
  // sent by my assert depending on #defines

  volatile usize failCount = 0; // Marked volatile to survive longjmp

  foreach (var_ test, vlap(VLAP(testList, testCount))) {
    current_test = test.name;
    printf("test %s: {\n", test.name);
    fflush(stdout);

    AllocatorV testAlloc = debugAllocator(.allocator = stdAlloc, .on_call = onalloc);
    volatile bool failed = false;

    if (!setjmp(test_jump_env)) {
      int res = test.function(testAlloc);
      printf("}\n");

      if (res) {
        printf(ASSERTMESSAGE_PRINTRED "test %s failed with %i\n" ASSERTMESSAGE_PRINTRESET, test.name, res);
        fflush(stdout);
        failed = true;
      }

    } else {
      failed = true;
    }

    int leaks = debugAllocatorDeInit(testAlloc);
    if (leaks && !failed) {
      printf(ASSERTMESSAGE_PRINTRED "test %s failed: leaked %i allocation(s)\n" ASSERTMESSAGE_PRINTRESET, test.name, leaks);
      fflush(stdout);
      failed = true;
    }
    failCount += failed;
  }

  printf("%zu tests failed out of %zu\n", failCount, testCount);
  aFree(stdAlloc, testList, testCount * sizeof(*testList));

  return failCount > 0 ? 1 : 0;
}
#define WHEELS_INCLUDE_ALL (1)
#if !defined __cplusplus
  #include "funct.h"
#endif
#include "wheels.h"
