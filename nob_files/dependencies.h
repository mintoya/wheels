#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined CC
  #define CC "clang"
#endif
#if defined(_WIN32)
  #define EXTENSION ".exe"
#else
  #define EXTENSION ""
#endif

char *strcats(char *c, ...) {
  va_list l;
  va_start(l, c);
  char *nc = 0;
  while ((nc = va_arg(l, char *)))
    strcat(c, nc);
  va_end(l);
  return c;
}
#define strcats(cp, ...) strcats(cp __VA_OPT__(, __VA_ARGS__, (char *)0))
char *strcats_space(char *c, ...) {
  va_list l;
  va_start(l, c);
  char *nc = 0;
  while ((nc = va_arg(l, char *)))
    strcat(strcat(c, nc), " ");
  va_end(l);
  return c;
}
#define strcats_space(cp, ...) strcats_space(cp __VA_OPT__(, __VA_ARGS__, (char *)0))
int clone(char *url) {
  return system(strcats((char[1024]){}, "git clone ", url));
}
int main(int argc, char **argv) {
  puts("Missing dependencies. Cloning...\n");
  if (
      clone("https://github.com/tsoding/nob.h deps/nob.h") ||
      clone("https://github.com/sheredom/subprocess.h deps/subprocess.h") ||
      clone("https://github.com/jtsiomb/c11threads")
  ) {
    puts("Failed to clone dependencies.\n");
    return 1;
  }

  if (rename("nob" EXTENSION, "nob.old" EXTENSION)) {
    puts("Failed to rename self\n");
    return 1;
  }
  if (system(strcats_space(((char[1024]){}), CC, "-o", "nob" EXTENSION, "nob.c"))) {
    puts("Failed to recompile self\n");
    return 1;
  }
  printf("done cloning dependencies, re-run nob");
}
