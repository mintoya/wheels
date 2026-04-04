#include <stdlib.h>
#if !defined(SHORT_LIST_H)
  #define SHORT_LIST_H (1)
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"
  #include <stdalign.h>
  #include <string.h>
  #ifndef SLIST_GROW_EQ
    #define SLIST_GROW_EQ(len) (len + len / 2 + 1)
  #endif

typedef struct sList_header {
  usize length;
  usize capacity;
  alignas(max_align_t) u8 buf[];
} sList_header;
static inline sList_header *sList_realloc(AllocatorV allocator, sList_header *header, usize width, usize newsize) {
  sList_header *res = (typeof(res))aResize(allocator, header, sizeof(sList_header) + newsize * width);
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;
  } else
    res->capacity = newsize;
  return res;
}
static inline sList_header *sList_new(AllocatorV allocator, usize initLen, usize width) {
  assertMessage(initLen && width);
  sList_header *res = (typeof(res))aAlloc(allocator, sizeof(sList_header) + initLen * width);
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;
  } else
    res->capacity = initLen;
  res->length = 0;
  return res;
}
static inline void *sList_getRef(sList_header *l, usize width, usize i) { return i < l->length ? (l->buf + width * i) : NULL; }
static inline void *sList_set(sList_header *l, usize width, usize index, const void *element) {
  void *place = sList_getRef(l, width, index);
  if (place) {
    element
        ? memcpy(place, element, width)
        : memset(place, /* */ 0, width);
  }
  return place;
}

