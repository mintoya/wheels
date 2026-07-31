#if !defined MY_SEGMENTTLIST_H
  #define MY_SEGMENTTLIST_H (1)
  #include "macros.h"
  #include "mytypes.h"
  #include "print.h"
  #define blog2(i) (64 - __builtin_clzll(i) - 1)
  // segmentted list of T
  #define sglist(T)                              \
    struct {                                     \
      allocfn allocator;                         \
      usize len;                                 \
      /*each array doubles length of last one */ \
      /*starting at 8*/                          \
      T *arrays[30];                             \
    }
  #define sglist_idx1(x) (blog2(x + 8) - 3)
  #define sglist_idx2(x) (x - ((1 << (sglist_idx1(x) + 3)) - 8))
  #define sglist_get(list, idx) (*({                      \
    let _idx = (idx);                                     \
    &(list).arrays[sglist_idx1(_idx)][sglist_idx2(_idx)]; \
  }))
  #define sglist_initSection(list, idx) (                                                                \
      (list).arrays[idx] = *acreate((list).allocator, typeof(typeof(**((list).arrays)))[1 << (idx + 3)]) \
  )
  #define sglist_push(list, val) ({                      \
    let _list = list;                                    \
    defer { list = _list; };                             \
    if_unlikely (!sglist_idx2(_list.len))                \
      sglist_initSection(_list, sglist_idx1(_list.len)); \
    sglist_get(_list, _list.len++) = val;                \
  })
  #define sglist_deinit(list) ({                                               \
    let _p = &(list);                                                          \
    if (_p->len) {                                                             \
      let _max = sglist_idx1(_p->len - 1);                                     \
      for (usize _i = 0; _i <= _max; _i++)                                     \
        adestroy(                                                              \
            _p->allocator,                                                     \
            (typeof(typeof(**(_p->arrays)))(*)[1 << (_i + 3)])(_p->arrays[_i]) \
        );                                                                     \
    }                                                                          \
  })

  #include "hxmap.h"
  #include "tests.h"
test_fn(segment_list_test) {
  sglist(int) list = {stdAlloc};
  defer { sglist_deinit(list); };
  let map = mxmap_init(stdAlloc, int, int *);
  defer { mxmap_deinit(map); };
  foreach (let i, range(0, 100)) {
    sglist_push(list, i);
    mxmap_set(map, i, &sglist_get(list, i));
  }
  foreach (let i, range(0, 100))
    assertMessage(*mxmap_get(map, i) == &sglist_get(list, i));
}
#endif
#define MY_SEGMENTTLIST_C (2) // has no source
