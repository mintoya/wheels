#if !defined(SHORT_LIST_H)
  #define SHORT_LIST_H (1)
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "mytypes.h"
  #ifndef SLIST_GROW_EQ
    #define SLIST_GROW_EQ(len) (len + len / 2 + 1)
  #endif

//
// stb-ds style strechy buffer
// does not store allocator or item size
// always pass the same alloator into any macro that needs one
//

typedef struct sList_header {
  usize isStack : 1;
  usize capacity : sizeof(usize) * 8 - 1;
  usize length;
  alignas(myAlign) u8 buf[];
} sList_header;

static inline sList_header *sList_new(AllocatorV allocator, usize initLen, usize width) {
  assertMessage(initLen && width);
  sList_header *res = (typeof(res))aAlloc(allocator, sizeof(sList_header) + initLen * width);
  *res = (typeof(*res)){};
  if (allocator->size) {
    usize s = allocator->size(allocator, res);
    res->capacity = (s - sizeof(sList_header)) / width;

  } else res->capacity = initLen;
  res->length = 0;
  res->isStack = 0;
  return res;
}
static inline sList_header *sList_realloc(AllocatorV allocator, sList_header *header, usize width, usize newsize) {
  assertMessage(header && header->capacity);
  if (!header->isStack) {
    sList_header *res = (typeof(res))aResize(
        allocator,
        header,
        sizeof(sList_header) + header->capacity * width,
        sizeof(sList_header) + newsize * width
    );
    if (allocator->size) {
      usize s = allocator->size(allocator, res);
      res->capacity = (s - sizeof(sList_header)) / width;
    } else
      res->capacity = newsize;
    return res;
  } else {
    var_ _new = sList_new(allocator, newsize, width);
    usize cl = (newsize < header->length ? newsize : header->length);
    memcpy(_new->buf, header->buf, width * cl);
    _new->length = cl;
    return _new;
  }
}
static inline void sList_free(AllocatorV allocator, sList_header *sl, usize width) {
  if (!sl->isStack)
    aFree(allocator, sl, width * sl->capacity + sizeof(*sl));
}

static inline void *sList_getRef(
    sList_header *l,
    usize width,
    usize i
) { return i < l->length ? (l->buf + width * i) : NULL; }
static inline void *sList_setArr(
    sList_header *l,
    usize width,
    usize length,
    usize index,
    const void *element
) {
  if (index >= l->length) return NULL;
  else if (index + length > l->length) length = l->length - index;

  void *place = sList_getRef(l, width, index);
  element
      ? memcpy(place, element, width * length)
      : memset(place, /* */ 0, width * length);
  return place;
}
static inline void *sList_set(
    sList_header *l,
    usize width,
    usize index,
    const void *element
) {
  return sList_setArr(l, width, 1, index, element);
}
static inline sList_header *sList_insertFromArr(
    AllocatorV allocator,
    sList_header *l,
    const void *source,
    usize length,
    usize location,
    size_t width
) {
  if (location > l->length)
    return l;

  bool inlist =
      (u8 *)source >= l->buf &&
      (u8 *)source + length * width <= l->buf + l->length * width;

  usize need = l->length + (inlist ? 2 * length : length);
  u8 *obuf = l->buf;
  if (l->capacity < need)
    l = sList_realloc(allocator, l, width, need);

  if (inlist) {
    source = (u8 *)source - obuf + l->buf;
    memcpy(l->buf + (l->capacity - length) * width, source, length * width);
    source = l->buf + (l->capacity - length) * width;
  }

  u8 *dest = l->buf + location * width;
  memmove(dest + length * width, dest, (l->length - location) * width);

  if (source)
    memcpy(dest, source, length * width);
  else
    memset(dest, 0, length * width);

  l->length += length;
  return l;
}
static inline void sList_removeArr(sList_header *l, usize width, usize len, usize idx) {
  if (idx >= l->length) return;
  else if (idx + len > l->length) len = l->length - idx;

  if (len) memmove(l->buf + idx * width, l->buf + (idx + len) * width, (l->length - idx - len) * width);
  l->length -= len;
}
static inline void sList_remove(sList_header *l, usize width, usize idx) { sList_removeArr(l, width, 1, idx); }

static inline sList_header *sList_append(
    AllocatorV allocator,
    sList_header *l,
    usize width,
    const void *element
) {
  if (l->capacity < l->length + 1)
    l = sList_realloc(allocator, l, width, SLIST_GROW_EQ(l->length));
  l->length++;
  sList_set(l, width, l->length - 1, element);
  return l;
}

