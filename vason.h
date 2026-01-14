#ifndef VASON_PARSER_H
#define VASON_PARSER_H
#include "allocator.h"
#include "my-list.h"
#include "print.h"
#include "types.h"
#include <string.h>
#include <time.h>
struct vason_span {
  u32 len, offset;
};
typedef enum : u8 {
  vason_STR,
  vason_ARR,
  vason_MAP,
  vason_ID,
  vason_INVALID,
} vason_tag;

/**
 * data based on `tag`
 * `span`:
 *  `vason_STR`: len&offset inside of vason_container.string
 *  `vason_ARR`: len&offset inside of vason_contianer.objects
 *  `vason_ID` : len&offset inside of vason_container.string
 *       - only within a map
 *  `vason_MAP`: len&offset inside of vason_contianer.objects
 *       - len must be even,
 *       - even indexes: keys (`vason_ID`)
 *       - odd indexes: keys !(`vason_ID`)
 *  `vason_INVALID`: no data contained
 *       - result of failed map or array parse
 *       - cannot exist in place o f `vason_ID`
 */
typedef struct vason_object {
  vason_tag tag;
  struct vason_span span;
} vason_object;
typedef struct vason_container {
  slice(c8) string;
  slice(vason_object) objects;
  vason_object top;
} vason_contianer;

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

vason_contianer parseStr(AllocatorV allocator, slice(c8) string);

#endif // VASON_PARSER_H
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define VASON_PARSER_C (1)
#endif
#ifdef VASON_PARSER_C
typedef struct vason_level {
  vason_tag kind;
  struct vason_span pos;
} vason_level;
struct breakdown_return {
  vason_tag kind;
  struct slice_vason_level {
    usize len;
    vason_level *ptr;
  } items;
};

// just control characters for now
bool vason_skip(char c) { return c < 33 && c != 0; }

typedef enum : c8 {
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

slice(vason_token) vason_tokenize(AllocatorV allocator, slice(c8) string) {
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
  for (typeof(t.len) i = 0; i < t.len - 1; i++)
    if (t.ptr[i] == ESCAPE) {
      t.ptr[i] = STR_;
      t.ptr[i + 1] = STR_;
    }
  return t;
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

struct breakdown_return breakdown(usize start, slice(vason_token) t, AllocatorV allocator, slice(c8) str) {
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
        //while_label:
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
  return malofrmed ? (struct breakdown_return){vason_INVALID, res} : (struct breakdown_return){mode, res};
}

vason_object bdconvert(
    mList(vason_object) containerList,
    struct breakdown_return bdr,
    // recursion for bdr
    slice(c8) str,
    slice(vason_token) t,
    AllocatorV allocator
) {

  vason_object res = {.tag = bdr.kind};
  switch (bdr.kind) {
    case vason_ID:  // impossible for now
    case vason_STR: // impossible for now
    case vason_INVALID:
      break;
    case vason_MAP:
    case vason_ARR: {
      res.span = (typeof(res.span)){
          .offset = mList_len(containerList),
          .len = bdr.items.len,
      };
      if (bdr.kind == vason_MAP)
        res.span.len = (res.span.len / 2) * 2;
      List_appendFromArr((void *)containerList, NULL, res.span.len);
      for (usize i = 0; i < res.span.len; i++) {
        switch (bdr.items.ptr[i].kind) {
          case vason_ID:
          case vason_STR: {
            mList_set(
                containerList,
                i + res.span.offset,
                ((vason_object){
                    .tag = bdr.items.ptr[i].kind,
                    .span = bdr.items.ptr[i].pos,
                })
            );
          } break;
          case vason_MAP:
          case vason_ARR: {
            struct breakdown_return next = breakdown(bdr.items.ptr[i].pos.offset, t, allocator, str);
            vason_object inner = bdconvert(containerList, next, str, t, allocator);
            aFree(allocator, next.items.ptr);
            mList_set(containerList, i + res.span.offset, inner);
          } break;
          case vason_INVALID:
            break;
        }
      }
    } break;
  }
  // clang-format off
  // invalidLabel:
  // clang-format on
  return res;
}
vason_contianer parseStr(AllocatorV allocator, slice(c8) str) {
  slice(vason_token) t = vason_tokenize(allocator, str);
  mList(vason_object) bucket = mList_init(allocator, vason_object);

  struct breakdown_return bdr = breakdown(0, t, allocator, str);
  vason_object bdc = bdconvert(bucket, bdr, str, t, allocator);

  aFree(allocator, bdr.items.ptr);
  aFree(allocator, t.ptr);

  vason_contianer res = {
      .string = str,
      .objects = (slice(vason_object)){
          .ptr = (void *)((List *)bucket)->head,
          .len = mList_len(bucket),
      },
      .top = bdc,
  };
  aFree(allocator, bucket);
  return res;
}
#endif