static inline sList_header *sList_append(AllocatorV allocator, sList_header *l, usize width, const void *element) {
  if (l->capacity < l->length + 1)
    l = sList_realloc(allocator, l, width, SLIST_GROW_EQ(l->length));
  l->length++;
  sList_set(l, width, l->length - 1, element);
  return l;
}
static inline void sList_remove(sList_header *l, usize width, usize i) {
  if (i >= l->length)
    return;
  memmove(l->buf + i * width, l->buf + (i + 1) * width, (l->length - i - 1) * width);
  l->length--;
}
static inline sList_header *sList_insertFromArr(AllocatorV allocator, sList_header *l, const void *source, usize length, usize location, size_t width) {
  void *ts = NULL;
  if (length && (u8 *)source >= l->buf && (u8 *)source < l->buf + l->capacity * width) {
    ts = aAlloc(allocator, width * length);
    memcpy(ts, source, width * length);
  }
  if (!length || location > l->length) {
    if (ts)
      aFree(allocator, ts);
    return l;
  }
  if (l->capacity < l->length + length)
    l = sList_realloc(allocator, l, width, l->length + length);
  void *res = l->buf + (location)*width;
  memmove(l->buf + (location + length) * width, res, (l->length - location) * width);
  if (source)
    memcpy(res, ts ?: source, length * width);
  else
    memset(res, 0, length * width);
  l->length += length;
  if (ts)
    aFree(allocator, ts);
  return l;
}
static inline sList_header *sList_appendFromArr(AllocatorV allocator, sList_header *l, usize width, void *source, usize ammount) {
  return sList_insertFromArr(allocator, l, source, ammount, l->length, width);
}
static inline sList_header *sList_insert(AllocatorV allocator, sList_header *l, usize width, usize i, void *element) {
  return sList_insertFromArr(allocator, l, element, 1, i, width);
}
  #if defined(__BLOCKS__)
    #pragma GCC warning "blocks enabled, non-stable pointers break"
    #define msList(T) __block T *
  #else
    #define msList(T) T *
  #endif

  #define SLIST_INIT_HELPER(allocator, T, initLength, ...) ((T *)(sList_new(allocator, initLength, sizeof(T))->buf))
  #define msList_init(allocator, T, ...) \
    SLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)
  #define msList_header(s) (((sList_header *)(s)) - 1)
  #define msList_deInit(allocator, s)     \
    do {                                  \
      aFree(allocator, msList_header(s)); \
    } while (0)
  #define msList_ins(allocator, s, idx, val)                                                 \
    do {                                                                                     \
      typeof(*s) _val = val;                                                                 \
      s = (typeof(s))sList_insert(allocator, msList_header(s), sizeof(*s), idx, &_val)->buf; \
    } while (0);

  #define msList_get(s, idx) ((typeof(s))sList_getRef(msList_header(s), sizeof(*s), idx))
  #define msList_rem(s, idx)                           \
    do {                                               \
      sList_remove(msList_header(s), sizeof(*s), idx); \
    } while (0)
  #define msList_push(allocator, s, val)                                                                          \
    do {                                                                                                          \
      var_ _v_temp = val;                                                                                         \
      if (__builtin_expect((msList_len(s) == msList_cap(s)), 0))                                                  \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), SLIST_GROW_EQ(msList_len(s)))->buf; \
      (s)[msList_len(s)++] = (_v_temp);                                                                           \
    } while (0)
  #define msList_insArr(allocator, s, place, vla)                     \
    do {                                                              \
      ASSERT_EXPR(_Generic(vla[0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_insertFromArr(                             \
              allocator,                                              \
              msList_header(s),                                       \
              vla,                                                    \
              countof(vla),                                           \
              place,                                                  \
              sizeof(vla[0])                                          \
      )                                                               \
              ->buf;                                                  \
    } while (0)
  #define msList_pushArr(allocator, s, vla)                           \
    do {                                                              \
      ASSERT_EXPR(_Generic(vla[0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                             \
              allocator,                                              \
              msList_header(s),                                       \
              sizeof(s[0]),                                           \
              vla,                                                    \
              countof(vla)                                            \
      )                                                               \
              ->buf;                                                  \
    } while (0)
  #define msList_pushVla(allocator, s, vla)                              \
    do {                                                                 \
      ASSERT_EXPR(_Generic(vla[0][0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                                \
              allocator,                                                 \
              msList_header(s),                                          \
              sizeof(s[0]),                                              \
              vla,                                                       \
              countof(vla[0])                                            \
      )                                                                  \
              ->buf;                                                     \
    } while (0)
  #define msList_setCap(allocator, s, capacity)                                             \
    do {                                                                                    \
      s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), capacity)->buf; \
    } while (0)
  #define msList_len(s) (msList_header(s)->length)
  #define msList_cap(s) (msList_header(s)->capacity)
  #define msList_pop(s) ((s)[--msList_header(s)->length])
  #define msList_popFront(s) ({typeof(*s) _res = *s;msList_rem(s, 0);_res; })
static inline sList_header *sList_insertFromArr(
    AllocatorV allocator,
    sList_header *l,
    const void *source,
    usize length,
    usize location,
    size_t width
);
  #define msList_pad(allocator, s, ammount) \
    do {                                    \
      s = (typeof(s))sList_insertFromArr(   \
              allocator,                    \
              msList_header(s),             \
              NULL,                         \
              ammount,                      \
              msList_len(s),                \
              sizeof(*s)                    \
      )                                     \
              ->buf;                        \
    } while (0)
  #define msList_vla(s) (VLAP(s, msList_len(s)))
  #define msList_clear(s) \
    do                    \
      msList_len(s) = 0;  \
    while (0)
  #define msList_reserve(allocator, s, new_cap)                                              \
    do {                                                                                     \
      if (msList_cap(s) < (new_cap))                                                         \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), new_cap)->buf; \
    } while (0)
  // #include "tests.c"
  #if defined(MAKE_TEST_FN)
    #include "macros.h"

MAKE_TEST_FN(msList_push_pop, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };
  for (each_RANGE(usize, i, 0, 50))
    msList_push(allocator, list, i * i);
  for (each_RANGE(usize, i, 0, 50))
    if (list[i] != i * i)
      return 1;
  return 0;
});

MAKE_TEST_FN(msList_insert_remove, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_push(allocator, list, 100);
  msList_push(allocator, list, 300);
  msList_ins(allocator, list, 1, 200);
  if (msList_len(list) != 3)
    return 1;
  if (list[1] != 200)
    return 1;

  msList_rem(list, 0);
  if (msList_len(list) != 2)
    return 1;
  if (list[0] != 200)
    return 1;

  if (msList_popFront(list) != 200)
    return 1;
  if (msList_len(list) != 1)
    return 1;
  if (list[0] != 300)
    return 1;

  return 0;
});

MAKE_TEST_FN(msList_array_operations, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  int arr1[] = {1, 2, 3};
  msList_pushArr(allocator, list, arr1);
  if (msList_len(list) != 3)
    return 1;
  if (list[2] != 3)
    return 1;

  int arr2[] = {0};
  msList_insArr(allocator, list, 0, arr2);
  if (msList_len(list) != 4)
    return 1;
  if (list[0] != 0)
    return 1;
  if (list[1] != 1)
    return 1;

  return 0;
});

MAKE_TEST_FN(msList_capacity_and_padding, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_reserve(allocator, list, 50);
  if (msList_cap(list) < 50)
    return 1;

  msList_pad(allocator, list, 5);
  if (msList_len(list) != 5)
    return 1;

  msList_setCap(allocator, list, 10);
  if (msList_cap(list) < 10)
    return 1;

  msList_clear(list);
  if (msList_len(list) != 0)
    return 1;

  return 0;
});

MAKE_TEST_FN(msList_vla_cast, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_push(allocator, list, 7);
  msList_push(allocator, list, 8);
  msList_push(allocator, list, 9);
  msList_pushArr(allocator, list, *msList_vla(list));
  if (msList_len(list) != 6)
    return 1;
  return 0;
});

  #endif
#endif // SHORT_LIST_H
