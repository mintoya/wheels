#ifndef MY_PRINTER_H
#define MY_PRINTER_H
#include "allocator.h"
#include "assertMessage.h"
#include "fptr.h"
#include "macros.h"
#include "mytypes.h"
#include "shmap.h"
#include <locale.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#if defined(PRINTER_NOC32TOMB) || !__has_include(<uchar.h>)
static inline usize c32rtomb(char *s, c32 chars, mbstate_t *_) {
  return s ? (*s = (char)chars, 0) : 1;
}
#else
  #include <uchar.h>
#endif

typedef void (*outputFunction)(
    const c32 *,
    void *,
    unsigned int,
    bool
);
typedef void (*printerFunction)(
    outputFunction,
    const void *,
    fptr args,
    void *
);
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
    const c32 *c,
    void *fileHandle,
    unsigned int length,
    bool flush
) {
  #define BUF_ELEMENTS (2 << 8)
  #define MAX_UTF8_BYTES 4
  FILE *file = (FILE *)fileHandle;

  static thread_local struct {
    c32 buf[BUF_ELEMENTS];
    c8 crBuf[BUF_ELEMENTS * MAX_UTF8_BYTES];
    usize place;
  } buffer = {0};

  static thread_local mbstate_t mbs = {0};
  if (flush || (buffer.place + length) > BUF_ELEMENTS) {
    usize writeByteCount = 0;
    char *dest = (char *)buffer.crBuf;
    for (usize i = 0; i < buffer.place; i++) {
      isize writelen = c32rtomb(dest + writeByteCount, buffer.buf[i], &mbs);
      if (writelen == -1) {
        memset(&mbs, 0, sizeof(mbs));
        continue;
      }
      writeByteCount += writelen;
    }

    for (usize i = 0; i < length; i++) {
      if (writeByteCount + MAX_UTF8_BYTES > sizeof(buffer.crBuf)) {
        fwrite(buffer.crBuf, 1, writeByteCount, file);
        writeByteCount = 0;
      }

      isize writelen = c32rtomb(dest + writeByteCount, c[i], &mbs);
      if (writelen == -1) {
        memset(&mbs, 0, sizeof(mbs));
        continue;
      }
      writeByteCount += writelen;
    }
    if (writeByteCount > 0) {
      fwrite(buffer.crBuf, 1, writeByteCount, file);
    }
    if (flush) {
      fflush(file);
    }

    buffer.place = 0;

  } else {
    memcpy(buffer.buf + buffer.place, c, length * sizeof(c32));
    buffer.place += length;
  }
}
#endif

static struct {
  msHmap(printerFunction) data;
} PrinterSingleton;

static void PrinterSingleton_init() {
  PrinterSingleton.data = msHmap_init(stdAlloc, printerFunction);
}
static void PrinterSingleton_append(fptr name, printerFunction function) {
  msHmap_set(PrinterSingleton.data, name, function);
}

static printerFunction PrinterSingleton_get(fptr name) {
  static printerFunction lastprinters[2] = {NULL, NULL};
  static fptr lastnames[2] = {nullFptr, nullFptr};
  static u8 lasttick = 0;

  if (!fptr_cmp(name, lastnames[lasttick])) {
    return lastprinters[lasttick];
  } else if (!fptr_cmp(name, lastnames[!lasttick])) {
    return lastprinters[!lasttick];
  }
  lasttick = !lasttick;

  __auto_type val = sHmap_find((sHmap *)PrinterSingleton.data, name);
  if (val) {
    __auto_type list = (printerFunction *)(((sHmap *)PrinterSingleton.data)->values)->buf;
    lastprinters[lasttick] = list[val[0].vidx];
    lastnames[lasttick] =
        stringList_get(
            ((sHmap *)PrinterSingleton.data)->strings,
            val[0].kidx
        );
    return list[val[0].vidx];
  }
  return NULL;
}

// arg utils
// clang-format off

unsigned int printer_arg_indexOf(fptr string, char c);
fptr printer_arg_until(char delim, fptr string) ;
fptr printer_arg_after(char delim, fptr slice) ;
fptr printer_arg_trim(fptr in) ;

// clang-format on

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

#define PUTS(characters) put(characters, _arb, (sizeof(characters) / sizeof(c32)) - 1, 0)
#define PUTC(character) put(REF(c32, character), _arb, 1, 0)

