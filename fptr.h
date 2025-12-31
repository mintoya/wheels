#ifndef UM_FP_H
#define UM_FP_H
#ifdef _WIN32
// #include <malloc.h>
// #else
// #include <alloca.h>
#endif
#include "types.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct {
  size_t width;
  u8 *ptr;
} fptr;
typedef struct {
  size_t width;
  u8 ptr[];
} bFptr;
#define fptr_fromB(bfptr) ((fptr){.width = (bfptr).width, .ptr = (uint8_t *)(bfptr).ptr})

struct fatter_pointer {
  fptr fpart;
  size_t capacity;
};

typedef union {
  struct
  {
    fptr fpart;
    size_t capacity;
  } ffptr;
  fptr fptrp;
} ffptr;

typedef fptr um_fp;

// vptr version of dereferencing a value
#define fptr_fromTypeDef(v) ((fptr){sizeof(typeof(v)), (u8 *)REF(typeof(v), v)})
// _INC_STRING
#include <string.h>
#define fptr_fromCS(cstr) \
  ((fptr){(size_t)strlen(cstr), (u8 *)cstr})

static inline fptr fptr_fromPL(const void *cstr, usize len) { return (fptr){len, (u8 *)cstr}; }
// only sign  of result matters
int fptr_cmp(const fptr a, const fptr b);
static int (*um_fp_cmp)(const fptr, const fptr) = fptr_cmp;

#include <string.h>
static inline void fpmemset(uint8_t *ptr, const fptr element, size_t ammount) {
  if (!ammount)
    return;
  memcpy(ptr, element.ptr, element.width);
  for (size_t set = 1; set < ammount; set *= 2) {
    size_t toset = ammount - set;
    toset = set < toset ? set : toset;
    memcpy(ptr + set * element.width, ptr, element.width * toset);
  }
}

static inline char fptr_eq(fptr a, fptr b) { return !fptr_cmp(a, b); }

#ifdef __cplusplus
static bool operator==(const fptr &a, const fptr &b) { return fptr_eq(a, b); }
static bool operator!=(const fptr &a, const fptr &b) { return !fptr_eq(a, b); }
  #define typeof(x) std::decay_t<decltype(x)>
#endif

#define setvar_aligned(var, ptr)                   \
  do {                                             \
    const size_t alignment = alignof(typeof(var)); \
    const uintptr_t address = (uintptr_t)(ptr);    \
                                                   \
    if (!(address & (alignment - 1))) {            \
      var = *(typeof(var) *)(ptr);                 \
    } else {                                       \
      memcpy(&(var), (ptr), sizeof(typeof(var)));  \
    }                                              \
  } while (0)

#define align_alloca(type) ({                                           \
  uintptr_t newptr = (uintptr_t)alloca(sizeof(type) + alignof(type));   \
  newptr += (alignof(type) - (newptr % alignof(type))) % alignof(type); \
  (type *)newptr;                                                       \
})

#ifndef __cplusplus
  #define REF(type, value) ((type[1]){value})
// #define REF(type, value) ((type *)(&(type){(value)}))
#else
template <typename T>
static inline T *ref_tmp(T &&v) {
  return &v;
}
  #define REF(type, value) ref_tmp(type{value})

#endif

