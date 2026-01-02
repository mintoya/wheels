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
typedef uintptr_t uptr;

#ifndef __cplusplus
  #ifndef thread_local
    #define thread_local _Thread_local
  #endif
#endif
