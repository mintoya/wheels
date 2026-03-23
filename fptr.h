#if !defined(FPTR_H)
  #define FPTR_H (1)
  #include "mytypes.h"
  #include <stdalign.h>
  #include <stdbool.h>
  #include <stddef.h>
  #include <string.h>

  #if defined(__cplusplus)
typedef slice(u8) fptr;
  #else
    #if __STDC_VERSION__ >= 202411L
typedef slice(u8) fptr;
    #endif
typedef sliceDef(u8) fptr;
  #endif

static inline fptr fptr_CS(void *cstr) { return ((fptr){(usize)strlen((char *)cstr), (u8 *)cstr}); }
static inline fptr fptr_fromPL(const void *cstr, usize len) { return (fptr){len, (u8 *)cstr}; }

static inline int fptr_cmp(const fptr a, const fptr b) {
  return a.len - b.len ?: memcmp(a.ptr, b.ptr, a.len);
}
  #if defined(__cplusplus)
template <typename T, typename T2>
static inline int fptr_cmp(const slice(T) a, const slice(T2) b) {
  return fptr_cmp(toFptr(a), toFptr(b));
}
  #endif
static inline char fptr_eq(fptr a, fptr b) {
  return !fptr_cmp(a, b);
}
static inline umax fptr_hash(fptr f) {
  umax hash = 5381;
  for (usize i = 0; i < f.len; i++) {
    hash ^= hash >> 3;
    hash = hash * 65;
    hash ^= (f.ptr[i]);
  }
  return hash;
}

  #ifdef __cplusplus
static bool operator==(const fptr &a, const fptr &b) { return fptr_eq(a, b); }
static bool operator!=(const fptr &a, const fptr &b) { return !fptr_eq(a, b); }

template <typename T>
fptr toFptr(const slice_t<T> &a) {
  return (fptr){
      a.len * sizeof(T),
      a.ptr,
  };
}
fptr toFptr(const fptr &a) { return a; }
template <typename T>
static bool operator==(const slice_t<T> &a, const slice_t<T> &b) {
  return fptr_eq(toFptr(a), toFptr(b));
}
template <typename T>
static bool operator!=(const slice_t<T> &a, const slice_t<T> &b) {
  return !fptr_eq(toFptr(a), toFptr(b));
}
  #endif

  #if defined(__cplusplus) || (defined(__STDC__VERSION__) && __STDC__VERSION__ >= 202311L)
constexpr fptr nullFptr = {0, nullptr};
  #else
    #define nullFptr ((fptr){0, 0})
  #endif

  #ifndef __cplusplus

    #define fp_fromT(struct)      \
      ((fptr){                    \
          .ptr = (u8 *)&(struct), \
          .len = sizeof(struct),  \
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
    #include <cstring>
    #include <string>
template <typename T>
inline fptr fp_from(T &val) {
  return {
      .len = sizeof(T),
      .ptr = reinterpret_cast<u8 *>(&val),
  };
}
inline fptr fp_from(fptr u) { return u; }
inline fptr fp_from(const std::string &s) {
  return {
      .len = s.size(),
      .ptr = const_cast<u8 *>(reinterpret_cast<const u8 *>(s.data())),
  };
}
inline fptr fp_from(const char *s) {
  return {
      .len = std::strlen(s),
      .ptr = (u8 *)s,
  };
}
inline fptr fp_from(char *s) {
  return {
      .len = std::strlen(s),
      .ptr = (u8 *)s,
  };
}
template <usize N>
inline fptr fp_from(const char (&s)[N]) {
  return {
      .len = N - 1,
      .ptr = const_cast<u8 *>(reinterpret_cast<const u8 *>(s)),
  };
}
  #endif
  #define fp fp_from
#endif // FPTR_H
