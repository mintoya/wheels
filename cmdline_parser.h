#include "fptr.h"
#include "macros.h"
#include "print.h"

#define TUPLE_GET_4_(a, b, c, d, ...) d
#define TUPLE_GET_4(...) TUPLE_GET_4_ __VA_ARGS__

#define TUPLE_GET_5_(a, b, c, d, e, ...) e
#define TUPLE_GET_5(...) TUPLE_GET_5_ __VA_ARGS__

#define cmd_arg_name_(a, b, ...) b
#define cmd_arg_name(tuple) cmd_arg_name_ tuple

#define cmd_arg_decl_(type, name, acess, message, ...) type name = VA_SWITCH({}, __VA_ARGS__);
#define cmd_arg_decl(tuple) cmd_arg_decl_ tuple

#define cmd_arg_type_(type, name, ...) type
#define cmd_arg_type(tuple) cmd_arg_type_ tuple

#define cmd_arg_param_(name, type, ...) name type
#define cmd_arg_param(tuple) cmd_arg_param_ tuple

#define cmd_arg_flags_(type, name, flags, ...) flags
#define cmd_arg_flags(tuple) cmd_arg_flags_ tuple

#define cmd_arg_parse(tuple)                                                                     \
  {                                                                                              \
    char *_f[] = (char *[]){REM_PAREN cmd_arg_flags(tuple), nullptr};                            \
    bool _matched = false;                                                                       \
    for (int _j = 0; _f[_j]; ++_j) {                                                             \
      if (fptr_eq(arg, _f[_j])) {                                                                \
        _matched = true;                                                                         \
        break;                                                                                   \
      }                                                                                          \
    }                                                                                            \
    if (_matched) {                                                                              \
      cmd_arg_name(tuple) = _Generic(((cmd_arg_type(tuple)){}), char *: argsp[++i], bool: true); \
      continue;                                                                                  \
    }                                                                                            \
  }

#define cmd_arg_message_(type, name, flags, message, ...) message
#define cmd_arg_message(tuple) cmd_arg_message_ tuple

#define cmd_arg_name_str_(type, name, ...) #name
#define cmd_arg_name_str(tuple) cmd_arg_name_str_ tuple

#define cmd_arg_usage_print(tuple)                                    \
  {                                                                   \
    char *_f[] = (char *[]){REM_PAREN cmd_arg_flags(tuple), nullptr}; \
    fputs("\t", stdout);                                              \
    for (usize _j = 0; _f[_j]; ++_j) {                                \
      if (_j) fputs(",", stdout);                                     \
      fputs(_f[_j], stdout);                                          \
    }                                                                 \
    fputs(":\t", stdout);                                             \
    fputs(cmd_arg_message(tuple), stdout);                            \
    fputs("\n", stdout);                                              \
  }

#define cmd_define_usage(...)                     \
  void cmd_usage(char *prog_name) {               \
    println();                                    \
    println("Usage: {} [options]...", prog_name); \
    println("Options:");                          \
    APPLY_N(cmd_arg_usage_print, __VA_ARGS__)     \
    exit(1);                                      \
  }
#define cmd_main(nargs, args, ...)                                                        \
  int _cmd_main_inner(int _nargss, char **_argss, APPLY_N_C(cmd_arg_param, __VA_ARGS__)); \
  cmd_define_usage(__VA_ARGS__);                                                          \
  int main(int nargsp, char *argsp[]) {                                                   \
    char *args_new[nargsp] = {};                                                          \
    int nargs_new = 0;                                                                    \
    APPLY_N(cmd_arg_decl, __VA_ARGS__);                                                   \
    for (int i = 1; i < nargsp; ++i) {                                                    \
      char *arg = argsp[i];                                                               \
      APPLY_N(cmd_arg_parse, __VA_ARGS__)                                                 \
      args_new[nargs_new++] = arg;                                                        \
    }                                                                                     \
    char *ars[] = (char *[]){APPLY_N_C(TUPLE_GET_4, __VA_ARGS__)};                        \
    return _cmd_main_inner(nargs_new, args_new, APPLY_N_C(cmd_arg_name, __VA_ARGS__));    \
  }                                                                                       \
  int _cmd_main_inner(nargs, args, APPLY_N_C(cmd_arg_param, __VA_ARGS__))
