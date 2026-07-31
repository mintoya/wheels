// -finstrument-functions
#include "sList.h"
// #include "thread_help.h"
#include "macros.h"
#include "print.h"
#include <stdatomic.h>
#include <stdbool.h>

thread_local static struct {
  msList(struct {
    void *fn;
    void *site;
  }) traceStack;
  _Atomic(bool) dotrace[1];
} traceData = {nullptr, true};

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  let flag = traceData.dotrace;
  if (!atomic_exchange(flag, false)) return;
  defer { atomic_store(flag, true); };
  let list = &traceData.traceStack;
  *list = *list ?: msList_init(stdAlloc, ptrstype(*list), 20);
  msList_push(stdAlloc, *list, {this_fn, call_site});
}
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  let flag = traceData.dotrace;
  if (!atomic_exchange(flag, false)) return;
  defer { atomic_store(flag, true); };
  let list = &traceData.traceStack;
  if (!*list || !msList_len(*list)) return;
  msList_len(*list)--;
}

// slice(ptrstype) getTrace() {}

void(test)(void) {
  println("Trace inside test():");
  let list = traceData.traceStack;
  if (list)
    foreach (let i, vlap(msList_vla(list)))
      println("fn: {ptr}, site: {ptr}", i.fn, i.site);
  println("personal pointer {ptr}", __builtin_return_address(0));
  println("super pointer {ptr}", __builtin_return_address(1));
}

int main(void) {
  test();

  println("Trace back in main():");
  let list = traceData.traceStack;
  if (list)
    foreach (let i, vlap(msList_vla(list)))
      println("fn: {ptr}, site: {ptr}", i.fn, i.site);
  println("final capacity {}", (usize)msList_cap(list));
}
#include "wheels.h"
