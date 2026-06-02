#ifndef ASSERTMESSAGE_H
#define ASSERTMESSAGE_H
#include "macros.h"
#include "mytypes.h"
#include <assert.h>
#include <stdarg.h>
#include <stdcountof.h>
#include <stdlib.h>
#define ASSERTMESSAGE_PRINTORANGE "\x1b[38;5;208m"
#define ASSERTMESSAGE_PRINTRESET "\x1b[0m"
#define ASSERTMESSAGE_PRINTRED "\x1b[31m\n\n"

// {output macors
// {int _am_write(void*,unsigned)
#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
static inline int _am_write(const void *buf, unsigned len) {
  return _write(2, buf, len);
}
#elif __has_include(<unistd.h>)
  #include <unistd.h>
static inline int _am_write(const void *buf, unsigned len) {
  return (int)write(2, buf, (size_t)len);
}
#else
static inline int _am_write(const void *buf, unsigned len) {
  (void)buf;
  (void)len;
  return -1; // unsupported platform
}
#endif

// }

static inline void _am_puts(const char *s) {
  if (!s) return;
  unsigned n = 0;
  while (s[n])
    n++;
  _am_write(s, n);
}
// { format
static inline void _am_write_char(char c) { _am_write(&c, 1); }

static inline void _am_write_str(const char *s) {
  s = s ?: "(nullstr)";
  while (s[0])
    _am_write(s++, 1);
}
static inline void _am_write_uint(unsigned long long v, int base, int upper) {
  const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char buf[32];
  buf[countof(buf) - 1] = 0;
  char *p = buf + countof(buf) - 2;
  while (v) {
    p[0] = digits[v % base];
    v /= base;
    p--;
  }
  _am_write_str(p + 1);
}
static inline void _am_write_int(long long v) {
  if (v < 0) {
    _am_write_char('-');
    _am_write_uint((unsigned long long)-v, 10, 0);
  } else {
    _am_write_uint((unsigned long long)v, 10, 0);
  }
}
static void _am_vfmt(const char *fmt, va_list ap) {
  for (; *fmt; fmt++) {
    if (*fmt != '%') {
      _am_write_char(*fmt);
      continue;
    }
    fmt++;
    switch (*fmt) {
      case 's':
        _am_write_str(va_arg(ap, const char *));
        break;
      case 'c':
        _am_write_char((char)va_arg(ap, int));
        break;
      case 'd':
      case 'i':
        _am_write_int((longlong)va_arg(ap, int));
        break;
      case 'u':
        _am_write_uint((ulonglong)va_arg(ap, unsigned), 10, 0);
        break;
      case 'x':
        _am_write_uint((ulonglong)va_arg(ap, unsigned), 16, 0);
        break;
      case 'X':
        _am_write_uint((ulonglong)va_arg(ap, unsigned), 16, 1);
        break;
      case 'z': {
        if (fmt[1] == 'u') {
          fmt++;
          _am_write_uint(va_arg(ap, usize), 10, 0);
        }
      } break;
      case '%':
        _am_write_char('%');
        break;
      default:
        _am_write_char('%');
        _am_write_char(*fmt);
        break;
    }
  }
}
// }
// }

#ifndef assertMessage_no_backtrace
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

#ifndef NDEBUG
  #if !defined(noAssertMessage)

// [[noreturn, gnu::cold, gnu::format(printf, 7, 8)]]
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

    #ifndef assertMessage_no_backtrace
      #define _ASSERT_GET_BT(arr) backtrace(arr, 5)
    #else
      #define _ASSERT_GET_BT(arr) 0
    #endif

    #define assertMessage(expr, ...)            \
      do {                                      \
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
      } while (0)
  #else
    #include <assert.h>
    #define assertMessage(bool, ...) assert(bool)
  #endif
#else
  #define assertMessage(expr, ...)      \
    if (__builtin_expect(!(expr), 0)) { \
      assertMessage_fail_ins();         \
    }
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

#if __has_builtin(__builtin_unreachable)
  #define unreachable() __builtin_unreachable()
#else
  #define unreachable() assertMessage(false, "reached unreachable code")
#endif

#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
#define ASSERTMESSAGE_C (1)
#endif
#if defined(ASSERTMESSAGE_C) && !defined(noAssertMessage)

// #if !defined(ASSERTMESSAGE_OUTPUT)
//    #define ASSERTMESSAGE_OUTPUT(...) fprintf(stderr, __VA_ARGS__)
// #endif
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
  va_list args;
  va_start(args, fmt);
  _am_vfmt(fmt, args);
  va_end(args);
  _am_puts(ASSERTMESSAGE_PRINTORANGE "\nassert:\t");
  _am_puts(expr_str);
  _am_puts("\nin fn :\t");
  _am_puts(func);
  _am_puts("\nfile  :\t");
  _am_puts(file);
  _am_puts("\nline  :\t");
  _am_write_uint(line, 10, 0);
  _am_puts("\n\nfailed\n" ASSERTMESSAGE_PRINTRESET);

#ifndef assertMessage_no_backtrace
  char **syms = backtrace_symbols(trace, traceLen);
  if (syms) {
    _am_puts(ASSERTMESSAGE_PRINTRED "backtrace:\n==========================\n");
    for (size_t i = 0; i < traceLen; i++) {
      _am_puts(syms[i]);
      _am_puts("\n");
    }
    _am_puts("==========================\n" ASSERTMESSAGE_PRINTRESET);
  }
#endif
  assertMessage_fail_ins();
}

#ifndef assertMessage_no_backtrace
  #if __has_include(<execinfo.h>)
    #include <execinfo.h>
    #include <unistd.h>
  #elif __has_include(<windows.h>) && __has_include ( <dbghelp.h> ) && __has_include ( <errhandlingapi.h> ) && __has_include ( <io.h> ) && __has_include ( <winbase.h> )
    //
    #include <windows.h>
    //
    #include <dbghelp.h>
    #include <errhandlingapi.h>
    #include <io.h>
    #include <winbase.h>

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
