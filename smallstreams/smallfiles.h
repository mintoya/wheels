#if !defined(MY_FILES_H)
  #define MY_FILES_H (1)
  #include "../allocator.h"
  #include "../smallstream.h"

/* access :
 *  read
 *  write
 * mode
 *  create
 *  doestn_exist
 *  append
 *  truncate
 * status
 *  nonblock
 *  sync
 */
typedef struct {
  struct {
    struct {
      bool read : 1;
      bool write : 1;
    } access;
    struct {
      bool create : 1;
      bool doesnt_exist : 1;

      bool append : 1;
      bool truncate : 1;
    } mode;
    struct {
      bool nonblock : 1;
      bool sync : 1;
    } status;
  };
} fileopenflags;

sstream file_stream_open(AllocatorV allocator, const char *const path, fileopenflags flags);
void file_stream_close(AllocatorV allocator, sstream stream);
  #if defined(MAKE_TEST_FN)

MAKE_TEST_FN(file_stream_write_read, {
  const char *test_file = "test_stream_wr.txt";
  fileopenflags wflags = {0};
  wflags.access.write = 1;
  wflags.mode.create = 1;
  wflags.mode.truncate = 1;

  sstream wstream = file_stream_open(allocator, test_file, wflags);
  if (!wstream) return 1;

  const u8 *data = (u8 *)"testing data";
  fptr wbuf = {.ptr = (u8 *)data, .len = 12};

  sPutsf(wstream, wbuf);
  sPutcf(wstream, '!');

  if (sTellf(wstream) != 13) return 1;
  file_stream_close(allocator, wstream);

  fileopenflags rflags = {0};
  rflags.access.read = 1;

  sstream rstream = file_stream_open(allocator, test_file, rflags);
  if (!rstream) return 1;

  u8 read_buffer[16] = {0};
  fptr rbuf = {.ptr = read_buffer, .len = 12};

  usize bytes_read = sGetsf(rstream, rbuf);
  if (bytes_read != 12) return 1;
  if (memcmp(read_buffer, "testing data", 12)) return 1;

  c8 last = sGetcf(rstream);
  if (last != '!') return 1;

  file_stream_close(allocator, rstream);
  remove(test_file);
  return 0;
});

MAKE_TEST_FN(file_stream_seek_append, {
  const char *test_file = "test_stream_seek.txt";
  fileopenflags wflags = {0};
  wflags.access.write = 1;
  wflags.access.read = 1;
  wflags.mode.create = 1;
  wflags.mode.truncate = 1;

  sstream stream = file_stream_open(allocator, test_file, wflags);
  if (!stream) return 1;

  sPutsf(stream, fp("AABBCC"));
  sSeekf(stream, 2);
  sPutsf(stream, fp("DD"));

  if (sTellf(stream) != 4) return 1;

  sSeekf(stream, 0);
  u8 buf[8] = {0};
  sGetsf(stream, (fptr){.ptr = buf, .len = 6});
  if (memcmp(buf, "AADDCC", 6)) return 1;

  file_stream_close(allocator, stream);

  fileopenflags aflags = {0};
  aflags.access.write = 1;
  aflags.access.read = 1;
  aflags.mode.append = 1;

  sstream astream = file_stream_open(allocator, test_file, aflags);
  if (!astream) return 1;

  sPutsf(astream, fp("EE"));

  sSeekf(astream, 0);
  sGetsf(astream, (fptr){.ptr = buf, .len = 8});
  if (memcmp(buf, "AADDCCEE", 8)) return 1;

  file_stream_close(allocator, astream);
  remove(test_file);
  return 0;
});

  #endif
#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_FILES_C (1)
#endif
#if defined(MY_FILES_C)
  #include "../assertMessage.h"
  #include "../macros.h"
  #include <string.h>

static const char *file_errors[] = {
    [0] = "ok",
    [1] = "READ",
    [2] = "WRITE",
    [3] = "SEEK",
    [4] = "IN_ERRNO",
    [5] = "NOT_READABLE",
    [6] = "NOT_WRITABLE",
};

  #if __has_include(<unistd.h>) && !__has_include(<windows.h>)
    // { posix
    #include <fcntl.h>
    #include <unistd.h>

struct file_stream {
  small_stream_vt vt[1];
  int fileno;
  usize file_len;
  usize curr_pos;
  fileopenflags flags;
};

sstream_c8 _file_sstream_getc(sstream stream) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.read) return (sstream_c8){.err = 5};
  c8 result;
  var_ rr = read(ptr->fileno, &result, 1);
  if (rr == 1) {
    ptr->curr_pos += 1;
    return (sstream_c8){.result = result};
  }
  if (!rr) return (sstream_c8){.err = 1}; // EOF
  return (sstream_c8){.err = 4};
}
sstream_size _file_sstream_gets(sstream stream, fptr buffer) {
  assertMessage(buffer.len);
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.read) return (sstream_size){.err = 5};
  var_ rr = read(ptr->fileno, buffer.ptr, buffer.len);
  if (rr >= 0) {
    ptr->curr_pos += (usize)rr;
    return (sstream_size){.result = (usize)rr};
  }
  return (sstream_size){.err = 4};
}

