#include "allocator.h"
#include "hxmap.h"
#include "macros.h"
#include "mytypes.h"
#include "oxmap.h"
typedef struct mapinterface mapinterface;
typedef const mapinterface *mapv;
struct mapinterface {
  fnptrof((mapv), void) /*             */ deinit;
  fnptrof((mapv, void *, void *), void *) set;
  fnptrof((mapv, void *), /*   */ void *) get;
  alignas(myAlign) u8 _padding[];
};

struct hmapi_s {
  mapinterface i[1];
  hxmap *map;
};
void hmapi_deinit(mapv mv) {
  struct hmapi_s *m = (typeof(m))mv;
  var_ allocator = m->map->allocator;
  hxmap_free(m->map);
  aDestroy(allocator, m);
}
void *hmapi_set(mapv mv, void *k, void *v) {
  struct hmapi_s *m = (typeof(m))mv;
  return hxmap_set(m->map, k, v);
}
void *hmapi_get(mapv mv, void *k) {
  struct hmapi_s *m = (typeof(m))mv;
  return hxmap_get(m->map, k);
}
mapv hmapi(
    AllocatorV allocator,
    usize ks,
    usize vs,
    fnptrof((const void *), u64) hash,
    fnptrof((const void *, const void *), i8) cmp
) {
  // clang-format off
  return aValue(
             allocator,
             ((struct hmapi_s){
                 {hmapi_deinit, hmapi_set, hmapi_get},
                 hxmap_new(allocator, ks, vs, 3, hash, cmp)
             })
  ) ->i;
  // clang-format on
}
struct odmapi_s {
  mapinterface i[1];
  oxmap *map;
};
void odmapi_deinit(mapv mv) {
  struct odmapi_s *m = (typeof(m))mv;
  var_ allocator = m->map->allocator;
  oxmap_free(m->map);
  aDestroy(allocator, m);
}
void *odmapi_set(mapv mv, void *k, void *v) {
  struct odmapi_s *m = (typeof(m))mv;
  return oxmap_set(m->map, k, v);
}
void *odmapi_get(mapv mv, void *k) {
  struct odmapi_s *m = (typeof(m))mv;
  return oxmap_get(m->map, k);
}
mapv odmapi(
    AllocatorV allocator,
    usize ks,
    usize vs,
    fnptrof((const void *, const void *), i8) cmp
) {
  // clang-format off
  return aValue(
             allocator,
             ((struct odmapi_s){
                 {odmapi_deinit, odmapi_set, odmapi_get},
                 oxmap_new(allocator, ks, vs, cmp)
             })
  ) ->i;
  // clang-format on
}
