#ifndef MY_LIST_H
#define MY_LIST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifndef LIST_GROW_EQ
#define LIST_GROW_EQ(uint) ((uint + (uint << 1)) + 1)
#endif
// clang-format on
#include "allocator.h"

typedef size_t List_index_t;
typedef struct List {
  size_t width;
  List_index_t length;
  List_index_t size;
  uint8_t *head;
  AllocatorV allocator;
} List;
// same as list_resize but it enforces size
void List_forceResize(List *l, List_index_t newSize);

[[gnu::pure]]
extern inline size_t List_headArea(const List *l) {
  return (l->width * l->length);
}
[[gnu::pure]]
extern inline size_t List_fullHeadArea(const List *l) { return (l->width * l->size); }
[[gnu::pure]]
extern inline void *List_getRefForce(const List *l, List_index_t i) { return (l->head + l->width * i); };
[[gnu::pure]]
extern void *List_getRef(const List *l, List_index_t i) { return l && (i < l->length) ? (l->head + l->width * i) : (NULL); }
void List_makeNew(AllocatorV allocator, List *l, size_t bytes, uint32_t init);
extern inline void List_resize(List *l, List_index_t newSize) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->size || newSize < l->size / 8))
    return List_forceResize(l, newSize);
  return;
}

extern inline List *List_new(AllocatorV allocator, size_t bytes) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, 2);
  return l;
}
void List_free(List *l);
extern inline List *List_newInitL(AllocatorV allocator, size_t bytes, uint32_t initSize) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, initSize);
  return l;
}

void *List_append(List *l, const void *element);
void List_insert(List *l, List_index_t i, void *element);
// helper function to append 0's
List *List_fromArr(AllocatorV, const void *source, size_t size, List_index_t length);
void *List_appendFromArr(List *l, const void *source, List_index_t);

[[gnu::pure]]
extern inline List_index_t List_length(const List *l) { return l ? l->length : 0; }
extern inline void *List_set(List *l, List_index_t i, const void *element) {
  if (i < l->length) {
    void *place = l->head + i * l->width;
    if (element) {
      memcpy(place, element, l->width);
    } else {
      memset(place, 0, l->width);
    }
    return place;
  }
  if (i == l->length)
    return List_append(l, element);
  return NULL;
}
extern inline List_index_t List_search(const List *l, const void *element);
extern inline void List_remove(List *l, List_index_t i);
extern inline void List_zeroOut(List *l);
void *List_toBuffer(List *l);
void *List_fromBuffer(void *ref);
List *List_deepCopy(List *l);

static void List_cleanup_handler(void *ListPtrPtr) {
  List **l = ListPtrPtr;
  if (l && *l)
    List_free(*l);
  *l = NULL;
}
// experimental idk
#define List_scoped [[gnu::cleanup(List_cleanup_handler)]] List

#define mList(T) typeof(T (*)(List *))
#define mList_scoped(T) [[gnu::cleanup(List_cleanup_handler)]] mList(T)

#define mList_init(allocator, T) ({                  \
  (mList(T))(List *) List_new(allocator, sizeof(T)); \
})

#define mList_deInit(list)   \
  do {                       \
    List_free((List *)list); \
  } while (0)

#define mList_len(list) (((List *)(list))->length)
#define mList_push(list, val)                                   \
  do {                                                          \
    List_append((List *)list, (typeof(list(NULL))[1]){val}); \
  } while (0)
#define mList_get(list, index) ({ (typeof(list(NULL)) *)List_getRef((List *)list, index); })
#define mList_set(list, index, val) ({ List_set((List *)list, index, (typeof(list(NULL))[1]){val}); })
#define mList_ins(list, index, val)                                    \
  do {                                                                 \
    List_insert((List *)list, index, (typeof(list(NULL))[1]){val}); \
  } while (0)
#define mList_rem(list, index)        \
  do {                                \
    List_remove((List *)list, index); \
  } while (0)

#endif // MY_LIST_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_LIST_C (1)
#endif
#ifdef MY_LIST_C
static inline bool memzeroed(void *mem, size_t len) {
  for (; len > 0; len--) {
    if (((uint8_t *)mem)[len - 1])
      return false;
  }
  return true;
}

// all bytes list owns
void List_makeNew(const My_allocator *allocator, List *l, size_t bytes, uint32_t initialSize) {
  *l = (List){
      .width = bytes,
      .length = 0,
      .size = initialSize,
      .head = (uint8_t *)aAlloc(allocator, bytes * initialSize),
      .allocator = allocator,
  };
  if (allocator->size) {
    l->size = allocator->size(allocator, l->head) / l->width;
  }
}
void List_free(List *l) {
  if (!l || !l->allocator)
    return;
  if (l->head)
    aFree(l->allocator, l->head);
  l->head = NULL;
  l->width = 0;
  aFree(l->allocator, l);
}
inline List_index_t List_search(const List *l, const void *element) {
  List_index_t i = 0;
  if (element) {
    for (; i < l->length; i++) {
      if (!memcmp(element, List_getRef(l, i), l->width))
        return i;
    }
  } else {
    for (; i < l->length; i++) {
      if (memzeroed(List_getRef(l, i), l->width))
        return i;
    }
  }
  return i;
}
inline void List_remove(List *l, List_index_t i) {
  if (i >= l->length)
    return;
  memmove(l->head + i * l->width, l->head + (i + 1) * l->width, (l->length - i - 1) * l->width);
  l->length--;
}
inline void List_zeroOut(List *l) {
  memset(l->head, 0, List_fullHeadArea(l));
}
void List_insert(List *l, List_index_t i, void *element) {
  if (i == l->length)
    return (void)List_append(l, element);
  if (i > l->length)
    return;
  if (l->size < l->length + 1)
    List_resize(l, LIST_GROW_EQ(l->length));
  memmove(l->head + (i + 1) * l->width, l->head + (i)*l->width, (l->length - i) * l->width);
  List_set(l, i, element);
  l->length++;
}
void *List_append(List *l, const void *element) {
  if (l->size < l->length + 1)
    List_resize(l, LIST_GROW_EQ(l->length));
  l->length++;
  return List_set(l, l->length - 1, element);
}
#include "stdio.h"
void List_forceResize(List *l, List_index_t newSize) {
  uint8_t *newPlace =
      (uint8_t *)aRealloc(l->allocator, l->head, newSize * l->width);
  if (!newPlace) {
    fprintf(stderr, "cant resize list");
    abort();
  } else {
    l->head = newPlace;
    l->size = newSize;
    if (l->allocator->size) {
      l->size = l->allocator->size(l->allocator, l->head) / l->width;
    }
    l->length = (l->length < l->size) ? (l->length) : (l->size);
  }
  return;
}
List *List_fromArr(AllocatorV allocator, const void *source, size_t width, List_index_t length) {
  List *res = (List *)aAlloc(allocator, sizeof(List));
  res->width = width;
  res->length = length;
  res->size = length;
  res->allocator = allocator;
  res->head = (uint8_t *)aAlloc(allocator, length * width);
  if (res && res->head && source)
    memcpy(res->head, source, length * width);
  return res;
}
void *List_appendFromArr(List *l, const void *source, List_index_t ammount) {
  if (!ammount)
    return NULL;
  if (l->size < l->length + ammount)
    List_resize(l, l->length + ammount);
  uint8_t *dest = l->head + l->length * l->width;
  if (source)
    memcpy(dest, source, ammount * l->width);
  else
    memset(dest, 0, ammount * l->width);
  l->length += ammount;
  return dest;
}

List *List_deepCopy(List *l) { return List_fromArr(l->allocator, l->head, l->width, l->length); }

#endif
