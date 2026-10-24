#include "../print.h"
#include "../smap.h"
#include <assert.h>
#define mapconfig iter_test_map, int, int, ((k) * 31), (!(a == b))
#include "../incmap.h"
#include "../wheels.h"

int main(void) {
  int count = 0, sum = 0;
  foreach (let i, range(0, 6, 2)) {
    ++count;
    sum += i;
  }
  assert(count == 3 && sum == 6);
  count = 0;
  foreach (let i, range(5, 0)) {
    if (i == 4) continue;
    if (i == 2) break;
    ++count;
  }
  assert(count == 2);
  foreach (let i, range(1, 1)) {
    (void)i;
    assert(0);
  }
  int values[] = {1, 2, 3};
  sum = 0;
  foreach (let p, range(values, values + 3))
    sum += *p;
  assert(sum == 6);
  sum = 0;
  foreach (let p, span(values, 3, 2))
    sum += *p;
  assert(sum == 4);
  int evaluations = 0;
  sum = 0;
  foreach (let i, vlap_iter((++evaluations, &values)))
    sum += i;
  assert(evaluations == 1 && sum == 6);
  count = 0;
  foreach (let i, vla_iter(values))
    foreach (let j, vla_iter(values)) {
      (void)i;
      (void)j;
      ++count;
    }
  assert(count == 9);

  let list = mList_init(stdAlloc, int);
  foreach (let i, mList_iter(list)) {
    (void)i;
    assert(0);
  }
  foreach (let i, vla_iter(values))
    mList_push(list, i);
  sum = 0;
  evaluations = 0;
  foreach (let i, mList_iter((++evaluations, list)))
    sum += i;
  assert(sum == 6 && evaluations == 1);
  mList_deinit(list);

  let map = mxmap_init(stdAlloc, int, int);
  foreach (let kv, mxmap_iter(map, int, int)) {
    (void)kv;
    assert(0);
  }
  foreach (let i, range(0, 20))
    mxmap_set(map, i, i * 2);
  foreach (let i, range(0, 20, 2))
    mxmap_rem(map, i);
  count = 0;
  evaluations = 0;
  foreach (let kv, mxmap_iter((++evaluations, map), int, int)) {
    assert(kv.key % 2 == 1 && *kv.val == kv.key * 2);
    ++count;
  }
  assert(count == 10 && evaluations == 1);
  count = 0;
  foreach (let kv, hxmap_iter((hxmap *)map)) {
    assert(*(int *)kv.val == *(int *)kv.key * 2);
    ++count;
    if (count == 2) break;
  }
  assert(count == 2);
  mxmap_deinit(map);

  let ordered = moxmap_init(stdAlloc, int, int, (oxmap_cmp){0});
  foreach (let kv, moxmap_iter(ordered, int, int)) {
    (void)kv;
    assert(0);
  }
  moxmap_set(ordered, 1, 2);
  count = 0;
  foreach (let kv, moxmap_iter(ordered, int, int)) {
    assert(kv.key == 1 && *kv.val == 2);
    ++count;
  }
  assert(count == 1);
  moxmap_deinit(ordered);

  let strings = msxmap_init(stdAlloc, int);
  foreach (let kv, msxmap_iter(strings)) {
    (void)kv;
    assert(0);
  }
  msxmap_set(strings, "first", 1);
  msxmap_set(strings, "second", 2);
  msxmap_rem(strings, "first");
  count = 0;
  foreach (let kv, msxmap_iter(strings)) {
    assert(fptr_eq(kv.key, fp("second")) && *kv.val == 2);
    ++count;
  }
  assert(count == 1);
  count = 0;
  foreach (let kv, sxmap_iter((sxmap *)strings)) {
    assert(*(int *)kv.val == 2);
    ++count;
  }
  assert(count == 1);
  msxmap_deinit(strings);

  let generated = iter_test_map_new(stdAlloc, 8);
  foreach (let kv, vtable(iter_test_map_iterator, generated)) {
    (void)kv;
    assert(0);
  }
  foreach (let i, range(0, 20))
    iter_test_map_set(generated, i, i * 2);
  count = 0;
  evaluations = 0;
  foreach (let kv, vtable((++evaluations, iter_test_map_iterator), generated)) {
    assert(*kv.val == kv.key * 2);
    ++count;
    continue;
  }
  assert(count == 20 && evaluations == 1);
  let state = iter_test_map_iter_init(generated);
  while (iter_test_map_iter_next(&state).hasvalue) {
  }
  assert(!iter_test_map_iter_next(&state).hasvalue);
  iter_test_map_free(generated);
}
