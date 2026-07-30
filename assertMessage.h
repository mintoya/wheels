#if !defined ASSERTMESSAGE_H
  #define ASSERTMESSAGE_H (1)
  #include "macros.h"
  #include "mytypes.h"
  #include <stdarg.h>
  #include <stdio.h>

  #define TODO(...)                \
    assert(false && #__VA_ARGS__); \
    exit(1) _Pragma("GCC warning \" todo in program  \"")

  #define ASSERTMESSAGE_PRINTORANGE "\x1b[38;5;208m" // ]
  #define ASSERTMESSAGE_PRINTRESET "\x1b[0m"         // ]
  #define ASSERTMESSAGE_PRINTRED "\x1b[31m\n\n"      // ]

static inline int _am_write(const void *buf, unsigned len) {
  return (int)fwrite(buf, 1, len, stderr);
}

static inline void _am_puts(const char *s) {
  if (!s) return;
  unsigned n = 0;
  while (s[n])
    n++;
  _am_write(s, n);
}

  #if !defined assertMessage_no_backtrace
    #if __has_include(<execinfo.h>)
      #include <execinfo.h>
    #else
EXTERN_C_START
extern char **backtrace_symbols(void *const *__array, int __size);
extern int backtrace(void **__array, int __size) __attribute__((nonnull(1)));
EXTERN_C_END
    #endif
  #endif

  #if __has_builtin(__builtin_trap)
    #define assertMessage_fail_ins() __builtin_trap()
  #else
    #define assertMessage_fail_ins() abort()
  #endif

  #if !defined NDEBUG
    #if !defined(noAssertMessage)

void __attribute__((noreturn)) _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    void *trace[5],
    uint traceLen,
    const char *message,
    ...
);

      #if !defined assertMessage_no_backtrace
        #define _ASSERT_GET_BT(arr) backtrace(arr, 5)
      #else
        #define _ASSERT_GET_BT(arr) 0
      #endif

      #define assertMessage(expr, ...)            \
        ({                                        \
          DIAGNOSTIC_PUSH("-Wunknown-attributes") \
          if_unlikely (!(expr)) {                 \
            DIAGNOSTIC_POP()                      \
            void *array[5];                       \
            size_t size = _ASSERT_GET_BT(array);  \
            _assertMessageFail(                   \
                #expr,                            \
                __PRETTY_FUNCTION__,              \
                __FILE__,                         \
                __LINE__,                         \
                array,                            \
                size,                             \
                "" __VA_ARGS__                    \
            );                                    \
          }                                       \
          0;                                      \
        })
    #else
      #include <assert.h>
      #define assertMessage(bool, ...) ({ assert(bool);0; })
    #endif
  #else
    #define assertMessage(expr, ...)                  \
      ({                                              \
        if_unlikely (!expr) assertMessage_fail_ins(); \
        0;                                            \
      })
  #endif

  #define assertOnce(...)           \
    do {                            \
                                    \
      static char hasRun = false;   \
                                    \
      if (!hasRun)                  \
        assertMessage(__VA_ARGS__); \
      hasRun = true;                \
                                    \
    } while (0)

  #if !defined unreachable
    #if __has_builtin(__builtin_unreachable)
      #define unreachable() __builtin_unreachable()
    #else
      #define unreachable() assertMessage(false, "reached unreachable code")
    #endif
  #endif

#endif
#if defined(noAssertMessage)
  #undef ASSERTMESSAGE_C
  #define ASSERTMESSAGE_C (2)
#endif
#if defined(ASSERTMESSAGE_C) && (!defined(noAssertMessage) && ASSERTMESSAGE_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)

  #undef ASSERTMESSAGE_C
  #define ASSERTMESSAGE_C (2)

