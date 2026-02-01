#include "my-list.h"
#include "print.h"

#define STRING_LIST_C (1)
#include "stringList.c"

int main(void) {
  stringList *sl = stringList_new(defaultAlloc, 10);
  println("null ?== {}", stringList_get(sl, 0));
  stringList_append(sl, fp_from("hello world"));
  stringList_append(sl, fp_from("hello world long "));
  stringList_append(sl, fp_from("hello "));
  stringList_insert(sl, 0, fp_from("inserted zero'th slot, actual data will be at the end of the buffer"));
  stringList_insert(sl, 1, fp_from("inserted really really long string in the second slot, this should be long enough to trigger the long encoding, some more text, finally its > 128 bytes (150) "));
  stringList_set(sl, 2, fp_from("set in the third slot, will actually be at the end of the buffer"));
  stringList_set(sl, 3, fp_from("short to fit in"));

  for (int i = 0; i < mList_len(sl->ulist); i++)
    println("{} -> {}", i, stringList_get(sl, i));
  println("byte count: {}", sl->len);
  println("capacity : {}", sl->cap);
  usize len = sl->len;
  usize stringLen = 0;
  for (int i = 0; i < mList_len(sl->ulist); i++)
    stringLen += stringList_get(sl, i).width;
  println("{}% string to data", stringLen * 100 / len);
  println("{}% string to footprint", stringLen * 100 / stringList_footprint(sl));

  stringList_free(sl);
  return 0;
}

#include "wheels.h"
