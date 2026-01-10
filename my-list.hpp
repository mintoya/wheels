#pragma once
#include "allocator.h"
#include "my-list.h"

template <typename T>
struct listPlus {
  List *ptr;
  bool dofree : 1;
  listPlus(List *p) { ptr = p; }

  listPlus(AllocatorV allocator = defaultAlloc) {
    ptr = List_new(allocator, sizeof(T));
    dofree = true;
  }

  listPlus(T *arr, unsigned int length, AllocatorV allocator = defaultAlloc) { return (T *)ptr->head; }
  inline const unsigned int length() { return ptr->length; }

  inline void dontfree() { dofree = false; }
  inline void unmake() {
    List_free(ptr);
    ptr = nullptr;
  }
  ~listPlus() {
    if (dofree)
      unmake();
  }

  void pad(unsigned int ammount) { List_pad(ptr, ammount); }
  void resize(unsigned int newSize) { List_resize(ptr, newSize); }

  inline void append(const T &value) { List_append(ptr, (void *)&value); }
  inline void push(const T &value) { List_append(ptr, (void *)&value); }
  inline T get(unsigned int i) { return *((T *)List_getRef(ptr, i)); }
  T pop() {
    T res = get(length() - 1);
    ptr->length--;
    return res;
  }

  void appendArr(const T values[], unsigned int len) {
    List_appendFromArr(ptr, values, len);
  }

  void insert(const T &value, int index) {
    List_insert(ptr, index, (void *)&value);
  }

  void set(unsigned int i, const T &value) { List_set(ptr, i, &value); }
  int searchFor(const T &value) { return List_search(ptr, &value); }
  inline void clear() { ptr->length = 0; }
  inline unsigned int length() const { return ptr->length; }
  inline unsigned int capacity() const { return ptr->size; }
  template <typename FN>
  void foreach (FN &&function) {
    for (int i = 0; i < length(); i++)
      function(i, get(i));
  }
};
