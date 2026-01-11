#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
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
          backtrace_symbols_fd(array, size, STDOUT_FILENO); \
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
