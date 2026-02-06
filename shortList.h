#if !defined(SHORT_LIST_H)
  #define SHORT_LIST_H (1)
  #include "allocator.h"
  #include "assertMessage.h"
  #include "stdalign.h"
  #include "string.h"
  #include "types.h"
  #ifndef SLIST_GROW_EQ
    #define SLIST_GROW_EQ(len) (len + len / 2 + 1)
  #endif

typedef struct sList_header {
  usize length;
  usize capacity;
  alignas(max_align_t) u8 buf[];
} sList_header;
sList_header *sList_realloc(AllocatorV allocator, sList_header *header, usize width, usize newsize) {
  sList_header *res = (typeof(res))aRealloc(allocator, header, sizeof(sList_header) + newsize * width);
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;
  } else
    res->capacity = width;
  return res;
}
[[nodiscard]] extern inline sList_header *sList_new(AllocatorV allocator, usize initLen, usize width) {
  assertMessage(initLen && width);
  sList_header *res = (typeof(res))aAlloc(allocator, sizeof(sList_header) + initLen * width);
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;
  } else
    res->capacity = width;
  return res;
}
extern inline void *sList_getRefForce(sList_header *l, usize width, usize i) { return (l->buf + width * i); }
extern inline void *sList_getRef(sList_header *l, usize width, usize i) { return i < l->length ? (l->buf + width * i) : NULL; }
extern inline void *sList_set(sList_header *l, usize width, usize index, void *element) {
  void *place = sList_getRef(l, width, index);
  if (place)
    if (element)
      memcpy(place, element, width);
    else
      memset(place, 0, width);
  return place;
}

[[nodiscard]] extern inline sList_header *sList_append(AllocatorV allocator, sList_header *l, usize width, void *element) {
  if (l->capacity < l->length + 1)
    l = sList_realloc(allocator, l, width, SLIST_GROW_EQ(l->length));
  l->length++;
  sList_set(l, width, l->length - 1, element);
  return l;
}
extern inline void sList_remove(sList_header *l, usize width, usize i) {
  if (i >= l->length)
    return;
  memmove(l->buf + i * width, l->buf + (i + 1) * width, (l->length - i - 1) * width);
  l->length--;
}
[[nodiscard]] extern inline sList_header *sList_insert(AllocatorV allocator, sList_header *l, usize width, usize i, void *element) {
  if (i == l->length)
    return sList_append(allocator, l, width, element);
  if (i > l->length)
    return l;
  if (l->capacity < l->length + 1)
    l = sList_realloc(allocator, l, width, SLIST_GROW_EQ(l->length));
  memmove(l->buf + (i + 1) * width, l->buf + (i)*width, (l->length - i) * width);
  sList_set(l, width, i, element);
  l->length++;
  return l;
}

  #define SLIST_INIT_HELPER(allocator, T, initLength, ...) ((T *)(sList_new(allocator, initLength, sizeof(T))->buf))
  #define msList_init(allocator, T, ...) \
    SLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)
  #define msList_header(s) ((sList_header *)(((u8 *)s) - offsetof(sList_header, buf)))
  #define msList_deInit(allocator, s)     \
    do {                                  \
      aFree(allocator, msList_header(s)); \
    } while (0)
  #define msList_ins(allocator, s, idx, val)                                                 \
    do {                                                                                     \
      typeof(*s) _val = val;                                                                 \
      s = (typeof(s))sList_insert(allocator, msList_header(s), sizeof(*s), idx, &_val)->buf; \
    } while (0);

  #define msList_rem(s, idx)                           \
    do {                                               \
      sList_remove(msList_header(s), sizeof(*s), idx); \
    } while (0)
  #define msList_push(allocator, s, val)                                                \
    do {                                                                                \
      typeof(*s) _val = val;                                                            \
      s = (typeof(s))sList_append(allocator, msList_header(s), sizeof(*s), &_val)->buf; \
    } while (0)
  #define msList_len(s) (msList_header(s)->length)
  #define msList_cap(s) (msList_header(s)->capacity)
  #define msList_pop(s) ((s)[--msList_header(s)->length])
  #define msList_vla(s) ((typeof (*s)(*)[msList_len(s)])s)

#endif // SHORT_LIST_H
// #if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
//   #define SHORT_LIST_C (1)
// #endif
// #if defined(SHORT_LIST_C)
// #endif // SHORT_LIST_C
