#include "my-list.h"
#include "print.h"
#include "types.h"
#include <stdlib.h>

#define STRING_LIST_C (1)
#include "stringList.c"

int main(void) {
  stringList *sl = stringList_new(defaultAlloc, 10);
  println("null ?== {}", stringList_get(sl, 0));
  stringList_append(sl, fp_from("hello world "));
  stringList_append(sl, fp_from("hello world long "));
  stringList_append(sl, fp_from("hello "));
  println("set {}", stringList_set(sl, 0, fp_from("hellow")));
  println("set {}", stringList_insert(sl,1,fp_from("one")));

  for (int i = 0; i < mList_len(sl->ulist); i++)
    println("{} -> {}", i, stringList_get(sl, i));
  println("byte count: {}", sl->len);
  println("capacity : {}", sl->cap);

  stringList_free(sl);
  return 0;
}

#include "wheels.h"
