#include "../allocators/arenaAllocator.h"
#include "../allocators/debugallocator.h"

#include "../bigint.h"
#include "../wheels.h"

int main(void) {
  AllocatorV debug = debugAllocator(
          .allocator = stdAlloc,
          .log = stdout
  );

  defer { debugAllocatorDeInit(debug); };
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from.i64(debug, 1024);
    defer { msList_deInit(debug, a); };
    bigint b = BInt.negate(debug, a);
    defer { msList_deInit(debug, b); };
    println(
        "{bigint:both} * -1 = {bigint:both}",
        a,
        b
    );
  }
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from.i64(debug, 1024);
    defer { msList_deInit(debug, a); };
    bigint b = BInt.from.i64(debug, -1024);
    defer { msList_deInit(debug, b); };
    println(
        "{bigint:both} * {bigint:both} = {bigint:both}",
        a,
        b,
        BInt.mul(debug, a, b)
    );
  }
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from.i64(debug, 409500);
    defer { msList_deInit(debug, a); };
    bigint b = BInt.from.i64(debug, 16);
    defer { msList_deInit(debug, b); };
    var_ c = BInt.div(debug, a, b);
    defer { msList_deInit(debug, c.div); };
    defer { msList_deInit(debug, c.mod); };
    println(
        "{bigint:both}//{bigint:both} = {bigint:both} ,{bigint:both} ",
        a,
        b,
        c.div,
        c.mod
    );
  }
  bigint a = BInt.from.i64(debug, 1);
  bigint b = BInt.from.i64(debug, 2);
  // 1 1 2 3 5
  int i = 3;
  while (1) {
    bigint c = bigint_add(debug, a, b);
    i++;
    println("{bigint} {}", c, i);
    msList_deInit(debug, a);
    a = b;
    b = c;
  }
}
