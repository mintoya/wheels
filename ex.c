#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
#include <string.h>
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
#define defarg_genshort(item) {REMOVE_PARENS(item)}
#define defargs(items, short)                                                           \
                                                                                        \
  ((struct {                                                                            \
    union {                                                                             \
      struct {                                                                          \
        APPLY_N(defarg_struct_item, REM_PAREN items)                                    \
      } options;                                                                        \
      defarg_struct_item_t array[COUNT_ARGS(REM_PAREN items) + 1];                      \
    };                                                                                  \
    defarg_short *shorts;                                                               \
  }){                                                                                   \
      .array = {                                                                        \
          APPLY_N_C(defarg_options_init, REM_PAREN items), {0}                          \
      },                                                                                \
      .shorts = (defarg_short[]){APPLY_N_C(defarg_genshort, REMOVE_PARENS(short)), {0}} \
  })
struct parse_error {
};
struct parse_error defarg_parse(
    int *nargs,
    char ***args,
    defarg_struct_item_t *i, // sentinel terminated
    defarg_short *sh         // sentinel terminated
) {
  return (struct parse_error){};
}
#include "stdio.h"
let Options = defargs(
    ((help, "--help", "print help message"),
     (out, "--out", "write output to file"), ),
    (("-h", "--help"),
     ("-o", "--out"), )
);
int main(int nargs, char *args[nargs]) {
  defarg_parse(&nargs, &args, Options.array, Options.shorts);
  foreach (let item, vla_iter(Options.array))
    printf("name : %s,description : %s\n", item.name, item.description);
  foreach (let item, vla_iter(*sentList_vla(Options.shorts)))
    printf("long : %s,  short: %s\n", item.longname, item.shortname);
}
