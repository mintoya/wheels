#ifndef MY_PRINTER_H
#define MY_PRINTER_H
// typedef long double max_align_t;
#include "allocator.h"
#include "assertMessage.h"
#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
#include "shmap.h"
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef void (*outputFunction)(
    const c8 *,
    void *,
    usize,
    bool
);
typedef struct {
  void (*function)(
      outputFunction,
      fptr,
      fptr args,
      void *
  );
  usize size;
} printerFunction;
// helper escape type
typedef unsigned int uint;
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
  FILE *file = (FILE *)fileHandle;
  static thread_local struct {
    c8 buf[1 << 9];
    usize place;
  } buffer = {0};
  if (flush || (buffer.place + length) > countof(buffer.buf)) {
    fwrite(buffer.buf, sizeof(c8), buffer.place, file);
    fwrite(c, sizeof(c8), length, file);
    buffer.place = 0;
  } else {
    memcpy(buffer.buf + buffer.place, c, length * sizeof(c8));
    buffer.place += length;
  }
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
  memcpy(loc->ptr + loc->len, c, length);
  loc->len += length;
}

static struct {
  msHmap(printerFunction) data;
} PrinterSingleton;

static void PrinterSingleton_init() {
  PrinterSingleton.data = msHmap_init(stdAlloc, printerFunction, 32);
}
static void PrinterSingleton_append(fptr name, printerFunction function) {
  msHmap_set(PrinterSingleton.data, name, function);
}

static printerFunction PrinterSingleton_get(fptr name) {
  static thread_local printerFunction lastprinters[2] = {{}, {}};
  static thread_local fptr lastnames[2] = {nullFptr, nullFptr};
  static thread_local u8 lasttick = 0;

  if (!fptr_cmp(name, lastnames[lasttick])) {
    return lastprinters[lasttick];
  } else if (!fptr_cmp(name, lastnames[!lasttick])) {
    return lastprinters[!lasttick];
  }
  lasttick = !lasttick;

  var_ val = sHmap_find((sHmap *)PrinterSingleton.data, name);
  if (val) {
    var_ list = (printerFunction *)(((sHmap *)PrinterSingleton.data)->values)->buf;
    lastprinters[lasttick] = list[val[0].vidx];
    lastnames[lasttick] =
        stringList_get(
            ((sHmap *)PrinterSingleton.data)->strings,
            val[0].kidx
        );
    return list[val[0].vidx];
  }
  return (printerFunction){};
}

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

#define GETTYPEPRINTERFN(T) _##T##_printer
#define MERGE_PRINTER_M(a, b) a##b
#define LABEL_PRINTER_GEN(l, a) MERGE_PRINTER_M(l, a)

#define UNIQUE_GEN_PRINTER \
  LABEL_PRINTER_GEN(LABEL_PRINTER_GEN(__LINE__, _), __COUNTER__)
#define UNIQUE_PRINTER_FN LABEL_PRINTER_GEN(PRINTERFN, UNIQUE_GEN_PRINTER)
#define UNIQUE_PRINTER_FN2 \
  LABEL_PRINTER_GEN(printerConstructor, UNIQUE_GEN_PRINTER)

#define PUTS(characters) put(characters, _arb, countof(characters) - 1, 0)
#define PUTC(character)                               \
  do {                                                \
    ASSERT_EXPR(types_eq(c8, typeof(character)), ""); \
    put(REF(c8, character), _arb, 1, 0);              \
  } while (0)

#define REGISTER_PRINTER(T, ...)                                 \
  static void GETTYPEPRINTERFN(T)(                               \
      outputFunction put, fptr _v_in_ptr, fptr args, void *_arb  \
  ) {                                                            \
    (void)args;                                                  \
    T in = *(T *)(_v_in_ptr.ptr);                                \
    __VA_ARGS__;                                                 \
  }                                                              \
  __attribute__((constructor(202))) static void register_##T() { \
    fptr key = (fptr){                                           \
        .len = sizeof(#T) - 1,                                   \
        .ptr = (uint8_t *)#T,                                    \
    };                                                           \
    PrinterSingleton_append(                                     \
        key,                                                     \
        (printerFunction){                                       \
            GETTYPEPRINTERFN(T),                                 \
            sizeof(T),                                           \
        }                                                        \
    );                                                           \
  }

