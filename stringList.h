#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
  #include "fptr.h"
  #include "mytypes.h"
  #include "shortList.h"
  #include <string.h>
typedef struct vlength {
  u8 hasNext : 1;
  u8 data : 7;
} vlength;

typedef struct stringList {
  ptrdiff_t *ulist; ///< use-list
                    /// not sorted,
  ptrdiff_t *flist; ///< free-list
                    /// sorted
  usize len, cap;
  vlength *buff;
  AllocatorV allocator;
} stringList;
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
fptr stringList_get(stringList *, usize);
fptr stringList_append(stringList *, fptr);
void stringList_remove(stringList *, usize);
usize stringList_len(stringList *);
usize stringList_footprint(stringList *);
fptr stringList_insert(stringList *, usize, fptr);
fptr stringList_set(stringList *, usize, fptr);
  #if defined __cplusplus
struct strList {
  stringList *ptr;
  inline strList(AllocatorV allocator = stdAlloc, usize initSize = 20) {
    ptr = stringList_new(allocator, initSize);
  }
  inline ~strList() { stringList_free(ptr); }
  inline fptr get(usize idx) { return stringList_get(ptr, idx); }
  inline fptr set(usize idx, fptr ptrf) { return stringList_set(ptr, idx, ptrf); }
  inline void remove(usize idx) { return stringList_remove(ptr, idx); }
  inline fptr push(fptr ptrf) { return stringList_append(ptr, ptrf); }
  inline fptr insert(usize idx, fptr ptrf) { return stringList_insert(ptr, idx, ptrf); }
  inline usize len() { return stringList_len(ptr); }
  const fptr operator[](usize idx) { return get(idx); }
};
  #endif
#endif

#if defined(STRING_LIST_C) && STRING_LIST_C == (1)
  #define STRING_LIST_C (2)
  #include "src/stringList.c"
#endif
