#ifndef MY_LIST_H
#define MY_LIST_H
#include "assertMessage.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef LIST_GROW_EQ
  #define LIST_GROW_EQ(uint) (uint + uint / 2 + 1)
#endif
#include "allocator.h"

typedef size_t List_index_t;
typedef struct List {
  List_index_t length;
  List_index_t capacity;
  AllocatorV allocator;
  uint8_t *head;
} List;

void List_forceResize(List *l, List_index_t newSize, size_t width);

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
void List_makeNew(AllocatorV allocator, List *l, size_t bytes, List_index_t init);
extern inline void List_resize(List *l, List_index_t newSize, size_t width);

/**
 * creates new list
 * @param allocator allocator
 * @param bytes size of each element
 * @return new list
 */
static inline List *List_new(AllocatorV allocator, size_t width) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
  List_makeNew(allocator, l, width, 2);
  return l;
}
/**
 * frees list and its array
 * @param l list
 */
void List_free(List *l);
static inline List *List_newInitL(AllocatorV allocator, size_t bytes, uint32_t initSize) {
  List *l = (List *)aAlloc(allocator, sizeof(List));
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
 * inserts element into list
 * @param l list
 * @param i index to insert
 * @param element pointer to value
 */
void List_insert(List *l, List_index_t i, void *element, size_t width);
/**
 * create list from array
 * @param allocator allocator
 * @param length element count
 * @reutrn new list
 */
List *List_fromArr(AllocatorV, const void *source, size_t size, List_index_t length);
/**
 * inserts elements into list
 * @param l list
 * @param source pointer to value
 * @param length element count
 * @return adress of first inserted element
 */
void *List_appendFromArr(List *l, const void *source, List_index_t length, size_t width);
/**
 * inserts list into list
 * @param l list
 * @param source pointer to values
 * @param length element count
 * @return adress of first inserted element
 */
void *List_insertFromArr(List *l, const void *source, List_index_t length, List_index_t location, size_t width);

__attribute__((pure)) static inline List_index_t List_length(const List *l) { return l ? l->length : 0; }
/*
 * searches for a value which has an identical value to element
 * @param list
 * @param element :pointer to list element searched
 * @return length of list if it doesnt exist
 */
extern inline List_index_t List_locate(const List *l, const void *element, size_t width);
extern inline void List_remove(List *l, List_index_t i, size_t width);
/*
 * sets all bits in space reserved to 0
 * @param list
 */
extern inline void List_zeroOut(List *l, size_t width);
List *List_deepCopy(List *l, size_t width);

__attribute__((unused)) static void List_cleanup_handler(void *ListPtrPtr) {
  List **l = (List **)ListPtrPtr;
  if (l && *l) {
    List_free(*l);
    *l = NULL;
  }
}
#ifdef __cplusplus
template <typename T>
using mList_t = T (**)(List *);
  #define mList(T) mList_t<T>
#else
  #define mList(T) typeof(T(**)(List *))
#endif

#include "macros.h"
#define mList_scoped(T) __attribute__((cleanup(List_cleanup_handler))) mList(T)

#define MLIST_INIT_HELPER(allocator, T, initLength, ...) ((mList(T))List_newInitL(allocator, sizeof(T), initLength))
#define mList_init(allocator, T, ...) \
  MLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)

#define mList_iType(list) typeof((*list)(NULL))
#define mList_deInit(list)   \
  do {                       \
    List_free((List *)list); \
  } while (0)

#define mList_arr(list) (((mList_iType(list) *)(((List *)(list))->head)))
#define mList_len(list) (((List *)(list))->length)
#define mList_cap(list) (((List *)(list))->capacity)

#define mList_push(list, val)                            \
  do {                                                   \
    if (mList_len(list) >= mList_cap(list)) [[unlikely]] \
      List_resize(                                       \
          (List *)list,                                  \
          LIST_GROW_EQ(mList_len(list)),                 \
          sizeof(mList_iType(list))                      \
      );                                                 \
    mList_arr(list)[mList_len(list)++] = (val);          \
  } while (0)

