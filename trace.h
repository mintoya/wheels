#if !defined MY_TRACE_H
  #define MY_TRACE_H (1)

  #if defined _WIN32
extern char __ImageBase;
  #else
extern char __executable_start;
  #endif

char *__base_address =
  #if defined _WIN32
    &__ImageBase
  #elif defined __linux__
    &__executable_start
  #else
    0
  #endif
    ;

  #include "macros.h"
  #include "mytypes.h"
  #include "print/print_pre.h"
  #include "sglist.h"

extern sglist(struct {
  void *fn;
  void *site;
}) traceData;
struct tracestack_slice {
  usize len;
  ptrstype(arrstype(itypeof(typeof(traceData), arrays))) * ptr;
};
typedef struct {
  void *addr;
} sym_off;
typePrinter(sym_off) { USENAMEDPRINTER("ptr", (char *)in.addr - __base_address); }

[[gnu::no_instrument_function]]
struct tracestack_slice getTrace(allocfn alloc);

#endif
#if (defined MY_TRACE_C && MY_TRACE_C == 1) || \
    (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)
  #undef MY_TRACE_C
  #define MY_TRACE_C (2)
typeof(traceData) traceData = {stdAlloc};
[[gnu::destructor(500)]]
void rtrace() {sglist_deinit(traceData);}
bool trace_dotrace = true;
[[gnu::no_instrument_function]]
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (trace_dotrace) trace_dotrace = false;
  else return;
  sglist_push(traceData, {this_fn, call_site});
  trace_dotrace = true;
}
[[gnu::no_instrument_function]]
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  if (trace_dotrace) trace_dotrace = false;
  else return;
  traceData.len -= !!traceData.len;
  trace_dotrace = true;
}
[[gnu::no_instrument_function]]
struct tracestack_slice getTrace(allocfn alloc) {
  let res = (typeof(getTrace(alloc))){traceData.len};
  if (!res.len) return res;
  res.ptr = *acreate(alloc, typeof(*slice_vla(res)));
  foreach (usize i, range(0, res.len))
    res.ptr[i] = sglist_get(traceData, i);
  return res;
}
#endif
