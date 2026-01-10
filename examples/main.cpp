#include "../print.h"
#include "../types.h"
int main(void) {
  // nullable(slice(int)) i = (int[4]){1,2,3,4};
  int sarr[] = {1, 2, 3, 4};
  slice(int) arr = sarr;
  println("{usize}", arr.len);
  return 0;
}
#include "../wheels.h"