struct bbs_result {
  void *p;
  bool f;
};
static struct bbs_result bbsearch(
    const void *key,
    const void *base0,
    usize nmemb,
    usize size,
    int (*compar)(const void *, const void *)
) {
  typedef typeof(bbsearch(nullptr, nullptr, 0, 0, nullptr)) r_t;
  const char *base = (const char *)base0;

  for (usize lim = nmemb; lim; lim /= 2) {
    var_ p = base + (lim >> 1) * size;
    var_ cmp = compar(key, p);
    if (!cmp)
      return (r_t){(void *)p, 1};
    if (cmp > 0) {
      base = (const char *)p + size;
      lim--;
    }
  }
  return (r_t){(void *)base, 0};
}
static inline sList_header *sList_appendFromArr(AllocatorV allocator, sList_header *l, usize width, void *source, usize ammount) {
  return sList_insertFromArr(allocator, l, source, ammount, l->length, width);
}
static inline sList_header *sList_insert(AllocatorV allocator, sList_header *l, usize width, usize i, void *element) {
  return sList_insertFromArr(allocator, l, element, 1, i, width);
}
  #if defined(__BLOCKS__)
    #pragma GCC warning "blocks enabled, non-stable pointers break"
    #define msList(T) typeof(__block T *)
  #else
    #define msList(T) typeof(T *)
  #endif

  #define SLIST_INIT_HELPER(allocator, T, initLength, ...) ({       \
    (T *)(sList_new(allocator, (initLength) ?: 1, sizeof(T))->buf); \
  })
  #define msList_init(allocator, T, ...) \
    SLIST_INIT_HELPER(allocator, T __VA_OPT__(, __VA_ARGS__), 2)

  #define msList_stackBuffer(buffer)                         \
    (struct {                                                \
      sList_header header[1];                                \
      typeof(typeof(((buffer){})[0])[countof(buffer)]) list; \
    }) {                                                     \
      {                                                      \
        {                                                    \
          .capacity = countof(buffer),                       \
          .isStack = true,                                   \
          .length = 0,                                       \
        }                                                    \
      }                                                      \
    }

  #define msList_initBuffer(buffer) ({                                   \
    (void)sizeof(int[_Generic(buffer, typeof(buffer): 1, default: -1)]); \
    buffer.header[0] = (typeof(buffer.header[0])){                       \
        .capacity = countof(buffer.list),                                \
        .isStack = true,                                                 \
        .length = 0,                                                     \
    };                                                                   \
    buffer.list;                                                         \
  })

  #define msList_header(s) (((sList_header *)(s)) - 1)
  #define msList_deInit(allocator, s)                             \
    do {                                                          \
      if (s) sList_free(allocator, msList_header(s), sizeof(*s)); \
    } while (0)
  #define msList_ins(allocator, s, idx, val)                                                 \
    do {                                                                                     \
      typeof(*s) _val = val;                                                                 \
      s = (typeof(s))sList_insert(allocator, msList_header(s), sizeof(*s), idx, &_val)->buf; \
    } while (0);

  #define msList_get(s, idx) ((typeof(s))sList_getRef(msList_header(s), sizeof(*s), idx))
  #define msList_rem(s, idx)                           \
    do {                                               \
      sList_remove(msList_header(s), sizeof(*s), idx); \
    } while (0)
  #define msList_push(allocator, s, val)                                                                          \
    do {                                                                                                          \
      if_unlikely (msList_len(s) == msList_cap(s))                                                                \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), SLIST_GROW_EQ(msList_len(s)))->buf; \
      (s)[msList_len(s)++] = (val);                                                                               \
    } while (0)

  #define msList_insArr(allocator, s, place, vla)                     \
    do {                                                              \
      ASSERT_EXPR(_Generic(vla[0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_insertFromArr(                             \
              allocator,                                              \
              msList_header(s),                                       \
              vla,                                                    \
              countof(vla),                                           \
              place,                                                  \
              sizeof(vla[0])                                          \
      )                                                               \
              ->buf;                                                  \
    } while (0)
  #define msList_pushArr(allocator, s, vla)                           \
    do {                                                              \
      ASSERT_EXPR(_Generic(vla[0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                             \
              allocator,                                              \
              msList_header(s),                                       \
              sizeof(s[0]),                                           \
              vla,                                                    \
              countof(vla)                                            \
      )                                                               \
              ->buf;                                                  \
    } while (0)
  #define msList_pushVla(allocator, s, vla)                              \
    do {                                                                 \
      ASSERT_EXPR(_Generic(vla[0][0], typeof(s[0]): 1, default: 0), ""); \
      s = (typeof(s))sList_appendFromArr(                                \
              allocator,                                                 \
              msList_header(s),                                          \
              sizeof(s[0]),                                              \
              vla,                                                       \
              countof(vla[0])                                            \
      )                                                                  \
              ->buf;                                                     \
    } while (0)
  #define msList_setCap(allocator, s, capacity)                                             \
    do {                                                                                    \
      s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), capacity)->buf; \
    } while (0)
  #define msList_clone(allocator, list) ({               \
    var_ res = msList_init(allocator, typeof(*list));    \
    msList_pushArr(allocator, res, (*msList_vla(list))); \
    res;                                                 \
  })
  #define msList_len(s) (msList_header(s)->length)
  #define msList_cap(s) (msList_header(s)->capacity)
  #define msList_pop(s) ((s)[--msList_header(s)->length])
  #define msList_popFront(s) ({typeof(*s) _res = *s;msList_rem(s, 0);_res; })
  #define msList_pad(allocator, s, ammount) \
    do {                                    \
      s = (typeof(s))sList_insertFromArr(   \
              allocator,                    \
              msList_header(s),             \
              NULL,                         \
              ammount,                      \
              msList_len(s),                \
              sizeof(*s)                    \
      )                                     \
              ->buf;                        \
    } while (0)
  #define msList_last(s) ((s)[msList_len(s) - 1])
  #define msList_vla(s) (VLAP(s, msList_len(s)))
  #define msList_clear(s) \
    do                    \
      msList_len(s) = 0;  \
    while (0)
  #define msList_reserve(allocator, s, new_cap)                                              \
    do {                                                                                     \
      if (msList_cap(s) < (new_cap))                                                         \
        s = (typeof(s))sList_realloc(allocator, msList_header(s), sizeof(*s), new_cap)->buf; \
    } while (0)
  #define msList_toOwned(allocator, s)                                \
    ({ /*TODO*/                                                       \
       unreachable();                                                 \
       usize c = msList_cap(s);                                       \
       usize l = msList_len(s);                                       \
       var_ _r = memcpy(msList_header(s), s, sizeof(*msList_vla(s))); \
       s = nullptr;                                                   \
       _r = aResize(allocator, _r, c * sizeof(*s), l * sizeof(*s));   \
       _r;                                                            \
    })

  // #include "tests.c"
  #if defined(MAKE_TEST_FN)
    #include "macros.h"

MAKE_TEST_FN(msList_push_pop, {
  var_ lbuf = msList_stackBuffer(int[25]);
  msList(int) list = msList_initBuffer(lbuf);
  // msList(int) list = msList_init(allocator,int);
  defer { msList_deInit(allocator, list); };
  foreach (usize i, range(0, 50))
    msList_push(allocator, list, i * i);
  foreach (usize i, range(0, 50))
    if (list[i] != i * i)
      return 1;
  return 0;
});
MAKE_TEST_FN(msList_push_pop2, {
  var_ lbuf = msList_stackBuffer(int[25]);
  msList(int) list = msList_initBuffer(lbuf);
  // msList(int) list = msList_init(nullptr,int);
  defer { msList_deInit(nullptr, list); };
  foreach (usize i, range(0, 22))
    msList_push(nullptr, list, i * i);
  foreach (usize i, range(0, 22))
    if (list[i] != i * i)
      return 1;
  return 0;
});
MAKE_TEST_FN(msList_push_pop3, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };
  foreach (usize i, range(0, 22))
    msList_push(allocator, list, i * i);
  foreach (usize i, range(0, 22))
    if (list[i] != i * i)
      return 1;
  return 0;
});