#define REGISTER_SPECIAL_PRINTER_NEEDID(id, str, type, ...)                   \
  static void id(outputFunction put, fptr _v_in_ptr, fptr args, void *_arb) { \
    type in = *(typeof(in) *)(_v_in_ptr.ptr);                                 \
    __VA_ARGS__;                                                              \
  }                                                                           \
  __attribute__((constructor(203))) static void UNIQUE_PRINTER_FN2() {        \
    fptr key = (fptr){                                                        \
        .len = strlen(str),                                                   \
        .ptr = (uint8_t *)str,                                                \
    };                                                                        \
    PrinterSingleton_append(                                                  \
        key,                                                                  \
        (printerFunction){                                                    \
            id,                                                               \
            sizeof(type),                                                     \
        }                                                                     \
    );                                                                        \
  }

#define REGISTER_SPECIAL_PRINTER(str, type, ...) \
  REGISTER_SPECIAL_PRINTER_NEEDID(UNIQUE_PRINTER_FN, str, type, __VA_ARGS__)
#define USETYPEPRINTER(T, val) \
  GETTYPEPRINTERFN(T)(put, (fptr){sizeof(T), (u8 *)(void *)REF(T, val)}, nullFptr, _arb)
#define USENAMEDPRINTER(strname, val)                                                                  \
  print_f_helper(                                                                                      \
      (struct print_arg){.ref = ((fptr){sizeof(val), (u8 *)REF(typeof(val), val)}), .name = nullFptr}, \
      printer_arg_trim(printer_arg_until(':', fp_from(strname))), put,                                 \
      printer_arg_after(':', fp_from(strname)), _arb                                                   \
  );
#define USENAMEDPRINTER_WA(strname, args, val)                                                         \
  print_f_helper(                                                                                      \
      (struct print_arg){.ref = ((fptr){sizeof(val), (u8 *)REF(typeof(val), val)}), .name = nullFptr}, \
      printer_arg_trim(printer_arg_until(':', fp_from(strname))), put,                                 \
      args, _arb                                                                                       \
  );

struct print_arg {
  fptr ref;
  fptr name;
};
void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *arb);

// examples with builtin types
// the behavior of PUTS is modular
//
// for building printers
// USENAMEDPRINTER(printerid,value)
// USETYPEPRINTER(type,value)
//
// typeprinter skips the search
// named printer skips the string parsing
// use print_wf with "put" as the ouputFunction
// to keep output consistant, but that makes it recursive
//
// you can pass args with a printerid and a colon
// ex: "fptr<void>: c0 length"
//

REGISTER_SPECIAL_PRINTER_NEEDID(_void_ptr_printerfn, "ptr", void *, {
  uintptr_t v = (uintptr_t)in;
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
});
REGISTER_SPECIAL_PRINTER_NEEDID(_slice_c8_printerfn, "slice(c8)", slice(c8), {
  for_each_((c8 c, slice_vla(in)), PUTC(c););
});

