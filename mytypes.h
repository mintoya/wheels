#ifndef MY_TYPES
#define MY_TYPES
#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if __has_include("uchar.h") && !defined(NOUCHAR_TYPES)
  #include <uchar.h>
#else
  #if !defined __cplusplus
typedef uint32_t c32;
  #endif
#endif

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uchar c8;
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
#define ntype_max_u(T) ((T)(~((T)0)))
#define ntype_max_i(T) ((T)(ntype_max_u(T) ^ (((T)1) << (sizeof(T) * 8 - 1))))

#if !defined(__cplusplus)
  #define REF(type, value) ((type[1]){value})
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  #else
    #define nullptr ((void *)0)
  #endif
  #define bitcast(to, from) ((typeof(union {typeof(to)a;typeof(from)b; })){.b = from}.a)
#else
template <typename T>
static inline T *ref_tmp(T &&v) { return &v; }
  #define REF(type, value) ref_tmp(type{value})
template <class To, class From>
inline To bit_cast_func(const From &src) noexcept {
  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
  #define bitcast(to, from) (bit_cast_func<to>(from))
  #define typeof(...) __typeof__(__VA_ARGS__)
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
  void set(usize idx, T val) { return idx < len ? ptr[idx] = val : (void)0; }
  T *get(usize idx) { return idx < len ? ptr + idx : nullptr; }

  T &operator[](usize idx) { return ptr[idx]; }
  const T &operator[](usize idx) const { return ptr[idx]; }

  T *begin() const { return ptr; }
  T *end() const { return ptr + len; }
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
#if !defined(__cplusplus) || defined(__clang__)
  #define slice_stat(s) \
    {sizeof(s) / sizeof((s)[0]), (typeof(s[0]) *)(s)}
  #define slice_vla(s) ((typeof (*(s.ptr))(*)[s.len])(s.ptr))
#endif
#define each_slice(slice, e)       \
  typeof(slice.ptr) e = slice.ptr; \
  e < slice.ptr + slice.len;       \
  e++

#define nullable_fromPtr(T, ptr)          \
  {                                       \
      .isnull = !ptr,                     \
      .data = ptr ? *ptr : (typeof(T)){}, \
  }
#if defined(__cplusplus) || !__has_include(<stdcountof.h>)
  #define _Countof(array) (sizeof(array) / sizeof(*(array)))
  #define countof(array) _Countof(array)
#else
  #include <stdcountof.h>

#endif
// provides defer_ and defer in case of cpp
#if defined(__cplusplus)
  #include <memory>
  #define DEFER_CONCAT(a, b) a##b
  #define DEFER_NAME(a, b) DEFER_CONCAT(a, b)
  #define defer_(...) \
    std::shared_ptr<void> DEFER_NAME(deferptr, __LINE__)(nullptr, [&](void *) -> void { __VA_ARGS__ })
template <typename F>
struct Deferrer {
  F fn;
  ~Deferrer() { fn(); }
};

struct DeferHelper {
  template <typename F>
  Deferrer<F> operator+(F &&f) { return {std::forward<F>(f)}; }
};

  #define DEFER_CONCAT(a, b) a##b
  #define DEFER_NAME(a, b) DEFER_CONCAT(a, b)
  #define defer auto DEFER_NAME(_defer_, __LINE__) = DeferHelper() + [&]()
#else
  #include <stddefer.h>
  #define defer_(...) defer{__VA_ARGS__};
#endif

#define enum_named(name, underlying, ...) \
  typedef underlying name##_t;            \
  constexpr struct {                      \
    underlying __VA_ARGS__;               \
  } name
#endif // MY_TYPES