#define REGISTER_PRINTER(T, ...)                                       \
  static void GETTYPEPRINTERFN(T)(                                     \
      outputFunction put, const void *_v_in_ptr, fptr args, void *_arb \
  ) {                                                                  \
    (void)args;                                                        \
    T in = *(T *)_v_in_ptr;                                            \
    __VA_ARGS__                                                        \
  }                                                                    \
  __attribute__((constructor(202))) static void register_##T() {       \
    fptr key = (fptr){                                                 \
        .len = sizeof(#T) - 1,                                         \
        .ptr = (uint8_t *)#T,                                          \
    };                                                                 \
    PrinterSingleton_append(key, GETTYPEPRINTERFN(T));                 \
  }

#define REGISTER_SPECIAL_PRINTER_NEEDID(id, str, type, ...)                          \
  static void id(outputFunction put, const void *_v_in_ptr, fptr args, void *_arb) { \
    type in = *(typeof(in) *)_v_in_ptr;                                              \
    __VA_ARGS__                                                                      \
  }                                                                                  \
  __attribute__((constructor(203))) static void UNIQUE_PRINTER_FN2() {               \
    fptr key = (fptr){                                                               \
        .len = strlen(str),                                                          \
        .ptr = (uint8_t *)str,                                                       \
    };                                                                               \
    PrinterSingleton_append(key, id);                                                \
  }

#define REGISTER_SPECIAL_PRINTER(str, type, ...) \
  REGISTER_SPECIAL_PRINTER_NEEDID(UNIQUE_PRINTER_FN, str, type, __VA_ARGS__)
#define USETYPEPRINTER(T, val) \
  GETTYPEPRINTERFN(T)(put, (u8 *)(void *)REF(T, val), nullFptr, _arb)
#define USENAMEDPRINTER(strname, val)                                     \
  print_f_helper(                                                         \
      (struct print_arg){.ref = REF(typeof(val), val), .name = nullFptr}, \
      printer_arg_trim(printer_arg_until(':', fp_from(strname))), put,    \
      printer_arg_after(':', fp_from(strname)), _arb                      \
  );
#define PRINTERARGSEACH(...)                     \
  fptr tempargs = printer_arg_trim(args);        \
  while (tempargs.len) {                         \
    fptr arg = printer_arg_until(' ', tempargs); \
    __VA_ARGS__                                  \
    tempargs = printer_arg_after(' ', tempargs); \
    tempargs = printer_arg_trim(tempargs);       \
  }

#define REGISTER_ALIASED_PRINTER(realtype, alias)                            \
  __attribute__((constructor(202))) static void register__##alias() {        \
    fptr key = (fptr){                                                       \
        .len = sizeof(EXPAND_AND_STRINGIFY(alias)) - 1,                      \
        .ptr = (uint8_t *)EXPAND_AND_STRINGIFY(alias),                       \
    };                                                                       \
    uint8_t *refFn =                                                         \
        (uint8_t *)(void *)REF(printerFunction, GETTYPEPRINTERFN(realtype)); \
    PrinterSingleton_append(key, GETTYPEPRINTERFN(realtype));                \
  }

struct print_arg {
  void *ref;
  fptr name;
};
void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *arb);
// clang-format off

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

REGISTER_SPECIAL_PRINTER_NEEDID(_void_ptr_printerfn,"ptr", void *, {
  uintptr_t v = (uintptr_t)in;
  PUTS(U"0x");

  int shift = (sizeof(uintptr_t) * 8) - 4;
  int leading = 1;
  while (shift >= 0) {
    unsigned char nibble = (v >> shift) & 0xF;
    if (nibble || !leading || shift == 0) {
      leading = 0;
      char c = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
      PUTC((c32)c);
    }
    shift -= 4;
  }
});
sliceDef(c8);
REGISTER_SPECIAL_PRINTER_NEEDID(_slice_c8_printerfn, "slice(c8)", slice(c8), {
    foreach(c32 c, slice_vla(in))
      PUTC(c);
});
REGISTER_PRINTER(c8, { PUTC((c32)in); });
REGISTER_PRINTER(c32, { PUTC(in); });
REGISTER_SPECIAL_PRINTER("cstr", char *, {
  if (!in)
    in = "__NULLCSTR__";
  while (*in) {
    PUTC((c32)*in);
    in++;
  }
});
REGISTER_SPECIAL_PRINTER("wcstr", c32 *, {
  if (!in)
    in = (c32 *)U"__NULLCSTR__";
  while (*in) {
    PUTC(*in);
    in++;
  }
});

REGISTER_PRINTER(usize, {
  c32 digits[sizeof(usize)*8/3];
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
});
REGISTER_PRINTER(isize, {
  usize uin = (usize)in;
  if (in < 0) { PUTC(L'-'); uin =0-in; }
  USETYPEPRINTER(usize, uin);
});

