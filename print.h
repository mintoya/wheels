#if !defined MY_PRINTER_DEFS_H
  #define MY_PRINTER_DEFS_H (1)
  #include "fptr.h"
  #include "mytypes.h"

// 1 : string
// 2 : context
// 3 : string length
// 4 : last print in a cluster
typedef fnptrof((const c8 *, void *, usize, bool), void) outputFunction;

  #define PRINTERFUNCTION_CONTEXT outputFunction put, void *_arb, fptr args
typedef struct {
  // 1 : fat value pointer
  // 2 : printing function
  // 3 : printing function context
  // 4 : args fat pointer
  void (*function)(fptr, PRINTERFUNCTION_CONTEXT);
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
  unsigned char poset : 1; // set position
  unsigned char bgset : 1; // enable bg
  unsigned char fgset : 1; // enable fg
  unsigned char clear : 1; // clear screen
  unsigned char reset : 1; // reset effects
} pEsc;
struct print_arg {
  fptr ref;
  fptr name;
};
fptr printer_arg_until(char delim, fptr string);
fptr printer_arg_after(char delim, fptr slice);
fptr printer_arg_after(char delim, fptr slice);
fptr printer_arg_trim(fptr in);
static inline fptr printer_arg_pop(char delim, fptr *in) {
  defer { *in = printer_arg_trim(printer_arg_after(delim, *in)); };
  return printer_arg_trim(printer_arg_until(delim, *in));
}
typedef struct PrinterSingleton_t PrinterSingleton_t;
void PrinterSingleton_init();
void PrinterSingleton_deInit();
void PrinterSingleton_append(fptr name, printerFunction function);

void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *arb);

  #if !defined(NOFILEPRINTER)
static void fileprint(
    const c8 *c,
    void *fileHandle,
    usize length,
    bool flush
);
  #endif
  #define GETTYPEPRINTERFN(T) ID_CONCAT(_, ID_CONCAT(T, _printer))

  #define PUTS(characters) put(characters, _arb, countof(characters) - 1, 0)
  #define PUTC(character) put(REF(character), _arb, 1, 0)

  #define typePrinter_name_inner(str, T, name)                        \
    static void ID_CONCAT(name, raw)(                                 \
        outputFunction put, fptr args, void *_arb, T in               \
    );                                                                \
    static void name(                                                 \
        fptr _v_in_ptr, PRINTERFUNCTION_CONTEXT                       \
    ) {                                                               \
      (void)args;                                                     \
      T in = *(T *)(_v_in_ptr.ptr);                                   \
      ID_CONCAT(name, raw)(put, args, _arb, in);                      \
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
        outputFunction put, fptr args, void *_arb, T in               \
    )

  #define typePrinter_name_function(str, T, fname) typePrinter_name_inner(str, T, fname)
  #define typePrinter_name(str, T) typePrinter_name_function(str, T, ID_CONCAT(PRINTERFN_, ID_CONCAT(__LINE__, ID_CONCAT(__, __COUNTER__))))
  #define typePrinter_type_inner(str, T, name) typePrinter_name_inner(str, T, name)
  #define typePrinter_type(T) typePrinter_type_inner(#T, T, GETTYPEPRINTERFN(T))

  #define typePrinter(a, ...)                              \
    REMOVE_PARENS(VA_SWITCH(                               \
        (typePrinter_type)__VA_OPT__(, (typePrinter_name)) \
    ))(a __VA_OPT__(, __VA_ARGS__))

  #define USETYPEPRINTER(T, val) \
    GETTYPEPRINTERFN(T)((fptr){sizeof(T), (u8 *)(void *)REF(T, val)}, put, _arb, args)

  #define USENAMEDPRINTER(strname, val)                                                     \
    print_f_helper(                                                                         \
        (struct print_arg){.ref = ((fptr){sizeof(val), (u8 *)REF(val)}), .name = nullFptr}, \
        printer_arg_trim(printer_arg_until(':', fp_from(strname))),                         \
        put,                                                                                \
        nullFptr,                                                                           \
        _arb                                                                                \
    );
  #define USENAMEDPRINTER_WA(strname, args, val)                                                         \
    print_f_helper(                                                                                      \
        (struct print_arg){.ref = ((fptr){sizeof(val), (u8 *)REF(typeof(val), val)}), .name = nullFptr}, \
        printer_arg_trim(printer_arg_until(':', fp_from(strname))),                                      \
        put,                                                                                             \
        args,                                                                                            \
        _arb                                                                                             \
    );

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
#if !defined MY_PRINTER_H
  #define MY_PRINTER_H (1)
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "sList.h"
  #include "smap.h"
  #include <locale.h>
  #include <stdio.h>
  #include <string.h>

