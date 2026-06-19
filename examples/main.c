#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../hhmap.h"
#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
#include "wheels/tu_macros.h"
int main(void) {
  tu_def(
      (integer, u8),
      (i32, i32),
      (u32, u32),
  );
  var_ i = (integer)tu_of(i32, 1);
}
