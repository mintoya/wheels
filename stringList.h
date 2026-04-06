#include "macros.h"
#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
  #include "fptr.h"
  #include "mytypes.h"
  #include "sList.h"
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
static inline struct u64_vl_max u64_toVlen(u64 len) {
  typeof(u64_toVlen(0)) res = {0};
  for (int i = countof(res._); i > 0; i--)
    res._[countof(res._) - i] = (typeof(*res._)){
        .hasNext = !!(i - 1),
        .data = (u8)((len >> ((i - 1) * 7)) & 0x7f),
    };
  return res;
}
static inline u64 vlen_toU64(struct vlength *vlen) {
  u64 res = 0;
  do
    res = (res << 7) | (vlen->data);
  while (vlen++->hasNext);
  return res;
}
static inline fptr vlqbuf_toFptr(vlength *b) {
  return (fptr){
      .len = (usize)vlen_toU64(b),
      .ptr = ({
        for (; b->hasNext; b++)
          ;
        (u8 *)(b + 1);
      })
  };
}
stringList *stringList_new(AllocatorV allocator, usize initSize);
stringList stringList_newVal(AllocatorV allocator, usize initSize);
void stringList_free(stringList *sl);
void stringList_free_data(stringList slp);
void stringList_remove(stringList *, usize);
usize stringList_len(stringList *);
usize stringList_footprint(const stringList *);

static inline fptr stringList_fptr_passthrough(const fptr f) { return f; }
fptr stringList_get(const stringList *, usize);

fptr stringList_push(stringList *, fptr);
fptr stringList_set(stringList *, usize, fptr);
fptr stringList_insert(stringList *, usize, fptr);
static inline fptr stringList_push_chars(stringList *sl, const char *cstr) {
  return stringList_push(sl, fptr_CSP(cstr));
}
static inline fptr stringList_set_chars(stringList *sl, usize idx, const char *cstr) {
  return stringList_set(sl, idx, fptr_CSP(cstr));
}
static inline fptr stringList_insert_chars(stringList *sl, usize idx, const char *cstr) {
  return stringList_insert(sl, idx, fptr_CSP(cstr));
}

  #define stringList_push(stringlist, ptr) \
    _Generic((ptr), char *: stringList_push_chars, const char *: stringList_push_chars, fptr: (stringList_push), const fptr: (stringList_push))(stringlist, ptr)
  #define stringList_set(stringlist, idx, ptr) \
    _Generic((ptr), char *: stringList_set_chars, const char *: stringList_set_chars, fptr: (stringList_set), const fptr: (stringList_set))(stringlist, idx, ptr)
  #define stringList_insert(stringlist, idx, ptr)                                                                                                        \
    _Generic((ptr), char *: stringList_insert_chars, const char *: stringList_insert_chars, fptr: (stringList_insert), const fptr: (stringList_insert))( \
        stringlist, idx, ptr                                                                                                                             \
    )

inline stringList *stringList_copy(AllocatorV allocator, stringList *sl) {
  stringList *res = stringList_new(allocator, sl->len > 10 ? sl->len : 10);
  for (each_RANGE(usize, i, 0, stringList_len(sl)))
    stringList_push(res, stringList_get(sl, i));
  return res;
}
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
  inline fptr push(fptr ptrf) { return stringList_push(ptr, ptrf); }
  inline fptr insert(usize idx, fptr ptrf) { return stringList_insert(ptr, idx, ptrf); }
  inline usize len() { return stringList_len(ptr); }
  const fptr operator[](usize idx) { return get(idx); }
};
  #endif

  // #include "tests.c"
  #if defined(MAKE_TEST_FN)
