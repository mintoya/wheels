#if !defined TS_INT
  #define TS_INT
  #include "print.h"
  #include <threads.h>
  #include <time.h>
typedef struct timespec timespec;
typedef unsigned _BitInt(
    (sizeof(itypeof(struct timespec, tv_sec))) * 8 +
    30
) ts_int;
CONST_EXPR ts_int ts_int_ns = 1;
CONST_EXPR ts_int ts_int_us = ts_int_ns * 1000;
CONST_EXPR ts_int ts_int_ms = ts_int_us * 1000;
CONST_EXPR ts_int ts_int_s = ts_int_ms * 1000;
CONST_EXPR ts_int ts_int_min = ts_int_s * 60;
CONST_EXPR ts_int ts_int_hour = ts_int_min * 60;
CONST_EXPR ts_int ts_int_day = ts_int_hour * 24;
static inline ts_int timespec_int(timespec in) { return ((ts_int)in.tv_sec * ts_int_s) + in.tv_nsec; }
static inline timespec int_timespec(ts_int in) { return (timespec){(itypeof(timespec, tv_sec))(in / ts_int_s), (itypeof(timespec, tv_nsec))(in % ts_int_s)}; }
static inline ts_int now() {
  var_ ts = (struct timespec){};
  assertMessage(timespec_get(&ts, TIME_MONOTONIC) == TIME_MONOTONIC);
  return timespec_int(ts);
}
typePrinter(ts_int) {
  if (!in) {
    PUTS("{0}");
    return;
  }
  CONST_EXPR ts_int ys = ts_int_day * 365;
  PUTS("{");
  #pragma push_macro("PRINT_TIME_FMT")
  #define PRINT_TIME_FMT(name, var)    \
    if_decl (var_ t, in / var) {       \
      PUTS(name ":");                  \
      USETYPEPRINTER(usize, (usize)t); \
      if (in %= var)                   \
        PUTS(",");                     \
    }
  PRINT_TIME_FMT("y", ys);
  PRINT_TIME_FMT("d", ts_int_day);
  PRINT_TIME_FMT("h", ts_int_hour);
  PRINT_TIME_FMT("m", ts_int_min);
  PRINT_TIME_FMT("s", ts_int_s);
  PRINT_TIME_FMT("ms", ts_int_ms);
  PRINT_TIME_FMT("us", ts_int_us);
  PRINT_TIME_FMT("ns", ts_int_ns);
  #pragma pop_macro("PRINT_TIME_FMT")
  PUTS("}");
}
typePrinter(timespec) { USETYPEPRINTER(ts_int, timespec_int(in)); }
#endif
