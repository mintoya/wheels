#include "cmdline_parser.h"
#include <stdio.h>
void encodeout() {}
cmd_main(
    int nargs,
    char **args,
    (bool, decode, ("--decode", "-d"), "weather to decode", false),
) {
  if (!decode) {
    int x = 0;
    while ((x = fgetc(stdin)) != EOF) {
      putc((x >> 4) + 'a', stdout);
      putc((x & 0xf) + 'a', stdout);
    }
  } else {
    int x[2] = {};
    while ((x[0] = fgetc(stdin)) != EOF && (x[1] = fgetc(stdin)) != EOF) {
      int m = (x[0] - 'a') << 4;
      m += x[1] - 'a';
      assert(m <= 0xff);
      putc(m, stdout);
    }
  }
  return 0;
}
