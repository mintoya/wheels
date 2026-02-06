#include "my-list.h"
#include "print.h"
#include "stringList.h"
#include "types.h"
#include <stdlib.h>

#include "shortList.h"

int main(void) {
  stringList *sl = stringList_new(defaultAlloc, 10);
  println("append");
  stringList_append(sl, fp_from("hello"));
  println("append");
  stringList_append(sl, fp_from("world"));
  println("append");
  stringList_append(sl, fp_from("hello"));
  println("insert");
  stringList_insert(sl, 3, fp_from("hello"));
  println("insert");
  stringList_insert(sl, 2, fp_from("one"));
  println("insert");
  stringList_insert(sl, 0, fp_from("zero"));
  for (int i = 0; i < mList_len(sl->ulist); i++)
    println("{} -> {}", i, stringList_get(sl, i));
}

#include "wheels.h"
