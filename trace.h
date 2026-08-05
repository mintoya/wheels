#ifdef _WIN32
extern char __ImageBase;
#else
extern char __executable_start;
#endif
char *__base_address =
#if defined _WIN32
    &__ImageBase;
#elif defined __linux__
    &__executable_start;
#endif

#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include "sglist.h"
#include "ts_int.h"
#include <stdatomic.h>
#include <stdbool.h>
typedef struct {
  void *addr;
} sym_off;
typePrinter(sym_off) { USENAMEDPRINTER("ptr", (char *)in.addr - __base_address); }

thread_local static struct {
  bool dotrace;
  sglist(struct {
    void *fn;
    void *site;
    ts_int start;
  }) traceStack;
} traceData = {true, {stdAlloc}};

[[gnu::no_instrument_function]]
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (!traceData.dotrace) return;
  traceData.dotrace = false;
  {
    sglist_push(traceData.traceStack, {this_fn, call_site, now()});
  }
  traceData.dotrace = true;
}
[[gnu::no_instrument_function]]
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  if (!traceData.dotrace) return;
  traceData.dotrace = false;
  {
    let list = &traceData.traceStack;
    list->len -= !!list->len;
  }
  traceData.dotrace = true;
}
struct tracestack_slice {
  usize len;
  ptrstype(arrstype(itypeof(itypeof(typeof(traceData), traceStack), arrays))) * ptr;
};
[[gnu::no_instrument_function]]
struct tracestack_slice getTrace(allocfn alloc) {
  let ot = !traceData.dotrace;
  traceData.dotrace = false;
  defer { traceData.dotrace = ot; };
  {
    let list = &traceData.traceStack;
    let res = (typeof(getTrace(alloc))){list->len};
    if (!res.len) return res;
    res.ptr = *acreate(alloc, typeof(*slice_vla(res)));
    foreach (usize i, range(0, res.len))
      res.ptr[i] = sglist_get(*list, i);
    return res;
  }
}
#include <unwind.h>

struct _am_backtrace_state {
  void **current;
  void **end;
};
static _Unwind_Reason_Code _am_unwind_callback(struct _Unwind_Context *context, void *arg) {
  struct _am_backtrace_state *state = (struct _am_backtrace_state *)arg;
  void *ip = (void *)_Unwind_GetIP(context);
  if (ip) {
    if (state->current == state->end) return _URC_END_OF_STACK;
    *state->current++ = ip;
  }
  return _URC_NO_REASON;
}
sliceDef(voidptr);

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
  test(2);
  println("Trace back in main():");
  let list = getTrace(stdAlloc);
  defer {
    if (list.len) slice_free(stdAlloc, list);
  };
  foreach (let i, vlap(slice_vla(list)))
    println("fn: {sym_off}, site: {sym_off}, called : {ts_int}", i.fn, i.site, i.start);
}
#include "wheels.h"
