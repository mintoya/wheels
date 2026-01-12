#ifndef VASON_PARSER_H
#define VASON_PARSER_H
#include "allocator.h"
#include "my-list.h"
#include "print.h"
#include "types.h"
#include <string.h>
struct vason_string {
  usize len, offset;
};
typedef enum : u8 {
  vason_STR,
  vason_ARR,
  vason_MAP,
  vason_ID, // not valid outside of parser
} vason_tag;
typedef struct vason_object {
  vason_tag tag;
  union {
    struct vason_string string;
    struct {
      usize len;
      struct vason_object *array;
    } list;
    struct {
      usize len;
      struct vason_string *names;
      struct vason_object *array;
    } object;
  } data;
} vason_object;
[[gnu::weak]]
void kmlFormatPrinter(
    const wchar *data,
    void *arb,
    unsigned int length,
    bool flush
) {
  static uint indentLevel = 0;

  for (size_t index = 0; index < length; index++) {
    wchar character = data[index];
    {
      switch (character) {
        case '{':
        case '[': {
          putwchar(character);
          putwchar('\n');
          indentLevel++;
          for (int i = 0; i < indentLevel; i++) {
            putwchar(' ');
          }
        } break;
        case '}':
        case ']': {
          putwchar('\033');
          putwchar('[');
          putwchar('1');
          putwchar('D');
          putwchar(character);
          putwchar('\n');
          indentLevel--;
          for (int i = 0; i < indentLevel; i++) {
            putwchar(' ');
          }
        } break;
        case ';':
        case ',': {
          putwchar(character);
          putwchar('\n');
          for (int i = 0; i < indentLevel; i++) {
            putwchar(' ');
          }
        } break;
        default:
          putwchar(character);
      }
    }
  }
  if (flush)
    indentLevel = 0;
}
typedef struct vason_container {
  vason_object top;
  const slice(u8) string;
  List *strings;
  List *objects;
} vason_contianer;

vason_contianer parseStr(AllocatorV allocator, slice(u8) string);

#endif // VASON_PARSER_H
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define VASON_PARSER_C (1)
#endif
#ifdef VASON_PARSER_C
typedef struct vason_level {
  vason_tag kind;
  struct vason_string pos;
} vason_level;
struct breakdown_return {
  vason_tag kind;
  bool malformed : 1;
  struct slice_vason_level {
    usize len;
    vason_level *ptr;
  } items;
};

// just control characters for now
bool vason_skip(char c) {
  return c < 33 && c != 0;
}

typedef enum : u8 {
  STR_,
  ESCAPE = '/',
  ARR_start = '[',
  ARR_end = ']',
  ARR_delim = ',',
  MAP_start = '{',
  MAP_end = '}',
  MAP_delim_ID = ':',
  MAP_delim = ';',
} vason_token;

slice(vason_token) vason_tokenize(AllocatorV allocator, slice(u8) string);
// typedef typeof(vason_tokenize(NULL, nullslice(u8))) tokenSlice;
slice(vason_token) vason_tokenize(AllocatorV allocator, slice(u8) string) {
  slice(vason_token) t = {
      string.len,
      aCreate(allocator, vason_token, string.len)
  };
  for (usize i = 0; i < t.len; i++) {
    t.ptr[i] = string.ptr[i];
    switch (t.ptr[i]) {
      case ESCAPE:
      case ARR_start:
      case ARR_end:
      case ARR_delim:
      case MAP_start:
      case MAP_end:
      case MAP_delim_ID:
      case MAP_delim:
        break;
      default: {
        t.ptr[i] = STR_;
      }
    }
  }
  return t;
}
void vason_addEscapes(slice(vason_token) t) {
  if (!t.len)
    return;
  for (auto i = 0; i < t.len - 1; i++)
    if (t.ptr[i] == ESCAPE) {
      t.ptr[i] = STR_;
      t.ptr[i + 1] = STR_;
    }
}
// returns end of id
bool isrid(vason_token t) {
  switch (t) {
    case ARR_start:
    case ARR_end:
    case MAP_start:
    case MAP_end: {
      return true;
    } break;
    default: {
      return false;
    } break;
  }
}
nullable(usize) stack_resolve_breakdown(AllocatorV, usize start, slice(vason_token) t, vason_token query) {
  switch (query) {
    case ARR_end:
    case MAP_end: {
      for (usize i = start; i < t.len; i++) {
        if (!isrid(t.ptr[i]))
          continue;

        if (t.ptr[i] == query)
          return nullable_real(usize, i);

        switch (t.ptr[i]) {
          case ARR_start: {
            nullable(usize) next = stack_resolve_breakdown(NULL, i + 1, t, ARR_end);
            if (next.isnull)
              return nullable_null(usize);
            i = next.data;
          } break;
          case MAP_start: {
            nullable(usize) next = stack_resolve_breakdown(NULL, i + 1, t, MAP_end);
            if (next.isnull)
              return nullable_null(usize);
            i = next.data;
          } break;
          default:
            return nullable_null(usize);
        }
      }
      return nullable_null(usize);
    }
    default:
      assertMessage(false, "cant search for that");
  }
  return nullable_null(usize);
}

struct breakdown_return breakdown(usize start, slice(vason_token) t, AllocatorV allocator, slice(u8) str) {
  List level;
  List_makeNew(allocator, &level, sizeof(vason_level), 5);
  enum { vason_ARR = vason_ARR,
         vason_MAP = vason_MAP
  } mode = {};

  // clang-format off
  for (; start < t.len; start++) {
    switch (t.ptr[start]) {
      case ARR_start: mode = vason_ARR; goto end_loop;
      case MAP_start: mode = vason_MAP; goto end_loop;
      default: break;
    }
  }
  goto early_return;
  end_loop:
  // clang-format on

