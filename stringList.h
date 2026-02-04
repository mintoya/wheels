#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
  #include "my-list.h"
  #include "types.h"
  #include <stddef.h>
  #include <string.h>
typedef struct vlength {
  u8 hasNext : 1;
  u8 data : 7;
} vlength;

  #include "fptr.h"
fptr vlqbuf_toFptr(vlength *b);
typedef struct stringList {
  mList(ptrdiff_t) ulist; ///< use-list not sorted,
                          /// but sorting this is fine
  mList(ptrdiff_t) flist; ///< free-list
                          /// sorted, dont move anything
  usize len;
  usize cap;
  vlength *ptr;
} stringList;
  #include "fptr.h"
  #include "my-list.h"
  #include <stddef.h>
  #include <stdint.h>
stringList *stringList_new(AllocatorV allocator, usize initSize);
void stringList_free(stringList *sl);
fptr stringList_get(stringList *, List_index_t);
void stringList_remove(stringList *, List_index_t);
fptr stringList_append(stringList *, fptr);
fptr stringList_set(stringList *, List_index_t, fptr);
fptr stringList_insert(stringList *, List_index_t, fptr);
extern inline AllocatorV stringList_allocator(stringList *);
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define STRING_LIST_C (1)
#endif

#if defined(STRING_LIST_C)

  #include "my-list.h"
  #include "types.h"
static_assert(sizeof(vlength) == sizeof(u8), "vlength warning");
[[gnu::const]] u8 vl_u8(vlength vl) { return bitcast(u8, vl); }
struct {
  vlength _[sizeof(u64) * 8 / 7 + 1];
} u64_toVlen(u64 len) {
  typeof(u64_toVlen(0)) res = {0};
  for (int i = countof(res._); i > 0; i--)
    res._[countof(res._) - i] = (typeof(*res._)){
        .data = (len >> ((i - 1) * 7)) & 0x7f,
        .hasNext = !!(i - 1),
    };
  return res;
}
u64 vlen_toU64(struct vlength *vlen) {
  u64 res = 0;
  do {
    res = (res << 7) | (vlen->data);
  } while (vlen++->hasNext);
  return res;
}
  #define vlen_stat(stringLiteral) ({                 \
    static struct [[gnu::packed]] {                   \
      typeof(u64_toVLen(0)) len;                      \
      typeof(stringLiteral) str;                      \
    } res;                                            \
    res = (typeof(res)){                              \
        .len = u64_toVLen(sizeof(stringLiteral) - 1), \
        .str = stringLiteral,                         \
    };                                                \
    (vlqbuf) & res;                                   \
  })
