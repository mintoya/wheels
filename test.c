#include "debugallocator.h"
#include "my-list.h"
#include "omap.h"
#include "print.h"
#include "printers/slices.c"
#include "stringList.h"
#include "types.h"
#include "vason.h"
#include <malloc.h>
#include <stdcountof.h>
#include <stdio.h>

int main(void) {
  println(
      "ff? {x}\n"
      "fa? {x}\n"
      "af? {x}",
      0xff, 0xfa, 0xaf
  );
  println("{slice:c8}", ((slice(c8)){.len = 7}));
}
#include "wheels.h"
