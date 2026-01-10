#ifndef MY_TYPES
  #define MY_TYPES
  #include <stdbool.h>
  #include <stddef.h>
  #include <stdint.h>
  #include <time.h>
  #define _XOPEN_SOURCE 700

typedef wchar_t wchar;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uintmax_t umax;
typedef intmax_t imax;
typedef size_t usize;
  #if defined(_MSC_VER)
    #include <BaseTsd.h>
typedef SSIZE_T ssize_t;
  #elif defined(__linux__)
    #include <sys/types.h>
  #endif
#endif
typedef ssize_t ssize;
static_assert(sizeof(ssize) == sizeof(usize), "ssize and usize have to be the same length");
typedef uintptr_t uptr;

#ifndef __cplusplus
  #ifndef thread_local
    #define thread_local _Thread_local
  #endif
#endif

#ifdef __cplusplus
template <typename T>
struct nullable_type {
  T data;
  bool exists : 1;
  nullable_type(T d) {
    data = d;
    exists = true;
  }
  nullable_type(T *d) {
    if (d) {
      data = *d;
      exists = true;
    } else {
      exists = false;
    }
  }
};
  #define nullable(type) nullable_type<type>
template <typename T>
struct slice_type {
  usize len;
  T *ptr;
  template <usize N>
  slice_type(T (&array)[N]) : len(N), ptr(array) {}
};

  #define slice(type) slice_type<type>
#else
  #define slice_t(type) slice_##type
  #define slice_cat_(a, b) a##b
  #define slice_cat(a, b) slice_cat_(a, b)

  #define nullable(type)                 \
    struct slice_cat_(nullable_, type) { \
      type data;                         \
      bool exists : 1;                   \
    }

  #define slice(type)                 \
    struct slice_cat_(slice_, type) { \
      usize len;                      \
      type *ptr;                      \
    }
#endif
#endif // MY_TYPES
