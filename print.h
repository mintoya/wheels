#ifndef MY_PRINTER_H
#define MY_PRINTER_H
#include <errno.h>
#include <locale.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "allocator.h"
#include "fptr.h"
#include "hhmap.h"
#include "my-list.h"
#include "printer/variadic.h"

typedef void (*outputFunction)(
    const wchar *,
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
#ifndef OVERRIDE_DEFAULT_PRINTER
  #include <stdio.h>
static void stdoutPrint(
    const wchar *c,
    void *,
    unsigned int length,
    bool flush
) {
  #define bufLen (2 << 16)
  mbstate_t mbs = {0};
  static thread_local struct
  {
    wchar buf[bufLen];
    char crBuf[bufLen / sizeof(wchar) * 4];
    usize place;
    usize crPlace;
  } buf = {
      0
  };

  if (buf.place + length >= bufLen || flush) {
    buf.crPlace = 0;
    for (usize i = 0; i < buf.place; i++)
      buf.crPlace += wcrtomb(buf.crBuf + buf.crPlace, buf.buf[i], &mbs);
    fwrite(buf.crBuf, sizeof(char), buf.crPlace, stdout);
    buf.crPlace = 0;

    char *useBuffer;
    usize useBufferPlace = 0;
    if (length > bufLen) {
      useBuffer = (char *)aAlloc(defaultAlloc, length * sizeof(char) * 4);
    } else {
      useBuffer = buf.crBuf;
    }
    for (usize i = 0; i < length; i++)
      useBufferPlace += wcrtomb(useBuffer + useBufferPlace, c[i], &mbs);
    fwrite(useBuffer, sizeof(char), useBufferPlace, stdout);

    if (length > bufLen)
      aFree(defaultAlloc, useBuffer);

    buf.place = 0;
    fflush(stdout);
  } else {
    memcpy(buf.buf + buf.place, c, length * sizeof(wchar));
    buf.place += length;
  }
}
static outputFunction defaultPrinter = stdoutPrint;
#else
extern outputFunction defaultPrinter;
#endif

static void snPrint(
    const char *c,
    void *buffer,
    unsigned int length,
    char flush
) {
  (void)flush;
  ffptr snBuff = *(ffptr *)buffer;

  usize start = snBuff.fptrp.width;
  usize end1 = snBuff.fptrp.width + length;
  usize end2 = snBuff.ffptr.capacity;
  const usize end = end1 < end2 ? end1 : end2;

  for (; start < end; start++)
    snBuff.fptrp.ptr[start] = c[start - snBuff.fptrp.width];
  snBuff.fptrp.width = end;
  *(ffptr *)buffer = snBuff;
}
// #include "assert.h"
static void asPrint(
    const wchar *c,
    void *listptr,
    unsigned int length,
    bool flush
) {
  (void)flush;
  assertMessage(listptr != NULL);
  List *list = *(List **)listptr;
  if (!list)
    list = List_new(defaultAlloc, sizeof(wchar));
  switch (list->width) {
    case sizeof(wchar):
      List_appendFromArr(list, c, length);
      break;
    case sizeof(char): {
      mbstate_t mbs = {0};

      usize out = list->length;
      List_resize(list, out + length * MB_CUR_MAX);

      char *dst = (char *)List_getRefForce(list, out);

      for (u32 i = 0; i < length; i++) {
        usize l = wcrtomb(dst, c[i], &mbs);
        assertMessage(l != (size_t)-1);
        dst += l;
        out += l;
      }

      list->length = out;
    } break;
    default:
      assertMessage(false, "sizes %lu & %lu \n %lu not supported\n", sizeof(wchar), sizeof(char), list->width);
      break;
  }
  *(List **)listptr = list;
}

static void fileprint(
    const wchar_t *c,
    void *fileHandle,
    unsigned int length,
    bool flush
) {
  FILE *f;
  mbstate_t st;
  char buf[MB_CUR_MAX];
  unsigned int i;

  assertMessage(fileHandle);
  f = (FILE *)fileHandle;

  memset(&st, 0, sizeof(st));

  for (i = 0; i < length; ++i) {
    size_t n = wcrtomb(buf, c[i], &st);
    if (n == (size_t)-1) {
      break; /* invalid wchar, stop */
    }
    fwrite(buf, 1, n, f);
  }

  if (flush) {
    fflush(f);
  }
}

static struct {
  HHMap *data;
} PrinterSingleton;

static void PrinterSingleton_init() {
  PrinterSingleton.data = HHMap_new(
      sizeof(printerFunction),
      sizeof(printerFunction),
      defaultAlloc, 10
  );
}
static void PrinterSingleton_deinit() { HHMap_free(PrinterSingleton.data); }
static void PrinterSingleton_append(fptr name, printerFunction function) {
  if (name.width > HHMap_getKeySize(PrinterSingleton.data))
    HHMap_transform(
        &PrinterSingleton.data,
        name.width,
        sizeof(printerFunction),
        NULL, 0
    );
  HHMap_fset(
      PrinterSingleton.data,
      name,
      REF(printerFunction, function)
  );
}

static printerFunction PrinterSingleton_get(fptr name) {
  static printerFunction lastprinters[2] = {NULL, NULL};
  static fptr lastnames[2] = {nullUmf, nullUmf};
  static u8 lasttick = 0;

  if (!fptr_cmp(name, lastnames[lasttick])) {
    return lastprinters[lasttick];
  } else if (!fptr_cmp(name, lastnames[!lasttick])) {
    return lastprinters[!lasttick];
  }

  u8 *nname = (u8 *)HHMap_getKeyBuffer(PrinterSingleton.data);
  memset(nname, 0, HHMap_getKeySize(PrinterSingleton.data));
  memcpy(nname, name.ptr, name.width);

  printerFunction p = NULL;
  HHMap_both r = HHMap_getBoth(PrinterSingleton.data, nname);

  if (r.val) {
    memcpy(&p, r.val, sizeof(p));
    lasttick = !lasttick;
    lastprinters[lasttick] = p;
    lastnames[lasttick] = ((fptr){name.width, (u8 *)r.key});
  }
  return p;
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
[[gnu::constructor(201)]] static void printerInit() {
  setlocale(LC_ALL, ".UTF-8");
  SetConsoleOutputCP(CP_UTF8);
  PrinterSingleton_init();
}
#else
[[gnu::constructor(201)]] static void printerInit() {
  setlocale(LC_ALL, "");
  PrinterSingleton_init();
}
#endif

[[gnu::destructor]] static void printerDeinit() { PrinterSingleton_deinit(); }

#define GETTYPEPRINTERFN(T) _##T##_printer
#define MERGE_PRINTER_M(a, b) a##b
#define LABEL_PRINTER_GEN(l, a) MERGE_PRINTER_M(l, a)

#define UNIQUE_GEN_PRINTER \
  LABEL_PRINTER_GEN(LABEL_PRINTER_GEN(__LINE__, _), __COUNTER__)
#define UNIQUE_PRINTER_FN LABEL_PRINTER_GEN(PRINTERFN, UNIQUE_GEN_PRINTER)
#define UNIQUE_PRINTER_FN2 \
  LABEL_PRINTER_GEN(printerConstructor, UNIQUE_GEN_PRINTER)

#define PUTS(characters) put(characters, _arb, (sizeof(characters) / sizeof(wchar)) - 1, 0)
#define PUTC(character) put(REF(wchar, character), _arb, 1, 0)

#define REGISTER_PRINTER(T, ...)                                       \
  static void GETTYPEPRINTERFN(T)(                                     \
      outputFunction put, const void *_v_in_ptr, fptr args, void *_arb \
  ) {                                                                  \
    (void)args;                                                        \
    T in = *(T *)_v_in_ptr;                                            \
    __VA_ARGS__                                                        \
  }                                                                    \
  [[gnu::constructor(202)]] static void register_##T() {               \
    fptr key = (fptr){                                                 \
        .width = sizeof(#T) - 1,                                       \
        .ptr = (uint8_t *)#T,                                          \
    };                                                                 \
    PrinterSingleton_append(key, GETTYPEPRINTERFN(T));                 \
  }

#define REGISTER_SPECIAL_PRINTER_NEEDID(id, str, type, ...)                          \
  static void id(outputFunction put, const void *_v_in_ptr, fptr args, void *_arb) { \
    type in = *(type *)_v_in_ptr;                                                    \
    __VA_ARGS__                                                                      \
  }                                                                                  \
  [[gnu::constructor(203)]] static void UNIQUE_PRINTER_FN2() {                       \
    fptr key = (fptr){                                                               \
        .width = strlen(str),                                                        \
        .ptr = (uint8_t *)str,                                                       \
    };                                                                               \
    PrinterSingleton_append(key, id);                                                \
  }

#define REGISTER_SPECIAL_PRINTER(str, type, ...) \
  REGISTER_SPECIAL_PRINTER_NEEDID(UNIQUE_PRINTER_FN, str, type, __VA_ARGS__)
#define USETYPEPRINTER(T, val) \
  GETTYPEPRINTERFN(T)(put, (u8 *)(void *)REF(T, val), nullUmf, _arb)
#define USENAMEDPRINTER(strname, val)                                     \
  print_f_helper(                                                         \
      (struct print_arg){.ref = REF(typeof(val), val), .name = nullFptr}, \
      printer_arg_trim(printer_arg_until(':', fp_from(strname))), put,    \
      printer_arg_after(':', fp_from(strname)), _arb                      \
  );
#define PRINTERARGSEACH(...)                     \
  fptr tempargs = printer_arg_trim(args);        \
  while (tempargs.width) {                       \
    fptr arg = printer_arg_until(' ', tempargs); \
    __VA_ARGS__                                  \
    tempargs = printer_arg_after(' ', tempargs); \
    tempargs = printer_arg_trim(tempargs);       \
  }

#define REGISTER_ALIASED_PRINTER(realtype, alias)                            \
  [[gnu::constructor(202)]] static void register__##alias() {                \
    fptr key = (fptr){                                                       \
        .width = sizeof(EXPAND_AND_STRINGIFY(alias)) - 1,                    \
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
// #include "printer/genericName.h"
// MAKE_PRINT_ARG_TYPE(usize);
// extends types assumed with ${} in c 
// 
// MAKE_PRINT_ARG_TYPE(usize);
// does the same in cpp

  REGISTER_PRINTER(fptr, {
    if (in.ptr) {for(usize i = 0;i<in.width;i++){
      wchar c = (wchar)in.ptr[i];
      PUTC(c);
    }} else { PUTS(L"__NULLUMF__"); }
  });
  REGISTER_SPECIAL_PRINTER("ptr", void*,{
    uintptr_t v = (uintptr_t)in;
    PUTS(L"0x");

    int shift = (sizeof(uintptr_t) * 8) - 4;
    int leading = 1;
    while (shift >= 0) {
      unsigned char nibble = (v >> shift) & 0xF;
      if (nibble || !leading || shift == 0) {
        leading = 0;
        char c = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
        PUTC((wchar)c);
      }
      shift -= 4;
    }

  });
  REGISTER_PRINTER(char, {PUTC((wchar)in);});
  REGISTER_PRINTER(wchar, {PUTC(in);});
  REGISTER_SPECIAL_PRINTER("cstr", char*,{
    in = nullElse(in,(char*)"__NULLCSTR__");
    while(*in){ PUTC((wchar)*in); in++; } 
  });
  REGISTER_SPECIAL_PRINTER("wcstr", wchar*,{
    in = nullElse(in,(wchar*)"__NULLCSTR__");
    while(*in){ PUTC(*in); in++; } 
  });

  REGISTER_PRINTER(usize, {
    usize l = 1;
    while (l <= in / 10)
      l *= 10;
    while (l) {
      char c = in / l + '0';
      PUTC((wchar)c);
      in %= l;
      l /= 10;
    }
  });
  REGISTER_PRINTER(uint, {
      USETYPEPRINTER(usize, (usize)in);
  });
  REGISTER_PRINTER(int, {
    if (in < 0) {
      PUTC(L'-');
      in = -in;
    }
    USETYPEPRINTER(uint, (uint)in);
  });
  REGISTER_PRINTER(f64, {
    if (in < 0) {
      PUTC(L'-');
      in *= -1;
    }
    int push = 0;
    while (((int)in)) {
      in /= 10;
      push++;
    }
    in *= 10;
    if(!push)
        PUTC(L'.');
    for (int i = 0; i < 12; i++) {
      wchar dig = L'0';
      dig += ((unsigned int)in) % 10;
      PUTC(dig);
      if (i + 1 == push)
        PUTC(L'.');
      in *= 10;
    }
  });

  REGISTER_SPECIAL_PRINTER("fptr<wchar>",fptr, {
      put((wchar*)in.ptr,_arb,in.width/sizeof(wchar),0);
  });
  REGISTER_SPECIAL_PRINTER("fptr<void>", fptr, {
      const wchar hex_chars[17] = L"0123456789abcdef";
      char cut0s = 0;
      char useLength = 0;
      PRINTERARGSEACH({
          UM_SWITCH(arg,{
              UM_CASE(fp_from("length"),{useLength = 1;});
              UM_DEFAULT();
          });
      });
      if(useLength){
        PUTS(L"<");
        USETYPEPRINTER(usize,in.width);
      }
      PUTS(L"<");
      usize start = 0;
      if(cut0s) {
          for(usize i = 0; i < in.width; i++) {
              if(in.ptr[i] != 0) {
                  start = i;
                  break;
              }
          }
      }
      
      for(usize i = start; i < in.width; i++) {
          u8 top = (in.ptr[i] & 0xF0) >> 4;  
          u8 bottom = in.ptr[i] & 0x0F;
          PUTC(hex_chars[top]);
          PUTC(hex_chars[bottom]);
      }
      PUTS(L">");
      if(useLength)
        PUTS(L">");
  });
  REGISTER_PRINTER(pEsc, {
    if (in.poset) {

      PUTS(L"\033[");
      USETYPEPRINTER(uint, in.pos.row);
      PUTS(L";");
      USETYPEPRINTER(uint, in.pos.col);
      PUTS(L"H");
    }
    if (in.fgset) {
      PUTS(L"\033[38;2;");
      USETYPEPRINTER(uint, in.fg.r);
      PUTS(L";");
      USETYPEPRINTER(uint, in.fg.g);
      PUTS(L";");
      USETYPEPRINTER(uint, in.fg.b);
      PUTS(L"m");
    }

    if (in.bgset) {
      PUTS(L"\033[48;2;");
      USETYPEPRINTER(uint, in.bg.r);
      PUTS(L";");
      USETYPEPRINTER(uint, in.bg.g);
      PUTS(L";");
      USETYPEPRINTER(uint, in.bg.b);
      PUTS(L"m");
    }
    if (in.clear) {
      PUTS(L"\033[2J");
      PUTS(L"\033[H");
    }
    if (in.reset) {
      PUTS(L"\033[0m");
    }
  });
  REGISTER_SPECIAL_PRINTER("u8", u8,{
    USETYPEPRINTER(uint, (uint)in);
  });
// type assumption
#ifndef __cplusplus

  #include "printer/genericName.h"
  #define MAKE_PRINT_ARG_TYPE(type) MAKE_NEW_TYPE(type)
  MAKE_PRINT_ARG_TYPE(int);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(fptr);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(usize);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(f64);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(char);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(uint);
  #include "printer/genericName.h"
  MAKE_PRINT_ARG_TYPE(pEsc);
   
  #define MAKE_PRINT_ARG(a)               \
    ((struct print_arg){                  \
        .ref = REF(typeof(a), a),         \
        .name = fp_from(GENERIC_NAME(a)), \
    })
  
  #else
  template <typename T>
  constexpr const char *type_name_cstr() { return ""; }
  
  #define MAKE_PRINT_ARG_TYPE(type) \
    template <>                     \
    constexpr const char *type_name_cstr<type>() { return #type; }
  
  MAKE_PRINT_ARG_TYPE(int);
  MAKE_PRINT_ARG_TYPE(fptr);
  MAKE_PRINT_ARG_TYPE(usize);
  MAKE_PRINT_ARG_TYPE(f64);
  MAKE_PRINT_ARG_TYPE(char);
  MAKE_PRINT_ARG_TYPE(uint);
  MAKE_PRINT_ARG_TYPE(pEsc);
   
  
  #define MAKE_PRINT_ARG(a)                                          \
    ((struct print_arg){                                             \
       .ref = REF(typeof(a), a),                                    \
       .name = fp_from(type_name_cstr<std::decay_t<decltype(a)>>()) \
   })
#endif
// clang-format on
#define EMPTY_PRINT_ARG ((struct print_arg){.ref = NULL, .name = nullUmf})

void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *arb);

static thread_local uchar print_f_shouldFlush = 1;
void print_f(outputFunction put, void *arb, fptr fmt, ...);

#define print_wfO(printerfn, arb, fmt, ...)                                               \
  print_f(                                                                                \
      printerfn, arb, fp_from("" fmt) __VA_OPT__(, APPLY_N(MAKE_PRINT_ARG, __VA_ARGS__)), \
      EMPTY_PRINT_ARG                                                                     \
  )

#define print_wf(print, fmt, ...) print_wfO(print, NULL, fmt, __VA_ARGS__)
#define print(fmt, ...) print_wf(defaultPrinter, fmt, __VA_ARGS__)
#define println(fmt, ...) print(fmt "\n", __VA_ARGS__)

#ifdef PRINTER_LIST_TYPENAMES
[[gnu::constructor(205)]] static void post_init() {
  outputFunction put = defaultPrinter;
  print("==============================\n"
        "printer debug\n"
        "==============================\n");
  println("list of printer type names: ");
  for (int i = 0; i < HHMap_count(PrinterSingleton.data); i++) {
    fptr key = ((fptr){
        HHMap_getKeySize(PrinterSingleton.data),
        (u8 *)HHMap_getKey(PrinterSingleton.data, i)
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
      HHMap_getMetaSize(PrinterSingleton.data),
      HHMap_footprint(PrinterSingleton.data),
      (usize)HHMap_count(PrinterSingleton.data),
      (int)HHMap_countCollisions(PrinterSingleton.data),
      (int)HHMap_getKeySize(PrinterSingleton.data)
  );
}
#endif // PRINTER_LIST_TYPENAMES

// converts wchars to chars
// writes length to olen
void wchar_toUtf8(const wchar *input, u32 wlen, u8 *output, u32 *olen);
#endif

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_PRINTER_C (1)
#pragma once
#endif
#ifdef MY_PRINTER_C
void wchar_toUtf8(const wchar *input, u32 wlen, u8 *output, u32 *olen) {
  // assertMessage(*olen == 0, "this function will increment olen");
  mbstate_t mbs = {0};
  for (usize i = 0; i < wlen; i++)
    *olen += wcrtomb(
        (char *)(output + *olen),
        input[i],
        &mbs
    );
}
inline unsigned int printer_arg_indexOf(fptr string, char c) {
  int i;
  char *ptr = (char *)string.ptr;
  for (i = 0; i < string.width && ptr[i] != c; i++)
    ;
  return i;
}

inline fptr printer_arg_until(char delim, fptr string) {
  usize i = 0;
  uint8_t *ptr = (uint8_t *)string.ptr;
  while (i < string.width && ptr[i] != delim)
    i++;
  string.width = i;
  return string;
}

inline fptr printer_arg_after(char delim, fptr slice) {
  usize i = 0;
  uint8_t *ptr = slice.ptr;
  while (i < slice.width && ptr[i] != delim)
    i++;
  i = (i < slice.width) ? (i + 1) : (i);
  slice.ptr += i;
  slice.width -= i;
  return slice;
}
inline fptr printer_arg_trim(fptr in) {
  fptr res = in;
  int front = 0;
  int back = in.width - 1;
  while (front < in.width && ((uint8_t *)in.ptr)[front] == ' ') {
    front++;
  }
  while (back > front && ((uint8_t *)in.ptr)[front] == ' ') {
    back--;
  }
  res = (fptr){
      .width = (usize)(back - front + 1),
      .ptr = in.ptr + front,
  };
  return res;
}

void print_f_helper(struct print_arg p, fptr typeName, outputFunction put, fptr args, void *_arb) {
  void *ref = p.ref;
  if (!typeName.width) {
    typeName = p.name;
  }
  printerFunction fn = PrinterSingleton_get(typeName);
  if (!fn) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = 1}));
    USETYPEPRINTER(fptr, fp_from("__ NO_TYPE("));
    USETYPEPRINTER(fptr, typeName);
    USETYPEPRINTER(fptr, fp_from(") __"));
    USETYPEPRINTER(pEsc, ((pEsc){.reset = 1}));
  } else {
    fn(put, ref, args, _arb);
  }
}

void print_f(outputFunction put, void *arb, const fptr fmt, ...) {
  const char *ccstr = (char *)fmt.ptr;
  va_list l;
  va_start(l, fmt);
  bool toggled = 0;
  for (u32 i = 0; i < fmt.width; i++) {
    if (ccstr[i] == '{' && !toggled) {
      u32 j = i + 1;
      while (j < fmt.width && ccstr[j] != '}')
        j++;
      fptr typeName = {
          .width = j - i - 1,
          .ptr = ((u8 *)fmt.ptr) + i + 1,
      };
      struct print_arg assumedName = va_arg(l, struct print_arg);

      fptr tname = printer_arg_until(':', typeName);
      fptr parseargs = printer_arg_after(':', typeName);
      tname = printer_arg_trim(tname);
      if (!assumedName.ref) {
        va_end(l);
        return put(L"__ NO ARGUMENT PROVIDED, ENDING PRINT __\n", arb, 41, 1);
      }
      print_f_helper(assumedName, tname, put, parseargs, arb);
      i = j;
      toggled = 0;

    } else if (ccstr[i] == '/') {
      toggled = !toggled;
      if (!toggled) {
        wchar c = (wchar)(ccstr[i]);
        put(REF(wchar, L'/'), arb, 1, 0);
      }
    } else {
      toggled = 0;
      wchar c = (wchar)(ccstr[i]);
      put(&c, arb, 1, 0);
    }
  }
  va_end(l);
  if (print_f_shouldFlush)
    put(NULL, arb, 0, 1);
}
#endif
