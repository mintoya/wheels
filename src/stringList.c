#include "../stringList.h"
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
fptr stringList_get(stringList *sl, usize i) {
  return i < msList_len(sl->ulist) ? vlqbuf_toFptr(sl->buff + sl->ulist[i]) : nullFptr;
}
fptr stringList_append(stringList *sl, fptr ptr) {
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
    }
  } else {
    offset = sl->len;
    if (offset + ptr.len + vlq_len > sl->len) {
      sl->buff = (vlength *)aResize(sl->allocator, sl->buff, offset + ptr.len + vlq_len + 10);
      if (sl->allocator->size)
        sl->cap = sl->allocator->size(sl->allocator, sl->buff);
      else
        sl->cap = offset + ptr.len + vlq_len + 10;
    }
    sl->len = offset + ptr.len + vlq_len;
  }
  msList_push(sl->allocator, sl->ulist, offset);
  memcpy(sl->buff + offset, vlq_ptr, vlq_len);
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
usize stringList_footprint(stringList *sl) {
  return sizeof(*sl->ulist) * msList_len(sl->ulist) + sizeof(*sl->flist) * msList_len(sl->flist) + sl->len;
};

fptr stringList_insert(stringList *sl, usize i, fptr ptr) {
  assertMessage(i <= msList_len(sl->ulist));
  fptr res = stringList_append(sl, ptr);
  msList_ins(sl->allocator, sl->ulist, i, msList_pop(sl->ulist));
  return res;
}

fptr stringList_set(stringList *sl, usize i, fptr ptr) {
  stringList_remove(sl, i);
  return stringList_insert(sl, i, ptr);
}
