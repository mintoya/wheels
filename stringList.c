#include <stdlib.h>
#if !defined(STRING_LIST_H)
  #define STRING_LIST_H (1)
typedef struct stringList stringList;
  #include "fptr.h"
  #include "my-list.h"
  #include <stddef.h>
  #include <stdint.h>
fptr stringList_get(stringList *, List_index_t);
void stringList_set(stringList *, List_index_t, fptr);
void stringList_remove(stringList *, List_index_t);
void stringList_append(stringList *, List_index_t, fptr);
void stringList_insert(stringList *, List_index_t, fptr);
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define STRING_LIST_C (1)
#endif

#if defined(STRING_LIST_C)

  #include "my-list.h"
  #include "types.h"
struct vlength {
  u8 hasNext : 1;
  u8 data : 7;
};
static_assert(sizeof(struct vlength) == 1, "check packing ");
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
    struct {                                            \
      typeof(usize_toVLen(0)) len;                      \
      typeof(stringLiteral) str;                        \
    } res;                                              \
    res = (typeof(res)){                                \
        .len = usize_toVLen(sizeof(stringLiteral) - 1), \
        .str = stringLiteral,                           \
    };                                                  \
    (vlqbuf) & res;                                     \
  })
fptr fconv(vlqbuf b) {
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

#endif
