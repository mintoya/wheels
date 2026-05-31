// include in the main file, no guard
#include "allocator.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include "sList.h"
#include "shmap.h"
#include "stringList.h"
#include "tu_macros.h"
#include <stdcountof.h>
#include <string.h>

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
struct cmdline_roll_t {
  i64 value;
  char *failed;
};
struct cmdline_roll_t cmdline_roll(sentList_t(char) n) {
  char *start = n;
  usize len = strlen(n);
  i64 neg = 1;
  i64 res = 0;
  enum { hex = 16,
         bin = 2,
         dec = 10 } base = dec;
  if (len > 2) {
    if (fptr_eq(fp("0x"), ((fptr){2, start}))) {
      base = hex;
      start += 2;
      len -= 2;
    } else if (fptr_eq(fp("0b"), ((fptr){2, start}))) {
      base = bin;
      start += 2;
      len -= 2;
    }
  }
  if (len > 0 && start[0] == '-') {
    neg = -1;
    start++;
    len--;
  }

  foreach (var_ c, span(start, len)) {
    switch (c[0]) {
      case '0' ... '9': {
        i64 add = c[0] - '0';
        if (add > base) return ((struct cmdline_roll_t){.failed = "base conversion failure"});
        res *= base;
        res += add;
      } break;
      case 'a' ... 'f': {
        i64 add = c[0] - 'a' + 10;
        if (add > base) return ((struct cmdline_roll_t){.failed = "base conversion failure"});
        res *= base;
        res += add;
      } break;
      default:
        return ((struct cmdline_roll_t){.failed = "character not part of any base"});
    }
  }
  return ((struct cmdline_roll_t){res * neg});
}
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
              var_ toi = cmdline_roll(args[i]);
              if (toi.failed) {
                println("couldnt parse {} using {} , error : {}", opt.name, args[i], toi.failed);
                exit(1);
              } else value = (cmdline_type)tu_of(cmdline_int, toi.value);
            } else {
              println("{} requires an argument", opt.name);
              exit(1);
            }
            break;
          case CMDARG_STRING:
            if (i + 1 < nargs) {
              i++;
              value = (cmdline_type)tu_of(cmdline_long, (c8 *)args[i]);
            } else {
              println("{} requires an argument", opt.name);
              exit(1);
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

  msHmap_set(result, ((fptr){8, "\0\0\0\0\0\0\0\0"}), (cmdline_type)tu_of(cmdline_long, (c8 *)positionals));
  return result;
}
void cmdLine_deInit(AllocatorV allocator, msHmap(cmdline_type) a) {
  cmdline_type *buffer = msHmap_get(a, ((fptr){8, "\0\0\0\0\0\0\0\0"}));
  assertMessage(buffer);
  tu_match(
      buffer[0],
      case (cmdline_long, sl, { msList_deInit(allocator, (c8 **)sl); }),
      default({ unreachable(); })
  );
  msHmap_deinit(a);
}

#define CMDLINE_GENERATE_MAIN(...)                                    \
  extern int cmdline_main(cmdline_args args);                         \
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
  println("Usage: {} [options]...\n", prog_name);
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

    println("\t{}:{}:\t{}", opt.name, type_str, opt.description ? opt.description : "");
  }
}
typedef msHmap(cmdline_type) cmdline_args;

char **cmdLine_positionals(cmdline_args args) {
  var_ v = cmdline_get(args, ((fptr){8, "\0\0\0\0\0\0\0\0"}), char *);
  return *(char ***)v;
}
