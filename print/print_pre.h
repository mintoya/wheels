#include <string.h>
#if !defined MY_PRINTER_DEFS_H
  #define MY_PRINTER_DEFS_H (1)
  #include "../allocator.h"
  #include "../fptr.h"
  #include "../mytypes.h"

// 1 : string
// 2 : context
// 3 : string length
// 4 : last print in a cluster
typedef fnptrof((const c8 *, void *, usize, bool), void) outputFunction;

typedef struct printerfunction_arg {
  fptr str;
  struct printerfunction_arg *next;
} printerfunction_arg;
typedef struct {
  outputFunction put;
  void *arb;
  printerfunction_arg args;
} printerfunction_context;

typedef struct {
  // 1 : fat value pointer
  // 2 : printing function
  // 3 : printing function context
  // 4 : args fat pointer
  fnptrof((fptr, const printerfunction_context), void) function;
  // size of what it prints, ignore if -1
  usize size;
} printerFunction;

typedef struct {
  struct {
    uint row, col;
  } pos;
  struct {
    uint8_t r, g, b;
  } fg;
  struct {
    uint8_t r, g, b;
  } bg;
  char poset : 1; // set position
  char bgset : 1; // enable bg
  char fgset : 1; // enable fg
  char clear : 1; // clear screen
  char reset : 1; // reset effects
} pEsc;
typedef pEsc printerEscape;

struct print_arg {
  fptr ref;
  fptr name;
};
fptr printer_arg_until(char delim, fptr string);
fptr printer_arg_after(char delim, fptr slice);
fptr printer_arg_after(char delim, fptr slice);
fptr printer_arg_trim(fptr in);
static inline printerfunction_context printerfunction_context_pop(const printerfunction_context ctx) {
  let x = ctx;
  x.args = x.args.next ? *x.args.next : (printerfunction_arg){};
  return x;
}
static inline fptr printerfunction_thisargs(const printerfunction_context _ctx) {
  return printer_arg_trim(_ctx.args.str);
}
typedef struct PrinterSingleton_t PrinterSingleton_t;
void PrinterSingleton_init();
void PrinterSingleton_deInit();
void PrinterSingleton_append(fptr name, printerFunction function);

void print_f_helper(struct print_arg p, fptr typeName, printerfunction_context ctx);

  #if !defined(NOFILEPRINTER)
static void fileprint(
    const c8 *c,
    void *fileHandle,
    usize length,
    bool flush
);
  #endif
  #define GETTYPEPRINTERFN(T) ID_CONCAT(_, ID_CONCAT(T, _printer))

  #define PUTS(characters) _ctx.put(characters, _ctx.arb, countof(characters) - 1, 0)
  #define PUTC(character) _ctx.put(REF(character), _ctx.arb, 1, 0)
  #define PRINTARGS() printerfunction_thisargs(_ctx)
  #define PRINTARGS_PUSH(arg)                                             \
    for (                                                                 \
        struct {                                                          \
          fptr oarg;                                                      \
          printerfunction_context octx;                                   \
          printerfunction_arg newarg;                                     \
          bool con;                                                       \
        } _pa_pu_st = {_ctx.args.str, _ctx, {fp(arg), &_ctx.args}, true}; \
        _pa_pu_st.con;                                                    \
        _pa_pu_st.con = false)                                            \
      for (                                                               \
          printerfunction_context _ctx = {                                \
              _pa_pu_st.octx.put,                                         \
              _pa_pu_st.octx.arb,                                         \
              {_pa_pu_st.oarg, &_pa_pu_st.newarg},                        \
          };                                                              \
          _pa_pu_st.con;                                                  \
          _pa_pu_st.con = false)