#define fptr_stack_split(string, ...) ({fptr* __temp__result = (fptr*)alloca( (sizeof((unsigned int[]){__VA_ARGS__}) / sizeof(unsigned int) + 1)*sizeof(fptr) ); \
  do {                                                                                 \
    uint8_t *last;                                                                     \
    unsigned int args[] = {__VA_ARGS__};                                               \
    for (int i = 0; i < sizeof(args) / sizeof(unsigned int); i++) {                    \
      args[i] = (i == 0)                                                               \
                    ? ((args[i] < string.width)                                        \
                           ? args[i]                                                   \
                           : string.width)                                             \
                    : ((string.width < ((args[i] > args[i - 1])                        \
                                            ? args[i]                                  \
                                            : args[i - 1]))                            \
                           ? string.width                                              \
                           : ((args[i] > args[i - 1])                                  \
                                  ? args[i]                                            \
                                  : args[i - 1]));                                     \
      __temp__result[i] = (fptr){                                                              \
          .width = (i == 0) ? (args[0]) : (args[i] - args[i - 1]),                     \
          .ptr = (i == 0) ? (string.ptr) : (last),                                     \
      };                                                                               \
      last = ((uint8_t *)__temp__result[i].ptr) + __temp__result[i].width;                             \
    }                                                                                  \
    __temp__result[sizeof(args) / sizeof(unsigned int)] = (fptr){                              \
        .width = string.width - ((uint8_t *)last - (uint8_t *)string.ptr),             \
        .ptr = last,                                                                   \
    };                                                                                 \
  } while (0);__temp__result; })
#define isSkip(char) ( \
    char == ' ' ||     \
    char == '\n' ||    \
    char == '\r' ||    \
    char == '\t'       \
)
#define UM_DEFAULT(...) {__VA_ARGS__}
#define UM_CASE(fp, ...)     \
  if (fptr_eq(fp, __temp)) { \
    __VA_ARGS__              \
  } else
#define UM_SWITCH(fp, ...) \
  do {                     \
    fptr __temp = fp;      \
    __VA_ARGS__            \
  } while (0)
#define lmemset(arr, arrlen, value)                     \
  do {                                                  \
    typeof(value) __temp = (value);                     \
    fptr __tempFp = ((fptr){                            \
        .width = sizeof(__temp),                        \
        .ptr = ((uint8_t *)&(__temp)),                  \
    });                                                 \
    fpmemset(((uint8_t *)(arr)), (__tempFp), (arrlen)); \
  } while (0)
#define nullFptr ((fptr){0})
#define nullUmf nullFptr
#define nullFFptr ((ffptr){0})

#define um_block(var)                          \
  ((fptr){                                     \
      .width = sizeof(var),                    \
      .ptr = (uint8_t *)(typeof(var)[1]){var}, \
  })
#define um_blockT(type, ...)                    \
  ((fptr){                                      \
      .width = sizeof(type),                    \
      .ptr = (uint8_t *)(type[1]){__VA_ARGS__}, \
  })

#define structEq(a, b)              \
  (fptr_eq(                         \
      (fptr){                       \
          .width = sizeof(a),       \
          .ptr = (uint8_t *)(&(a)), \
      },                            \
      (fptr){                       \
          .width = sizeof(b),       \
          .ptr = (uint8_t *)(&(b)), \
      }                             \
  ))

#ifndef __cplusplus

  #define fp_fromT(struct)           \
    ((fptr){                         \
        .ptr = (uint8_t *)&(struct), \
        .width = sizeof(struct),     \
    })
  #define fp_fromP(ref, size)  \
    ((fptr){                   \
        .ptr = (uint8_t *)ref, \
        .width = size,         \
    })
  #define is_comparr(x) \
    (!__builtin_types_compatible_p(__typeof__(x), __typeof__(&(x)[0])))
  #define fp_from(val)                                                         \
    ((fptr){                                                                   \
        .width =                                                               \
            (is_comparr(val) ? sizeof(val) - 1 : strlen((const char *)(val))), \
        .ptr = (uint8_t *)(val),                                               \
    })

#else
  #include <cstdint>
  #include <cstring>
  #include <string>
template <typename T>
inline fptr fp_from(T &val) {
  return {
      .width = sizeof(T),
      .ptr = reinterpret_cast<uint8_t *>(&val),
  };
}
inline fptr fp_from(fptr u) { return u; }
inline fptr fp_from(const std::string &s) {
  return {
      .width = s.size(),
      .ptr = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(s.data())),
  };
}
inline fptr fp_from(const char *s) {
  return {
      .width = std::strlen(s),
      .ptr = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(s)),
  };
}
template <size_t N>
inline fptr fp_from(const char (&s)[N]) {
  return {
      .width = N - 1,
      .ptr = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(s)),
  };
}