MAKE_TEST_FN(test_stringList_lifecycle, {
  stringList *sl = stringList_new(allocator, 10);
  defer { stringList_free(sl); };
  if (!sl)
    return 1;

  fptr data = fp("Hello");
  stringList_push(sl, data);
  if (stringList_len(sl) != 1)
    return 1;

  fptr retrieved = stringList_get(sl, 0);
  if (retrieved.len != 5)
    return 1;
  for (usize i = 0; i < 5; i++) {
    if (retrieved.ptr[i] != data.ptr[i])
      return 1;
  }
  if (stringList_footprint(sl) == 0)
    return 1;

  return 0;
})
MAKE_TEST_FN(test_stringList_manipulation, {
  stringList *sl = stringList_new(allocator, 4);
  defer { stringList_free(sl); };
  if (!sl)
    return 1;

  stringList_push(sl, "one");
  stringList_push(sl, "two");

  // Test Insert: ["one", "mid", "two"]
  stringList_insert(sl, 1, fp("mid"));
  if (stringList_len(sl) != 3)
    return 1;

  fptr mid = stringList_get(sl, 1);
  if (mid.len != 3 || mid.ptr[0] != 'm')
    return 1;

  // Test Set: ["one", "new", "two"]
  stringList_set(sl, 1, fp("new"));
  fptr updated = stringList_get(sl, 1);
  if (updated.ptr[0] != 'n')
    return 1;

  // Test Remove: ["new", "two"]
  stringList_remove(sl, 0);
  if (stringList_len(sl) != 2)
    return 1;

  fptr shifted = stringList_get(sl, 0);
  if (shifted.ptr[0] != 'n')
    return 1;

  return 0;
})
MAKE_TEST_FN(test_stringList_churn_stats, {
  usize ITERS = 100;
  stringList *sl = stringList_new(allocator, 1024);
  defer { stringList_free(sl); };
  if (!sl)
    return 1;

  for (each_RANGE(usize, i, 0, ITERS))
    stringList_push(sl, "medium_length_string");

  printf("  Footprint: %zu bytes\n"
         "  Free list: %zu blocks\n\n",
         (size_t)stringList_footprint(sl), (size_t)msList_len(sl->flist));

  for (ptrdiff_t i = ITERS - 1; i >= 0; i -= 2)
    stringList_remove(sl, (usize)i);

  printf("  Footprint: %zu bytes\n"
         "  buffer size: %zu \n"
         "  Free list: %zu blocks\n\n",
         (size_t)stringList_footprint(sl), sl->len, (size_t)msList_len(sl->flist));

  for (usize i = 0; i < ITERS / 2; i++)
    stringList_push(sl, "short");

  printf("  Footprint: %zu bytes\n"
         "  buffer size: %zu \n"
         "  Free list: %zu blocks\n\n",
         (size_t)stringList_footprint(sl), sl->len, (size_t)msList_len(sl->flist));

  return 0;
})
  #endif
#endif

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define STRING_LIST_C (1)
#endif

#if defined(STRING_LIST_C)
  #include <stddef.h>

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

stringList stringList_newVal(AllocatorV allocator, usize initSize) {
  stringList res = (typeof(res)){
      .ulist = msList_init(allocator, ptrdiff_t),
      .flist = msList_init(allocator, ptrdiff_t),
      .len = 0,
      .buff = (typeof(res.buff))aAlloc(allocator, initSize),
      .allocator = allocator,
  };
  assertMessage(!msList_len(res.ulist), "should be 0, is %llu", (unsigned long long)msList_len(res.ulist));
  assertMessage(!msList_len(res.flist), "should be 0, is %llu", (unsigned long long)msList_len(res.flist));
  if (allocator->size)
    res.cap = allocator->size(allocator, res.buff);
  else
    res.cap = initSize;
  return res;
}
stringList *stringList_new(AllocatorV allocator, usize initSize) {
  stringList *res = aCreate(allocator, stringList);
  *res = stringList_newVal(allocator, initSize);
  return res;
}
void stringList_free_data(stringList slp) {
  AllocatorV allocator = slp.allocator;
  msList_deInit(allocator, slp.ulist);
  msList_deInit(allocator, slp.flist);
  aFree(allocator, slp.buff);
}
void stringList_free(stringList *slp) {
  AllocatorV allocator = slp->allocator;
  stringList_free_data(*slp);
  aFree(allocator, slp);
}
struct flsr {
  usize i;
  bool f;
};

