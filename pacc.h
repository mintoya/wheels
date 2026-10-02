#include <stdio.h>
#include <stdlib.h>
typedef struct dependency dependency;
struct dependency {
  const char *const dirname; // dir name within deps/...
  const char *const url;     // git url
  struct {
    const char *const in;  // command to run within the pwd of the dependency
    const char *const out; // command to run here
  } post;
};

dependency nob = {
    .dirname = "nob",
    .url = "https://github.com/tsoding/nob.h",
};
#include "unistd.h"
int main(int nargs, char **args) {
  printf("%s\n", getenv("PWD"));
  for (typeof(_Generic(1, int: 1)) i = 0; i < nargs; i++)
    printf("%s\n", args[i]);
  printf("%s\n", realpath(args[0], NULL));
}
