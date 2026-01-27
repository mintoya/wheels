#ifndef MY_LIST_H
#define MY_LIST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifndef LIST_GROW_EQ
#define LIST_GROW_EQ(uint) ((uint + (uint/2)) + 1)
#endif
// clang-format on
#include "allocator.h"

typedef size_t List_index_t;
typedef struct List {
  size_t width;
  List_index_t length;
  List_index_t capacity;
  uint8_t *head;
  AllocatorV allocator;
} List;
// same as list_resize but it enforces size
void List_forceResize(List *l, List_index_t newSize);

[[gnu::pure]]
/**
 * `@param` **l** list
 * `@return` size of memory used by list
 */
extern inline size_t List_headArea(const List *l) { return (l->width * l->length); }
[[gnu::pure]]
/**
 * `@param` **l** list
 * `@return` size of memory reserved by list
 */
extern inline size_t List_fullHeadArea(const List *l) { return (l->width * l->capacity); }
[[gnu::pure]]
/**
 * `@param` **l** list
 * `@param` **i** index
 * `@return` pointer to i`th element
 *      - even if i is out of bounds
 */
extern inline void *List_getRefForce(const List *l, List_index_t i) { return (l->head + l->width * i); };
[[gnu::pure]]
/**
 * `@param` **l** list
 * `@param` **i** index
 * `@return` pointer to i`th element
 *      - null if i is more than **l**.length
 */
extern void *List_getRef(const List *l, List_index_t i) { return (i < l->length) ? (l->head + l->width * i) : (NULL); }
/**
 * writes list to **l**
 * `@param` **allocator** allocator
 * `@param` **l** size of each element
 * `@param` **bytes** size of each element
 * `@param` **init** initial capacity
 */
void List_makeNew(AllocatorV allocator, List *l, size_t bytes, List_index_t init);
extern inline void List_resize(List *l, List_index_t newSize) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->capacity || newSize < l->capacity / 8))
    return List_forceResize(l, newSize);
  return;
}

/**
 * creates new list
 * `@param` **allocator** allocator
 * `@param` **bytes** size of each element
 * `@return` new list
 */
extern inline List *List_new(AllocatorV allocator, size_t bytes) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, 2);
  return l;
}
/**
 * frees list and its array
 * `@param` **l** list
 */
void List_free(List *l);
extern inline List *List_newInitL(AllocatorV allocator, size_t bytes, uint32_t initSize) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, bytes, initSize);
  return l;
}

/**
 * appends element to list
 * `@param` **l** list
 * `@param` **element** pointer to value
 * `@return` **element** pointer to value *inside list*
 */
void *List_append(List *l, const void *element);
/**
 * inserts element into list
 * `@param` **l** list
 * `@param` **i** index to insert
 * `@param` **element** pointer to value
 */
void List_insert(List *l, List_index_t i, void *element);
/**
 * create list from array
 * `@param` **allocator** allocator
 * `@param` **source** pointer to value
 * `@param` **size**  element size
 * `@param` **length** element count
 * `@reutrn` new list
 */
List *List_fromArr(AllocatorV, const void *source, size_t size, List_index_t length);
/**
 * inserts elements into list
 * `@param` **l** list
 * `@param` **source** pointer to value
 * `@param` **length** element count
 * `@return` adress of first inserted element
 */
void *List_appendFromArr(List *l, const void *source, List_index_t length);

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
/*
 * searches for a value which has an identical value to element
 * `@param` **list**
 * `@param` **element** :pointer to list element searched
 * `@return` length of list if it doesnt exist
 */
extern inline List_index_t List_locate(const List *l, const void *element);
extern inline void List_remove(List *l, List_index_t i);
/*
 * sets all bits in space reserved to 0
 * `@param` **list**
 */
extern inline void List_zeroOut(List *l);
List *List_deepCopy(List *l);

typedef bool (*List_searchFunc)(void *ctx, void *, void *);
/**
 * insert into sorted list
 * `@param` **1** list     : list
 * `@param` **2** element  : pointer to element
 * `@param` **3** search fn: list search function pointer
 */

void List_insertSorted(List *, void *, List_searchFunc, void *ctx);
/**
 * binary search for sorted list
 * `@param` **1** list     : list
 * `@param` **2** element  : pointer to element
 * `@param` **3** search fn: list search function pointer
 */
List_index_t List_searchSorted(List *, void *, List_searchFunc, void *arb);
void List_qsort(List *l, List_searchFunc sorter, void *ctx);

static void List_cleanup_handler(void *ListPtrPtr) {
  List **l = (List **)ListPtrPtr;
  if (l && *l)
    List_free(*l);
  *l = NULL;
}
#define List_scoped [[gnu::cleanup(List_cleanup_handler)]] List
#ifdef __cplusplus
  #include <type_traits>
template <typename T>
using mList_t = T (**)(List *);
  #define mList(T) mList_t<T>
#else
  #define mList(T) typeof(T(**)(List *))
#endif

#define mList_scoped(T) [[gnu::cleanup(List_cleanup_handler)]] mList(T)

#define MLIST_INIT_HELPER(allocator, T, initLength, ...) ((mList(T))List_newInitL(allocator, sizeof(T), initLength))
#define mList_init(allocator, T, ...) \
  MLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)
#define mList_iType(list) typeof((*list)(NULL))
#define mList_deInit(list)   \
  do {                       \
    List_free((List *)list); \
  } while (0)