MAKE_TEST_FN(msList_insert_remove, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_push(allocator, list, 100);
  msList_push(allocator, list, 300);
  msList_ins(allocator, list, 1, 200);
  if (msList_len(list) != 3)
    return 1;
  if (list[1] != 200)
    return 1;

  msList_rem(list, 0);
  if (msList_len(list) != 2)
    return 1;
  if (list[0] != 200)
    return 1;

  if (msList_popFront(list) != 200)
    return 1;
  if (msList_len(list) != 1)
    return 1;
  if (list[0] != 300)
    return 1;

  return 0;
});

MAKE_TEST_FN(msList_array_operations, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  int arr1[] = {1, 2, 3};
  msList_pushArr(allocator, list, arr1);
  if (msList_len(list) != 3)
    return 1;
  if (list[2] != 3)
    return 1;

  msList_insArr(allocator, list, 0, *msList_vla(list));

  if (msList_len(list) != 6)
    return 1;
  if (list[0] != 1)
    return 2;
  if (list[1] != 2)
    return 3;

  return 0;
});

MAKE_TEST_FN(msList_capacity_and_padding, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_reserve(allocator, list, 50);
  if (msList_cap(list) < 50)
    return 1;

  msList_pad(allocator, list, 5);
  if (msList_len(list) != 5)
    return 1;

  msList_setCap(allocator, list, 10);
  if (msList_cap(list) < 10)
    return 1;

  msList_clear(list);
  if (msList_len(list) != 0)
    return 1;

  return 0;
});

MAKE_TEST_FN(msList_vla_cast, {
  msList(int) list = msList_init(allocator, int);
  defer { msList_deInit(allocator, list); };

  msList_push(allocator, list, 7);
  msList_push(allocator, list, 8);
  msList_push(allocator, list, 9);
  int *arr = aCreate(allocator, int, 3);
  defer { aFree(allocator, arr, 3); };
  memcpy(arr, list, 3 * sizeof(int));

  msList_pushArr(allocator, list, *VLAP(arr, 3));
  if (msList_len(list) != 6)
    return 1;
  return 0;
});

  #endif
#endif // SHORT_LIST_H