// helper escape type

typedef pEsc printerEscape;
  #define pEscRst      \
    ((pEsc){           \
        .pos = {0, 0}, \
        .clear = 1,    \
        .reset = 1,    \
    })

  #if !defined(NOFILEPRINTER)
static void fileprint(
    const c8 *c,
    void *fileHandle,
    usize length,
    bool flush
) {
  let static buf = msList_stackBuffer(c8[1 << 9]);
  let static l = (msList(c8)) nullptr;
  l = l ?: msList_initBuffer(buf);

  FILE *file = (FILE *)fileHandle;
  if (flush || msList_len(l) + length >= msList_cap(l)) {
    fwrite(l, sizeof(*l), msList_len(l), file);
    msList_clear(l);
    fwrite(c, sizeof(*c), length, file);
  } else msList_pushArr(nullptr, l, *VLAP(c, length));
}
  #endif

static void vsn_print(
    const c8 *_,
    void *lptr,
    usize length,
    bool __
) {
  ((usize *)lptr)[0] += length;
}
static void sn_print(
    const c8 *c,
    void *cptr,
    usize length,
    bool ___
) {
  slice(c8) *loc = (typeof(loc))cptr;
  assertMessage(loc && loc->ptr);
  if (length)
    memcpy(loc->ptr + loc->len, c, length);
  loc->len += length;
}

  #define headconfig printermap, fptr, printerFunction, (fptr_hash(k)), (fptr_cmp(a, b))
  #define mapconfig headconfig

  #pragma push_macro("headconfig")
  #include "incmap.h"
  #pragma pop_macro("headconfig")

typedef struct PrinterSingleton_t {
  printermap data[1];
} PrinterSingleton_t;
extern PrinterSingleton_t PrinterSingleton;

printerFunction PrinterSingleton_get(fptr name);

// arg utils

unsigned int printer_arg_indexOf(fptr string, char c);
fptr printer_arg_until(char delim, fptr string);
fptr printer_arg_after(char delim, fptr slice);
fptr printer_arg_trim(fptr in);

  #ifdef _WIN32
    #include <windows.h>
__attribute__((constructor(201))) static void printerInit() {
  setlocale(LC_ALL, ".UTF-8");
  SetConsoleOutputCP(CP_UTF8);
  PrinterSingleton_init();
}
  #else
__attribute__((constructor(201))) static void printerInit() {
  setlocale(LC_ALL, "");
  PrinterSingleton_init();
}
  #endif
__attribute__((destructor(201))) static void printerDeInit() { PrinterSingleton_deInit(); }

typePrinter("ptr", void *) {
  uintptr_t v = (uintptr_t)in;
  if (!v) return PUTS("(nil)");
  PUTS("0x");

  int shift = (sizeof(uintptr_t) * 8) - 4;
  int leading = 1;
  while (shift >= 0) {
    unsigned char nibble = (v >> shift) & 0xF;
    if (nibble || !leading || shift == 0) {
      leading = 0;
      char c = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
      PUTC(c);
    }
    shift -= 4;
  }
}
typePrinter("slice(c8)", slice(c8)) {
  foreach (c8 *c, span(in.ptr, in.len))
    PUTC(*c);
}
typePrinter(c8) { PUTC(in); }
typePrinter(cstr) {
  in = in ?: (char *)"__NULLCSTR__";
  while (*in)
    PUTC(*in++);
}

