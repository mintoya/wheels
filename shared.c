#include "macros.h"
#include "mytypes.h"
typedef struct shared_ptr {
  usize refcount;
  void *ptr;
} shared_ptr;

#define Shared(T) ptrof(fnptrof((shared_ptr *), T))

Shared(u32) x;
