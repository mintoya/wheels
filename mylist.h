#if !defined MY_LIST_H
  #define MY_LIST_H (1)
  #include <stddef.h>
  #include <stdint.h>
  #include <stdlib.h>
  #include <string.h>

  #if !defined LIST_GROW_EQ
    #define LIST_GROW_EQ(uint) (uint + uint / 2)
  #endif
  #include "allocator.h"

typedef size_t List_index_t;
typedef struct List {
  List_index_t length;
  List_index_t capacity;
  uint8_t *__restrict head;
  allocfn allocator;
} List;

static inline void List_forceResize(List *l, List_index_t newlength, size_t width) {
  l->head = **aresize(l->allocator, (u8(*)[width][l->capacity])(l->head), u8[width][newlength]);
  l->capacity = newlength;
  l->length = MIN$(l->length, l->capacity);
}

__attribute__((pure))
/**
 * @param l list
 * @param i index
 * @return pointer to i`th element
 *      - even if i is out of bounds
 */
static inline void *
List_getRefForce(const List *l, List_index_t i, size_t width) { return (l->head + width * i); };
__attribute__((pure))
/**
 * @param l list
 * @param i index
 * @return pointer to i`th element
 *      - null if i is more than l.length
 */
static inline void *
List_getRef(const List *l, List_index_t i, size_t width) { return (i < l->length) ? (l->head + width * i) : (NULL); }
/**
 * writes list to l
 * @param allocator allocator
 * @param l size of each element
 * @param bytes size of each element
 * @param init initial capacity
 */
static inline void List_makeNew(allocfn allocator, List *l, size_t width, List_index_t initialSize) {
  l->length = 0;
  l->allocator = allocator;
  l->head = (typeof(l->head))acreate(allocator, uint8_t[width][initialSize]);
  l->capacity = initialSize;
}
static inline void List_resize(List *l, List_index_t newSize, size_t width) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->capacity || newSize < l->capacity / 8))
    return List_forceResize(l, newSize, width);
  return;
}

/**
 * creates new list
 * @param allocator allocator
 * @param bytes size of each element
 * @return new list
 */
static inline List *List_new(allocfn allocator, size_t width) {
  let l = acreate(allocator, List);
  List_makeNew(allocator, l, width, 2);
  return l;
}
/**
 * frees list and its array
 * @param l list
 */
static inline void List_free(List *l, size_t bw) {
  if (!l || !l->allocator)
    return;
  if (l->head) adestroy(l->allocator, (u8(*)[bw][l->capacity])l->head);
  l->head = NULL;
  adestroy(l->allocator, l);
}
static inline List *List_newInitL(allocfn allocator, size_t bytes, uint32_t initSize) {
  let l = acreate(allocator, List);
  List_makeNew(allocator, l, bytes, initSize);
  return l;
}
__attribute__((always_inline)) static inline void List_set(List *l, List_index_t i, const void *element, size_t width) {
  void *place = List_getRef(l, i, width);
  if (place)
    element
        ? memcpy(place, element, width)
        : memset(place, 0, width);
}

/**
 * inserts list into list
 * @param l list
 * @param source pointer to values
 * @param length element count
 * @return adress of first inserted element
 */
void *List_insertFromArr(List *l, const void *source, List_index_t length, List_index_t location, size_t width);
/**
 * inserts element into list
 * @param l list
 * @param i index to insert
 * @param element pointer to value
 */
static inline void List_insert(List *l, List_index_t i, void *element, size_t w) {
  List_insertFromArr(l, element, 1, i, w);
}
/**
 * inserts elements into list
 * @param l list
 * @param source pointer to value
 * @param length element count
 * @return adress of first inserted element
 */
static inline void *List_appendFromArr(List *l, const void *source, List_index_t ammount, size_t width) {
  return List_insertFromArr(l, source, ammount, l->length, width);
}

__attribute__((pure)) static inline List_index_t List_length(const List *l) { return l ? l->length : 0; }
/*
 * searches for a value which has an identical value to element
 * @param list
 * @param index :index to be removed
 * @param width :item width
 */
