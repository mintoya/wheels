#define ENABLE_ERROR_IN_NORMAL_FUNCTION
#include "../errable.h"
#include "../print.h"
#include "stdckdint.h"
#include <limits.h>

int add1 errs(int, int);
int add1 errs(int i, int j) {
  println("hello from errable, given {} and {}", i, j);
  defer { println("bye"); };
  int res = 0;
  if (ckd_add(&res, i, j))
    return_err("INTEGER_FLOW");
  return res;
}
void adds errs() {
  let i = try_err(add1, (INT_MIN, -1));
}
int main(void) {
  catch_errcall(add1, (INT_MAX, 1), (e) { //
    if (!strcmp(e.err_code, "INTEGER_FLOW"))
      println("cought integer overflow");
    else err_panic(e);
  });
  try_err(adds, (), 1);
  return 0;
}
#include "../wheels.h"
