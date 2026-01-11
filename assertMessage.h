#ifndef ASSERTMESSAGE_H
#define ASSERTMESSAGE_H
#if defined(__linux__)
  #include <execinfo.h>
  #include <unistd.h>
#else
  #define STDOUT_FILENO (_fileno(stdout))
  #define STDERR_FILENO (_fileno(stderr))
  //
  #include <windows.h>
  //
  #include <dbghelp.h>
  #include <errhandlingapi.h>
  #include <io.h>
  #include <winbase.h>
size_t backtrace(void **array, size_t size);
void backtrace_symbols_fd(void *array[], size_t size, int filnumber);
#endif
#ifndef NDEBUG
  #define PRINTORANGE "\x1b[38;5;208m"
  #define PRINTRESET "\x1b[0m"
  #define PRINTRED "\x1b[31m\n\n"
  #ifndef noAssertMessage
    #define assertMessage(expr, ...)                        \
      do {                                                  \
        bool result = (expr);                               \
        if (!(result)) {                                    \
          fprintf(                                          \
              stderr,                                       \
              PRINTRED                                      \
              "\nmessage:\n"                                \
              "" __VA_ARGS__                                \
          );                                                \
          fprintf(                                          \
              stderr,                                       \
              PRINTORANGE                                   \
              "\nassert:\t%s\n"                             \
              "in fn :\t%s\n"                               \
              "file  :\t%s\n"                               \
              "line  :\t%d\n"                               \
              "\nfailed\n",                                 \
              #expr,                                        \
              __PRETTY_FUNCTION__,                          \
              __FILE__,                                     \
              __LINE__                                      \
          );                                                \
          fprintf(stderr, PRINTRESET);                      \
          fflush(stderr);                                   \
          /* TODO stack trace some other way? */            \
          void *array[5];                                   \
          size_t size = backtrace(array, 5);                \
          backtrace_symbols_fd(array, size, STDERR_FILENO); \
          abort();                                          \
        }                                                   \
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
size_t backtrace(void **array, size_t size) {
  return CaptureStackBackTrace(
      0, // Changed from 1 to 0 - skip frames in backtrace_symbols_fd instead
      size,
      array,
      NULL
  );
}
#include <stdio.h>
void backtrace_symbols_fd(void *array[], size_t size, int filnumber) {
  HANDLE process = GetCurrentProcess();

  // Initialize symbol handler with better options
  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
  if (!SymInitialize(process, NULL, TRUE)) {
    char error_msg[] = "[Symbol initialization failed]\n";
    _write(filnumber, error_msg, sizeof(error_msg) - 1);
    return;
  }

  // Allocate buffer for symbol info
  char buffer[sizeof(SYMBOL_INFO) + 256 * sizeof(char)];
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = 255;

  // Line info
  IMAGEHLP_LINE64 line;
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for (unsigned int i = 0; i < size; i++) {
    DWORD64 address = (DWORD64)(array[i]);
    DWORD64 displacement = 0;
    DWORD line_displacement = 0;

    char output[512];
    int len = 0;

    if (SymFromAddr(process, address, &displacement, symbol)) {
      // Try to get line information
      if (SymGetLineFromAddr64(process, address, &line_displacement, &line)) {
        len = snprintf(output, sizeof(output), "%s+0x%llx (%s:%lu)\n", symbol->Name, displacement, line.FileName, line.LineNumber);
      } else {
        len = snprintf(output, sizeof(output), "%s+0x%llx\n", symbol->Name, displacement);
      }
    } else {
      len = snprintf(output, sizeof(output), "[Unable to resolve symbol at 0x%llx]\n", address);
    }

    _write(filnumber, output, len);
  }

  SymCleanup(process);
}
#endif
