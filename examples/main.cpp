#include "../allocator.h"
#include "../hhmap.h"
#include "../my-list.h"
int main(void) {
  mHmap(int, int) j = mHmap_init(defaultAlloc, int, int);
  mHmap_set(j, 6, 7);
}
