#if !defined(SMALL_STREAM_H)
  #define SMALL_STREAM_H (1)
  #include "assertMessage.h"
  #include "fptr.h"
  #include "macros.h"
  #include "mytypes.h"

typedef u32 sstream_errn;
typedef struct {
  u32 err;
  c8 result;
} sstream_c8;
typedef struct {
  u32 err;
  usize result;
} sstream_size;

typedef struct small_stream_vt small_stream_vt;
typedef const small_stream_vt *const sstream;

typedef fnptr_((sstream /*              */), sstream_c8) sstream_getc;
typedef fnptr_((sstream, fptr /*        */), sstream_size) sstream_gets;

typedef fnptr_((sstream, u8 /*          */), sstream_errn) sstream_putc;
typedef fnptr_((sstream, fptr /*        */), sstream_errn) sstream_puts;

typedef fnptr_((sstream /*              */), sstream_size) sstream_tell;
typedef fnptr_((sstream, usize /*       */), sstream_errn) sstream_seek;

typedef fnptr_((sstream, sstream_errn /**/), const char *) __attribute__((pure)) sstream_sterr;

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

static inline sstream_c8(sGetc)(sstream stream) {
  assertMessage(stream, "stream cant be null");
  assertMessage(stream->getc, "getc vcall cant be null");
  return stream->getc(stream);
}
static inline sstream_size(sGets)(sstream stream, fptr buffer) {
  assertMessage(stream, "stream cant be null");
  assertMessage(buffer.len && buffer.ptr, "buffer cant be null");
  assertMessage(stream->gets, "gets vcall cant be null");
  return stream->gets(stream, buffer);
}
static inline sstream_errn(sPutc)(sstream stream, u8 ch) {
  assertMessage(stream, "stream cant be null");
  assertMessage(stream->putc, "putc vcall cant be null");
  return stream->putc(stream, ch);
}
static inline sstream_errn(sPuts)(sstream stream, fptr buffer) {
  assertMessage(stream, "stream cant be null");
  assertMessage(buffer.len && buffer.ptr, "buffer cant be null");
  assertMessage(stream->puts, "puts vcall cant be null");
  return stream->puts(stream, buffer);
}
static inline sstream_size(sTell)(sstream stream) {
  assertMessage(stream, "stream cant be null");
  assertMessage(stream->tell, "tell vcall cant be null");
  return stream->tell(stream);
}
static inline sstream_errn(sSeek)(sstream stream, usize pos) {
  assertMessage(stream, "stream cant be null");
  assertMessage(stream->seek, "seek vcall cant be null");
  if (stream->tell_max) {
    assertMessage(pos <= *(stream->tell_max), "seek exceeds max bound");
  }
  return stream->seek(stream, pos);
}

static inline const char *_sGetErr(sstream stream, sstream_errn err) {
  return (stream->efromint) ? stream->efromint(stream, err) : "unknown error";
}

c8(sGetcf)(sstream stream) {
  sstream_c8 res = (sGetc)(stream);
  assertMessage(!res.err, "sGetc failed [%u: %s]", res.err, _sGetErr(stream, res.err));
  return res.result;
}
usize(sGetsf)(sstream stream, fptr buffer) {
  sstream_size res = (sGets)(stream, buffer);
  assertMessage(!res.err, "sGets failed [%u: %s]", res.err, _sGetErr(stream, res.err));
  return res.result;
}
void(sPutcf)(sstream stream, u8 ch) {
  sstream_errn err = (sPutc)(stream, ch);
  assertMessage(!err, "sPutc failed [%u: %s]", err, _sGetErr(stream, err));
}
void(sPutsf)(sstream stream, fptr buffer) {
  sstream_errn err = (sPuts)(stream, buffer);
  assertMessage(!err, "sPuts failed [%u: %s]", err, _sGetErr(stream, err));
}
usize(sTellf)(sstream stream) {
  sstream_size res = (sTell)(stream);
  assertMessage(!res.err, "sTell failed [%u: %s]", res.err, _sGetErr(stream, res.err));
  return res.result;
}
void(sSeekf)(sstream stream, usize pos) {
  sstream_errn err = (sSeek)(stream, pos);
  assertMessage(!err, "sSeek failed [%u: %s]", err, _sGetErr(stream, err));
}
#endif
