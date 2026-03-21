#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../mylist.h"
#include "../wheels.h"
int main(void) {
  Arena_scoped *s = arena_new_ext(stdAlloc, 1024);
  AllocatorV local = debugAllocator(
      log = stderr,
      allocator = stdAlloc,
  );
  defer { print("{} leaks detected\n", (isize)debugAllocatorDeInit(local)); };

  print("--- Testing mList ---\n");
  mList_scoped(int) m_list = mList_init(local, int);

  mList_push(m_list, 10);
  mList_push(m_list, 20);
  mList_push(m_list, 30);
  mList_ins(m_list, 1, 15);
  mList_rem(m_list, 2);

  int m_popped = mList_pop(m_list);
  print("mList popped: {} (Expected: 30)\n", m_popped);

  for (each_VLAP(it, mList_vla(m_list)))
      print("m_list iter: {}\n", *it);

  return 0;
}
