#ifndef ASSERTMESSAGE_H
#define ASSERTMESSAGE_H
#include <assert.h>
#include <stdio.h>
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
  #define PRINTORANGE "\x1b[38;5;208m"
  #define PRINTRESET "\x1b[0m"
  #define PRINTRED "\x1b[31m\n\n"
  #ifndef noAssertMessage
    // TODO move to a functoin
    #define assertMessage(expr, ...)                                   \
      do {                                                             \
        bool result = (expr);                                          \
        if (!(result)) {                                               \
          fprintf(                                                     \
              stderr,                                                  \
              PRINTRED                                                 \
              "\nmessage:\n"                                           \
              "" __VA_ARGS__                                           \
          );                                                           \
          fprintf(                                                     \
              stderr,                                                  \
              PRINTORANGE                                              \
              "\nassert:\t%s\n"                                        \
              "in fn :\t%s\n"                                          \
              "file  :\t%s\n"                                          \
              "line  :\t%d\n"                                          \
              "\nfailed\n",                                            \
              #expr,                                                   \
              __PRETTY_FUNCTION__,                                     \
              __FILE__,                                                \
              __LINE__                                                 \
          );                                                           \
          fprintf(stderr, PRINTRESET);                                 \
          fflush(stderr);                                              \
          /* TODO stack trace some other way? */                       \
          void *array[5];                                              \
          size_t size = backtrace(array, 5);                           \
          char **syms = backtrace_symbols(array, size);                \
          printf(PRINTRED "backtrace:\n==========================\n"); \
          for (usize i = 0; i < size; i++) {                           \
            printf("%s\n", syms[i]);                                   \
          }                                                            \
          printf("==========================\n" PRINTRESET);           \
          abort();                                                     \
        }                                                              \
      } while (0)
  #else
    #include <assert.h>
    #define assertMessage(bool, fmstr, ...) assert(bool)
  #endif
#else
  #define assertMessage(...)
#endif
#define assertOnce(...) ({static bool hasRun = false; if(!hasRun)assertMessage(__VA_ARGS__);hasRun=true; })
#endif
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define ASSERTMESSAGE_C (1)
#endif
#ifdef ASSERTMESSAGE_C
#if defined(_WIN32) || (_WIN64)
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
  for (auto i = 0; i < size; i++)
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
