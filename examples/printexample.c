#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
typedef struct {
  int x;
  int y;
} point;
REGISTER_PRINTER(point, {
  PUTS("{x:");
  USETYPEPRINTER(isize, in.x);
  PUTS(",");
  PUTS("y:");
  USETYPEPRINTER(isize, in.y);
  PUTS("}");
})

int main() {
  mList(point) points = mList_init(stdAlloc, point);
  mList_push(points, ((point){6, 1}));
  mList_push(points, ((point){0, 2}));
  mList_ins(points, 1, ((point){1, 0}));
  for_each_((var_ p, mList_vla(points)), {
    println("foreach : ${}", p);
  });
  println("length  : ${int}\n"
          "capacity: ${int}",
          mList_len(points), mList_cap(points));
  unsigned int i;
  println("no type test : ${}", i);
  println("no type test2: ${thingy}", i);
  println("missing parameter test: ${}");
  return 0;
}
