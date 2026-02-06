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
  ptrdiff_t *ulist; ///< use-list
                    /// not sorted,
  ptrdiff_t *flist; ///< free-list
                    /// sorted
  vlength *buff;
  AllocatorV allocator;
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
List_index_t stringList_len(stringList *);
usize stringList_footprint(stringList *);
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
        .hasNext = !!(i - 1),
        .data = (u8)((len >> ((i - 1) * 7)) & 0x7f),
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
  #include "shortList.h"
stringList *stringList_new(AllocatorV allocator, usize initSize) {
  stringList *res = aCreate(allocator, stringList);
  *res = (typeof(*res)){
      .ulist = msList_init(allocator, ptrdiff_t),
      .flist = msList_init(allocator, ptrdiff_t),
      .buff = msList_init(allocator, vlength),
      .allocator = allocator,
  };
  return res;
}
void stringList_free(stringList *sl) {
  AllocatorV allocator = sl->allocator;
  msList_deInit(allocator, sl->flist);
  msList_deInit(sl->allocator, sl->ulist);
  aFree(allocator, sl->buff);
  aFree(allocator, sl);
}
struct flsr {
  List_index_t i;
  bool f;
};
struct flsr freeList_sorter(stringList *sl, usize size, ptrdiff_t *freelist) {
  List_index_t b = 0;
  List_index_t t = msList_len(freelist);
  while (b < t) {
    List_index_t m = (b + t) / 2;
    i64 blen = vlen_toU64((vlength *)(sl->buff + freelist[m]));
    int r = (i64)size - (i64)blen;
    if (!r)
      return (struct flsr){.i = m, .f = 1};
    else if (r > 0)
      t = m;
    else
      b = m + 1;
  }
  return (struct flsr){.i = b, .f = 0};
}
  #include "assertMessage.h"
fptr stringList_append(stringList *sl, fptr ptr) {
  typeof(u64_toVlen(0)) vl_ = u64_toVlen(ptr.width);
  ptrdiff_t place = 0;
  vlength *vl = vl_._;
  usize width_length = countof(vl_._);
  for (; vl_u8(*vl) == vl_u8((vlength){.hasNext = 1, .data = 0}); vl++)
    width_length--;

  struct flsr potentialFree = freeList_sorter(sl, ptr.width, sl->flist);
  if (potentialFree.f || potentialFree.i) {
    place = sl->flist[potentialFree.i - 1];
    msList_rem(sl->flist, potentialFree.i - 1);
  } else {
    place = msList_len(sl->buff);
    AllocatorV allocator = sl->allocator;
    if (msList_len(sl->buff) + countof(vl_._) + ptr.width > msList_cap(sl->buff)) {
      sList_realloc(sl->allocator, msList_header(sl->buff), sizeof(*sl->buff), ptr.width + countof(vl_._) + msList_len(sl->buff));
    }
    msList_header(sl->buff)->length += width_length + ptr.width;
  }
  msList_push(sl->allocator, sl->ulist, place);
  memcpy(sl->buff + place, vl, width_length);
  memcpy(sl->buff + place + width_length, ptr.ptr, ptr.width);
  return ((fptr){
      .width = ptr.width,
      .ptr =
          (u8 *)sl->buff + place + width_length,
  });
}
fptr stringList_get(stringList *sl, List_index_t idx) {
  assertMessage(sl);
  ptrdiff_t *place = msList_get(sl->ulist, idx);
  return place ? vlqbuf_toFptr(sl->buff + *place) : nullFptr;
}
void stringList_remove(stringList *sl, List_index_t idx) {
  assertMessage(msList_len(sl->ulist) > idx);
  msList_ins(
      sl->allocator, sl->flist,
      freeList_sorter(sl, vlen_toU64(sl->buff + sl->ulist[idx]), sl->flist).i,
      sl->ulist[idx]
  );
  msList_rem(sl->ulist, idx);
}
fptr stringList_set(stringList *sl, List_index_t idx, fptr ptr) {
  assertMessage(idx < msList_len(sl->ulist));
  ptrdiff_t *lastLen = sl->ulist + idx;
  u64 lastLen_length = vlen_toU64(sl->buff + *lastLen);
  fptr res;
  if (lastLen_length < ptr.width) {
    res = stringList_append(sl, ptr);

    ptrdiff_t nextPlace = msList_pop(sl->ulist);

    ptrdiff_t last = sl->ulist[idx];
    msList_ins(
        sl->allocator, sl->flist,
        freeList_sorter(sl, vlen_toU64(sl->buff + last), sl->flist).i,
        last
    );
    sl->ulist[idx] = nextPlace;
  } else {
    typeof(u64_toVlen(0)) newlen = u64_toVlen(ptr.width);
    vlength *vl_new = newlen._;
    usize width_length = countof(newlen._);
    for (; vl_u8(*vl_new) == vl_u8((vlength){.hasNext = 1, .data = 0}); vl_new++)
      width_length--;
    memcpy(sl->buff + *lastLen, vl_new, width_length);
    memcpy(sl->buff + *lastLen + width_length, ptr.ptr, ptr.width);
    res = ((fptr){
        .width = ptr.width,
        .ptr =
            (u8 *)sl->buff + *lastLen + width_length,
    });
  }
  return res;
}
fptr stringList_insert(stringList *sl, List_index_t idx, fptr ptr) {
  fptr res = stringList_append(sl, ptr);
  msList_ins(sl->allocator, sl->ulist, idx, msList_pop(sl->ulist));
  return res;
}
usize stringList_footprint(stringList *sl) {
  return msList_len(sl->buff) * sizeof(*sl->buff) +
         msList_len(sl->ulist) * sizeof(*sl->ulist) +
         msList_len(sl->flist) * sizeof(*sl->flist);
}
List_index_t stringList_len(stringList *sl) { return msList_len(sl->ulist); }
#endif
