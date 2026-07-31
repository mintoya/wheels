#include "macros.h"
#include "mytypes.h"
usize blog2(usize s) {
#if __has_builtin(__builtin_clzll)
  return 64 - __builtin_clzll(s) - 1;
#else
  usize r = 0;
  while (s) {
    r++;
    s >>= 1;
  }
  return r;
#endif
}
int main(void) {}
