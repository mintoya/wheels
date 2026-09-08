#if !defined ASSERTMESSAGE_H
  #define ASSERTMESSAGE_H (1)
  #include "macros.h"
  #include "mytypes.h"
  #include "trace.h"
  #include <stdarg.h>
  #include <stdio.h>

  #define TODO(str, ...) \
    ({ assertMessage(false, "todo : " #str); \
    _Pragma("GCC warning \" todo in program  \"")__VA_OPT__(( __VA_ARGS__ ){};) })

  #define ASSERTMESSAGE_PRINTORANGE "\x1b[38;5;208m" // ]
  #define ASSERTMESSAGE_PRINTRESET "\x1b[0m"         // ]
  #define ASSERTMESSAGE_PRINTRED "\x1b[31m\n\n"      // ]

  #if __has_builtin(__builtin_trap)
    #define assertMessage_fail_ins() __builtin_trap()
  #else
    #define assertMessage_fail_ins() abort()
  #endif

void __attribute__((noreturn)) _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    const char *message,
    ...
);

  #define assertMessage(expr, ...)            \
    ({                                        \
      DIAGNOSTIC_PUSH("-Wunknown-attributes") \
      if_unlikely (!(expr)) {                 \
        DIAGNOSTIC_POP()                      \
        _assertMessageFail(                   \
            #expr,                            \
            __PRETTY_FUNCTION__,              \
            __FILE__,                         \
            __LINE__,                         \
            "" __VA_ARGS__                    \
        );                                    \
      }                                       \
      0;                                      \
    })

  #if !defined unreachable
    #if __has_builtin(__builtin_unreachable)
      #define unreachable() __builtin_unreachable()
    #else
      #define unreachable() assertMessage(false, "reached unreachable code")
    #endif
  #endif

#endif
#if defined(ASSERTMESSAGE_C) && (ASSERTMESSAGE_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)

  #undef ASSERTMESSAGE_C
  #define ASSERTMESSAGE_C (2)
void __attribute__((noreturn)) _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    const char *fmt,
    ...
) {
  fprintf(stderr, ASSERTMESSAGE_PRINTRED "\nmessage:\n");
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (len) fwrite(buf, (unsigned)len < sizeof(buf) ? (unsigned)len : sizeof(buf) - 1, sizeof(char), stderr);

  len = snprintf(buf, sizeof(buf), ASSERTMESSAGE_PRINTORANGE "\nassert:\t%s\nin fn :\t%s\nfile  :\t%s\nline  :\t%u\n\nfailed\n" ASSERTMESSAGE_PRINTRESET, expr_str, func, file, line);
  if (len) fwrite(buf, (unsigned)len < sizeof(buf) ? (unsigned)len : sizeof(buf) - 1, sizeof(char), stderr);
  fflush(stderr);
  let trace = getTrace(stdAlloc);
  for (int i = 0; i < trace.len; i++)
    printf("\t%p\n", (char *)((char *)trace.ptr[i].fn - (char *)__base_address));
  assertMessage_fail_ins();
}
#endif