static void GETTYPEPRINTERFN(carr)(fptr _v_in_ptr, PRINTERFUNCTION_CONTEXT) {
  PUTS(*VLAP((char *)_v_in_ptr.ptr, _v_in_ptr.len));
}
__attribute__((constructor(203))) static void printerConstructor_carr() {
  PrinterSingleton_append(fp("carr"), (printerFunction){GETTYPEPRINTERFN(carr), ~(usize)0});
}

typePrinter(c32) {
  if (in <= 0x7F)
    PUTC((c8)in);
  else if (in <= 0x7FF) {
    PUTC((c8)(0xC0 | (in >> 6)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  } else if (in <= 0xFFFF) {
    PUTC((c8)(0xE0 | (in >> 12)));
    PUTC((c8)(0x80 | ((in >> 6) & 0x3F)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  } else {
    PUTC((c8)(0xF0 | (in >> 18)));
    PUTC((c8)(0x80 | ((in >> 12) & 0x3F)));
    PUTC((c8)(0x80 | ((in >> 6) & 0x3F)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  }
}

typePrinter("c32str", c32 *) {
  if (in)
    while (*in)
      USETYPEPRINTER(c32, *in++);
  else
    PUTS("__NULLCSTR__");
}
// {int printers
typePrinter(u64) {
  c8 digits[sizeof(u64) * 8 / 3];
  u8 digit = 0;
  usize l = 1;
  while (l <= in / 10) {
    if (l * 10 < l) break;
    else l = l * 10;
  }
  while (l) {
    char c = in / l + '0';
    digits[digit++] = c;
    in %= l;
    l /= 10;
  }
  put(digits, _arb, digit, 0);
}
typePrinter(i64) {
  usize uin = (usize)in;
  if (in < 0) {
    PUTC((c8)'-');
    uin = 0 - in;
  }
  USETYPEPRINTER(u64, uin);
}
typePrinter(usize) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(isize) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(u8) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(u16) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(u32) { USETYPEPRINTER(u64, (usize)in); }
typePrinter(i8) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(i16) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(i32) { USETYPEPRINTER(i64, (isize)in); }
// }
typePrinter(f128) {
  usize digits = 0;
  if ((args = printer_arg_trim(args)).len)
    for (var_ i = 0; i < args.len && (args.ptr[i] <= '9' && args.ptr[i] >= '0'); i++) {
      digits *= 10;
      digits += args.ptr[i] - '0';
    }

  digits = digits ?: 3;
  in = in < 0 ? (({ PUTC((c8)'-'); }), -in) : in;
  f128 u = in;

  f128 round = 0.5;
  for (usize i = 0; i < digits; i++)
    round /= 10.0;
  u += round;

  f128 tens = 1;
  while (u / tens >= 10)
    tens *= 10;

  while (tens >= 1) {
    int d = (int)(u / tens);
    PUTC((c8)(d + '0'));
    u -= (f128)d * tens;
    tens /= 10;
  }

  if (digits > 0) {
    PUTC((c8)'.');
    for (usize i = 0; i < digits; i++) {
      u *= 10;
      int d = (int)u;
      PUTC((c8)(d + '0'));
      u -= (f128)d;
    }
  }
}
typePrinter(float) { USETYPEPRINTER(f128, (f128)in); }
typePrinter(double) { USETYPEPRINTER(f128, (f128)in); }
typePrinter(ldouble) { USETYPEPRINTER(f128, (f128)in); }
typePrinter(fptr) {
  const c8 hex_chars[17] = "0123456789abcdef";
  char cut0s = 0;
  char useLength = 0;
  if (fptr_eq(fp_from("length"), printer_arg_trim(args)))
    useLength = 1;
  if (useLength) {
    PUTS("<");
    USETYPEPRINTER(u64, in.len);
  }
  PUTS("<");

  foreach (usize i, range(in.len, 0)) {
    u8 top = (in.ptr[i - 1] & 0xF0) >> 4;
    u8 bottom = in.ptr[i - 1] & 0x0F;
    PUTC(hex_chars[top]);
    PUTC(hex_chars[bottom]);
  }
  PUTS(">");
  if (useLength)
    PUTS(">");
}
typePrinter(pEsc) {
  if (in.poset) {

    PUTS("\033["); // ]
    USETYPEPRINTER(usize, in.pos.row);
    PUTS(";");
    USETYPEPRINTER(usize, in.pos.col);
    PUTS("H");
  }
  if (in.fgset) {
    PUTS("\033[38;2;"); // ]
    USETYPEPRINTER(usize, in.fg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.b);
    PUTS("m");
  }

  if (in.bgset) {
    PUTS("\033[48;2;"); // ]
    USETYPEPRINTER(usize, in.bg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.b);
    PUTS("m");
  }
  if (in.clear) {
    PUTS("\033[2J"); // ]
    PUTS("\033[H");  // ]
  }
  if (in.reset) {
    PUTS("\033[0m"); // ]
  }
}

typePrinter("x", u8) {
  const c8 hex_chars[17] = "0123456789abcdef";
  PUTC((c8)(hex_chars[in >> 4 & 0xf]));
  PUTC((c8)(hex_chars[in & 0xf]));
}

struct slice_any_t {
  usize len;
  void *ptr;
};

// * : * : int can print a ptr to a ptr to an int
typePrinter("*", void *) { // least safe printer of all time
  var_ typef = printer_arg_trim(printer_arg_until(':', args));
  args = printer_arg_after(':', args);
  PUTS("[*]");
  if (!in) PUTS("null");
  else {
    var_ np = PrinterSingleton_get(typef);
    if (np.function)
      np.function((fptr){np.size, (u8 *)in}, put, _arb, args);
    else {
      PUTS("UNKNOWN PRINTER ");
      USENAMEDPRINTER("slice(c8)", typef)
    }
  }
}
typePrinter("slice", struct slice_any_t) {
  fptr farg = printer_arg_trim(args);
  void *ptr = in.ptr;
  var_ printer = PrinterSingleton_get(farg);
  if (!printer.function) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {.r = 255}, .fgset = 1}));
    PUTS("__could'nt find printer for ");
    USENAMEDPRINTER("slice(c8)", farg);
    PUTS("__");
    USETYPEPRINTER(pEsc, (pEsc){.reset = 1});
  } else {
    PUTC((c8)'[');
    usize size = printer.size;
    foreach (usize i, range(0, in.len)) {
      if (i)
        PUTC((c8)',');
      printer.function((fptr){size, size * i + (u8 *)ptr}, put, _arb, nullFptr);
    }
    PUTC((c8)']');
  }
}
typePrinter("msList", void *) {
  fptr farg = printer_arg_trim(args);
  var_ printer = PrinterSingleton_get(
      printer_arg_trim(
          printer_arg_until(':', farg)
      )
  );
  if (!printer.function) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {.r = 255}, .fgset = 1}));
    PUTS("__could'nt find printer for ");
    USENAMEDPRINTER("slice(c8)", printer_arg_until(':', farg));
    PUTS("__");
    USETYPEPRINTER(pEsc, (pEsc){.reset = 1});
  } else {
    PUTC((c8)'[');
    usize size = printer.size;
    foreach (usize i, range(0, msList_len(in))) {
      if (i)
        PUTC((c8)',');
      printer.function(
          (fptr){size, size * i + (u8 *)in},
          put,
          _arb,
          printer_arg_after(':', farg)
      );
    }
    PUTC((c8)']');
  }
}
typePrinter("mxmap", hxmap *) {
  args = printer_arg_trim(args);
  var_ kvs = printer_arg_until(':', args);
  args = printer_arg_after(':', args);
  var_ kprinter = P$(
      printer_arg_until(',', kvs),
      printer_arg_trim($),
      PrinterSingleton_get($)
  );
  var_ vprinter = P$(
      printer_arg_after(',', kvs),
      printer_arg_trim($),
      PrinterSingleton_get($)
  );
  if (!(kprinter.function && vprinter.function)) {
    PUTS("__could'nt find printer for ");
    USENAMEDPRINTER("slice(c8)", kvs);
    PUTS("__");
  } else if (!EQUAL_ANY(kprinter.size, ~(usize)0, in->ksize) || !EQUAL_ANY(vprinter.size, ~(usize)0, in->vsize)) {
    PUTS("__size for ");
    USENAMEDPRINTER("slice(c8)", kvs);
    PUTS(" doesn't match map");
    PUTS("__");
  } else {
    PUTS("{");

    bool comma = false;
    foreach (var_ sp, hxmap_iter(in)) {
      if (comma) PUTS(",");
      comma = true;
      kprinter.function((fptr){in->ksize, (u8 *)sp.key}, put, _arb, args);
      PUTS(":");
      vprinter.function((fptr){in->vsize, (u8 *)sp.val}, put, _arb, args);
    }
    PUTS("}");
  }
}

