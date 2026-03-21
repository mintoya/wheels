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
extern inline sList_header *sList_realloc(AllocatorV allocator, sList_header *header, usize width, usize newsize) {
  sList_header *res = (typeof(res))aResize(allocator, header, sizeof(sList_header) + newsize * width);
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;
  } else
    res->capacity = newsize;
  return res;
}
extern inline sList_header *sList_new(AllocatorV allocator, usize initLen, usize width) {
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
extern inline void *sList_getRef(sList_header *l, usize width, usize i) { return i < l->length ? (l->buf + width * i) : NULL; }
extern inline void *sList_set(sList_header *l, usize width, usize index, void *element) {
  void *place = sList_getRef(l, width, index);
  if (place) {
    element
        ? memcpy(place, element, width)
        : memset(place, /* */ 0, width);
  }
  return place;
}

extern inline sList_header *sList_append(AllocatorV allocator, sList_header *l, usize width, void *element) {
  if (l->capacity < l->length + 1)
    l = sList_realloc(allocator, l, width, SLIST_GROW_EQ(l->length));
  l->length++;
  sList_set(l, width, l->length - 1, element);
  return l;
}
extern inline sList_header *sList_appendFromArr(AllocatorV allocator, sList_header *l, usize width, void *source, usize ammount) {
  if (!ammount)
    return l;
  if (l->capacity < l->length + ammount)
    l = sList_realloc(allocator, l, width, l->length + ammount);
  uint8_t *dest = l->buf + l->length * width;
  if (source)
    memcpy(dest, source, ammount * width);
  else
    memset(dest, 0, ammount * width);
  l->length += ammount;
  return l;
}
extern inline void sList_remove(sList_header *l, usize width, usize i) {
  if (i >= l->length)
    return;
  memmove(l->buf + i * width, l->buf + (i + 1) * width, (l->length - i - 1) * width);
  l->length--;
}
extern inline sList_header *sList_insertFromArr(AllocatorV allocator, sList_header *l, const void *source, usize length, usize location, size_t width) {
  if (!length || location > l->length)
    return l;
  if (l->capacity < l->length + length)
    l = sList_realloc(allocator, l, width, l->length + length);
  void *res = l->buf + (location)*width;
  memmove(l->buf + (location + length) * width, res, (l->length - location) * width);
  if (source)
    memcpy(res, source, length * width);
  else
    memset(res, 0, length * width);
  l->length += length;
  return l;
}
extern inline sList_header *sList_insert(AllocatorV allocator, sList_header *l, usize width, usize i, void *element) {
  return sList_insertFromArr(allocator, l, element, 1, i, width);
}
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
      if (__builtin_expect((msList_len(s) == msList_cap(s)), 0))                                                  \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), SLIST_GROW_EQ(msList_len(s)))->buf; \
      (s)[msList_len(s)++] = (val);                                                                               \
    } while (0)
  #define msList_pushArr(allocator, s, vla)                             \
    do {                                                                \
      static_assert(_Generic(vla[0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                               \
              allocator,                                                \
              msList_header(s),                                         \
              sizeof(s[0]),                                             \
              vla,                                                      \
              countof(vla)                                              \
      )                                                                 \
              ->buf;                                                    \
    } while (0)
  #define msList_pushVla(allocator, s, vla)                                \
    do {                                                                   \
      static_assert(_Generic(vla[0][0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                                  \
              allocator,                                                   \
              msList_header(s),                                            \
              sizeof(s[0]),                                                \
              vla,                                                         \
              countof(vla[0])                                              \
      )                                                                    \
              ->buf;                                                       \
    } while (0)
  #define msList_len(s) (msList_header(s)->length)
  #define msList_cap(s) (msList_header(s)->capacity)
  #define msList_pop(s) ((s)[--msList_header(s)->length])
  #define msList_vla(s) (VLAP(s, msList_len(s)))
  #define msList_reserve(allocator, s, new_cap)                                              \
    do {                                                                                     \
      if (msList_cap(s) < (new_cap))                                                         \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), new_cap)->buf; \
    } while (0)
#endif // SHORT_LIST_H
