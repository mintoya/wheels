#ifndef MY_LIST_H
#define MY_LIST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
  const My_allocator *allocator;
} List;
//
typedef struct LList_element LList_element;
typedef struct LList_element {
  LList_element *next;
  uint8_t data[];
} LList_element;

typedef struct LList_head {
  LList_element *first;
  LList_element *last;
  size_t elementSize;
  const My_allocator *allocator;
} LList_head;

typedef enum : uint8_t {
  OK = 0,
  CANTRESIZE,
  INVALID,
} List_opError;
struct List_opErrorStruct {
  List_opError Ok;
  List_opError CantResize;
  List_opError Invalid;
};
extern const struct List_opErrorStruct List_opErrorS;
extern inline List_opError List_validState(const List *l);
// same as list_resize but it enforces size
List_opError List_forceResize(List *l, unsigned int newSize);

ATTR(pure, gnu)
extern inline size_t List_headArea(const List *l);
// all bytes list owns
ATTR(pure, gnu)
extern inline size_t List_fullHeadArea(const List *l);
ATTR(pure, gnu)
inline void *List_getRefForce(const List *l, unsigned int i);
ATTR(pure, gnu)
static void *List_getRef(const List *l, unsigned int i) {
  if (List_validState(l) != OK)
    return NULL;
  return l && (i < l->length) ? (l->head + l->width * i) : (NULL);
}
extern inline void List_makeNew(const My_allocator *allocator, List *l, size_t bytes, uint32_t init);
extern inline List_opError List_resize(List *l, unsigned int newSize);

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

List_opError List_append(List *l, const void *element);
List_opError List_insert(List *l, unsigned int i, void *element);
// helper function to append 0's
List_opError List_pad(List *l, unsigned int ammount);
List *List_fromArr(const My_allocator *, const void *source, unsigned int size, unsigned int length);
List_opError List_appendFromArr(List *l, const void *source, unsigned int i);

extern inline uint32_t List_length(const List *l);
extern inline void List_set(List *l, unsigned int i, const void *element);
extern inline uint32_t List_search(const List *l, const void *element);
extern inline void List_remove(List *l, unsigned int i);
extern inline void List_zeroOut(List *l);
void *List_toBuffer(List *l);
void *List_fromBuffer(void *ref);
List *List_deepCopy(List *l);

