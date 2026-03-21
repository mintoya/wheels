#include "../mylist.h"
static inline _Bool memzeroed(void *mem, size_t len) {
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
