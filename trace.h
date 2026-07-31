extern char __executable_start;
// -finstrument-functions
#include "mytypes.h"
#include "sList.h"
// #include "thread_help.h"
#include "macros.h"
#include "print.h"
#include "sglist.h"
#include "ts_int.h"
#include <stdatomic.h>
#include <stdbool.h>
typedef struct {
  void *addr;
} sym_off;
typePrinter(sym_off) {
  PUTS("{");
  USENAMEDPRINTER("ptr", (char *)in.addr - &__executable_start);
  PUTS(",");
  USENAMEDPRINTER("ptr", (char *)in.addr);
  PUTS("}");
}

thread_local static struct {
  _Atomic(bool) dotrace[1];
  sglist(struct {
    void *fn;
    void *site;
    ts_int start;
  }) traceStack;
} traceData = {true, {stdAlloc}};

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (!atomic_exchange(traceData.dotrace, false)) return;
  sglist_push(traceData.traceStack, {this_fn, call_site, now()});
  atomic_store(traceData.dotrace, true);
}
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  if (!atomic_exchange(traceData.dotrace, false)) return;
  let list = &traceData.traceStack;
  list->len -= !!list->len;
  atomic_store(traceData.dotrace, true);
}
struct tracestack_slice {
  usize len;
  ptrstype(arrstype(itypeof(itypeof(typeof(traceData), traceStack), arrays))) * ptr;
};
struct tracestack_slice getTrace(allocfn alloc) {
  atomic_exchange(traceData.dotrace, false);
  let list = &traceData.traceStack;
  let res = (typeof(getTrace(alloc))){list->len};
  if (!res.len) return res;
  res.ptr = *acreate(alloc, typeof(*slice_vla(res)));
  foreach (usize i, range(0, res.len))
    res.ptr[i] = sglist_get(*list, i);
  atomic_store(traceData.dotrace, true);
  return res;
}

void(test)(int recurse) {
  println("Trace inside test():");
  let list = getTrace(stdAlloc);
  defer {
    if (list.len) slice_free(stdAlloc, list);
  };
  foreach (let i, vlap(slice_vla(list)))
    println("fn: {sym_off}, site: {sym_off}, called : {ts_int}", i.fn, i.site, i.start);
  println("personal pointer {ptr}", __builtin_return_address(0));
  println("super pointer {ptr}", __builtin_return_address(1));
  if (recurse) return test(recurse - 1);
}

int main(void) {
  test(1);
  println("Trace back in main():");
  let list = getTrace(stdAlloc);
  defer {
    if (list.len) slice_free(stdAlloc, list);
  };
  foreach (let i, vlap(slice_vla(list)))
    println("fn: {sym_off}, site: {sym_off}, called : {ts_int}", i.fn, i.site, i.start);
}
#include "wheels.h"
