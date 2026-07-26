#if !defined MY_PRINTER_H
  #define MY_PRINTER_H (1)
  #include "allocator.h"
  #include "assertMessage.h"
  #include "macros.h"
  #include "print/print_pre.h"
  #include "sList.h"
  #include "smap.h"
  #include <locale.h>
  #include <stdio.h>
  #include <string.h>

// helper escape type

  #define pEscRst      \
    ((pEsc){           \
        .pos = {0, 0}, \
        .clear = 1,    \
        .reset = 1,    \
    })

  #if !defined(NOFILEPRINTER)
void fileprint(
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

void vsn_print(const c8 *_, void *lptr, usize length, bool __) {
  ((usize *)lptr)[0] += length;
}
void sn_print(const c8 *c, void *cptr, usize length, bool _) {
  slice(c8) *loc = (typeof(loc))cptr;
  assertMessage(loc && loc->ptr);
  if (length)
    memcpy(loc->ptr + loc->len, c, length);
  loc->len += length;
}

  #define headconfig printermap, fptr, printerFunction, (fptr_hash(k)), (fptr_cmp(a, b))
  #define mapconfig headconfig
  #include "incmap.h"

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

static void GETTYPEPRINTERFN(carr)(fptr _v_in_ptr, printerfunction_context _ctx) {
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
  #include "print/int_printers.h"
typePrinter(f128) {
  usize digits = 0;
  let args = PRINTARGS();
  if (args.len)
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
  if (fptr_eq(fp_from("length"), PRINTARGS()))
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
  let fn = PrinterSingleton_get(PRINTARGS());
  if (!fn.function) {
    let red = (pEsc){.fg.r = 255, .fgset = true};
    let reset = (pEsc){.reset = true};
    USETYPEPRINTER(pEsc, red);
    PUTS("__unknown printer ");
    let str = VLAP((c8 *)PRINTARGS().ptr, PRINTARGS().len);
    PUTS(*str);
    PUTS("__");
    USETYPEPRINTER(pEsc, reset);
    return;
  }
  PUTS("[*]");
  fn.function((fptr){fn.size, in}, printerfunction_context_pop(_ctx));
}
typePrinter("slice", struct slice_any_t) { // second least safe printer
  let const red = (pEsc){.fg.r = 255, .fgset = true};
  let const reset = (pEsc){.reset = true};

  let fn = PrinterSingleton_get(PRINTARGS());
  if (!fn.function) {
    USETYPEPRINTER(pEsc, red);
    PUTS("__unknown printer ");
    let str = VLAP((c8 *)PRINTARGS().ptr, PRINTARGS().len);
    PUTS(*str);
    PUTS("__");
    USETYPEPRINTER(pEsc, reset);
    return;
  } else if (fn.size == ~(usize)0) {
    PUTS("printer ");
    let str = VLAP((c8 *)PRINTARGS().ptr, PRINTARGS().len);
    PUTS(*str);
    PUTS(" must have defined size");
  }

  foreach (let i, span(in.ptr, in.len * fn.size, fn.size)) {
    if (i != in.ptr) PUTS(",");
    fn.function((fptr){fn.size, (u8 *)i}, printerfunction_context_pop(_ctx));
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

void print_f_helper(struct print_arg p, fptr typeName, printerfunction_context _ctx) {
  if (!typeName.len) {
    typeName = p.name;
  }
  printerFunction fn = PrinterSingleton_get(typeName);
  if (!fn.function) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = true}));
    PUTS("__ NO_TYPE(");
    if (typeName.len)
      foreach (var_ i, span(typeName.ptr, typeName.len))
        PUTC((c8)i[0]);
    PUTS(") __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = true}));
  } else if (p.ref.len != ~(usize)0 && fn.size != p.ref.len) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {255, 0, 0}, .fgset = true}));
    PUTS("__ PRINTER TRIED TO READ ");
    USETYPEPRINTER(usize, fn.size);
    PUTS(" BUT ITEM HAS ");
    USETYPEPRINTER(usize, p.ref.len);
    PUTS(" BYTES __");
    USETYPEPRINTER(pEsc, ((pEsc){.reset = true}));
  } else fn.function(p.ref, _ctx);
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
      if (!assumedName.ref.ptr)
        return put("__ NO ARGUMENT PROVIDED, ENDING PRINT __\n", arb, 41, 1);
      print_f_helper(assumedName, tname, (printerfunction_context){put, arb, parseargs});
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
