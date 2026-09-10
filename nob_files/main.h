#include "../deps/nob.h/nob.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if __has_include(<stdcountof.h>)
  #include <stdcountof.h>
#else
  #define countof(x) (sizeof(x) / sizeof(x[0]))
#endif

#if !defined CC
  #define CC "clang"
#endif

#if defined(_WIN32)
  #define EXTENSION ".exe"
#else
  #define EXTENSION ""
#endif

#if !defined INPUT_FLAGS
  #define INPUT_FLAGS ""
#endif

#define COMMON_CFLAGS            \
  INPUT_FLAGS,                   \
      "-std=c2y",                \
      "-fdefer-ts",              \
      "-fno-sanitize=vla-bound", \
      "-fsanitize=alignment",    \
      "-finstrument-functions",  \
      "-g"

#define let __auto_type
#define cmd_imm(...) ({Cmd _r = {};cmd_append(&_r , __VA_ARGS__);_r; })
#define nob_da_appends(procs, items) \
  nob_da_append_many(procs, items, countof(items))

int build_wheels(const char *cfile, int nargs, char **args) {
  if (!mkdir_if_not_exists("build")) return false;

  // clang-format off
  if (!cmd_run_sync(cmd_imm(
    CC, COMMON_CFLAGS, 
    "-rdynamic", 
    "-o", "build/wheels" EXTENSION, 
    "-xc", cfile
  ))) return false;
  // clang-format on

  Cmd runcmd = {};
  cmd_append(&runcmd, "./build/wheels" EXTENSION);
  for (int i = 0; i < nargs; i++)
    cmd_append(&runcmd, args[i]);
  return !cmd_run_sync(runcmd);
}

void usage(const char *program) {
  nob_log(INFO, "Usage: %s [<subcommand>]", program);
  nob_log(INFO, "Subcommands:               \n"
                "    help                   \n"
                "        Print this message \n"
                "    run [--file <file>] [args]\n"
                "        Build and run wheels\n");
}

int actualmain(int argc, char **argv) {
  const char *program = shift_args(&argc, &argv);

  if (!argc) return build_wheels("tests.h", argc, argv);

  const char *subcmd = shift_args(&argc, &argv);

  if (!strcmp(subcmd, "help")) return (usage(program), 0);

  if (!strcmp(subcmd, "run")) {
    const char *cfile = "tests.h";
    if (argc >= 2 && !strcmp(argv[0], "--file")) {
      shift_args(&argc, &argv);
      cfile = shift_args(&argc, &argv);
    }
    return build_wheels(cfile, argc, argv);
  }

  nob_log(ERROR, "Unknown subcommand %s", subcmd);
  usage(program);
  return 1;
}

#define NOB_IMPLEMENTATION
#include "../deps/nob.h/nob.h"
