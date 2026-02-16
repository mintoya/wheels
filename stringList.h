#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
  #include "my-list.h"
  #include "mytypes.h"
  #include <stddef.h>
  #include <string.h>
typedef struct [[gnu::packed]] vlength {
  u8 hasNext : 1;
  u8 data : 7;
} vlength;

  #include "fptr.h"
typedef struct stringList {
  ptrdiff_t *ulist; ///< use-list
                    /// not sorted,
  ptrdiff_t *flist; ///< free-list
                    /// sorted
  usize len, cap;
  vlength *buff;
  AllocatorV allocator;
} stringList;
  #include "fptr.h"
  #include "my-list.h"
  #include <stddef.h>
  #include <stdint.h>
struct u64_vl_max {
  vlength _[sizeof(u64) * 8 / 7 + 1];
};
extern inline struct u64_vl_max u64_toVlen(u64 len) {
  typeof(u64_toVlen(0)) res = {0};
  for (int i = countof(res._); i > 0; i--)
    res._[countof(res._) - i] = (typeof(*res._)){
        .hasNext = !!(i - 1),
        .data = (u8)((len >> ((i - 1) * 7)) & 0x7f),
    };
  return res;
}
extern inline u64 vlen_toU64(struct vlength *vlen) {
  u64 res = 0;
  do
    res = (res << 7) | (vlen->data);
  while (vlen++->hasNext);
  return res;
}
extern inline fptr vlqbuf_toFptr(vlength *b) {
  return (fptr){
      .width = (usize)vlen_toU64(b),
      .ptr = ({
        for (; b->hasNext; b++)
          ;
        (u8 *)(b + 1);
      })
  };
}
stringList *stringList_new(AllocatorV allocator, usize initSize);
void stringList_free(stringList *sl);
fptr stringList_get(stringList *, List_index_t);
fptr stringList_append(stringList *, fptr);
void stringList_remove(stringList *, List_index_t);
List_index_t stringList_len(stringList *);
usize stringList_footprint(stringList *);
fptr stringList_insert(stringList *, List_index_t, fptr);
fptr stringList_set(stringList *, List_index_t, fptr);
  #if defined __cplusplus
struct strList {
  stringList *ptr;
  inline strList(AllocatorV allocator = stdAlloc, usize initSize = 20) {
    ptr = stringList_new(allocator, initSize);
  }
  inline ~strList() { stringList_free(ptr); }
  inline fptr get(List_index_t idx) { return stringList_get(ptr, idx); }
  inline fptr set(List_index_t idx, fptr ptrf) { return stringList_set(ptr, idx, ptrf); }
  inline void remove(List_index_t idx) { return stringList_remove(ptr, idx); }
  inline fptr push(fptr ptrf) { return stringList_append(ptr, ptrf); }
  inline fptr insert(List_index_t idx, fptr ptrf) { return stringList_insert(ptr, idx, ptrf); }
  inline List_index_t len() { return stringList_len(ptr); }
  const fptr operator[](usize idx) { return get(idx); }
};
  #endif
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define STRING_LIST_C (1)
#endif

#if defined(STRING_LIST_C)
  #include "my-list.h"
static_assert(sizeof(vlength) == sizeof(u8), "vlength warning");
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
  #include "shortList.h"
stringList *stringList_new(AllocatorV allocator, usize initSize) {
  stringList *res = aCreate(allocator, stringList);
  *res = (typeof(*res)){
      .ulist = msList_init(allocator, ptrdiff_t),
      .flist = msList_init(allocator, ptrdiff_t),
      .len = 0,
      .buff = (typeof(res->buff))aAlloc(allocator, initSize),
      .allocator = allocator,
  };
  assertMessage(!msList_len(res->ulist), "should be 0, is %llu", (unsigned long long)msList_len(res->ulist));
  assertMessage(!msList_len(res->flist), "should be 0, is %llu", (unsigned long long)msList_len(res->flist));
  if (allocator->size)
    res->cap = allocator->size(allocator, res->buff);
  else
    res->cap = initSize;
  return res;
}
void stringList_free(stringList *slp) {
  stringList *sl = (typeof(sl))slp;
  AllocatorV allocator = sl->allocator;
  msList_deInit(allocator, sl->ulist);
  msList_deInit(allocator, sl->flist);
  aFree(allocator, sl->buff);
  aFree(allocator, sl);
}
struct flsr {
  List_index_t i;
  bool f;
};

