#include "my-list.h"
#include "print.h"
// sorts based on distance from favorite number now
// example use of ctx

bool(int_sortFunc)(int *f, int *a, int *b) {
  return *a < *b;
};
int main(void) {
  mList_scoped(i32) ilist = mList_init(defaultAlloc, i32);
  mList_pushArr(ilist, ((int[10]){1, 2, 3, 5, 1, 3, 4, 0, -100, -3}), 10);
  List_qsort((List *)ilist, (List_searchFunc)int_sortFunc, (i32[1]){3});
  slice(i32) intslice = (typeof(intslice)){mList_len(ilist), mList_arr(ilist)};
  for (each_slice(intslice, int32)) {
    println("{}", *int32);
  }
}
#include "wheels.h"