// outputFunction put;
// void *arb;
// printerfunction_arg args;

  #define typePrinter_name_inner(str, T, name)                        \
    static void ID_CONCAT(name, raw)(                                 \
        T, printerfunction_context                                    \
    );                                                                \
    static void name(                                                 \
        fptr _v_in_ptr, printerfunction_context _ctx                  \
    ) {                                                               \
      T in = *(T *)(_v_in_ptr.ptr);                                   \
      ID_CONCAT(name, raw)(in, _ctx);                                 \
    }                                                                 \
    __attribute__((constructor(202))) static void register_##name() { \
      fptr key = (fptr){                                              \
          .len = sizeof(str) - 1,                                     \
          .ptr = (uint8_t *)str,                                      \
      };                                                              \
      PrinterSingleton_append(                                        \
          key,                                                        \
          (printerFunction){                                          \
              name,                                                   \
              sizeof(T),                                              \
          }                                                           \
      );                                                              \
    }                                                                 \
    static void ID_CONCAT(name, raw)(                                 \
        T in, printerfunction_context _ctx                            \
    )

  #define typePrinter_name_function(str, T, fname) typePrinter_name_inner(str, T, fname)
  #define typePrinter_name(str, T) typePrinter_name_function(str, T, ID_CONCAT(PRINTERFN_, ID_CONCAT(__LINE__, ID_CONCAT(__, __COUNTER__))))
  #define typePrinter_type_inner(str, T, name) typePrinter_name_inner(str, T, name)
  #define typePrinter_type(T) typePrinter_type_inner(#T, T, GETTYPEPRINTERFN(T))

  #define typePrinter(a, ...)                              \
    REMOVE_PARENS(VA_SWITCH(                               \
        (typePrinter_type)__VA_OPT__(, (typePrinter_name)) \
    ))(a __VA_OPT__(, __VA_ARGS__))

  #define USETYPEPRINTER(T, val)                      \
    GETTYPEPRINTERFN(T)(                              \
        (fptr){sizeof(T), (u8 *)(void *)REF(T, val)}, \
        printerfunction_context_pop(_ctx)             \
    )

  #define USENAMEDPRINTER(strname, val)    \
    print_f_helper(                        \
        (struct print_arg){                \
            {sizeof(val), (u8 *)REF(val)}, \
        },                                 \
        fp(strname),                       \
        printerfunction_context_pop(_ctx)  \
    )

