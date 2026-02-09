#ifndef ASSERTMESSAGE_H
#define ASSERTMESSAGE_H
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#if defined(__linux__)
  #include <execinfo.h>
  #include <unistd.h>
#elif defined(_WIN32) || (_WIN64)
  //
  #include <windows.h>
  //
  #include <dbghelp.h>
  #include <errhandlingapi.h>
  #include <io.h>
  #include <winbase.h>
size_t backtrace(void **array, size_t size);
char **backtrace_symbols(void *array[], size_t size);
#endif
#ifndef NDEBUG
  #if !defined(noAssertMessage)
void _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    void *trace[5],
    uint traceLen,
    const char *fmt,
    ...
);
    #define assertMessage(expr, ...)         \
      do {                                   \
        bool result = (expr);                \
        if (!(result)) {                     \
          void *array[5];                    \
          size_t size = backtrace(array, 5); \
          _assertMessageFail(                \
              #expr,                         \
              __PRETTY_FUNCTION__,           \
              __FILE__,                      \
              __LINE__,                      \
              array,                         \
              size, "" __VA_ARGS__           \
          );                                 \
        }                                    \
      } while (0)
  #else
    #include <assert.h>
    #define assertMessage(bool, ...) assert(bool)
  #endif
#else
  #define assertMessage(...)
#endif
#define assertOnce(...) ({static bool hasRun = false; if(!hasRun)assertMessage(__VA_ARGS__);hasRun=true; })
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define ASSERTMESSAGE_C (1)
#endif
#if defined(ASSERTMESSAGE_C) && !defined(noAssertMessage)

#define PRINTORANGE "\x1b[38;5;208m"
#define PRINTRESET "\x1b[0m"
#define PRINTRED "\x1b[31m\n\n"
[[noreturn, gnu::cold, gnu::format(printf, 7, 8)]]
void _assertMessageFail(
    const char *expr_str,
    const char *func,
    const char *file,
    uint line,
    void *trace[5],
    uint traceLen,
    const char *fmt,
    ...
) {
  fprintf(stderr, PRINTRED "\nmessage:\n");

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(
      stderr,
      PRINTORANGE "\nassert:\t%s\n"
                  "in fn :\t%s\n"
                  "file  :\t%s\n"
                  "line  :\t%d\n"
                  "\nfailed\n" PRINTRESET,
      expr_str, func, file, line
  );
  fflush(stderr);

  char **syms = backtrace_symbols(trace, traceLen);

  if (syms) {
    fprintf(stderr, PRINTRED "backtrace:\n==========================\n");
    for (size_t i = 0; i < traceLen; i++)
      fprintf(stderr, "%s\n", syms[i]);
    fprintf(stderr, "==========================\n" PRINTRESET);
  }
  exit(1);
}
#if (defined(_WIN32) || defined(_WIN64))
size_t backtrace(void **array, size_t size) {
  return CaptureStackBackTrace(
      0, // Changed from 1 to 0 - skip frames in backtrace_symbols_fd instead
      size,
      array,
      NULL
  );
}
  #include <stdio.h>
char **backtrace_symbols(void *array[], size_t size) {
  char **result = (char **)malloc(sizeof(char *) * size + sizeof(char[512]) * size);
  memset(result, 0, sizeof(char *) * size + sizeof(char[512]) * size);
  for (int i = 0; i < size; i++)
    result[i] = ((char *)result) + sizeof(char *) * size + sizeof(char[512]) * i;

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
#endif
#endif