#define mList_arr(list) (((typeof((*list)(NULL)) *)(((List *)(list))->head)))
#define mList_len(list) (((List *)(list))->length)
#define mList_cap(list) (((List *)(list))->capacity)

#define mList_push(list, val)                  \
  do {                                         \
    typeof(typeof((*list)(NULL))) value = val; \
    List_append((List *)list, &value);         \
  } while (0)
#define mList_pop(list) ({                                                            \
  ((List *)(list))->length--;                                                         \
  *(typeof((*list)(NULL)) *)List_getRefForce((List *)(list), ((List *)list)->length); \
})
#define mList_get(list, index) ({ (typeof((*list)(NULL)) *)List_getRef((List *)list, index); })
#define mList_set(list, index, val)        \
  do {                                     \
    typeof((*list)(NULL)) value = val;     \
    List_set((List *)list, index, &value); \
  } while (0)
#define mList_ins(list, index, val)           \
  do {                                        \
    typeof((*list)(NULL)) value = val;        \
    List_insert((List *)list, index, &value); \
  } while (0)
#define mList_rem(list, index)        \
  do {                                \
    List_remove((List *)list, index); \
  } while (0)
#define each_mList(list, e)                   \
  typeof((*list)(NULL)) *e = mList_arr(list); \
  e < mList_arr(list)[mList_len(list)];       \
  e++
#define mList_foreach(list, valtype, value, ...)            \
  do {                                                      \
    for (List_index_t _i = 0; _i < mList_len(list); _i++) { \
      valtype value = *mList_get(list, _i);                 \
      {                                                     \
        __VA_ARGS__                                         \
      }                                                     \
    }                                                       \
  } while (0)
#define mList_setCap(list, capacity)            \
  do {                                          \
    List_forceResize((List *)(list), capacity); \
  } while (0)
#define mList_pushArr(list, ptr, len)           \
  do {                                          \
    List_appendFromArr((List *)list, ptr, len); \
  } while (0)
// #define mList_sort(list, sortFunc) \
//   do {                             \
//     static_assert();                \
//   } while (0)

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
void List_makeNew(AllocatorV allocator, List *l, size_t bytes, List_index_t initialSize) {
  *l = (List){
      .width = bytes,
      .length = 0,
      .capacity = initialSize,
      .head = (uint8_t *)aAlloc(allocator, bytes * initialSize),
      .allocator = allocator,
  };
  if (allocator->size) {
    l->capacity = allocator->size(allocator, l->head) / l->width;
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
inline List_index_t List_locate(const List *l, const void *element) {
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
  if (l->capacity < l->length + 1)
    List_resize(l, LIST_GROW_EQ(l->length));
  memmove(l->head + (i + 1) * l->width, l->head + (i)*l->width, (l->length - i) * l->width);
  List_set(l, i, element);
  l->length++;
}
void *List_append(List *l, const void *element) {
  if (l->capacity < l->length + 1)
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
    l->capacity = newSize;
    if (l->allocator->size)
      l->capacity = l->allocator->size(l->allocator, l->head) / l->width;
    l->length = (l->length < l->capacity) ? (l->length) : (l->capacity);
  }
  return;
}
List *List_fromArr(AllocatorV allocator, const void *source, size_t width, List_index_t length) {
  List *res = (List *)aAlloc(allocator, sizeof(List));
  res->width = width;
  res->length = length;
  res->capacity = length;
  res->allocator = allocator;
  res->head = (uint8_t *)aAlloc(allocator, length * width);
  if (res && res->head && source)
    memcpy(res->head, source, length * width);
  return res;
}
void *List_appendFromArr(List *l, const void *source, List_index_t ammount) {
  if (!ammount)
    return NULL;
  if (l->capacity < l->length + ammount)
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

List_index_t List_searchSorted(List *l, void *element, List_searchFunc sf, void *ctx) {
  List_index_t low = 0;
  List_index_t high = List_length(l);

  while (low < high) {
    List_index_t mid = low + (high - low) / 2;
    void *mid_val = List_getRefForce(l, mid);

    if (sf(ctx, element, mid_val))
      high = mid;
    else
      low = mid + 1;
  }
  return low;
}
inline void List_swap(List *l, List_index_t a, List_index_t b) {
  void *sc = List_append(l, NULL);
  l->length--;
  memcpy(
      List_getRefForce(l, l->length),
      List_getRefForce(l, a),
      l->width
  );
  memcpy(
      List_getRefForce(l, a),
      List_getRefForce(l, b),
      l->width
  );
  memcpy(
      List_getRefForce(l, b),
      List_getRefForce(l, l->length),
      l->width
  );
}
inline void List_qsortRecursive(List *l, List_index_t low, List_index_t high, List_searchFunc sorter, void *ctx) {
  if (high - low < 2)
    return;

  void *pivot = List_getRefForce(l, high - 1);
  List_index_t split = low;

  for (List_index_t j = low; j < high - 1; j++) {
    void *curr = List_getRefForce(l, j);
    if (sorter(ctx, curr, pivot)) {
      List_swap(l, split, j);
      split++;
    }
  }

  List_swap(l, split, high - 1);

  List_qsortRecursive(l, low, split, sorter, ctx);
  List_qsortRecursive(l, split + 1, high, sorter, ctx);
}

void List_qsort(List *l, List_searchFunc sorter, void *ctx) {
  List_qsortRecursive(l, 0, List_length(l), sorter, ctx);
}

void List_insertSorted(List *l, void *element, List_searchFunc sf, void *ctx) {
  List_insert(l, List_searchSorted(l, element, sf, ctx), element);
}
#endif
