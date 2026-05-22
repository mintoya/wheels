
#include "hhmap.h"
#include "macros.h"
#include "print.h"

int main() {
  auto imap = mHmap_init(stdAlloc, i32, i64);

  foreach (auto i, range(0, 30))
    mHmap_set(imap, i, i * i);

  foreach (auto j, iter(mHmap_iterator(imap, i32))) {
    int key = j->key;
    int val = j->val;
    println("{} -> {}", key, val);
  }
}
#include "wheels.h"