  start++;

  nullable(usize) end_nullable = stack_resolve_breakdown(
      allocator,
      start,
      t,
      mode == vason_ARR ? ARR_end : MAP_end
  );
  if (end_nullable.isnull)
    goto early_return;
  usize end = end_nullable.data;

  usize i = start;
  enum { index,
         value,
  } mapMode = index;
  while (i < end) {
    switch (t.ptr[i]) {
      case STR_: {
        usize next = i + 1;

        // clang-format off
        while_label:
        // clang-format on
        while (next < end && t.ptr[next] == STR_)
          next++;

        if (
            t.ptr[next] == MAP_start ||
            t.ptr[next] == ARR_start
        ) {
          i = next;
          break;
        }

        usize istart = i;
        usize inext = next;
        while (istart < inext && vason_skip(str.ptr[istart]))
          istart++;
        while (inext > istart && vason_skip(str.ptr[inext - 1]))
          inext--;

        vason_level stritem = {
            .kind = vason_STR,
            .pos = {
                .len = inext - istart,
                .offset = istart,
            },
        };
        if (mode == vason_MAP) {
          stritem.kind = mapMode == value ? vason_STR : vason_ID;
        }
        if (mode == vason_ARR || (inext != istart))
          List_append(&level, &stritem);
        i = next;
      } break;

      case ARR_start:
      case MAP_start: {
        nullable(usize) next_nullable = stack_resolve_breakdown(allocator, i + 1, t, t.ptr[i] == ARR_start ? ARR_end : MAP_end);
        if (next_nullable.isnull)
          goto early_return;
        usize next = next_nullable.data;

        List_append(
            &level,
            &(vason_level){
                .kind = t.ptr[i] == ARR_start ? vason_ARR : vason_MAP,
                .pos = {
                    .len = next - i + 1,
                    .offset = i,
                },
            }
        );
        i = next + 1;
      } break;

      case ARR_delim:
        if (mode != vason_ARR)
          goto early_return;
        i++;
        break;
      case MAP_delim_ID: {
        if (mode != vason_MAP)
          goto early_return;
        if (mapMode != index)
          goto early_return;
        mapMode = value;
        i++;
      } break;
      case MAP_delim: {
        if (mode != vason_MAP)
          goto early_return;
        if (mapMode != value)
          goto early_return;
        mapMode = index;
        i++;
      } break;
      default:
        i++;
        break;
    }
  }
  // clang-format off
  bool malofrmed = false;
  if(false){
  early_return:
    malofrmed = true;
  }
  // clang-format on
  List_forceResize(&level, level.length);
  slice(vason_level) res = {level.length, (vason_level *)level.head};
  return (struct breakdown_return){mode, malofrmed, res};
}

vason_contianer vason_new(
    AllocatorV allocator,
    slice(u8) string
) {
  auto res = (vason_contianer){
      .string = string,
      .strings = mList(allocator, struct vason_string),
      .objects = mList(allocator, vason_object),
  };
  return res;
}
void vason_free(vason_contianer c) {
  List_free(c.objects);
  List_free(c.strings);
}
vason_object vason_decender(
    AllocatorV allocator,
    vason_contianer container,
    slice(vason_token) t,
    slice(u8) string,
    usize position
) {
  auto bd = breakdown(position, t, allocator, string);

  vason_object this;
  if (bd.malformed) {
    this.tag = bd.kind;
    memset(&this.data, 0, sizeof(this.data));
    goto early_return;
  }
  if (bd.kind == vason_MAP) {
    this = (typeof(this)){
        .tag = vason_MAP,
        .data = {
            .object = {
                .array = List_appendFromArr(container.objects, NULL, bd.items.len / 2),
                .names = List_appendFromArr(container.strings, NULL, bd.items.len / 2),
                .len = bd.items.len / 2,
            }
        }
    };
    for (usize i = 0; i < bd.items.len / 2; i++) {
      this.data.object.names[i] = bd.items.ptr[i * 2].pos;
      switch (bd.items.ptr[i * 2 + 1].kind) {
        case vason_STR:
          this.data.object.array[i].tag = vason_STR;
          this.data.object.array[i].data.string = bd.items.ptr[i * 2 + 1].pos;
          break;
        default:
          usize pos = bd.items.ptr[i * 2 + 1].pos.offset;
          this.data.object.array[i] = vason_decender(allocator, container, t, string, pos);
          break;
      }
    }
  } else {
    this = (typeof(this)){
        .tag = vason_ARR,
        .data = {
            .list = {
                .len = bd.items.len,
                .array = List_appendFromArr(container.objects, NULL, bd.items.len),
            },
        }
    };
    for (usize i = 0; i < bd.items.len; i++) {
      switch (bd.items.ptr[i].kind) {
        case vason_STR:
          this.data.list.array[i].tag = vason_STR;
          this.data.list.array[i].data.string = bd.items.ptr[i].pos;
          break;
        default:
          usize pos = bd.items.ptr[i].pos.offset;
          this.data.list.array[i] = vason_decender(allocator, container, t, string, pos);
          break;
      }
    }
  }
  // clang-format off
  early_return:
  // clang-format on
  aFree(allocator, bd.items.ptr);
  return this;
}
vason_contianer parseStr(AllocatorV allocator, slice(u8) string) {
  auto t = vason_tokenize(allocator, string);
  vason_addEscapes(t);
  struct vason_container res = vason_new(allocator, string);
  res.top = vason_decender(allocator, res, t, string, 0);
  aFree(allocator, t.ptr);
  return res;
}

#endif
