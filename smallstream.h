#if !defined(SMALL_STREAM_H)
  #define SMALL_STREAM_H (1)
  #include "fptr.h"
  #include "macros.h"
  #include "mytypes.h"

typedef u32 sstream_errn;
typedef struct {
  u32 err;
  c8 result;
} getc_eu;
typedef struct {
  u32 err;
  usize result;
} gets_eu;

typedef struct small_stream_vt small_stream_vt;
typedef const small_stream_vt *const sstream;

typedef fnptr_((sstream /*              */), getc_eu) /*     */ sstream_getc;
typedef fnptr_((sstream, fptr /*        */), gets_eu) /*     */ sstream_gets;

typedef fnptr_((sstream, u8 /*          */), sstream_errn) /**/ sstream_putc;
typedef fnptr_((sstream, fptr /*        */), sstream_errn) /**/ sstream_puts;

typedef fnptr_((sstream /*              */), gets_eu) /*     */ sstream_tell;
typedef fnptr_((sstream, usize /*       */), sstream_errn) /**/ sstream_seek;

typedef fnptr_((sstream, sstream_errn /**/), const char *) /**/ __attribute__((const)) sstream_sterr;

typedef const struct small_stream_vt {
  struct /*reading*/ {
    // read one character
    sstream_getc _Nullable getc;
    // read buffer
    sstream_gets _Nullable gets;
  };
  struct /*writing*/ {
    // write one character
    sstream_putc _Nullable putc;
    // write buffer
    sstream_puts _Nullable puts;
  };
  struct /*positions*/ {
    sstream_tell _Nullable tell;
    sstream_seek _Nullable seek;
    const usize *_Nullable tell_max;
  };
  sstream_sterr efromint;
  alignas(myAlign) u8 user[];
} *const sstream;

#endif
