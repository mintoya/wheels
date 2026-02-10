#if !defined(FPTR_H)
  #define FPTR_H (1)
  #include "types.h"
  #include <stdalign.h>
  #include <stdbool.h>
  #include <stddef.h>
  #include <stdlib.h>
  #include <string.h>
  #include <wchar.h>

typedef struct {
  usize width;
  u8 *ptr;
} fptr;

static inline fptr fptr_CS(void *cstr) { return ((fptr){(usize)strlen((char *)cstr), (u8 *)cstr}); }
static inline fptr fptr_fromPL(const void *cstr, usize len) { return (fptr){len, (u8 *)cstr}; }

static inline int fptr_cmp(const fptr a, const fptr b) {
  return a.width - b.width ?: memcmp(a.ptr, b.ptr, a.width);
}
static inline char fptr_eq(fptr a, fptr b) {
  return !fptr_cmp(a, b);
}

  #ifdef __cplusplus
static bool operator==(const fptr &a, const fptr &b) { return fptr_eq(a, b); }
static bool operator!=(const fptr &a, const fptr &b) { return !fptr_eq(a, b); }
  #endif

  #define nullFptr ((fptr){0})

  #define structEq(a, b) \
    (fptr_eq(            \
        fp_from(a),      \
        fp_from(b)       \
    ))

  #ifndef __cplusplus

    #define fp_fromT(struct)       \
      ((fptr){                     \
          .ptr = (u8 *)&(struct),  \
          .width = sizeof(struct), \
      })
    #define is_comparr(x) \
      (!__builtin_types_compatible_p(__typeof__(x), __typeof__(&(x)[0])))

    #define fp_from(arr)                           \
      _Generic(                                    \
          arr,                                     \
          char *: is_comparr(arr)                  \
              ? (fptr){sizeof(arr) - 1, (u8 *)arr} \
              : fptr_CS(arr),                      \
          fptr: arr,                               \
          default: (fptr){sizeof(arr), (u8 *)&arr} \
      )

  #else
    #include <cstdint>
    #include <cstring>
    #include <string>
template <typename T>
inline fptr fp_from(T &val) {
  return {
      .width = sizeof(T),
      .ptr = reinterpret_cast<u8 *>(&val),
  };
}
inline fptr fp_from(fptr u) { return u; }
inline fptr fp_from(const std::string &s) {
  return {
      .width = s.size(),
      .ptr = const_cast<u8 *>(reinterpret_cast<const u8 *>(s.data())),
  };
}
inline fptr fp_from(const char *s) {
  return {
      .width = std::strlen(s),
      .ptr = const_cast<u8 *>(reinterpret_cast<const u8 *>(s)),
  };
}
template <usize N>
inline fptr fp_from(const char (&s)[N]) {
  return {
      .width = N - 1,
      .ptr = const_cast<u8 *>(reinterpret_cast<const u8 *>(s)),
  };
}
  #endif
  #define fp fp_from
#endif // FPTR_H