volatile static thread_local bool print_f_shouldFlush = 1;

static slice(c8) vsn_print_fn(AllocatorV allocator, char *fmt, struct print_arg *args) {
  usize sn_length_ = 0;
  print_f(
      vsn_print,
      &sn_length_,
      fmt,
      args
  );
  var_ sn_slice_result = slice_alloc(allocator, c8, sn_length_);
  sn_slice_result.len = 0;
  print_f(
      sn_print,
      &sn_slice_result,
      fmt,
      args
  );
  assertMessage(sn_slice_result.len == sn_length_);
  return sn_slice_result;
}
  #define snprint(allocator, fmt, ...) ({            \
    vsn_print_fn(                                    \
        allocator,                                   \
        (char *)fmt,                                 \
        (struct print_arg[]){                        \
            APPLY_N_C(MAKE_PRINT_ARG, __VA_ARGS__)   \
                __VA_OPT__(, )((struct print_arg){}) \
        }                                            \
    );                                               \
  })
  #ifdef PRINTER_LIST_TYPENAMES
__attribute__((constructor(205))) static void printer_post_initfn() {
  print("==============================\n"
        "printer debug\n"
        "==============================\n");
  println("list of printer type names: ");
  foreach (var_ i, msxmap_iter(PrinterSingleton.data))
    println("{slice(c8)}", i.key);
  // println(
  //     "buckets   : {}\n"
  //     "footprint : {}\n"
  //     "collisions: {}\n"
  //     "==============================\n",
  //     ((sHmap *)PrinterSingleton.data)->num_buckets,
  //     sHmap_footprint((sHmap *)PrinterSingleton.data),
  //     sHmap_countCollisions((sHmap *)PrinterSingleton.data),
  // );
}
  #endif // PRINTER_LIST_TYPENAMES
  #undef MY_PRINTER_H
  #define MY_PRINTER_H (2)
