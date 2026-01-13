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
typedef struct List {
  size_t width;
  uint32_t length;
  uint32_t size;
  uint8_t *head;
  AllocatorV allocator;
} List;
// same as list_resize but it enforces size
void List_forceResize(List *l, unsigned int newSize);

[[gnu::pure]]
extern inline size_t List_headArea(const List *l);
[[gnu::pure]]
extern inline size_t List_fullHeadArea(const List *l) { return (l->width * l->size); }
[[gnu::pure]]
extern inline void *List_getRefForce(const List *l, unsigned int i) { return (l->head + l->width * i); };
[[gnu::pure]]
static void *List_getRef(const List *l, unsigned int i) { return l && (i < l->length) ? (l->head + l->width * i) : (NULL); }
extern inline void List_makeNew(const My_allocator *allocator, List *l, size_t bytes, uint32_t init);
extern inline void List_resize(List *l, unsigned int newSize);

static inline List *List_new(const My_allocator *allocator, size_t bytes) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, 2);
  return l;
}
void List_free(List *l);
static inline List *List_newInitL(const My_allocator *allocator, size_t bytes, uint32_t initSize) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, initSize);
  return l;
}

void *List_append(List *l, const void *element);
void List_insert(List *l, unsigned int i, void *element);
// helper function to append 0's
void List_pad(List *l, unsigned int ammount);
List *List_fromArr(const My_allocator *, const void *source, unsigned int size, unsigned int length);
void *List_appendFromArr(List *l, const void *source, unsigned int i);

inline uint32_t List_length(const List *l);
extern inline void *List_set(List *l, unsigned int i, const void *element);
extern inline uint32_t List_search(const List *l, const void *element);
extern inline void List_remove(List *l, unsigned int i);
extern inline void List_zeroOut(List *l);
void *List_toBuffer(List *l);
void *List_fromBuffer(void *ref);
List *List_deepCopy(List *l);

static void List_cleanup_handler(List **l) {
  if (l && *l)
    List_free(*l);
  *l = NULL;
}
// experimental idk
#define List_scoped [[gnu::cleanup(List_cleanup_handler)]] List

#define LIST_t(T) typeof(T (*)(size_t))
#define LIST_INIT(allocator, T) ({                    \
  (LIST_t(T))(void *) List_new(allocator, sizeof(T)); \
})

#define LIST_DEINIT(list)    \
  do {                       \
    List_free((void *)list); \
  } while (0)

#define LIST_LENGTH(list) (((List *)((void *)list))->length)
#define LIST_PUSH(list, val)                             \
  do {                                                   \
    List_append((void *)list, (typeof(list(0))[1]){val}) \
  } while (0)
#define LIST_GET(list, index) ({ (typeof(list(0)) *)List_getRef((void *)list, index); })
#define LIST_SET(list, index, val) ({ List_set((void *)list, index, (typeof(list(0))[1]){val}); })
#define LIST_INSERT(list, index, val)                            \
  do {                                                           \
    List_insert((void *)list, index, (typeof(list(0))[1]){val}); \
  } while (0)
#define LIST_REMOVE(list, index)      \
  do {                                \
    List_remove((void *)list, index); \
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
inline void List_resize(List *l, unsigned int newSize) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->size || newSize < l->size / 8))
    return List_forceResize(l, newSize);
  return;
}

inline size_t List_headArea(const List *l) {
  return (l->width * l->length);
}
// all bytes list owns
inline void List_makeNew(const My_allocator *allocator, List *l, size_t bytes, uint32_t initialSize) {
  if (!l)
    return;
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
[[gnu::pure]]
extern inline uint32_t List_length(const List *l) {
  if (l)
    return l->length;
  else
    return 0;
}
inline void *List_set(List *l, unsigned int i, const void *element) {
  if (i < l->length) {
    void *place = l->head + i * l->width;
    if (element) {
      memcpy(place, element, l->width);
    } else {
      memset(place, 0, l->width);
    }
    return place;
  } else if (i == l->length) {
    return List_append(l, element);
  } else {
    return NULL;
  }
}
inline uint32_t List_search(const List *l, const void *element) {
  unsigned int i = 0;
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
inline void List_remove(List *l, unsigned int i) {
  if (!l || i >= l->length)
    return;
  memmove(l->head + i * l->width, l->head + (i + 1) * l->width, (l->length - i - 1) * l->width);
  l->length--;
}
inline void List_zeroOut(List *l) {
  memset(l->head, 0, List_fullHeadArea(l));
}
void List_insert(List *l, unsigned int i, void *element) {
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
void List_forceResize(List *l, unsigned int newSize) {
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
void List_pad(List *l, unsigned int ammount) {
  if (l->size < l->length + ammount)
    List_resize(l, l->length + ammount);
  memset(l->head + l->length * l->width, 0, ammount * l->width);
  l->length += ammount;
  return;
}
List *List_fromArr(const My_allocator *allocator, const void *source, unsigned int width, unsigned int length) {
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
void *List_appendFromArr(List *l, const void *source, unsigned int ammount) {
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
