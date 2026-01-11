// #ifdef __cplusplus
// #error macrolist.h not supported in cpp, just use the class
// #else
#ifndef MACROLIST_H
  #define MACROLIST_H
  #include "fptr.h"
  #include "my-list.h"
  #include "types.h"

  #define MList(type)        \
    struct MacroList##type { \
      usize *length;         \
      type *elements;        \
    }

  // automatically freed
  #define MList_init(list)                                             \
    List_scoped *MList_heapList(list) =                                \
        List_new(&defaultAllocator, sizeof(typeof(list.elements[0]))); \
    list = MList_deconvert((*MList_heapList(list)), list);

  #define MList_push(list, ...)                              \
    do {                                                     \
      MList_heapList(list)->length = list.length;            \
      typeof(*(list.elements)) __val = __VA_ARGS__;          \
      List_append(MList_heapList(list), &__val);             \
      list = MList_deconvert((*MList_heapList(list)), list); \
    } while (0)

  #define MList_pop(list)                                    \
    ({                                                       \
      MList_heapList(list)->length--;                        \
      list = MList_deconvert((*MList_heapList(list)), list); \
      list.elements[list.length];                            \
    })

  #define MList_insert(list, index, ...)                        \
    do {                                                        \
      MList_heapList(list)->length = list.length;               \
      typeof(*(list.elements)) __val = __VA_ARGS__;             \
      List_insert(MList_heapList(list), index, (void *)&__val); \
      list = MList_deconvert((*MList_heapList(list)), list);    \
    } while (0)

  #define MList_foreach(list, index, element, ...)               \
    do {                                                         \
      for (size_t index = 0; index < list.length; index++) {     \
        typeof(list.elements[0]) element = list.elements[index]; \
        __VA_ARGS__                                              \
      }                                                          \
    } while (0)
  #define MList_addArr(list, size, ...)                                  \
    do {                                                                 \
      MList_heapList(list)->length = list.length;                        \
      typeof(list.elements) __temp = (typeof(list.elements))__VA_ARGS__; \
      void *__ref = (void *)__temp;                                      \
      List_appendFromArr(MList_heapList(list), __ref, size);             \
      list = MList_deconvert((*MList_heapList(list)), list);             \
    } while (0)

  #define MList_capacity(list) MList_heapList(list)->size

#endif // MACROLIST_H
// #endif // cpp