#endif // MY_PRINTER_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_PRINTER_C (1)
#endif

#if defined MY_PRINTER_C && MY_PRINTER_C == 1 && MY_PRINTER_H == 2
PrinterSingleton_t PrinterSingleton = {};
void PrinterSingleton_init() { printermap_newm(stdAlloc, 3, PrinterSingleton.data); }
void PrinterSingleton_deInit() { printermap_freem(PrinterSingleton.data[0]); }
void PrinterSingleton_append(fptr name, printerFunction function) {
  printermap_set(PrinterSingleton.data, name, function);
}
printerFunction PrinterSingleton_get(fptr name) {
  static thread_local printerFunction lastprinters[2] = {};
  static thread_local fptr lastnames[2] = {};
  static thread_local u8 lasttick = 0;

  if (fptr_eq(name, lastnames[lasttick]))
    return lastprinters[lasttick];
  else if (fptr_eq(name, lastnames[!lasttick]))
    return lastprinters[!lasttick];

  lasttick = !lasttick;

  if_decl (var_ val, printermap_get(PrinterSingleton.data, name)) {
    mcpy(lastprinters[lasttick], *val);
    lastnames[lasttick] = *printermap_val_key(PrinterSingleton.data, val);
    return *val;
  }
  return (printerFunction){};
}