void List_remove(List *l, List_index_t i, size_t width);

  #define mList(T) ptrof(fnptrof((List *), T))

  #include "macros.h"

  #define MLIST_INIT_HELPER(allocator, T, initLength, ...) ((mList(T))List_newInitL(allocator, sizeof(T), initLength))
  #define mList_init(allocator, T, ...) \
    MLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)

  #define mList_iType(list) typeof((*list)((List *)0))
  #define mList_listptr(list) ({       \
    (void)(sizeof(mList_iType(list))); \
    (List *)list;                      \
  })
  #define mList_deinit(list)                                     \
    do {                                                         \
      (void)(sizeof(mList_iType(list)));                         \
      List_free(mList_listptr(list), sizeof(mList_iType(list))); \
    } while (0)

  #define mList_arr(list) (((mList_iType(list) *)(mList_listptr(list)->head)))
  #define mList_len(list) (*({        \
    (void)sizeof((*list)((List *)0)); \
    &(((List *)(list))->length);      \
  }))
  #define mList_cap(list) (mList_listptr(list)->capacity)
  #define mList_vla(list) ((typeof(typeof(mList_iType(list)))(*)[mList_len(list)])mList_arr(list))
  #define mList_allocator(list) ({ mList_listptr(list)->allocator; })
  #define mList_push(list, ...)                        \
    do {                                               \
      if_unlikely (mList_len(list) >= mList_cap(list)) \
        List_resize(                                   \
            mList_listptr(list),                       \
            LIST_GROW_EQ(mList_len(list)),             \
            sizeof(mList_iType(list))                  \
        );                                             \
      mList_arr(list)[mList_len(list)++] =             \
          (mList_iType(list))__VA_ARGS__;              \
    } while (0)

  #define mList_pop(list) ({            \
    mList_arr(list)[--mList_len(list)]; \
  })
  #define mList_last(l) (mList_arr(l)[mList_len(l) - 1])
  #define mList_popFront(list)                       \
    ({                                               \
      mList_iType(list) result = mList_arr(list)[0]; \
      mList_rem(list, 0);                            \
      result;                                        \
    })
  #define mList_ins(list, index, val)                          \
    do {                                                       \
      mList_iType(list) value = val;                           \
      List_insert((List *)list, index, &value, sizeof(value)); \
    } while (0)

  #define mList_rem(list, index)                                   \
    ({                                                             \
      mList_iType(list) x;                                         \
      x = mList_len(list) > index ? mList_arr(list)[index] : x;    \
      List_remove((List *)list, index, sizeof(mList_iType(list))); \
      x;                                                           \
    })
  #define mList_setCap(list, capacity) \
    do {                               \
      List_forceResize(                \
          (List *)(list),              \
          capacity,                    \
          sizeof(mList_iType(list))    \
      );                               \
    } while (0)
  #define mList_reserve(list, capacity)                                 \
    do {                                                                \
      List_resize((List *)(list), capacity, sizeof(mList_iType(list))); \
    } while (0)
  #define mList_insArr(list, position, vla)                             \
    do {                                                                \
      let _vla = &vla;                                                  \
      ASSERT_EXPR(types_eq(typeof((*_vla)[0]), mList_iType(list)), ""); \
      List_insertFromArr(                                               \
          mList_listptr(list),                                          \
          (*_vla),                                                      \
          countof(*_vla),                                               \
          position,                                                     \
          sizeof((*_vla)[0])                                            \
      );                                                                \
    } while (0)
  #define mList_pushArr(list, vla) \
    mList_insArr(list, mList_len(list), vla)
  #define mList_initArr(allocator, arr) ({         \
    let _l = mList_init(allocator, arrstype(arr)); \
    mList_pushArr(_l, arr);                        \
    _l;                                            \
  })
  #define mList_pad(list, ammount) \
    mList_insArr(list, mList_len(list), *VLAP((mList_iType(list) *)nullptr, ammount))
  #define mList_clear(list)            \
    do                                 \
      mList_listptr(list)->length = 0; \
    while (0)

  #define mList_toOwned(alloc, list) ({                                                                              \
    allocfn _alloc = alloc;                                                                                          \
    mList_iType(list) *_res = nullptr;                                                                               \
    if (_alloc == mList_allocator(list)) {                                                                           \
      _res = mList_arr(list);                                                                                        \
    } else {                                                                                                         \
      _res = *acreate(_alloc, typeof(mList_iType(list))[mList_len(list)]);                                           \
      memcpy(_res, mList_arr(list), mList_len(list) * sizeof(*_res));                                                \
      adestroy(mList_allocator(list), (u8(*)[sizeof(mList_iType(list))][mList_cap(list)])mList_listptr(list)->head); \
    }                                                                                                                \
    mList_listptr(list)->head = nullptr;                                                                             \
    _res;                                                                                                            \
  })

  #define FOREACH_mList_init(list) ( \
      struct {                       \
        typeof(list) _list;          \
        size_t _current;             \
      },                             \
      {list, 0}                      \
  )
  #define FOREACH_mList_increase(is) (is._current++)
  #define FOREACH_mList_valid(is) (is._current < mList_len(is._list))
  #define FOREACH_mList_cast(is) (*(is._current + mList_arr(is._list)))

  #define FOREACH_mList_iter    \
    (                           \
        FOREACH_mList_init,     \
        FOREACH_mList_increase, \
        FOREACH_mList_valid,    \
        FOREACH_mList_cast)
  #define mList_map(allocator, list, ...) ({ \
    typedef typeof(({                        \
      mList_iType(list) $;                   \
      __VA_ARGS__;                           \
    })) l_rt;                                \
    let _res =                               \
        mList_init(allocator, l_rt);         \
    foreach (let $, mList_iter(list))        \
      mList_push(_res, __VA_ARGS__);         \
    _res;                                    \
  })

  //
  // test functions
  //
  #include "tests.h"
