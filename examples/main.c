#include "../allocators/arenaAllocator.h"
#include "../allocators/debugallocator.h"
#include "../hxmap.h"
#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
int main(void) {
  tu_def(
      (integer, u8),
      (i32, i32),
      (u32, u32),
  );
  var_ i = (integer)tu_of(i32, 1);

  foreach (let i, range(0, 5)) {
  }
}
