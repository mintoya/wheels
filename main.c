#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include <stdio.h>
#include <threads.h>
#include <time.h>

typedef _BitInt(sizeof(struct timespec) * 8) time_int;

typePrinter(time_int) {
  USETYPEPRINTER(fptr, ((fptr){sizeof(in), (u8 *)&in}));
  USENAMEDPRINTER("ptr", (usize)in);
  usize ns = in % (time_int)1E9;

  constexpr usize minuteSeconds = 60;
  constexpr usize hourSeconds = minuteSeconds * 60;
  constexpr usize daySeconds = hourSeconds * 24;
  constexpr usize yearSeconds = daySeconds * 365;

  usize s = in / (time_int)1E9;
  usize y = s / yearSeconds;
  s %= yearSeconds;
  usize d = s / daySeconds;
  s %= daySeconds;
  usize h = s / hourSeconds;
  s %= hourSeconds;
  usize m = s / minuteSeconds;
  s %= minuteSeconds;
  PUTS("{y:");
  USETYPEPRINTER(usize, y);
  PUTS(",d:");
  USETYPEPRINTER(usize, d);
  PUTS(",h:");
  USETYPEPRINTER(usize, h);
  PUTS(",m:");
  USETYPEPRINTER(usize, m);
  PUTS(",s:");
  USETYPEPRINTER(usize, s);
  PUTS("}");
}

time_int timespec_toi(struct timespec x) { return (time_int)x.tv_nsec + (time_int)x.tv_sec * (time_int)1E9; }

time_int timespec_now() {
  var_ res = (struct timespec){};
  timespec_get(&res, TIME_UTC);
  return timespec_toi(res);
}

int main(void) {
  println("{time_int}", timespec_now());
  while (1) {
    println("{time_int}", timespec_now());
    thrd_sleep(&(struct timespec){1}, nullptr);
  }
}
#include "wheels.h"