void __attribute__((noreturn)) _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    void *trace[5],
    uint traceLen,
    const char *fmt,
    ...
) {
  _am_puts(ASSERTMESSAGE_PRINTRED "\nmessage:\n");

  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (len > 0) {
    _am_write(buf, (unsigned)len < sizeof(buf) ? (unsigned)len : sizeof(buf) - 1);
  }

  len = snprintf(buf, sizeof(buf), ASSERTMESSAGE_PRINTORANGE "\nassert:\t%s\nin fn :\t%s\nfile  :\t%s\nline  :\t%u\n\nfailed\n" ASSERTMESSAGE_PRINTRESET, expr_str, func, file, line);

  if (len > 0) {
    _am_write(buf, (unsigned)len < sizeof(buf) ? (unsigned)len : sizeof(buf) - 1);
  }

  #if !defined assertMessage_no_backtrace
  _am_puts(ASSERTMESSAGE_PRINTRED "backtrace:\n==========================\n");
    #if __has_include(<execinfo.h>)
  backtrace_symbols_fd(trace, traceLen, 2);
    #else
  char **syms = backtrace_symbols(trace, traceLen);
  if (syms) {
    for (size_t i = 0; i < traceLen; i++) {
      _am_puts(syms[i]);
      _am_puts("\n");
    }
  }
    #endif
  _am_puts("==========================\n" ASSERTMESSAGE_PRINTRESET);
  #endif

  fflush(stderr);
  assertMessage_fail_ins();
}

  #if !defined assertMessage_no_backtrace
    #if __has_include(<execinfo.h>)
      #include <execinfo.h>
      #include <unistd.h>
    #elif __has_include(<windows.h>) && __has_include ( <dbghelp.h> ) && __has_include ( <errhandlingapi.h> ) && __has_include ( <io.h> ) && __has_include ( <winbase.h> )
      #include <dbghelp.h>
      #include <errhandlingapi.h>
      #include <io.h>
      #include <winbase.h>
      #include <windows.h>

int __attribute__((nonnull(1))) backtrace(void **array, int size) {
  return CaptureStackBackTrace(
      0,
      size,
      array,
      NULL
  );
}

char **backtrace_symbols(void *const *array, int size) {
  thread_local static char backtraceResult[5][512];
  thread_local static char *result[5];
  size = size < countof(backtraceResult) ? size : countof(backtraceResult);
  for (int i = 0; i < size; i++)
    result[i] = backtraceResult[i];

  HANDLE process = GetCurrentProcess();

  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
  if (!SymInitialize(process, NULL, TRUE))
    return result;
  struct {
    SYMBOL_INFO info;
    char buffer[256];
  } sinfo = {
      .info = {
          .SizeOfStruct = sizeof(SYMBOL_INFO),
          .MaxNameLen = 255,
      }
  };
  SYMBOL_INFO *symbol = &(sinfo.info);
  IMAGEHLP_LINE64 line;

  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for (unsigned int i = 0; i < size; i++) {
    DWORD64 address = (DWORD64)(array[i]);
    DWORD64 displacement = 0;
    DWORD line_displacement = 0;

    char *output = result[i];
    int len = 0;

    if (SymFromAddr(process, address, &displacement, symbol)) {
      if (SymGetLineFromAddr64(process, address, &line_displacement, &line)) {
        len = snprintf(output, 512, "%s+0x%llx (%s:%lu)\n", symbol->Name, displacement, line.FileName, line.LineNumber);
      } else {
        len = snprintf(output, 512, "%s+0x%llx\n", symbol->Name, displacement);
      }
    } else {
      len = snprintf(output, 512, "[Unable to resolve symbol at 0x%llx]\n", address);
    }
  }
  SymCleanup(process);
  return result;
}
    #elif __has_include(<stm32u5xx.h>)
EXTERN_C_START

static char trace_buf[4][12];
static char *trace_ptrs[countof(trace_buf)];

char **backtrace_symbols(void *const *array, int size) {
  int count = (size < countof(trace_buf)) ? size : countof(trace_buf);
  for (int i = 0; i < count; i++) {
    snprintf(trace_buf[i], countof(trace_buf[0]), "0x%p", array[i]);
    trace_ptrs[i] = trace_buf[i];
  }
  return trace_ptrs;
}
int backtrace(void **__array, int __size) {
  if (__size > 0) {
    __array[0] = __builtin_return_address(0);
    return 1;
  }
  return 0;
}
EXTERN_C_END
    #else
EXTERN_C_START
char **backtrace_symbols(void *const *array, int size) { return NULL; }
extern int backtrace(void **__array, int __size) { return 0; }
EXTERN_C_END
    #endif
  #endif
#endif