fptr vlqbuf_toFptr(vlength *b) {
  return (fptr){
      .width = vlen_toU64(b),
      .ptr = ({
        for (; b->hasNext; b++)
          ;
        (u8 *)(b + 1);
      })
  };
}
stringList *stringList_new(AllocatorV allocator, usize initSize) {
  stringList *res = aCreate(allocator, stringList);
  *res = (typeof(*res)){
      .flist = mList_init(allocator, ptrdiff_t),
      .ulist = mList_init(allocator, ptrdiff_t),
      .len = 0,
      .ptr = aAlloc(allocator, initSize + sizeof(u64_toVlen(0))),
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
// will cause freelist to be sorted from high to low
bool freeList_searchFunc(struct List_sortArg *s) {
  stringList *slp = (typeof(slp))s->arg;
  ptrdiff_t *a = (typeof(a))s->a;
  ptrdiff_t *b = (typeof(a))s->b;

  u64 alen = vlen_toU64((vlength *)(slp->ptr + *a));
  u64 blen = vlen_toU64((vlength *)(slp->ptr + *b));

  return alen > blen;
}
extern inline AllocatorV stringList_allocator(stringList *sl) {
  return ((List *)sl->flist)->allocator;
}
  #include "assertMessage.h"
fptr stringList_append(stringList *sl, fptr ptr) {
  typeof(u64_toVlen(0)) vl_ = u64_toVlen(ptr.width);
  ptrdiff_t place = 0;
  vlength *vl = vl_._;
  usize width_length = countof(vl_._);
  for (; vl_u8(*vl) == vl_u8((vlength){.hasNext = 1, .data = 0}); vl++)
    width_length--;

  if (sl->len + width_length > sl->cap)
    sl->ptr = aRealloc(stringList_allocator(sl), sl->ptr, sl->len + sizeof(vl_));
  memcpy(sl->ptr + sl->len, vl, width_length);
  ptrdiff_t potentialFree = mList_sortedSearch(
      sl->flist,
      ((List_searchFunc){freeList_searchFunc, sl}),
      sl->len
  );
  if (potentialFree) {
    place = *mList_get(sl->flist, potentialFree - 1);
    mList_rem(sl->flist, potentialFree - 1);
  } else {
    place = sl->len;
    AllocatorV allocator = stringList_allocator(sl);
    if (sl->len + countof(vl_._) + ptr.width > sl->cap) {
      sl->ptr =
          aRealloc(
              allocator,
              sl->ptr,
              sl->len + countof(vl_._) + ptr.width + sizeof(u64_toVlen(0))
          );
    }
    sl->len += width_length + ptr.width;
    if (allocator->size)
      sl->cap = allocator->size(allocator, sl->ptr);
    else
      sl->cap = sl->len + width_length + ptr.width;
  }
  mList_push(sl->ulist, place);
  memcpy(sl->ptr + place, vl, width_length);
  memcpy(sl->ptr + place + width_length, ptr.ptr, ptr.width);
  return ((fptr){
      .width = ptr.width,
      .ptr =
          (u8 *)sl->ptr + place + width_length,
  });
}
fptr stringList_get(stringList *sl, List_index_t idx) {
  ptrdiff_t *place = mList_get(sl->ulist, idx);
  return place ? vlqbuf_toFptr(sl->ptr + *place) : nullFptr;
}
void stringList_remove(stringList *sl, List_index_t idx) {
  mList_sortedInsert(
      sl->flist,
      ((List_searchFunc){freeList_searchFunc, sl}),
      *mList_get(sl->ulist, idx)
  );
  mList_rem(sl->ulist, idx);
}
fptr stringList_set(stringList *sl, List_index_t idx, fptr ptr) {
  ptrdiff_t *lastLen = mList_get(sl->ulist, idx);
  assertMessage(lastLen);
  u64 lastLen_length = vlen_toU64(sl->ptr + *lastLen);
  fptr res;
  if (lastLen_length < ptr.width) {
    res = stringList_append(sl, ptr);

    ptrdiff_t nextPlace = mList_pop(sl->ulist);

    ptrdiff_t last = *mList_get(sl->ulist, idx);
    mList_sortedInsert(sl->flist, ((List_searchFunc){freeList_searchFunc, sl}), last);

    mList_set(sl->ulist, idx, nextPlace);
  } else {
    typeof(u64_toVlen(0)) newlen = u64_toVlen(ptr.width);
    vlength *vl_new = newlen._;
    usize width_length = countof(newlen._);
    for (; vl_u8(*vl_new) == vl_u8((vlength){.hasNext = 1, .data = 0}); vl_new++)
      width_length--;
    memcpy(sl->ptr + *lastLen, vl_new, width_length);
    memcpy(sl->ptr + *lastLen + width_length, ptr.ptr, ptr.width);
    res = ((fptr){
        .width = ptr.width,
        .ptr =
            (u8 *)sl->ptr + *lastLen + width_length,
    });
  }
  return res;
}
fptr stringList_insert(stringList *sl, List_index_t idx, fptr ptr) {
  fptr res = stringList_append(sl, ptr);
  ptrdiff_t place = mList_pop(sl->ulist);
  mList_ins(sl->ulist, idx, place);
  return res;
}

#endif