REGISTER_PRINTER(f128, {
  usize digits = 0;
  if (( args = printer_arg_trim(args) ).len)  
    for(auto i = 0;i<args.len && (args.ptr[i]<= '9' && args.ptr[i]>= '0');i++){
      digits*=10;
      digits+=args.ptr[i]-'0';
    }
   
  digits = digits?:3;
  in = in<0?(PUTC(L'-'),-in):in;
  f128 u = in;

  f128 round = 0.5;
  for(usize i = 0; i < digits; i++){
    round /= 10.0;
  }
  u += round;

  f128 tens = 1;
  while(u / tens >= 10){
    tens*=10;
  }

  while(tens >= 1){
    int d = (int)(u / tens);
    PUTC(d + '0');
    u -= (f128)d * tens;
    tens /= 10;
  }

  if(digits > 0){
    PUTC('.');
    for(usize i = 0;i<digits;i++){
      u *= 10;
      int d = (int)u;
      PUTC(d + '0');
      u -= (f128)d;
    }
  }
});
REGISTER_PRINTER(float, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg) {.ref = (void*)&r},
    fp("f128"), put, args, _arb
  );
});
REGISTER_PRINTER(double, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg) {.ref = (void*)&r},
    fp("f128"), put, args, _arb
  );
});
REGISTER_PRINTER(longdouble, {
  f128 r = (f128)in;
  print_f_helper((struct print_arg) {.ref = (void*)&r},
    fp("f128"), put, args, _arb
  );
});
REGISTER_PRINTER(u32, {
  USETYPEPRINTER(usize, (usize)in);
});
REGISTER_PRINTER(i32, {
    USETYPEPRINTER(isize, (isize)in);
});
REGISTER_PRINTER(fptr ,{
  const c32 hex_chars[17] = U"0123456789abcdef";
  char cut0s = 0;
  char useLength = 0;
  PRINTERARGSEACH({
    if (fptr_eq(fp_from("length"), arg)) 
      useLength = 1;
  });
  if (useLength) {
    PUTS(U"<");
    USETYPEPRINTER(usize, in.len);
  }
  PUTS(U"<");
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
    PUTC(hex_chars[top]);
    PUTC(hex_chars[bottom]);
  }
  PUTS(U">");
  if (useLength)
    PUTS(U">");
});
REGISTER_PRINTER(pEsc, {
  if (in.poset) {

    PUTS(U"\033[");
    USETYPEPRINTER(usize, in.pos.row);
    PUTS(U";");
    USETYPEPRINTER(usize, in.pos.col);
    PUTS(U"H");
  }
  if (in.fgset) {
    PUTS(U"\033[38;2;");
    USETYPEPRINTER(usize, in.fg.r);
    PUTS(U";");
    USETYPEPRINTER(usize, in.fg.g);
    PUTS(U";");
    USETYPEPRINTER(usize, in.fg.b);
    PUTS(U"m");
  }

  if (in.bgset) {
    PUTS(U"\033[48;2;");
    USETYPEPRINTER(usize, in.bg.r);
    PUTS(U";");
    USETYPEPRINTER(usize, in.bg.g);
    PUTS(U";");
    USETYPEPRINTER(usize, in.bg.b);
    PUTS(U"m");
  }
  if (in.clear) {
    PUTS(U"\033[2J");
    PUTS(U"\033[H");
  }
  if (in.reset) {
    PUTS(U"\033[0m");
  }
});

