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
  }
#define defargs(items, short)                                      \
  ((struct {                                                       \
    struct {                                                       \
      APPLY_N(defarg_struct_item, REM_PAREN items)                 \
    } options;                                                     \
    unsigned int shortcount;                                       \
    struct {                                                       \
      char *shortname, *name;                                      \
    } *shorts;                                                     \
  }){                                                              \
      .options = {                                                 \
          APPLY_N_C(defarg_options_init, REM_PAREN items)          \
      },                                                           \
      .shortcount = countof((struct {char * a ; char* b; }[]){REMOVE_PARENS(short)}), \
  })

let static const Options = defargs(
    (
        (help, "--help", "print help message"),
        (out, "--out", "write output to file"),
    ),
    // short
    ({"-h", "--help"},
     {"-o", "--out"},
     {"bs", "bs"})
);
#include "stdio.h"
int main(void) {
  printf("%i\n", Options.shortcount);
  int count = sizeof(Options.options) / sizeof(defarg_struct_item_t);
  defarg_struct_item_t(*items)[count] = (typeof(items))(&Options.options);
  foreach (let item, vla_iter(*items))
    printf("name : %s,description : %s\n", item.name, item.description);
}