#define mList_pop(list) ({            \
  mList_arr(list)[--mList_len(list)]; \
})
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
  do {                                                           \
    List_remove((List *)list, index, sizeof(mList_iType(list))); \
  } while (0)
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
#define mList_pushArr(list, vla)                                      \
  do {                                                                \
    0 + ASSERT_EXPR(types_eq(typeof(vla[0]), mList_iType(list)), ""); \
    List_appendFromArr(                                               \
        (List *)list,                                                 \
        vla,                                                          \
        sizeof(vla) / sizeof(vla[0]),                                 \
        sizeof(vla[0])                                                \
    );                                                                \
  } while (0)
#define mList_insArr(list, position, vla)                                                          \
  do {                                                                                             \
    0 + ASSERT_EXPR(types_eq(typeof(vla[0]), mList_iType(list)), "");                              \
    List_insertFromArr((List *)list, vla, sizeof(vla) / sizeof(vla[0]), position, sizeof(vla[0])); \
  } while (0)
#define mList_pad(list, ammount)  \
  do {                            \
    List_appendFromArr(           \
        (List *)list,             \
        NULL, ammount,            \
        sizeof(mList_iType(list)) \
    );                            \
  } while (0)
#define mList_clear(list)       \
  do                            \
    ((List *)list)->length = 0; \
  while (0)
#define mList_vla(list) ((typeof(typeof(mList_iType(list)))(*)[mList_len(list)])mList_arr(list))
// #include "tests.c"
#if defined(MAKE_TEST_FN)
MAKE_TEST_FN(mlist_push_pop, {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deInit(list); };

  for (each_RANGE(usize, i, 0, 50))
    mList_push(list, i * i);
  for (each_RANGE(usize, i, 0, 50))
    if (mList_arr(list)[i] != i * i)
      return 1;

  return 0;
});
MAKE_TEST_FN(mlist_insert_remove, {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deInit(list); };
  mList_push(list, 100);
  mList_push(list, 300);
  mList_ins(list, 1, 200);
  if (mList_len(list) != 3)
    return 1;
  if (mList_arr(list)[1] != 200)
    return 1;
  mList_rem(list, 0);
  if (mList_len(list) != 2)
    return 1;
  if (mList_arr(list)[0] != 200)
    return 1;
  if (mList_popFront(list) != 200)
    return 1;
  if (mList_len(list) != 1)
    return 1;
  if (mList_arr(list)[0] != 300)
    return 1;
  return 0;
});
MAKE_TEST_FN(mlist_array_operations, {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deInit(list); };

  int arr1[] = {1, 2, 3};
  mList_pushArr(list, arr1);
  if (mList_len(list) != 3)
    return 1;
  if (mList_arr(list)[2] != 3)
    return 1;

  int arr2[] = {0};
  mList_insArr(list, 0, arr2);
  if (mList_len(list) != 4)
    return 1;
  if (mList_arr(list)[0] != 0)
    return 1;
  if (mList_arr(list)[1] != 1)
    return 1;

  return 0;
});
MAKE_TEST_FN(mlist_capacity_and_padding, {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deInit(list); };

  mList_reserve(list, 50);
  if (mList_cap(list) < 50)
    return 1;

  mList_pad(list, 5);
  if (mList_len(list) != 5)
    return 1;

  mList_setCap(list, 10);
  if (mList_cap(list) < 10)
    return 1;

  mList_clear(list);
  if (mList_len(list) != 0)
    return 1;

  return 0;
});
MAKE_TEST_FN(mlist_vla_cast, {
  mList(int) list = mList_init(allocator, int);
  defer { mList_deInit(list); };
  mList_push(list, 7);
  mList_push(list, 8);
  mList_push(list, 9);
  mList_pushArr(list, *mList_vla(list));
  if (mList_len(list) != 6)
    return 1;
  return 0;
});
#endif
#endif // MY_LIST_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
#define MY_LIST_C (1)
#endif

