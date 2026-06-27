#ifndef MY_TYPES
#define MY_TYPES
#include "macros.h"
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#if defined(__cplusplus)
  #include <cstddef>
#else
  #include <stddef.h>
#endif
#include <stdint.h>
#if __has_include("uchar.h") && !defined(NOUCHAR_TYPES)
  #include <uchar.h>
#else
  #if !defined __cplusplus
typedef uint32_t char32_t;
  #endif
#endif

//
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;
typedef long double ldouble;
//
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef char c8;
typedef char32_t c32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;
// long double is a double on windows -_-
typedef ldouble f128;
typedef void *voidptr;
typedef char *cstr;

typedef uintmax_t umax;
typedef intmax_t imax;
typedef size_t usize;
typedef struct nothing_t {
} nothing_t;
#define nothing_v ((nothing_t){})

// max_align_t standin
typedef struct myAlign {
#if defined __BIGGEST_ALIGNMENT__
  alignas(__BIGGEST_ALIGNMENT__)
#endif
      union {
    void *_vp;
    usize _us;
    ldouble _ld;
    void (*_fp)(void *);
  };
} myAlign;

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
typedef ptrdiff_t iptr;

#if !defined(__cplusplus)
  #ifndef thread_local
    #define thread_local _Thread_local
  #endif
  #if __STDC_VERSION__ >= 202411L
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
    #define sliceDef(type)
    #define nullableDef(type)
  #else
    #define nullable(type) \
      struct nullable_##type

    #define slice(type) \
      struct slice_##type
    #define sliceDef(type)  \
      struct slice_##type { \
        usize len;          \
        type *ptr;          \
      }
    #define nullableDef(type)  \
      struct nullable_##type { \
        bool isnull : 1;       \
        type data;             \
      }
  #endif
#else
  #define sliceDef(type)
  #define nullableDef(type)
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
#define slice_stat(s) \
  {sizeof(s) / sizeof((s)[0]), (typeof(s[0]) *)(s)}
#define slice_vla(s) (VLAP(s.ptr, s.len))
#define nullslice(T) ((slice(T)){})

#define nullable_fromPtr(T, ptr)          \
  {                                       \
      .isnull = !ptr,                     \
      .data = ptr ? *ptr : (typeof(T)){}, \
  }
#if defined(__cplusplus) || !__has_include(<stdcountof.h>)
  #define _Countof(x) (sizeof(x) / sizeof((*((__typeof__(x) *)0))[0]))
  #define countof(array) _Countof(array)
#else
  #include <stdcountof.h>
#endif
#define sliceCast(to_elem, from)                             \
  ({                                                         \
    ASSERT_EXPR(sizeof(to_elem) == sizeof(from.ptr[0]), );   \
    var_ from_tmp = from;                                    \
    (slice(to_elem)){from_tmp.len, (to_elem *)from_tmp.ptr}; \
  })
sliceDef(c8);

#define slice_alloc(alloccator, type, length) ({ \
  slice(type) _s;                                \
  _s.ptr = aCreate(alloccator, type, length);    \
  _s.len = length;                               \
  _s;                                            \
})
#define slice_free(allocator, slice) \
  aFree(allocator, (slice).ptr, (sizeof(*(slice).ptr)) * (slice).len);

#if defined __cplusplus
template <typename T, usize size>
struct slice_array {
  T items[size];
  T &operator[](usize idx) { return items[idx]; };
};
  #define GET_INT(...) "a"
  #define ARRAY_COUNT(...) (sizeof(APPLY_N(GET_INT, __VA_ARGS__)) - 1)
  #define slice_ij(slice, tup)                     \
    ([&]() -> decltype(slice) {                    \
      usize _st = TUPLE_EXPAND_A(tup);             \
      usize _et = TUPLE_EXPAND_B(tup);             \
      _st = _st > (slice).len ? (slice).len : _st; \
      _et = _et > (slice).len ? (slice).len : _et; \
      _et = _et < _st ? _st : _et;                 \
      return (typeof_unqual(slice)){               \
          _et - _st,                               \
          (slice).ptr + _st                        \
      };                                           \
    }())

  #define slice_split(slice, ...)                            \
    (slice_array<typeof(slice), ARRAY_COUNT(__VA_ARGS__)>) { \
      APPLY_N_WITH_C(slice_ij, slice, __VA_ARGS__)           \
    }
#else
  #define slice_split(slice, ...)                \
    (typeof(slice)[]) {                          \
      APPLY_N_WITH(slice_ij, slice, __VA_ARGS__) \
    }
  #define slice_ij(slice, tup)                 \
    ({                                         \
      usize _st = TUPLE_EXPAND_A(tup);         \
      usize _et = TUPLE_EXPAND_B(tup);         \
      _st = _st > slice.len ? slice.len : _st; \
      _et = _et > slice.len ? slice.len : _et; \
      _et = _et < _st ? _st : _et;             \
      (typeof(slice)){                         \
          _et - _st,                           \
          slice.ptr + _st,                     \
      };                                       \
    }),
#endif

#define SLICE_CUT_HELPER(slice, prev, curr, ...) \
  slice_ij(slice, (prev, curr))                  \
      __VA_OPT__(SLICE_CUT_HELPER_INVOKE PARENTHESIS_HELPER(slice, curr, __VA_ARGS__))

#define SLICE_CUT_HELPER_INVOKE() SLICE_CUT_HELPER

#define slice_cut(slice, ...)                                                       \
  (typeof(slice)[]) {                                                               \
    MACRO_EXPAND(SLICE_CUT_HELPER(slice, 0, __VA_ARGS__ __VA_OPT__(, )(slice).len)) \
  }

#define slice_last(slice) ({var_ _slice = slice; _slice.ptr[_slice.len -1 ]; })
#define slice_first(slice) ({var_ _slice = slice; _slice.ptr[0]; })

#define sentList_t(type) typeof(/*sentinel termintated list*/ type *)
#define sentList(type, ...) \
  (type[]) { __VA_OPT__(__VA_ARGS__, )(type){0} }
#define sentList_vla(list) ({var_ _list = list; VLAP(_list, sentList_length(_list, sizeof(*_list))); })

#endif // MY_TYPES