test_fn(mlist_tests) {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deinit(list); };

  foreach (usize i, range(0, 50))
    mList_push(list, i * i);

  foreach (usize i, range(0, 50))
    test_assert(mList_arr(list)[i] == i * i);

  foreach (usize i, range(0, 25))
    mList_rem(list, i);

  test_assert(mList_len(list) == 25);

  foreach (let v, mList_iter(list))
    test_assert(v % 2);

  foreach (usize i, range(0, 50, 2))
    mList_ins(list, i, i * i);

  test_assert(mList_len(list) == 50);

  foreach (usize i, range(0, 50))
    test_assert(mList_arr(list)[i] == i * i);
}
test_fn(mlist_vla_cast) {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deinit(list); };
  mList_push(list, 7);
  mList_push(list, 8);
  mList_push(list, 9);
  let arr = acreate(allocator, int[3]);
  defer { adestroy(allocator, arr); };
  mcpy(*arr, *mList_vla(list));
  mList_pushArr(list, *arr);
  test_assert(mList_len(list) == 6);

  test_assert(!memcmp(mList_arr(list), *arr, sizeof(*arr)));
  test_assert(!memcmp(mList_arr(list), mList_arr(list) + 3, sizeof(*arr)));
}

#endif // MY_LIST_H
#if (defined MY_LIST_C && MY_LIST_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef MY_LIST_C
  #define MY_LIST_C (2)
void List_remove(List *l, List_index_t i, size_t width) {
  if (i >= l->length) return;
  memmove(l->head + i * width, l->head + (i + 1) * width, (l->length - i - 1) * width);
  l->length--;
}
void *List_insertFromArr(List *l, const void *source, List_index_t length, List_index_t location, size_t width) {
  if (location > l->length) return nullptr;

  bool inlist =
      (u8 *)source >= l->head &&
      (u8 *)source < l->head + l->capacity * width;

  usize need = l->length + (inlist ? 2 * length : length);
  u8 *obuf = l->head;
  if (l->capacity < need) List_resize(l, need, width);

  if (inlist) {
    source = (u8 *)source - obuf + l->head;
    memcpy(l->head + (l->capacity - length) * width, source, length * width);
    source = l->head + (l->capacity - length) * width;
  }

  u8 *dest = l->head + location * width;
  memmove(dest + length * width, dest, (l->length - location) * width);

  if (source) memcpy(dest, source, length * width);
  else memset(dest, 0, length * width);

  l->length += length;
  return dest;
}
#endif