#if defined(MY_LIST_C)
static inline int memzeroed(void *mem, size_t len) {
  for (; len > 0; len--) {
    if (((uint8_t *)mem)[len - 1])
      return false;
  }
  return true;
}

// all bytes list owns
void List_makeNew(AllocatorV allocator, List *l, size_t width, List_index_t initialSize) {
  l->length = 0;
  l->allocator = allocator;
  l->head = aCreate(allocator, uint8_t, width *initialSize);
  l->capacity = allocator->size
                    ? allocator->size(allocator, l->head) / width
                    : initialSize;
}
void List_free(List *l) {
  if (!l || !l->allocator)
    return;
  if (l->head)
    aFree(l->allocator, l->head);
  l->head = NULL;
  aFree(l->allocator, l);
}
extern inline void List_resize(List *l, List_index_t newSize, size_t width) {
  newSize = newSize ? newSize : 1;
  if ((newSize > l->capacity || newSize < l->capacity / 8))
    return List_forceResize(l, newSize, width);
  return;
}
inline List_index_t List_locate(const List *l, const void *element, size_t width) {
  List_index_t i = 0;
  if (element) {
    for (; i < l->length; i++) {
      if (!memcmp(element, List_getRef(l, i, width), width))
        return i;
    }
  } else {
    for (; i < l->length; i++) {
      if (memzeroed(List_getRef(l, i, width), width))
        return i;
    }
  }
  return i;
}
inline void List_zeroOut(List *l, size_t w) { memset(l->head, 0, w * l->capacity); }
void List_insert(List *l, List_index_t i, void *element, size_t w) {
  List_insertFromArr(l, element, 1, i, w);
}
extern inline void List_remove(List *l, List_index_t i, size_t width) {
  if (i >= l->length)
    return;
  memmove(l->head + i * width, l->head + (i + 1) * width, (l->length - i - 1) * width);
  l->length--;
}
void List_forceResize(List *l, List_index_t newSize, size_t width) {
  uint8_t *newPlace =
      (uint8_t *)aResize(l->allocator, l->head, newSize * width);
  assertMessage(newPlace);
  if (!newPlace) {
    fprintf(stderr, "cant resize list");
    abort();
  } else {
    l->head = newPlace;
    l->capacity = newSize;
    if (l->allocator->size)
      l->capacity = l->allocator->size(l->allocator, l->head) / width;
    l->length = (l->length < l->capacity) ? (l->length) : (l->capacity);
  }
  return;
}
List *List_fromArr(AllocatorV allocator, const void *source, size_t width, List_index_t length) {
  List *res = (List *)aAlloc(allocator, sizeof(List));
  width = width;
  res->length = length;
  res->capacity = length;
  res->allocator = allocator;
  res->head = (uint8_t *)aAlloc(allocator, length * width);
  if (res && res->head && source)
    memcpy(res->head, source, length * width);
  return res;
}
void *List_insertFromArr(List *l, const void *source, List_index_t length, List_index_t location, size_t width) {
  if (!length || location > l->length)
    return NULL;
  if (l->capacity < l->length + length)
    List_resize(l, l->length + length, width);
  void *res = l->head + (location)*width;
  memmove(l->head + (location + length) * width, res, (l->length - location) * width);
  if (source)
    memcpy(res, source, length * width);
  else
    memset(res, 0, length * width);
  l->length += length;
  return res;
}
void *List_appendFromArr(List *l, const void *source, List_index_t ammount, size_t width) {
  return List_insertFromArr(l, source, ammount, l->length, width);
}

List *List_deepCopy(List *l, size_t width) { return List_fromArr(l->allocator, l->head, width, l->length); }
#endif
