#ifndef MY_LIST_H
#define MY_LIST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NEW_ATTR

#ifdef OLD_ATTR
  #define ATTR(arg, ...) __attribute__((arg))
#elifdef NEW_ATTR
  #define ATTR(arg, ...) [[__VA_OPT__(__VA_ARGS__::) arg]]
#else
  #define ATTR(...)
#endif

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

ATTR(pure, gnu)
extern inline size_t List_headArea(const List *l);
ATTR(pure, gnu)
extern inline size_t List_fullHeadArea(const List *l) { return (l->width * l->size); }
ATTR(pure, gnu)
extern inline void *List_getRefForce(const List *l, unsigned int i) { return (l->head + l->width * i); };
ATTR(pure, gnu)
static void *List_getRef(const List *l, unsigned int i) { return l && (i < l->length) ? (l->head + l->width * i) : (NULL); }
extern inline void List_makeNew(const My_allocator *allocator, List *l, size_t bytes, uint32_t init);
extern inline void List_resize(List *l, unsigned int newSize);

static inline List *List_new(const My_allocator *allocator, size_t bytes) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, 2);
  return l;
}
extern inline void List_free(List *l);
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
void List_appendFromArr(List *l, const void *source, unsigned int i);

extern inline uint32_t List_length(const List *l);
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

#define mList_forEach(list, type, name, ...)               \
  do {                                                     \
    type name;                                             \
    for (unsigned int _i = 0; _i < (list)->length; _i++) { \
      name = (*(type *)List_getRef((list), _i));           \
      __VA_ARGS__                                          \
    }                                                      \
  } while (0)
#define mList(allocator, type) List_new(allocator, sizeof(type))
#define mList_get(list, type, index) (*(type *)List_getRef(list, index))
#define mList_add(list, type, value)         \
  do {                                       \
    type __val = value;                      \
    List_append(list, (const void *)&__val); \
  } while (0)

#define mList_insert(list, type, value, index) \
  do {                                         \
    type __val = value;                        \
    List_insert(list, index, (void *)&__val);  \
  } while (0)

#define mList_set(list, type, value, index)      \
  do {                                           \
    type __val = value;                          \
    List_set(list, index, (const void *)&__val); \
  } while (0)

#endif // MY_LIST_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#pragma once
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
ATTR(pure, gnu)
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
inline void List_free(List *l) {
  if (!l || !l->allocator)
    return;
  if (l->head)
    aFree(l->allocator, l->head);
  l->head = NULL;
  l->width = 0;
  aFree(l->allocator, l);
}
ATTR(pure, gnu)
inline uint32_t List_length(const List *l) {
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
void List_appendFromArr(List *l, const void *source, unsigned int ammount) {
  if (!ammount)
    return;
  if (l->size < l->length + ammount)
    List_resize(l, l->length + ammount);
  if (source)
    memcpy(l->head + l->length * l->width, source, ammount * l->width);
  else
    memset(l->head + l->length * l->width, 0, ammount * l->width);
  l->length += ammount;
  return;
}

List *List_deepCopy(List *l) { return List_fromArr(l->allocator, l->head, l->width, l->length); }

#endif