sstream_errn _file_sstream_putc(sstream stream, u8 c) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.write) return 6;
  var_ rr = write(ptr->fileno, &c, 1);
  if (rr == 1) {
    if (ptr->flags.mode.append) {
      ptr->file_len += 1;
      ptr->curr_pos = ptr->file_len;
    } else {
      ptr->curr_pos += 1;
      if (ptr->curr_pos > ptr->file_len) ptr->file_len = ptr->curr_pos;
    }
    return 0;
  }
  return 3;
}
sstream_errn _file_sstream_puts(sstream stream, fptr buffer) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.write) return 6;
  var_ rr = write(ptr->fileno, buffer.ptr, buffer.len);
  if (rr >= 0) {
    if (ptr->flags.mode.append) {
      ptr->file_len += rr;
      ptr->curr_pos = ptr->file_len;
    } else {
      ptr->curr_pos += rr;
      if (ptr->curr_pos > ptr->file_len) ptr->file_len = ptr->curr_pos;
    }
    if (rr == (ssize_t)buffer.len) return 0;
  }
  return 4;
}

sstream_size _file_sstream_tell(sstream stream) {
  struct file_stream *ptr = (typeof(ptr))stream;
  var_ rr = lseek(ptr->fileno, 0, SEEK_CUR);
  assertMessage(rr == ptr->curr_pos, "%i != %i", (int)rr, (int)ptr->curr_pos);
  if (rr >= 0) return (sstream_size){.result = (usize)rr};
  return (sstream_size){.err = 4};
}

sstream_errn _file_sstream_seek(sstream stream, usize pos) {
  struct file_stream *ptr = (typeof(ptr))stream;
  var_ rr = lseek(ptr->fileno, (off_t)pos, SEEK_SET);
  if (rr >= 0) {
    ptr->curr_pos = pos;
    return 0;
  }
  return 4;
}

const char *_file_sstream_efromint(sstream stream, sstream_errn err) {
  (void)stream;
  if (err < countof(file_errors)) return file_errors[err];
  unreachable();
}

const small_stream_vt _file_vt = {
    .getc = _file_sstream_getc,
    .gets = _file_sstream_gets,
    .putc = _file_sstream_putc,
    .puts = _file_sstream_puts,
    .tell = _file_sstream_tell,
    .seek = _file_sstream_seek,
    .tell_max = NULL,
    .efromint = _file_sstream_efromint,
};

sstream file_stream_open(AllocatorV allocator, const char *const path, fileopenflags flags) {
  int flagint = 0;

  if (flags.access.read && flags.access.write) flagint |= O_RDWR;
  else if (flags.access.read) flagint |= O_RDONLY;
  else if (flags.access.write) flagint |= O_WRONLY;

  if (flags.mode.append) flagint |= O_APPEND;
  if (flags.mode.create) flagint |= O_CREAT;
  if (flags.mode.truncate) flagint |= O_TRUNC;
  if (flags.mode.doesnt_exist) flagint |= O_EXCL;

  if (flags.status.nonblock) flagint |= O_NONBLOCK;
  if (flags.status.sync) flagint |= O_SYNC; // buffered

  int fd = flags.mode.create ? open(path, flagint, 0666) : open(path, flagint);
  if (fd < 0) return nullptr;
  struct file_stream *result = aCreate(allocator, struct file_stream);
  memcpy(result[0].vt, &_file_vt, sizeof(_file_vt));
  result[0].vt->tell_max = &result[0].file_len;
  result[0].fileno = fd;
  result[0].flags = flags;
  result[0].file_len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  result->flags = flags;

  return (sstream)result;
}

void file_stream_close(AllocatorV allocator, sstream stream) {
  struct file_stream *meta = (typeof(meta))stream;
  close(meta->fileno);
  aFree(allocator, meta, sizeof(*meta));
}

  // }
  #elif __has_include(<windows.h>)
    //{ windows
    #include <windows.h>

struct file_stream {
  small_stream_vt vt[1];
  HANDLE handle;
  usize file_len;
  fileopenflags flags;
};

sstream_c8 _file_sstream_getc(sstream stream) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.read) return (sstream_c8){.err = 5};
  c8 result;
  DWORD rr;
  if (ReadFile(ptr->handle, &result, 1, &rr, NULL)) {
    if (rr == 1) return (sstream_c8){.result = result};
    if (!rr) return (sstream_c8){.err = 1}; // EOF
  }
  return (sstream_c8){.err = 3};
}

sstream_size _file_sstream_gets(sstream stream, fptr buffer) {
  assertMessage(buffer.len);
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.read) return (sstream_size){.err = 5};
  DWORD rr;
  if (ReadFile(ptr->handle, buffer.ptr, (DWORD)buffer.len, &rr, NULL)) {
    return (sstream_size){.result = (usize)rr};
  }
  return (sstream_size){.err = 3};
}