#endif
static fptr fptr_trim(fptr in) {
  while (isSkip(*in.ptr)) {
    in.ptr++;
    in.width--;
  }
  while (isSkip(*(in.ptr + in.width - 1))) {
    in.width--;
  }
  return in;
}
#undef isSkip
#define isDigit(char) (char <= '9' && char >= '0')
static unsigned int fptr_toUint(const fptr in) {
  fptr copy = fptr_trim(in);
  unsigned int res = 0;
  for (size_t place = 0; place < copy.width && isDigit(copy.ptr[place]); place++) {
    res *= 10;
    res += copy.ptr[place] - '0';
  }
  return res;
}
#undef isDigit
static int fptr_toInt(const fptr in) {
  fptr number = fptr_trim(in);
  if (!number.width)
    return 0;
  char negetive = number.ptr[0] == '-';
  if (negetive) {
    number.ptr++;
    number.width--;
  }
  return (negetive ? -1 : 1) * fptr_toUint(number);
}

#define OBJ(ptr, fnel, ...) ((ptr)->fnel(ptr __VA_OPT__(, __VA_ARGS__)))
// #define noAssertMessage
#ifndef NDEBUG
  #define PRINTORANGE "\x1b[38;5;208m"
  #define PRINTRESET "\x1b[0m"
  #define PRINTRED "\x1b[31m\n\n"
  #ifndef noAssertMessage
    #define assertMessage(expr, ...)   \
      do {                             \
        bool result = (expr);          \
        if (!(result)) {               \
          fprintf(                     \
              stderr,                  \
              PRINTRED                 \
              "\nmessage:\n"           \
              "" __VA_ARGS__           \
          );                           \
          fprintf(                     \
              stderr,                  \
              PRINTORANGE              \
              "\nassert:\t%s\n"        \
              "in fn :\t%s\n"          \
              "file  :\t%s\n"          \
              "line  :\t%d\n"          \
              "\nfailed\n",            \
              #expr,                   \
              __PRETTY_FUNCTION__,     \
              __FILE__,                \
              __LINE__                 \
          );                           \
          fprintf(stderr, PRINTRESET); \
          fflush(stderr);              \
          abort();                     \
        }                              \
      } while (0)

  #else
    #include <assert.h>
    #define assertMessage(bool, fmstr, ...) assert(bool)
  #endif
#else
  #define assertMessage(...)
#endif
#define assertOnce(...) ({static bool hasRun = false; if(!hasRun)assertMessage(__VA_ARGS__);hasRun=true; })

#define valElse(expr, ovalue, ...)   \
  /* returns "else" block, or exit*/ \
  ({                                 \
    typeof((expr)) res = (expr);     \
    typeof(res) other = (ovalue);    \
    if (res == ovalue) {             \
      typeof(res) elseBlock = ({     \
        (typeof(res))(ovalue);       \
        __VA_OPT__((__VA_ARGS__);)   \
      });                            \
      res = elseBlock;               \
      assertMessage(res != ovalue);  \
    }                                \
    res;                             \
  })
#define nullElse(expr, ...) \
  valElse(expr, NULL, __VA_ARGS__);
#define countof(v) (sizeof(v) / sizeof(v[i]))

#define valFullElse(expr, onerror, ...)    \
  /*if expr == anything in __VA_ARGS__,    \
   do  onerror;*/                          \
  do {                                     \
    typeof((expr)) res = (expr);           \
    typeof(res) errvals[] = {__VA_ARGS__}; \
    int i = 0;                             \
    for (; i < countof(errvals); i++) {    \
      if (res == errvals[i]) {             \
        onerror;                           \
        break;                             \
      }                                    \
    }                                      \
  } while (0)
#endif // UM_FP_H
#ifdef UM_FP_C
[[gnu::pure]] int fptr_cmp(const fptr a, const fptr b) {
  int wd = a.width - b.width;
  if (wd)
    return wd;
  if (!a.ptr)
    return 0;
  return memcmp(a.ptr, b.ptr, a.width);
}
#endif
