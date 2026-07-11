#include "allocator.h"
#include "macros.h"
#include "mytypes.h"
typedef struct preslice {
  usize len;
  void *ptr;
} preslice;
#define mslice(t) ptrof( \
    fnptrof(             \
        (preslice),      \
        t                \
    )                    \
)

#define mslice_init(allocator, T) (mslice(T)) aCreate(allocator, preslice)
#define mslice_deinit(allocator, s) aFree(allocator, s, sizeof(preslice))

int main(void) {
  var_ x = mslice_init(stdAlloc, int);
  defer { mslice_deinit(stdAlloc, x); };
  var_ x2 = mslice_init(stdAlloc, int);
  defer { mslice_deinit(stdAlloc, x); };
  x = x2;
}
