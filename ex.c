#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
// #include "wheels/macros/foreach4.h"
typedef struct {
  char *name;
  char *description;
  bool present;
  char *value;
} defarg_struct_item_t;
#define defarg_struct_item(item) \
  defarg_struct_item_t ITER_FIRST(item);
#define defarg_options_init(item)                          \
  {                                                        \
      .name = ITER_FIRST(ITER_POP(item)),                  \
      .description = ITER_FIRST(ITER_POP(ITER_POP(item))), \
      .present = false,                                    \
      .value = nullptr,                                    \
  }
#define defarg_countargs(x) +1
typedef struct {
  char *shortname, *longname;
} defarg_short;
#define defargs(items, short)                                  \
  ((struct {                                                   \
    union {                                                    \
      struct {                                                 \
        APPLY_N(defarg_struct_item, REM_PAREN items)           \
      } options;                                               \
      defarg_struct_item_t array[COUNT_ARGS(REM_PAREN items)]; \
    };                                                         \
    defarg_short *shorts;                                      \
  }){                                                          \
      .array = {                                               \
          APPLY_N_C(defarg_options_init, REM_PAREN items)      \
      },                                                       \
      .shorts = (defarg_short[]){REMOVE_PARENS(short), {0}}    \
  })

let static const Options = defargs(
    (
        (help, "--help", "print help message"),
        (out, "--out", "write output to file"),
    ),
    // short
    ({"-h", "--help"},
     {"-o", "--out"})
);
#include "stdio.h"
int main(void) {
  int count = sizeof(Options.options) / sizeof(defarg_struct_item_t);
  foreach (let item, vla_iter(Options.array))
    printf("name : %s,description : %s\n", item.name, item.description);
  foreach (let item, vla_iter(*sentList_vla(Options.shorts)))
    printf("long : %s,  short: %s\n", item.longname, item.shortname);
}