LList_head *LList_new(const My_allocator *allocator, size_t size);
List_opError LList_append(LList_head *l, const void *val);
void LList_free(LList_head *l);

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
#ifndef LIST_NOCHECKS
inline List_opError List_validState(const List *l) {
  // clang-format off
  return (
      l &&
      l->width &&
      l->size &&
      l->head &&
      l->allocator &&
      l->size >= l->length
  )?OK:INVALID;
  // clang-format on
}
#else
inline List_opError List_validState(const List *l) { return (OK); }
#endif
const struct List_opErrorStruct List_opErrorS = {
    OK, CANTRESIZE, INVALID
};
static inline bool memzeroed(void *mem, size_t len) {
  for (; len > 0; len--) {
    if (((uint8_t *)mem)[len - 1])
      return false;
  }
  return true;
}
inline List_opError List_resize(List *l, unsigned int newSize) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->size || newSize < l->size / 8))
    return List_forceResize(l, newSize);
  return OK;
}
ATTR(pure, gnu)
inline size_t List_headArea(const List *l) {
  return (l->width * l->length);
}
// all bytes list owns
ATTR(pure, gnu)
inline size_t List_fullHeadArea(const List *l) {
  return (l->width * l->size);
}
ATTR(pure, gnu)
void *List_getRefForce(const List *l, unsigned int i) {
  return (l->head + l->width * i);
}
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
inline void List_set(List *l, unsigned int i, const void *element) {
  if (List_validState(l) != OK)
    return;
  if (i < l->length) {
    if (element) {
      memcpy(l->head + i * l->width, element, l->width);
    } else {
      memset(l->head + i * l->width, 0, l->width);
    }
  }
}
inline uint32_t List_search(const List *l, const void *element) {
  if (!List_validState(l))
    return ((uint32_t)-1);
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
  if (List_validState(l) != OK)
    return;
  memmove(l->head + i * l->width, l->head + (i + 1) * l->width, (l->length - i - 1) * l->width);
  l->length--;
}
inline void List_zeroOut(List *l) {
  if (!List_validState(l))
    return;
  memset(l->head, 0, List_fullHeadArea(l));
}
List_opError List_insert(List *l, unsigned int i, void *element) {
  if (List_validState(l) != OK)
    return INVALID;
  if (i == l->length)
    return List_append(l, element);
  if (i > l->length)
    return OK;
  if (l->size < l->length + 1) {
    if (List_resize(l, LIST_GROW_EQ(l->length)) != OK)
      return CANTRESIZE;
  }
  if (List_validState(l) != OK)
    return INVALID;
  memmove(l->head + (i + 1) * l->width, l->head + (i)*l->width, (l->length - i) * l->width);
  List_set(l, i, element);
  l->length++;
  return OK;
}
List_opError List_append(List *l, const void *element) {
  List_opError state = OK;
  if (List_validState(l) != OK)
    return INVALID;
  if (l->size < l->length + 1)
    if (List_resize(l, LIST_GROW_EQ(l->length)) != OK)
      return CANTRESIZE;
  if (List_validState(l) != OK)
    return INVALID;
  l->length++;
  List_set(l, l->length - 1, element);
  return OK;
}
List_opError List_forceResize(List *l, unsigned int newSize) {
  uint8_t *newPlace =
      (uint8_t *)aRealloc(l->allocator, l->head, newSize * l->width);
  if (!newPlace) {
    if (newSize < l->size)
      return INVALID;
    else
      return CANTRESIZE;
  } else {
    l->head = newPlace;
    l->size = newSize;
    l->length = (l->length < l->size) ? (l->length) : (l->size);
  }
  return List_validState(l);
}
List_opError List_pad(List *l, unsigned int ammount) {
  if (l->size < l->length + ammount) {
    unsigned int newsize = l->size;
    while (newsize < l->length + ammount) {
      newsize = LIST_GROW_EQ(newsize);
    }
    if (List_resize(l, newsize) != OK)
      return CANTRESIZE;
  }
  memset(l->head + l->length * l->width, 0, ammount * l->width);
  l->length += ammount;
  return List_opErrorS.Ok;
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
List_opError List_appendFromArr(List *l, const void *source, unsigned int ammount) {
  if (List_validState(l) != OK)
    return INVALID;
  if (!ammount)
    return OK;
  if (l->size < l->length + ammount) {
    unsigned int newsize = l->size;
    while (newsize < l->length + ammount) {
      newsize = LIST_GROW_EQ(newsize);
    }
    if (List_resize(l, newsize) != OK)
      return CANTRESIZE;
  }
  memcpy(l->head + l->length * l->width, source, ammount * l->width);
  l->length += ammount;
  return OK;
}

List *List_deepCopy(List *l) {
  return List_fromArr(l->allocator, l->head, l->width, l->length);
}

LList_head *LList_new(const My_allocator *allocator, size_t size) {
  LList_head *res = (LList_head *)aAlloc(allocator, size + sizeof(LList_head));
  *res = (LList_head){
      NULL,
      NULL,
      size,
      allocator,
  };
  return res;
}
void LList_free(LList_head *l) {
  LList_element *e = l->first;
  while (e) {
    LList_element *n = e->next;
    aFree(l->allocator, e);
    e = n;
  }
  aFree(l->allocator, l);
}
List_opError LList_push(LList_head *l, const void *val) {
  LList_element *newelement = (LList_element *)aAlloc(l->allocator, l->elementSize + sizeof(LList_element));
  if (!newelement)
    return List_opErrorS.CantResize;
  if (val)
    memcpy(newelement->data, val, l->elementSize);
  else
    memset(newelement->data, 0, l->elementSize);

  if (!l->first) {
    l->first = newelement;
    l->last = newelement;
    return List_opErrorS.Ok;
  }
  l->last->next = newelement;
  l->last = newelement;

  return List_opErrorS.Ok;
}

#endif
