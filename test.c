#include "my-list.h"
#include "print.h"
#include "types.h"
#include <stdlib.h>

#include "shortList.h"

int main(void) {
  usize width = sizeof(int);
  println("new");
  sList_header *l = sList_new(defaultAlloc, 4, width);

  for (int i = 0; i < 10; i++) {
    println("appending {}", i);
    l = sList_append(defaultAlloc, l, width, &i);
  }

  // Modern, readable array-style printing
  print("[ ");
  for (usize i = 0; i < l->length; i++) {
    int val = *(int *)sList_getRef(l, width, i);

    print("{}", (int)val);

    // Add a comma separator, except for the last element
    if (i < l->length - 1) {
      print(", ");
    }
  }
  print(" ]\n");

  return 0;
}

#include "wheels.h"