usize stringList_len(stringList *sl) { return msList_len(sl->ulist); }
struct flsr stringListFreeList_search(stringList *sl, usize size) {
  usize b = 0;
  usize t = msList_len(sl->flist);
  while (b < t) {
    usize m = (b + t) / 2;
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
fptr(stringList_get)(const stringList *sl, usize i) {
  return i < msList_len(sl->ulist) ? vlqbuf_toFptr(sl->buff + sl->ulist[i]) : nullFptr;
}
fptr(stringList_push)(stringList *sl, fptr ptr) {
  struct flsr insert = stringListFreeList_search(sl, ptr.len);
  typeof(u64_toVlen(0)) vlq_struct = u64_toVlen(ptr.len);

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
      // if (false)
      { // split buffer
        fptr op = vlqbuf_toFptr((vlength *)sl->buff + offset);
        usize newlen =
            op.len - ptr.len - countof(u64_toVlen(0)._);
        var_ b = u64_toVlen(newlen);
        var_ bp = b._;
        usize bpl = countof(b._);
        while (bitcast(u8, *bp) == bitcast(u8, ((vlength){.hasNext = 1, .data = 0}))) {
          bp++;
          bpl--;
        }
        if (op.len > ptr.len * 2 - bpl - 1) { // TODO adjust?
          ptrdiff_t newplace = op.ptr - (u8 *)sl->buff + op.len - newlen - bpl;

          memcpy((u8 *)sl->buff + newplace, bp, bpl);
          msList_ins(
              sl->allocator,
              sl->flist,
              stringListFreeList_search(sl, vlen_toU64(sl->buff + newplace)).i,
              newplace
          );
        }
      }
    }
  } else {
    offset = sl->len;
    if (offset + ptr.len + vlq_len > sl->len) {
      sl->buff = (vlength *)aResize(sl->allocator, sl->buff, offset + offset / 2 + ptr.len + vlq_len + 10);
      sl->cap = sl->allocator->size
                    ? sl->allocator->size(sl->allocator, sl->buff)
                    : offset + offset / 2 + ptr.len + vlq_len + 10;
    }
    sl->len = offset + ptr.len + vlq_len;
  }
  msList_push(sl->allocator, sl->ulist, offset);
  memcpy(sl->buff + offset, vlq_ptr, vlq_len);
  if (ptr.ptr)
    memcpy(sl->buff + offset + vlq_len, ptr.ptr, ptr.len);
  return (fptr){
      .len = ptr.len,
      .ptr = (u8 *)(sl->buff + offset + vlq_len),
  };
}
void stringList_remove(stringList *sl, usize i) {
  assertMessage(i < msList_len(sl->ulist));
  msList_ins(
      sl->allocator,
      sl->flist,
      stringListFreeList_search(sl, vlen_toU64(sl->buff + sl->ulist[i])).i,
      sl->ulist[i]
  );
  msList_rem(sl->ulist, i);
}
usize stringList_footprint(const stringList *sl) {
  // clang-format off
  return
      + sizeof(*msList_vla(sl->ulist))
      + sizeof(*msList_vla(sl->flist))
      +sl->len;
  // clang-format on
};
fptr(stringList_insert)(stringList *sl, usize i, fptr ptr) {
  assertMessage(i <= msList_len(sl->ulist));
  fptr res = stringList_push(sl, ptr);
  msList_ins(sl->allocator, sl->ulist, i, msList_pop(sl->ulist));
  return res;
}
fptr(stringList_set)(stringList *sl, usize i, fptr ptr) {
  stringList_remove(sl, i);
  return stringList_insert(sl, i, ptr);
}
#endif
