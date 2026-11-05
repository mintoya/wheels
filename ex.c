#include "fptr.h"
#include "macros.h"
enum defarg_kind {
  DEFARG_BOOL,
  DEFARG_STRING,
};
#define defarg_kind(type) \
  _Generic((type){0}, bool: DEFARG_BOOL, char *: DEFARG_STRING)
typedef struct {
  char *name;
  const char *description;
  enum defarg_kind kind;
  bool present;
  char *value;
} defarg_struct_item_t;
#define defarg_struct_item(item) \
  defarg_struct_item_t ITER_FIRST(item);
#define defarg_options_init(item)                                          \
  {                                                                        \
      .name = ITER_FIRST(ITER_POP(item)),                                  \
      .description = ITER_FIRST(ITER_POP(ITER_POP(item))),                 \
      .kind = defarg_kind(ITER_FIRST(ITER_POP(ITER_POP(ITER_POP(item))))), \
      .present = false,                                                    \
      .value = nullptr,                                                    \
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
  const char *message; // NULL on success
  const char *argument;
};
struct parse_error defarg_parse(
    int *nargs,
    char ***args,
    defarg_struct_item_t *i, // sentinel terminated
    defarg_short *sh         // sentinel terminated
) {
  // Keep argv[0] and compact unrecognized/positional arguments in place.
  // Use '=' for string values starting with '-'. On error, retain the failing
  // argument and everything after it; previously parsed options remain set.
  struct parse_error error = {0};
  int kept = *nargs > 0 ? 1 : 0;
  for (int arg = kept; arg < *nargs; ++arg) {
    char *word = (*args)[arg];
    if (strcmp(word, "--") == 0) {
      while (++arg < *nargs)
        (*args)[kept++] = (*args)[arg];
      break;
    }
    if (word[0] != '-' || word[1] == '\0') {
      (*args)[kept++] = word;
      continue;
    }

    char *equal = strchr(word, '=');
    size_t length = equal ? (size_t)(equal - word) : strlen(word);
    const char *name = word;
    for (defarg_short *alias = sh; alias && alias->shortname; ++alias) {
      if (alias->longname && strlen(alias->shortname) == length &&
          strncmp(word, alias->shortname, length) == 0) {
        name = alias->longname;
        length = strlen(name);
        break;
      }
    }

    defarg_struct_item_t *option = i;
    while (option->name &&
           (strlen(option->name) != length ||
            strncmp(name, option->name, length) != 0))
      ++option;
    if (!option->name) {
      (*args)[kept++] = word;
      continue;
    }

    if (option->kind == DEFARG_BOOL && equal) {
      error = (struct parse_error){"boolean flag does not accept a value", word};
    } else if (option->kind == DEFARG_STRING && !equal && (arg + 1 >= *nargs || ((*args)[arg + 1][0] == '-' && (*args)[arg + 1][1] != '\0'))) {
      error = (struct parse_error){"option requires a value", word};
    }
    if (error.message) {
      while (arg < *nargs)
        (*args)[kept++] = (*args)[arg++];
      break;
    }

    option->present = true;
    option->value = option->kind == DEFARG_BOOL ? nullptr
                    : equal                     ? equal + 1
                                                : (*args)[++arg];
  }
  (*args)[kept] = nullptr;
  *nargs = kept;
  return error;
}
#include "stdio.h"

int main(int nargs, char *args[nargs]) {
  let Options = defargs(
      ((help, "--help", "print help message", bool),
       (out, "--out", "write output to file", char *), ),
      (("-h", "--help"),
       ("-o", "--out"), )
  );
  struct parse_error error = defarg_parse(&nargs, &args, Options.array, Options.shorts);
  if (error.message) {
    fprintf(stderr, "%s: %s\n", error.argument, error.message);
    return 1;
  }
  let opts = Options.options;
  printf("help\t: %s\n", opts.help.present ? "true" : "false");
  printf("out\t: %s\n", opts.out.value);
  printf("positionals:\n");
  foreach (let i, range(0, nargs))
    printf("\t%s\n", args[i]);
}