sstream_errn _file_sstream_putc(sstream stream, u8 c) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.write) return 6;
  DWORD rr;
  if (WriteFile(ptr->handle, &c, 1, &rr, NULL)) {
    if (rr == 1) {
      if (ptr->flags.mode.append) {
        ptr->file_len += 1;
      } else {
        LARGE_INTEGER move = {0}, pos;
        if (SetFilePointerEx(ptr->handle, move, &pos, FILE_CURRENT)) {
          if ((usize)pos.QuadPart > ptr->file_len) ptr->file_len = (usize)pos.QuadPart;
        }
      }
      return 0;
    }
  }
  return 3;
}

sstream_errn _file_sstream_puts(sstream stream, fptr buffer) {
  struct file_stream *ptr = (typeof(ptr))stream;
  if (!ptr->flags.access.write) return 6;
  DWORD rr;
  if (WriteFile(ptr->handle, buffer.ptr, (DWORD)buffer.len, &rr, NULL)) {
    if (rr > 0) {
      if (ptr->flags.mode.append) {
        ptr->file_len += rr;
      } else {
        LARGE_INTEGER move = {0}, pos;
        if (SetFilePointerEx(ptr->handle, move, &pos, FILE_CURRENT)) {
          if ((usize)pos.QuadPart > ptr->file_len) ptr->file_len = (usize)pos.QuadPart;
        }
      }
    }
    if (rr == (DWORD)buffer.len) return 0;
  }
  return 3;
}

sstream_size _file_sstream_tell(sstream stream) {
  struct file_stream *ptr = (typeof(ptr))stream;
  LARGE_INTEGER move = {0};
  LARGE_INTEGER new_pos;
  if (SetFilePointerEx(ptr->handle, move, &new_pos, FILE_CURRENT)) {
    return (sstream_size){.result = (usize)new_pos.QuadPart};
  }
  return (sstream_size){.err = 3};
}

sstream_errn _file_sstream_seek(sstream stream, usize pos) {
  struct file_stream *ptr = (typeof(ptr))stream;
  LARGE_INTEGER move;
  move.QuadPart = pos;
  if (SetFilePointerEx(ptr->handle, move, NULL, FILE_BEGIN)) return 0;
  return 3;
}

const char *_file_sstream_efromint(sstream stream, sstream_errn err) {
  (void)stream;
  if (err < countof(file_errors)) return file_errors[err];
  unreachable();
}

const small_stream_vt _file_vt = {
    .getc = _file_sstream_getc,
    .gets = _file_sstream_gets,
    .putc = _file_sstream_putc,
    .puts = _file_sstream_puts,
    .tell = _file_sstream_tell,
    .seek = _file_sstream_seek,
    .tell_max = NULL,
    .efromint = _file_sstream_efromint,
};

sstream file_stream_open(AllocatorV allocator, const char *const path, fileopenflags flags) {
  DWORD access = 0;
  if (flags.access.read) access |= GENERIC_READ;
  if (flags.access.write) {
    if (flags.mode.append) {
      // Exclude FILE_WRITE_DATA to force strict append behavior, even if read is enabled
      access |= (FILE_GENERIC_WRITE & ~FILE_WRITE_DATA) | FILE_APPEND_DATA;
    } else {
      access |= GENERIC_WRITE;
    }
  }

  DWORD creation = OPEN_EXISTING;
  if (flags.mode.create && flags.mode.doesnt_exist) creation = CREATE_NEW;
  else if (flags.mode.create && flags.mode.truncate) creation = CREATE_ALWAYS;
  else if (flags.mode.create) creation = OPEN_ALWAYS;
  else if (flags.mode.truncate) creation = TRUNCATE_EXISTING;

  DWORD attributes = FILE_ATTRIBUTE_NORMAL;
  if (flags.status.sync) attributes |= FILE_FLAG_WRITE_THROUGH;

  HANDLE handle = CreateFileA(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creation, attributes, NULL);

  if (handle != INVALID_HANDLE_VALUE && flags.mode.append && (access & FILE_APPEND_DATA)) {
    SetFilePointer(handle, 0, NULL, FILE_END);
  }
  if (handle == INVALID_HANDLE_VALUE) return nullptr;

  struct file_stream *result = aCreate(allocator, struct file_stream);
  memcpy(result[0].vt, &_file_vt, sizeof(_file_vt));
  result[0].vt->tell_max = &result[0].file_len;
  result[0].handle = handle;
  result[0].flags = flags;

  LARGE_INTEGER file_size;
  if (GetFileSizeEx(handle, &file_size)) {
    result[0].file_len = (usize)file_size.QuadPart;
  } else {
    result[0].file_len = 0;
  }

  return (sstream)result;
}

void file_stream_close(AllocatorV allocator, sstream stream) {
  struct file_stream *meta = (typeof(meta))stream;
  CloseHandle(meta->handle);
  aFree(allocator, meta, sizeof(*meta));
}

  //}
  #else
    #error no smallfile implementation for this os
  #endif
#endif
