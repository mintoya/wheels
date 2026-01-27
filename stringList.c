#include <cstddef>
#include <stdlib.h>
#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
typedef struct stringList stringList;
  #include "fptr.h"
  #include "my-list.h"
  #include <stddef.h>
  #include <stdint.h>
stringList *stringList_new(AllocatorV allocator, usize initSize);
void stringList_free(stringList *sl);
fptr stringList_get(stringList *, List_index_t);
fptr stringList_set(stringList *, List_index_t, fptr);
void stringList_remove(stringList *, List_index_t);
fptr stringList_append(stringList *, List_index_t, fptr);
void stringList_insert(stringList *, List_index_t, fptr);
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define STRING_LIST_C (1)
#endif

#if defined(STRING_LIST_C)

  #include "my-list.h"
  #include "types.h"
struct vlength {
  #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  u8 hasNext : 1;
  u8 data : 7;
  #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  u8 data : 7;
  u8 hasNext : 1;
  #else
    #error "undefiend byte order"
  #endif
};
static_assert(sizeof(struct vlength) == sizeof(u8), "vlength warning");

struct {
  struct vlength arr[sizeof(usize) * 8 / 7 + 1];
} u64_toVlen(usize len) {
  typeof(u64_toVlen(0)) res = {0};
  for (int i = _countof(res.arr), j = 0; i > 0; i--, j++) {
    res.arr[j].data = (len >> ((i - 1) * 7)) & 0x7f;
    res.arr[j].hasNext = 1;
  }
  res.arr[_countof(res.arr) - 1].hasNext = 0;
  return res;
}
usize vlen_toUsize(struct vlength *vlen) {
  usize res = 0;
  while (1) {
    res = (res << 7) | (vlen->data & 0x7f);
    if (!vlen->hasNext)
      break;
    vlen++;
  }
  return res;
}
typedef struct {
  struct vlength header[1];
} *vlqbuf;
  #define vlen_stat(stringLiteral) ({                   \
    static struct [[gnu::packed]] {                     \
      typeof(usize_toVLen(0)) len;                      \
      typeof(stringLiteral) str;                        \
    } res;                                              \
    res = (typeof(res)){                                \
        .len = usize_toVLen(sizeof(stringLiteral) - 1), \
        .str = stringLiteral,                           \
    };                                                  \
    (vlqbuf) & res;                                     \
  })
fptr vlqbuf_toFptr(vlqbuf b) {
  return (fptr){
      .width = vlen_toUsize(b->header),
      .ptr = ({
        struct vlength *ptr = b->header;
        for (; ptr->hasNext; ptr++)
          ;
        (u8 *)(ptr + 1);
      })
  };
}
typedef struct stringList {
  mList(ptrdiff_t) ulist;
  mList(ptrdiff_t) flist;
  usize len;
  usize cap;
  u8 *ptr;
} stringList;
stringList *stringList_new(AllocatorV allocator, usize initSize) {
  stringList *res = aCreate(allocator, stringList);
  *res = (typeof(*res)){
      .flist = mList_init(allocator, ptrdiff_t),
      .ulist = mList_init(allocator, ptrdiff_t),
      .len = 0,
      .ptr = aAlloc(allocator, initSize),
  };
  if (allocator->size)
    res->cap = allocator->size(allocator, res->ptr);
  else
    res->cap = initSize;
  return res;
}
void stringList_free(stringList *sl) {
  AllocatorV allocator = ((List *)(sl->flist))->allocator;
  mList_deInit(sl->flist);
  mList_deInit(sl->ulist);
  aFree(allocator, sl->ptr);
  aFree(allocator, sl);
}
bool freeList_searchFunc(void *slp, void *a, void *b) {
  u64 alen = vlen_toUsize((struct vlength *)(((stringList *)slp)->ptr + *(ptrdiff_t *)a));
  u64 blen = vlen_toUsize((struct vlength *)(((stringList *)slp)->ptr + *(ptrdiff_t *)b));
  return alen > blen;
}
fptr stringList_append(stringList *sl, List_index_t idx, fptr ptr) {
  typeof(u64_toVlen(0)) lenStruct = u64_toVlen(ptr.width);
  struct vlength *vl = lenStruct.arr;
  List_index_t nextPlace = List_searchSorted((List *)sl->flist, vl, freeList_searchFunc);
}
fptr stringList_set(stringList *sl, List_index_t idx, fptr ptr) {
}

#endif
