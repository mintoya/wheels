#ifndef MY_TYPES
#define MY_TYPES
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if __has_include("uchar.h")
  #include <uchar.h>
#else
typedef uint8_t char8_t;
typedef uint16_t char16_t;
typedef uint32_t char32_t;
#endif
#define _XOPEN_SOURCE 700

typedef wchar_t wchar;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uchar c8;
typedef char16_t c16;
typedef char32_t c32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;
// long double is a double on windows -_-
typedef long double f128;
typedef void *voidptr;

typedef uintmax_t umax;
typedef intmax_t imax;
typedef size_t usize;
#if !defined(__cplusplus)
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  #else
    #define nullptr ((void *)0)
  #endif
#endif

#if __has_include(<BaseTsd.h>)
  #include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#elif __has_include(<sys/types.h>)
  #include <sys/types.h>
#else
typedef ptrdiff_t ssize_t;
static_assert(sizeof(ssize_t) == sizeof(usize), "ssize and usize have to be the same length");
#endif
typedef ssize_t isize;
typedef uintptr_t uptr;

#if !defined(__cplusplus)
  #ifndef thread_local
    #define thread_local _Thread_local
  #endif
  #define nullable(type)     \
    struct nullable_##type { \
      bool isnull : 1;       \
      type data;             \
    }

  #define slice(type)     \
    struct slice_##type { \
      usize len;          \
      type *ptr;          \
    }

#else
template <typename T>
struct slice_t {
  usize len;
  T *ptr;
};
template <typename T>
struct nullable_t {
  bool isnull : 1;
  T data;
};
  #define nullable(T) nullable_t<T>
  #define slice(T) slice_t<T>
#endif
#define nullable_null(type) ((nullable(type)){.isnull = true})
#define nullable_real(type, value) ((nullable(type)){.isnull = false, .data = (value)})
#define slice_stat(s) \
  {sizeof(s) / sizeof(s[0]), (typeof(s[0]) *)s}
#define each_slice(slice, e)       \
  typeof(slice.ptr) e = slice.ptr; \
  e < slice.ptr + slice.len;       \
  e++
#define nullable_fromPtr(type, ptr) ({type *p = ptr; nullable(type) r; r.isnull = p == NULL; if (!r.isnull) r.data = *p; r; })

#endif // MY_TYPES
