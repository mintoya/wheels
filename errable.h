#include "limits.h"
#include "wheels/mytypes.h"
#include <stdio.h>
#include <stdlib.h>
#define err(T)               \
  struct err_##T {           \
    const char *const ecode; \
    T result;                \
  }
#define err_return(T, v) return ((err(T)){nullptr, v})
#define err_throw(T, msg) return ((err(T)){              \
    "[ERROR] " msg "from \n\t" STRFRY(__LINE__) __FILE__ \
})
#define err_panic(v) ({      \
  let _v = v;                \
  if_unlikely (_v.ecode) {   \
    fputs(_v.ecode, stderr); \
    exit(1);                 \
  };                         \
  _v.result;                 \
})
// if(err!=nil)
#define err_inn(T, t) ({       \
  let _x = t;                  \
  if (_x.ecode)                \
    return (err(T)){_x.ecode}; \
  _x.result;                   \
})

err(int) countup(int i) {
  if (i != INT_MAX)
    err_return(int, i + 1);
  err_throw(int, "integer overflow");
}
void countupall(void) {
  int i = INT_MAX - 2;
  while (1)
    i = err_panic(countup(i));
}
err(nothing_t) countupallfailable(void) {
  int i = INT_MAX - 2;
  while (1)
    i = err_inn(nothing_t, countup(i));
}
