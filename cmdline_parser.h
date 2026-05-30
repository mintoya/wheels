#include "allocator.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include "sList.h"
#include "shmap.h"
#include "stringList.h"
#include "tu_macros.h"

typedef struct {
  char *name;
  enum : u8 {
    CMDARG_BOOLEAN, // present or not
    CMDARG_INTEGER,
    CMDARG_STRING,
  } kind;
  char *description;
} cmdline_option;

tu_def(
    (cmdline_type, u8),
    (cmdline_int, i64),
    (cmdline_bool, bool),
    (cmdline_long, c8 *)
);
msHmap(cmdline_type) cmdLine_parse(AllocatorV allocator, cmdline_option *options, int nargs, char **args) {
  var_ opts = sentList_vla(options);
  var_ result = msHmap_init(allocator, cmdline_type, countof(*opts) + 1);
  msList(c8 *) positionals = msList_init(allocator, c8 *);

  for (int i = 0; i < nargs; i++) {
    int found = 0;
    foreach (var_ opt, vla(*opts)) {
      if (fptr_eq(fp(args[i]), fp(opt.name))) {
        found = true;

        cmdline_type value;
        switch (opt.kind) {
          case CMDARG_BOOLEAN:
            value = (cmdline_type)tu_of(cmdline_bool, true);
            break;
          case CMDARG_INTEGER:
            if (i + 1 < nargs) {
              i++;
              value = (cmdline_type)tu_of(cmdline_int, atoll(args[i]));
            }
            break;
          case CMDARG_STRING:
            if (i + 1 < nargs) {
              i++;
              value = (cmdline_type)tu_of(cmdline_long, (c8 *)args[i]);
            }
            break;
          default:
            unreachable();
            break;
        }

        msHmap_set(result, opt.name, value);
        break;
      }
    }

    if (!found)
      msList_push(allocator, positionals, (c8 *)args[i]);
  }

  msHmap_set(result, " p", (cmdline_type)tu_of(cmdline_long, (c8 *)positionals));
  return result;
}
void cmdLine_deInit(AllocatorV allocator, msHmap(cmdline_type) a) {
  cmdline_type *buffer = msHmap_get(a, " p");
  assertMessage(buffer);
  tu_match(
      buffer[0],
      case (cmdline_long, sl, { msList_deInit(allocator, (c8 **)sl); }),
      default({ unreachable(); })
  );
  msHmap_deinit(a);
}

#define CMDLINE_GENERATE_MAIN(...)                                    \
  extern int cmdline_main(msHmap(cmdline_type) args);                 \
  int main(int argc, char **argv) {                                   \
    var_ parsed = cmdLine_parse(stdAlloc, (__VA_ARGS__), argc, argv); \
    int result = cmdline_main(parsed);                                \
    cmdLine_deInit(stdAlloc, parsed);                                 \
    return result;                                                    \
  }

#define cmdline_get(Map, Name, Type) ({                                      \
  Type *_res = {0};                                                          \
  var_ _v = msHmap_get((Map), (Name));                                       \
  var_ _tt = _Generic(                                                       \
      (Type *){},                                                            \
      bool *: CMDARG_BOOLEAN,                                                \
      int *: CMDARG_INTEGER,                                                 \
      u32 *: CMDARG_INTEGER,                                                 \
      u64 *: CMDARG_INTEGER,                                                 \
      char **: CMDARG_STRING                                                 \
  );                                                                         \
  if (_v) {                                                                  \
    switch (_tt) {                                                           \
      case CMDARG_BOOLEAN: {                                                 \
        assertMessage(_v->tag == TU_TAG(cmdline_bool));                      \
      } break;                                                               \
      case CMDARG_INTEGER: {                                                 \
        assertMessage(_v->tag == TU_TAG(cmdline_int));                       \
      } break;                                                               \
      case CMDARG_STRING: {                                                  \
        assertMessage(_v->tag == TU_TAG(cmdline_long));                      \
      } break;                                                               \
    }                                                                        \
    _res = (typeof(_res))((u8 *)_v + offsetof(typeof(_v[0]), cmdline_long)); \
  }                                                                          \
  _res;                                                                      \
})
void cmdLine_printUsage(char *prog_name, cmdline_option *options) {
  println("Usage: {} [options]...\n\n", prog_name);
  println("Options:\n");

  var_ opts = sentList_vla(options);

  foreach (var_ opt, vla(*opts)) {
    char *type_str = "";
    switch (opt.kind) {
      case CMDARG_BOOLEAN:
        type_str = "<bool>";
        break;
      case CMDARG_INTEGER:
        type_str = "<int>";
        break;
      case CMDARG_STRING:
        type_str = "<str>";
        break;
    }

    println("\t{}:{}:\t{} \n", opt.name, type_str, opt.description ? opt.description : "");
  }
}
char **cmdLine_positionals(msHmap(cmdline_type) args) {
  var_ v = cmdline_get(args, " p", char *);
  return *(char ***)v;
}
// cmdline_option my_options[] = sentList(
//     cmdline_option,
//     {"--help", CMDARG_BOOLEAN, "Print help"},
//     {"--count", CMDARG_INTEGER, "Number of items"}
// );

// CMDLINE_GENERATE_MAIN(stdAlloc, my_options)
// int cmdline_main(msHmap(cmdline_type) args) {
//   var_ t = cmdline_get(args, "--help", u32);
// }