REGISTER_PRINTER(c8, { PUTC(in); });
REGISTER_SPECIAL_PRINTER("cstr", char *, {
  if (!in)
    in = "__NULLCSTR__";
  while (*in)
    PUTC(*in++);
});
REGISTER_SPECIAL_PRINTER("carr", char, {
  // string literals will end up here
  // this will stop a seg fault, since they are passed in themselves instead of the type
  PUTS(*VLAP((char *)_v_in_ptr.ptr, _v_in_ptr.len));
});
REGISTER_PRINTER(c32, {
  if (in <= 0x7F) {
    PUTC((c8)in);
  } else if (in <= 0x7FF) {
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
});
REGISTER_SPECIAL_PRINTER("c32str", c32 *, {
  if (in)
    while (*in)
      USETYPEPRINTER(c32, *in++);
  else
    PUTS("__NULLCSTR__");
});
REGISTER_PRINTER(usize, {
  c8 digits[sizeof(usize) * 8 / 3];
  u8 digit = 0;
  usize l = 1;
  while (l <= in / 10) {
    if (l * 10 < l)
      break;
    else
      l = l * 10;
  }
  while (l) {
    char c = in / l + '0';
    digits[digit++] = c;
    in %= l;
    l /= 10;
  }
  put(digits, _arb, digit, 0);
});
REGISTER_PRINTER(isize, {
  usize uin = (usize)in;
  if (in < 0) {
    PUTC((c8)'-');
    uin = 0 - in;
  }
  USETYPEPRINTER(usize, uin);
});

REGISTER_PRINTER(f128, {
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
  for (usize i = 0; i < digits; i++) {
    round /= 10.0;
  }
  u += round;

  f128 tens = 1;
  while (u / tens >= 10) {
    tens *= 10;
  }

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
});
REGISTER_PRINTER(float, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg){.ref = (fptr){sizeof(f128), (u8 *)&r}}, fp("f128"), put, args, _arb);
});
REGISTER_PRINTER(double, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg){.ref = (fptr){sizeof(f128), (u8 *)&r}}, fp("f128"), put, args, _arb);
});
REGISTER_PRINTER(ldouble, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg){.ref = (fptr){sizeof(f128), (u8 *)&r}}, fp("f128"), put, args, _arb);
});
REGISTER_PRINTER(u32, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_PRINTER(i32, {
  USETYPEPRINTER(isize, (isize)in);
});
REGISTER_PRINTER(fptr, {
  const c32 hex_chars[17] = U"0123456789abcdef";
  char cut0s = 0;
  char useLength = 0;
  if (fptr_eq(fp_from("length"), printer_arg_trim(args)))
    useLength = 1;
  if (useLength) {
    PUTS("<");
    USETYPEPRINTER(usize, in.len);
  }
  PUTS("<");
  usize start = 0;
  if (cut0s) {
    for (usize i = 0; i < in.len; i++) {
      if (in.ptr[i] != 0) {
        start = i;
        break;
      }
    }
  }

  for (usize i = start; i < in.len; i++) {
    u8 top = (in.ptr[i] & 0xF0) >> 4;
    u8 bottom = in.ptr[i] & 0x0F;
    PUTC((c8)(hex_chars[top]));
    PUTC((c8)(hex_chars[bottom]));
  }
  PUTS(">");
  if (useLength)
    PUTS(">");
});
REGISTER_PRINTER(pEsc, {
  if (in.poset) {

    PUTS("\033[");
    USETYPEPRINTER(usize, in.pos.row);
    PUTS(";");
    USETYPEPRINTER(usize, in.pos.col);
    PUTS("H");
  }
  if (in.fgset) {
    PUTS("\033[38;2;");
    USETYPEPRINTER(usize, in.fg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.b);
    PUTS("m");
  }

  if (in.bgset) {
    PUTS("\033[48;2;");
    USETYPEPRINTER(usize, in.bg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.b);
    PUTS("m");
  }
  if (in.clear) {
    PUTS("\033[2J");
    PUTS("\033[H");
  }
  if (in.reset) {
    PUTS("\033[0m");
  }
});

REGISTER_SPECIAL_PRINTER("x", u8, {
  const c8 hex_chars[17] = "0123456789abcdef";
  PUTC((c8)(hex_chars[in >> 4 & 0xf]));
  PUTC((c8)(hex_chars[in & 0xf]));
});
REGISTER_SPECIAL_PRINTER("u8", u8, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_SPECIAL_PRINTER("u16", u16, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_SPECIAL_PRINTER("u32", u32, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_SPECIAL_PRINTER("u64", u64, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_SPECIAL_PRINTER("i8", i8, {
  USETYPEPRINTER(isize, (isize)in);
});
REGISTER_SPECIAL_PRINTER("i16", i16, {
  USETYPEPRINTER(isize, (isize)in);
});
REGISTER_SPECIAL_PRINTER("i32", i32, {
  USETYPEPRINTER(isize, (isize)in);
});
REGISTER_SPECIAL_PRINTER("i64", i64, {
  USETYPEPRINTER(isize, (isize)in);
});
struct slice_any_t {
  usize len;
  void *ptr;
};
REGISTER_SPECIAL_PRINTER_NEEDID(slice_printer_generic_version, "slice", struct slice_any_t, {
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
    for (each_RANGE(usize, i, 0, in.len)) {
      if (i)
        PUTC((c8)',');
      printer.function(put, (fptr){size, size * i + (u8 *)ptr}, nullFptr, _arb);
    }
    PUTC((c8)']');
  }
  //
});
REGISTER_SPECIAL_PRINTER_NEEDID(msList_printer_generic, "msList", void *, {
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
    for (each_RANGE(usize, i, 0, msList_len(in))) {
      if (i)
        PUTC((c8)',');
      printer.function(
          put,
          (fptr){size, size * i + (u8 *)in},
          printer_arg_after(':', farg),
          _arb
      );
    }
    PUTC((c8)']');
  }
  //
});

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
            &(typeof(a)){0},                                                   \
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
    }),

#else
template <typename T>
constexpr const char *type_name_cstr(T arg) { return ""; }

  #define MAKE_PRINT_ARG_TYPE(type) \
    template <>                     \
    constexpr const char *type_name_cstr<type>(type arg) { return #type; }

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

  #define MAKE_PRINT_ARG(a)                                        \
    ((struct print_arg){                                           \
        .ref = (fptr){sizeof(typeof(a)), (u8 *)REF(typeof(a), a)}, \
        .name = fp_from(type_name_cstr(a))                         \
    }),
#endif

volatile static thread_local bool print_f_shouldFlush = 1;
void print_f(outputFunction put, void *arb, const char *fmt, struct print_arg *);

#define print_wfO(printerfn, arb, fmt, ...)                                        \
  do {                                                                             \
    print_f(                                                                       \
        printerfn,                                                                 \
        arb,                                                                       \
        fmt,                                                                       \
        (struct print_arg[]){                                                      \
            __VA_OPT__(APPLY_N(MAKE_PRINT_ARG, __VA_ARGS__))((struct print_arg){}) \
        }                                                                          \
    );                                                                             \
  } while (0)

#define print_wf(print, fmt, ...) print_wfO(print, NULL, fmt, __VA_ARGS__)
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
#define snprint(allocator, fmt, ...) ({                                                            \
  struct print_arg eval_print_sn[countof((                                                         \
      (struct print_arg[]){__VA_OPT__(APPLY_N(MAKE_PRINT_ARG, __VA_ARGS__))((struct print_arg){})} \
  ))] = {                                                                                          \
      __VA_OPT__(APPLY_N(MAKE_PRINT_ARG, __VA_ARGS__))((struct print_arg){})                       \
  };                                                                                               \
  vsn_print_fn(allocator, (char *)fmt, eval_print_sn);                                             \
})
#define print_(fmt, ...) print_wfO(fileprint, stdout, fmt, __VA_ARGS__)
#define println_(fmt, ...) print(fmt "\n", __VA_ARGS__)
#define print(fmt, ...) print_(fmt, __VA_ARGS__)
#define println(fmt, ...) println_(fmt, __VA_ARGS__)

