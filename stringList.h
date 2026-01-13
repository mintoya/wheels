#ifndef STRING_LIST_H
#include "assertMessage.h"
#define STRING_LIST_H
#include "allocator.h"
#include "fptr.h"
#include "my-list.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  u32 index;
  u32 width;
} stringMetaData;

typedef struct {
  List List_char;
  List List_stringMetaData;
} stringList;
static inline size_t stringList_footprint(stringList *sl) {
  return List_fullHeadArea(&(sl->List_stringMetaData)) +
         List_fullHeadArea(&(sl->List_char));
}

static inline void stringList_preload(stringList *sl, uint elementCount, uint elementSize) {
  List_resize(&(sl->List_stringMetaData), elementCount);
  List_resize(&(sl->List_char), elementCount * elementSize);
}

stringList *stringList_new(const My_allocator *);
fptr stringList_get(stringList *l, u32 index);
ffptr stringList_getExt(stringList *l, u32 index);
void stringList_insert(stringList *l, fptr, u32 index);
void stringList_set(stringList *l, fptr, u32);
unsigned int stringList_append(stringList *l, fptr);
void stringList_remove(stringList *l, u32 idx);

#define stringList_fromStat(allocator, char_ptr_ptr)                 \
  ({                                                                 \
    stringList *res = stringList_new(allocator);                     \
    for (uint i = 0; i < sizeof(char_ptr_ptr) / sizeof(char *); i++) \
      stringList_append(res, fp_from(char_ptr_ptr[i]));              \
    res;                                                             \
  })

typedef struct {
  unsigned int metaSize;
  u8 *Arr_stringMetaData;
  u8 *Arr_char;
} stringList_Solid;

// returns length if not found
u32 stringList_search(stringList *l, fptr key);
// new sl with unused space removed
stringList *stringList_remake(stringList *);
void stringList_free(stringList *l);
[[gnu::pure]] static inline unsigned int stringList_length(stringList *l) {
  return l ? l->List_stringMetaData.length : 0;
}
static inline void stringList_cleanup_handler(stringList **sl) {
  if (sl && *sl)
    stringList_free(*sl);
  *sl = NULL;
}

#define stringList_scoped \
  [[gnu::cleanup(stringList_cleanup_handler)]] stringList

#endif // STRING_LIST_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#pragma once
#define STRING_LIST_C (1)
#endif

#ifdef STRING_LIST_C
#include <string.h>

stringList *stringList_new(const My_allocator *allocator) {
  stringList *res = (stringList *)aAlloc(allocator, sizeof(stringList));
  List_makeNew(allocator, &(res->List_char), sizeof(char), 2);
  List_makeNew(allocator, &(res->List_stringMetaData), sizeof(stringMetaData), 2);
  return res;
}
#define max(a, b) ((a < b) ? (b) : (a))

fptr stringList_get(stringList *l, u32 index) {
  if (index >= l->List_stringMetaData.length)
    return nullFptr;
  stringMetaData thisS =
      mList_get(&(l->List_stringMetaData), stringMetaData, index);
  u8 *listPlace = (uint8_t *)(List_getRef(&(l->List_char), thisS.index));
  assertMessage(listPlace != NULL, "stringlist index out of bounds\n"
                                   "data length: %u:\n"
                                   "  index :%u\n"
                                   "  length:%u\n",
                l->List_char.length, thisS.index, thisS.width);
  return ((fptr){
      .width = thisS.width,
      .ptr = listPlace,
  });
}
unsigned int stringList_search(stringList *l, fptr what) {
  stringMetaData *meta = (stringMetaData *)l->List_stringMetaData.head;
  unsigned int res = 0;
  unsigned int length = stringList_length(l);

  for (; res < length; res++) {
    auto thisS = ((fptr){
        .width = meta[res].width,
        .ptr = (uint8_t *)List_getRef(&(l->List_char), meta[res].index),
    });
    if (fptr_eq(thisS, what))
      return res;
  }
  return res;
}
stringList *stringList_remake(stringList *origional) {
  stringList *res = stringList_new(origional->List_char.allocator);
  for (unsigned int i = 0; i < stringList_length(origional); i++)
    stringList_append(res, stringList_get(origional, i));
  return res;
}
unsigned int stringList_append(stringList *l, fptr value) {
  uint uwidth = (uint)value.width;
  stringMetaData thisS = {
      .index = l->List_char.length,
      .width = uwidth,
  };
  List_append(&(l->List_stringMetaData), &thisS);
  List_appendFromArr(&(l->List_char), value.ptr, value.width);
  return l->List_stringMetaData.length - 1;
}
void stringList_insert(stringList *l, fptr value, unsigned int index) {
  uint uwidth = (uint)value.width;
  stringMetaData thisS = {
      .index = l->List_char.length,
      .width = uwidth,
  };
  mList_insert(&(l->List_stringMetaData), stringMetaData, thisS, index);
  List_appendFromArr(&(l->List_char), value.ptr, value.width);
}
void stringList_remove(stringList *l, u32 idx) {
  List_remove(&(l->List_stringMetaData), idx);
}
void stringList_set(stringList *l, fptr value, u32 index) {
  assert(value.width < (size_t)((u32)-1));
  if (index == stringList_length(l)) {
    stringList_append(l, value);
    return;
  }
  stringMetaData thisS =
      mList_get(&(l->List_stringMetaData), stringMetaData, index);
  if (thisS.width < value.width) {
    stringMetaData *ref =
        (stringMetaData *)List_getRef(&(l->List_stringMetaData), index);
    ref->width = value.width;
    ref->index = l->List_char.length;
    List_appendFromArr(&(l->List_char), value.ptr, value.width);
  } else {
    stringMetaData *ref =
        (stringMetaData *)List_getRef(&(l->List_stringMetaData), index);
    ref->width = value.width;
    if (value.width && value.ptr)
      memcpy(List_getRef(&(l->List_char), ref->index), value.ptr, value.width);
    else
      assertMessage(!value.width && !value.ptr, "null pointer to %llu bytes!", value.width);
  }
}

void stringList_free(stringList *l) {
  const My_allocator *allocator = l->List_char.allocator;
  aFree(allocator, l->List_char.head);
  aFree(allocator, l->List_stringMetaData.head);
  aFree(allocator, l);
}

#endif // STRING_LIST_C
