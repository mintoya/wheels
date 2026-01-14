#include "../print.h"
#include "../wheels.h"
typedef struct {
  int x;
  int y;
} point;
REGISTER_PRINTER(point, {
  PUTS(L"{x:");
  USETYPEPRINTER(int, in.x);
  PUTS(L",");
  PUTS(L"y:");
  USETYPEPRINTER(int, in.y);
  PUTS(L"}");
})

#include "../printer/genericName.h" // advances typenaems
MAKE_PRINT_ARG_TYPE(point);

int main() {
  mList(point) points = mList_init(defaultAlloc, point);
  mList_push(points, ((point){6, 1}));
  mList_push(points, ((point){0, 2}));
  mList_ins(points, 1, ((point){1, 0}));
  mList_foreach(points, point, p, {
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