#ifdef PRINTER_LIST_TYPENAMES
__attribute__((constructor(205))) static void printer_post_initfn() {
  print("==============================\n"
        "printer debug\n"
        "==============================\n");
  println("list of printer type names: ");
  for (each_RANGE(usize, i, 0, stringList_len((stringList *)PrinterSingleton.data)))
    println("{slice(c8)}", stringList_get((stringList *)(PrinterSingleton.data), i));
  println(
      "buckets   : {}\n"
      "footprint : {}\n"
      "collisions: {}\n"
      "==============================\n",
      ((sHmap *)PrinterSingleton.data)->num_buckets,
      sHmap_footprint((sHmap *)PrinterSingleton.data),
      sHmap_countCollisions((sHmap *)PrinterSingleton.data),
  );
}
#endif // PRINTER_LIST_TYPENAMES

#endif

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_PRINTER_C (1)
#endif
#ifdef MY_PRINTER_C

inline fptr printer_arg_until(char delim, fptr string) {
  usize i = 0;
  u8 *ptr = (u8 *)string.ptr;
  while (i < string.len && ptr[i] != delim)
    i++;
  string.len = i;
  return string;
}
inline fptr printer_arg_after(char delim, fptr slice) {
  usize i = 0;
  uint8_t *ptr = slice.ptr;
  while (i < slice.len && ptr[i] != delim)
    i++;
  i = (i < slice.len) ? (i + 1) : (i);
  slice.ptr += i;
  slice.len -= i;
  return slice;
}
inline fptr printer_arg_trim(fptr in) {
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
      for_each_((var_ i, VLAP(typeName.ptr, typeName.len)), PUTC((c8)i););
    PUTS(") __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else if (fn.size > p.ref.len) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = 1}));
    PUTS("__ PRINTER TRIED TO READ ");
    USETYPEPRINTER(usize, fn.size);
    PUTS(" BUT ITEM ONLY HAS ");
    USETYPEPRINTER(usize, p.ref.len);
    PUTS(" BYTES __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else {
    fn.function(put, p.ref, args, _arb);
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
#endif