fptr printer_arg_until(char delim, fptr string) {
  usize i = 0;
  u8 *ptr = (u8 *)string.ptr;
  while (i < string.len && ptr[i] != delim)
    i++;
  string.len = i;
  return string;
}
fptr printer_arg_after(char delim, fptr slice) {
  usize i = 0;
  uint8_t *ptr = slice.ptr;
  while (i < slice.len && ptr[i] != delim)
    i++;
  i = (i < slice.len) ? (i + 1) : (i);
  slice.ptr += i;
  slice.len -= i;
  return slice;
}
fptr printer_arg_trim(fptr in) {
  while (
      in.len &&
      in.ptr[0] <= ' ') {
    in.ptr++;
    in.len--;
  }
  while (
      in.len &&
      in.ptr[in.len - 1] <= ' ')
    in.len--;
  return in;
}

void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *_arb) {
  if (!typeName.len) {
    typeName = p.name;
  }
  printerFunction fn = PrinterSingleton_get(typeName);
  if (!fn.function) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = 1}));
    PUTS("__ NO_TYPE(");
    if (typeName.len)
      foreach (var_ i, span(typeName.ptr, typeName.len))
        PUTC((c8)i[0]);
    PUTS(") __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else if (p.ref.len != ~(usize)0 && fn.size != p.ref.len) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = 1}));
    PUTS("__ PRINTER TRIED TO READ ");
    USETYPEPRINTER(usize, fn.size);
    PUTS(" BUT ITEM HAS ");
    USETYPEPRINTER(usize, p.ref.len);
    PUTS(" BYTES __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else {
    fn.function(p.ref, put, _arb, args);
  }
}

void print_f(outputFunction put, void *arb, const char *fmt, struct print_arg *args) {
  bool toggled = 0;
  for (u32 i = 0; fmt[i]; i++) {
    if (fmt[i] == '{' && !toggled) {
      u32 j = i + 1;
      while (fmt[j] && fmt[j] != '}')
        j++;
      fptr typeName = {
          .len = j - i - 1,
          .ptr = ((u8 *)fmt) + i + 1,
      };
      var_ assumedName = *args++;

      fptr tname = printer_arg_until(':', typeName);
      fptr parseargs = printer_arg_after(':', typeName);
      tname = printer_arg_trim(tname);
      if (!assumedName.ref.ptr) {
        return put("__ NO ARGUMENT PROVIDED, ENDING PRINT __\n", arb, 41, 1);
      }
      print_f_helper(assumedName, tname, put, parseargs, arb);
      i = j;
      toggled = 0;

    } else if (fmt[i] == '/') {
      toggled = !toggled;
      if (!toggled)
        put("/", arb, 1, 0);
    } else {
      toggled = 0;
      put(fmt + i, arb, 1, 0);
    }
  }
  put("\0", arb, 1, 0);
  if (print_f_shouldFlush)
    put(NULL, arb, 0, 1);
}
  #undef MY_PRINTER_C
  #define MY_PRINTER_C (2)
#endif