REGISTER_SPECIAL_PRINTER("x", u8, {
  const c32 hex_chars[17] = U"0123456789abcdef";
  PUTC(hex_chars[in >> 4 & 0xf]);
  PUTC(hex_chars[in & 0xf]);
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
#if !defined(__cplusplus)

  #define MAKE_PRINT_ARG_TYPE(type) \
  type: ((fptr){sizeof(#type)-1,(u8*)#type})
  #if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
    #define MAKE_PRINTINTS MAKE_PRINT_ARG_TYPE(i32), MAKE_PRINT_ARG_TYPE(u32),
  #else
    #define MAKE_PRINTINTS
  #endif

  #define MAKE_PRINT_ARG(a)                                                                                                                                                                                                     \
    ((struct print_arg){                                                                                                                                                                                                        \
        .ref = REF(typeof(a), a),                                                                                                                                                                                               \
        .name = _Generic(\
            a,\
            MAKE_PRINT_ARG_TYPE(fptr),\
            MAKE_PRINT_ARG_TYPE(isize),\
            MAKE_PRINT_ARG_TYPE(usize),\
            MAKE_PRINT_ARG_TYPE(float),\
            MAKE_PRINT_ARG_TYPE(double),\
            MAKE_PRINT_ARG_TYPE(longdouble),\
            MAKE_PRINT_ARG_TYPE(pEsc),\
            MAKE_PRINTINTS\
            void *: ((fptr){sizeof("ptr")-1,(u8*)"ptr"}),\
            slice(c8): ((fptr){sizeof("slice(c8)")-1,(u8*)"slice(c8)"}),\
            default: nullFptr\
            ),\
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
MAKE_PRINT_ARG_TYPE(longdouble);
MAKE_PRINT_ARG_TYPE(pEsc);
  #if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
MAKE_PRINT_ARG_TYPE(i32);
MAKE_PRINT_ARG_TYPE(u32);
  #endif

  #define MAKE_PRINT_ARG(a)                \
    ((struct print_arg){                   \
        .ref = REF(typeof(a), a),          \
        .name = fp_from(type_name_cstr(a)) \
    }),
#endif
// clang-format on

static thread_local bool print_f_shouldFlush = 1;
void print_f(outputFunction put, void *arb, const char *fmt, ...);

#define print_wfO(printerfn, arb, fmt, ...)                                                       \
  print_f(                                                                                        \
      printerfn, arb, fmt, __VA_OPT__(APPLY_N(MAKE_PRINT_ARG, __VA_ARGS__))((struct print_arg){}) \
  )

#define print_wf(print, fmt, ...) print_wfO(print, NULL, fmt, __VA_ARGS__)
#define print(fmt, ...) print_wfO(fileprint, stdout, fmt, __VA_ARGS__)
#define println(fmt, ...) print(fmt "\n", __VA_ARGS__)
#define print_(fmt, ...) print_wfO(fileprint, stdout, fmt, __VA_ARGS__)
#define println_(fmt, ...) print(fmt "\n", __VA_ARGS__)

#ifdef PRINTER_LIST_TYPENAMES
__attribute__((constructor(205))) static void post_init() {
  assert(false & "rerite");
  // outputFunction put = defaultPrinter;
  print("==============================\n"
        "printer debug\n"
        "==============================\n");
  println("list of printer type names: ");
  for (int i = 0; i < HMap_count(PrinterSingleton.data); i++) {
    fptr key = ((fptr){
        HMap_getKeySize(PrinterSingleton.data),
        (u8 *)HMap_getKey(PrinterSingleton.data, i)
    });
    println(" {}", key);
  }
  println(
      "buckets   : {}\n"
      "footprint : {}\n"
      "types     : {}\n"
      "collisions: {}\n"
      "keysize   : {}\n"
      "==============================\n",
      HMap_getMetaSize(PrinterSingleton.data),
      HMap_footprint(PrinterSingleton.data),
      (usize)HMap_count(PrinterSingleton.data),
      (int)HMap_countCollisions(PrinterSingleton.data),
      (int)HMap_getKeySize(PrinterSingleton.data)
  );
}
#endif // PRINTER_LIST_TYPENAMES

// converts wchars to chars
// writes length to olen
#endif

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_PRINTER_C (1)
#pragma once
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
  void *ref = p.ref;
  if (!typeName.len) {
    typeName = p.name;
  }
  printerFunction fn = PrinterSingleton_get(typeName);
  if (!fn) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = 1}));
    PUTS(U"__ NO_TYPE(");
    for (__auto_type i = 0; i < typeName.len; i++)
      PUTC(typeName.ptr[i]);
    PUTS(U") __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else {
    fn(put, ref, args, _arb);
  }
}

void print_f(outputFunction put, void *arb, const char *fmt, ...) {
  va_list l;
  va_start(l, fmt);
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
      struct print_arg assumedName = va_arg(l, struct print_arg);

      fptr tname = printer_arg_until(':', typeName);
      fptr parseargs = printer_arg_after(':', typeName);
      tname = printer_arg_trim(tname);
      if (!assumedName.ref) {
        va_end(l);
        return put(U"__ NO ARGUMENT PROVIDED, ENDING PRINT __\n", arb, 41, 1);
      }
      print_f_helper(assumedName, tname, put, parseargs, arb);
      i = j;
      toggled = 0;

    } else if (fmt[i] == '/') {
      toggled = !toggled;
      if (!toggled)
        put(REF(c32, U'/'), arb, 1, 0);
    } else {
      toggled = 0;
      c32 c = (c32)(fmt[i]);
      put(&c, arb, 1, 0);
    }
  }
  va_end(l);
  if (print_f_shouldFlush)
    put(NULL, arb, 0, 1);
}
#endif