void print_f(outputFunction put, void *arb, const char *fmt, struct print_arg *);

  #define print_wfO(printerfn, arb, fmt, ...)          \
    do {                                               \
      print_f(                                         \
          printerfn,                                   \
          arb,                                         \
          fmt,                                         \
          (struct print_arg[]){                        \
              APPLY_N_C(MAKE_PRINT_ARG, __VA_ARGS__)   \
                  __VA_OPT__(, )((struct print_arg){}) \
          }                                            \
      );                                               \
    } while (0)
  #define tuprint_item(datatuple, printtuple)                       \
    GETTYPEPRINTERFN(TUPLE_EXPAND_FIRST(printtuple))(               \
        TUPLE_EXPAND_FIRST(datatuple),                              \
        (fptr){                                                     \
            sizeof(TUPLE_EXPAND_FIRST(printtuple)),                 \
            (u8 *)REF(                                              \
                TUPLE_EXPAND_FIRST(printtuple),                     \
                TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(printtuple))) \
            ),                                                      \
        },                                                          \
        fptr_CSP(VA_SWITCH(                                         \
            "",                                                     \
            TUPLE_EXPAND_REST((TUPLE_EXPAND_REST(printtuple)))      \
        )),                                                         \
        TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(datatuple)))          \
    );
  #define MAKE_FPTR_ITEM(printtuple)                              \
    (fptr) {                                                      \
      sizeof(TUPLE_EXPAND_FIRST(printtuple)),                     \
          (u8 *)REF(                                              \
              TUPLE_EXPAND_FIRST(printtuple),                     \
              TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(printtuple))) \
          ),                                                      \
    }

  #define TUPRINT_ITEM_PTR(datatuple, printtuple)              \
    GETTYPEPRINTERFN(TUPLE_EXPAND_FIRST(printtuple))(          \
        TUPLE_EXPAND_FIRST(datatuple),                         \
        *(_ptr++),                                             \
        fptr_CSP(VA_SWITCH(                                    \
            "",                                                \
            TUPLE_EXPAND_REST((TUPLE_EXPAND_REST(printtuple))) \
        )),                                                    \
        TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(datatuple)))     \
    );

  #define tupfmt(allocator, ...)                                          \
    ({                                                                    \
      fptr _fptrs[] = {                                                   \
          APPLY_N_C(MAKE_FPTR_ITEM, __VA_ARGS__)                          \
              __VA_OPT__(, )(fptr){}                                      \
      };                                                                  \
                                                                          \
      usize _req_len = 0;                                                 \
      fptr *_ptr = _fptrs;                                                \
      APPLY_N_WITH(TUPRINT_ITEM_PTR, (vsn_print, &_req_len), __VA_ARGS__) \
      fptr _res = {0, aCreate(allocator, u8, _req_len ?: 1)};             \
                                                                          \
      if (_req_len)                                                       \
        APPLY_N_WITH(TUPRINT_ITEM_PTR, (sn_print, &_res), __VA_ARGS__)    \
      _res;                                                               \
    })
  #define tuprint_wfo(printerfn, arb, ...)              \
    do {                                                \
      let fn = printerfn;                               \
      let fa = arb;                                     \
      APPLY_N_WITH(tuprint_item, (fn, fa), __VA_ARGS__) \
    } while (0)

  #define tuprint(...) tuprint_wfo(fileprint, stdout, __VA_ARGS__)
  #define print_wf(print, fmt, ...) print_wfO(print, NULL, fmt, __VA_ARGS__)
  #define print_(fmt, ...) print_wfO(fileprint, stdout, fmt, __VA_ARGS__)
  #define println_(fmt, ...) print(fmt "\n", __VA_ARGS__)
  #if !defined PRINT_NDEF
    // #define fprint(...) fprint_(__VA_ARGS__)
    // #define fprintln(...) fprintln_(__VA_ARGS__)
    #define print print_
    #define println println_
  #endif

  #if !defined(__cplusplus)

    #define MAKE_PRINT_ARG_TYPE(type) \
      type * : ((fptr){sizeof(#type) - 1, (u8 *)#type})
    #if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
      #define MAKE_PRINTINTS_SIZE MAKE_PRINT_ARG_TYPE(i32), MAKE_PRINT_ARG_TYPE(u32),
    #else
      #define MAKE_PRINTINTS_SIZE
    #endif
    #if __SIZEOF_DOUBLE__ != __SIZEOF_LONG_DOUBLE__
      #define MAKE_PRINTS_D MAKE_PRINT_ARG_TYPE(double), MAKE_PRINT_ARG_TYPE(long double),
    #else
      #define MAKE_PRINTS_D MAKE_PRINT_ARG_TYPE(ldouble),
    #endif

    #define MAKE_PRINT_ARG(a)                                                    \
      ((struct print_arg){                                                       \
          .ref = ((fptr){sizeof(a), (u8 *)REF(typeof(a), a)}),                   \
          .name = _Generic(                                                      \
              &(__typeof_unqual__(a)){0},                                        \
              MAKE_PRINT_ARG_TYPE(fptr),                                         \
              MAKE_PRINT_ARG_TYPE(isize),                                        \
              MAKE_PRINT_ARG_TYPE(usize),                                        \
              MAKE_PRINT_ARG_TYPE(float),                                        \
              MAKE_PRINTS_D /**/                                                 \
                  MAKE_PRINT_ARG_TYPE(pEsc),                                     \
              MAKE_PRINTINTS_SIZE /**/                                           \
              void **: ((fptr){sizeof("ptr") - 1, (u8 *)"ptr"}),                 \
              slice(c8) *: ((fptr){sizeof("slice(c8)") - 1, (u8 *)"slice(c8)"}), \
              char **: ((fptr){sizeof("cstr") - 1, (u8 *)"cstr"}),               \
              char (*)[sizeof(a)]: ((fptr){sizeof("carr") - 1, (u8 *)"carr"}),   \
              default: nullFptr                                                  \
          ),                                                                     \
      })

  #else
template <typename T>
constexpr const char *type_name_cstr() { return ""; }

    #define MAKE_PRINT_ARG_TYPE(type) \
      template <>                     \
      constexpr const char *type_name_cstr<type>(void) { return #type; }

MAKE_PRINT_ARG_TYPE(fptr);
MAKE_PRINT_ARG_TYPE(slice(c8));
MAKE_PRINT_ARG_TYPE(isize);
MAKE_PRINT_ARG_TYPE(usize);
MAKE_PRINT_ARG_TYPE(float);
MAKE_PRINT_ARG_TYPE(double);
MAKE_PRINT_ARG_TYPE(ldouble);
MAKE_PRINT_ARG_TYPE(pEsc);
    #if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
MAKE_PRINT_ARG_TYPE(i32);
MAKE_PRINT_ARG_TYPE(u32);
    #endif

    #define MAKE_PRINT_ARG(a)                                 \
      ((struct print_arg){                                    \
          .ref = (fptr){sizeof(typeof(a)), (u8 *)REF(a)},     \
          .name = fp_from(type_name_cstr<typeof_unqual(a)>()) \
      })
  #endif
#endif