List_index_t stringList_len(stringList *sl) { return msList_len(sl->ulist); }
struct flsr stringListFreeList_search(stringList *sl, usize size) {
  List_index_t b = 0;
  List_index_t t = msList_len(sl->flist);
  while (b < t) {
    List_index_t m = (b + t) / 2;
    int cmp = (i64)size - (i64)vlen_toU64(sl->buff + sl->flist[m]);
    if (cmp > 0)
      t = m;
    else if (cmp < 0)
      b = m + 1;
    else
      return (struct flsr){.i = m, .f = 1};
  }
  return (struct flsr){.i = b, .f = 0};
}
fptr stringList_get(stringList *sl, List_index_t i) {
  return i < msList_len(sl->ulist) ? vlqbuf_toFptr(sl->buff + sl->ulist[i]) : nullFptr;
}
fptr stringList_append(stringList *sl, fptr ptr) {
  struct flsr insert = stringListFreeList_search(sl, ptr.width);
  typeof(u64_toVlen(0)) vlq_struct = u64_toVlen(ptr.width);

  ptrdiff_t offset = -1;
  usize vlq_len = countof(vlq_struct._);
  vlength *vlq_ptr = vlq_struct._;

  while (bitcast(u8, *vlq_ptr) == bitcast(u8, ((vlength){.hasNext = 1, .data = 0}))) {
    vlq_ptr++;
    vlq_len--;
  }
  if (insert.f || insert.i) {
    if (insert.f) {
      offset = sl->flist[insert.i];
      msList_rem(sl->flist, insert.i);
    } else {
      offset = sl->flist[insert.i - 1];
      msList_rem(sl->flist, insert.i - 1);
    }
  } else {
    offset = sl->len;
    if (offset + ptr.width + vlq_len > sl->len) {
      sl->buff = (vlength *)aResize(sl->allocator, sl->buff, offset + ptr.width + vlq_len + 10);
      if (sl->allocator->size)
        sl->cap = sl->allocator->size(sl->allocator, sl->buff);
      else
        sl->cap = offset + ptr.width + vlq_len + 10;
    }
    sl->len = offset + ptr.width + vlq_len;
  }
  msList_push(sl->allocator, sl->ulist, offset);
  memcpy(sl->buff + offset, vlq_ptr, vlq_len);
  memcpy(sl->buff + offset + vlq_len, ptr.ptr, ptr.width);
  return (fptr){
      .width = ptr.width,
      .ptr = (u8 *)(sl->buff + offset + vlq_len),
  };
}
void stringList_remove(stringList *sl, List_index_t i) {
  assertMessage(i < msList_len(sl->ulist));
  msList_ins(
      sl->allocator,
      sl->flist,
      stringListFreeList_search(sl, vlen_toU64(sl->buff + sl->ulist[i])).i,
      sl->ulist[i]
  );
  msList_rem(sl->ulist, i);
}
usize stringList_footprint(stringList *sl) {
  return sizeof(*sl->ulist) * msList_len(sl->ulist) + sizeof(*sl->flist) * msList_len(sl->flist) + sl->len;
};

fptr stringList_insert(stringList *sl, List_index_t i, fptr ptr) {
  assertMessage(i <= msList_len(sl->ulist));
  fptr res = stringList_append(sl, ptr);
  msList_ins(sl->allocator, sl->ulist, i, msList_pop(sl->ulist));
  return res;
}

fptr stringList_set(stringList *sl, List_index_t i, fptr ptr) {
  stringList_remove(sl, i);
  return stringList_insert(sl, i, ptr);
}
#endif
