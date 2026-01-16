#ifndef VASON_PARSER_H
#define VASON_PARSER_H
#include "allocator.h"
#include "my-list.h"
#include "print.h"
#include "types.h"
#include <string.h>
#include <time.h>

typedef struct vason_span {
  u32 len, offset;
} vason_span;

typedef enum : u8 {
  vason_STR = 1 << 1,
  vason_ARR = 1 << 2,
  vason_MAP = 1 << 3,
  vason_ID = 1 << 4,
  vason_INVALID = 1 << 5,
} vason_tag;

/**
 * data based on `tag`
 * `span`:
 *  `vason_STR`: len&offset inside of vason_container.string
 *  `vason_ARR`: len&offset inside of vason_contianer.objects
 *  `vason_ID` : len&offset inside of vason_container.string
 *       - only within a map @ the even indexes
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

// TODO
typedef struct vason_lazyObject {
  vason_tag tag;
  bool parsed : 1;
  struct vason_span span;
} vason_lazyObject;
typedef struct vason_lazyContainer {
  slice(c8) string;
  mList(vason_object) objects;
  vason_object top;
} vason_lazyContainer;

static void vason_formatter_print(
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

// just control characters for now
bool vason_skip(char c) { return c < 33 && c != 0; }
typedef enum : c8 {
  STR_,
  STR_start = '`',
  STR_end = '\'',
  ESCAPE = '/',
  // ESCAPE = '\\', // handeled in tokenizer
  ARR_start = '[',
  ARR_end = ']',
  ARR_delim = ',',
  MAP_start = '{',
  MAP_end = '}',
  MAP_delim_ID = ':',
  MAP_delim = ';',
} vason_token;
REGISTER_PRINTER(vason_token, {
  switch (in) {
    case STR_:
      PUTC(L's');
      break;
    case STR_start:
      PUTC(L'X');
      break;
    case STR_end:
      PUTC(L'x');
      break;
    case ESCAPE:
      PUTC('_');
      break;
    case ARR_start:
      PUTC('[');
      break;
    case ARR_end:
      PUTC(']');
      break;
    case ARR_delim:
      PUTC(',');
      break;
    case MAP_start:
      PUTC('{');
      break;
    case MAP_end:
      PUTC('}');
      break;
    case MAP_delim_ID:
      PUTC(':');
      break;
    case MAP_delim:
      PUTC(';');
      break;
  }
});

slice(vason_token) vason_tokenize(AllocatorV allocator, slice(c8) string) {
  slice(vason_token) t = {
      string.len,
      aCreate(allocator, vason_token, string.len)
  };
  for (usize i = 0; i < t.len; i++) {
    t.ptr[i] = string.ptr[i];
    switch (t.ptr[i]) {
      case ESCAPE:
      case STR_start:
      case STR_end:
      case ARR_start:
      case ARR_end:
      case ARR_delim:
      case MAP_start:
      case MAP_end:
      case MAP_delim_ID:
      case MAP_delim:
        break;
      case '\\': {
        t.ptr[i] = ESCAPE;
      } break;
      default: {
        t.ptr[i] = STR_;
      }
    }
  }
  // escape escape'd characters
  for (typeof(t.len) i = 0; i + 1 < t.len; i++)
    if (t.ptr[i] == ESCAPE) {
      t.ptr[i] = STR_;
      t.ptr[i + 1] = STR_;
      i++;
    }
  // force strings inside delimiters
  for (typeof(t.len) i = 0; i + 1 < t.len; i++) {
    if (t.ptr[i] == STR_start) {
      i++;
      while (i < t.len && t.ptr[i] != STR_end) {
        t.ptr[i] = STR_;
        i++;
      }
    } else if (t.ptr[i] == STR_) {
      usize j = i;
      while (j < t.len && t.ptr[j] == STR_)
        j++;
      switch (t.ptr[j]) {
        case MAP_end:
        case MAP_start:
        case ARR_start:
        case STR_start: {
          for (usize ii = i; ii < j; ii++)
            t.ptr[ii] = ESCAPE;

        } break;
        case STR_:
        case STR_end:
        case ESCAPE:
        case ARR_end:
        case ARR_delim:
        case MAP_delim_ID:
        case MAP_delim:
          break;
      }
      i = j;
    }
  }
  // force strings inside delimiters
  for (ssize i = t.len; i > 0; i--) {
    if (t.ptr[i - 1] == STR_) {
      usize j = i;
      while (j > 0 && t.ptr[j - 1] == STR_)
        j--;
      if (j == 0)
        continue;
      switch (t.ptr[j - 1]) {
        case ARR_end:
        case MAP_end: {
          for (usize ii = j; ii < i - 1; ii++)
            t.ptr[ii] = ESCAPE;

        } break;
        case STR_start:
        case STR_:
        case STR_end:
        case MAP_start:
        case ARR_start:
        case ESCAPE:
        case ARR_delim:
        case MAP_delim_ID:
        case MAP_delim:
          break;
      }
      i = j;
    }
  }
  // remove strings after end of delimiter
  for (typeof(t.len) i = 0; i < t.len; i++) {
    if (t.ptr[i] == STR_end) {
      i++;
      while (i < t.len && t.ptr[i] == STR_)
        t.ptr[i] = ESCAPE;
    }
  }
  // remove spaces after strings
  for (ssize i = t.len; i > 0; i--) {
    while (i > 0 && t.ptr[i - 1] == STR_ && isSkip(string.ptr[i - 1])) {
      t.ptr[i - 1] = ESCAPE;
      i--;
      continue;
    }
    while (i > 0 && t.ptr[i - 1] == STR_)
      i--;
  }
  // remove spaces before strings
  for (typeof(t.len) i = 0; i < t.len; i++) {
    if (t.ptr[i] == STR_start) {
      while (t.ptr[i] != STR_end)
        i++;
      continue;
    }
    while (i < t.len && t.ptr[i] == STR_ && isSkip(string.ptr[i])) {
      t.ptr[i] = ESCAPE;
      i++;
    }
    while (i < t.len && t.ptr[i] == STR_)
      i++;
  }
  // discard delimiters
  for (typeof(t.len) i = 0; i < t.len; i++) {
    switch (t.ptr[i]) {
      case STR_start:
      case STR_end:
        t.ptr[i] = ESCAPE;
        break;
      default:
        break;
    }
  }
  for (typeof(t.len) i = 0; i < t.len; i++) {
    print("{vason_token}", t.ptr[i]);
  }
  println();
  return t;
}
// ! assumes query is on the stack list
nullable(usize) find_obj_end_heap(mList(vason_token) stack, usize start, slice(vason_token) t) {
  while (start < t.len && mList_len(stack)) {
    switch (t.ptr[start]) {
      case ARR_start:
      case MAP_start: {
        mList_push(stack, t.ptr[start]);
      } break;
      case MAP_end:
      case ARR_end: {
        vason_token it = mList_pop(stack);
        if (
            (it == ARR_start && t.ptr[start] == ARR_end) ||
            (it == MAP_start && t.ptr[start] == MAP_end)
        ) {
          break;
        } else {
          return nullable_null(usize);
        }
      } break;
      default:
        break;
    }
    start++;
  }

  if (!mList_len(stack))
    return nullable_real(usize, start);
  else
    return nullable_null(usize);
}
struct breakdown_return {
  vason_tag kind;
  slice(vason_level) items;
};
struct breakdown_return breakdown(usize start, slice(vason_token) t, AllocatorV allocator, slice(c8) str) {
  List level;
  mList_scoped(vason_token) endList = mList_init(allocator, vason_token, 10);
  List_makeNew(allocator, &level, sizeof(vason_level), 5);
  bool malofrmed = false;
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

  mList_push(endList, t.ptr[start - 1]);
  nullable(usize) end_nullable = find_obj_end_heap(endList, start, t);
  if (end_nullable.isnull)
    goto early_return;
  usize end = end_nullable.data;

  usize i = start;
  while (i < end) {
    switch (t.ptr[i]) {
      case STR_: {
        usize next = i + 1;
        while (next < end && t.ptr[next] == STR_)
          next++;

        vason_level stritem = {
            .kind = vason_STR,
            .pos = {
                .len = next - i,
                .offset = i,
            },
        };
        if (mode == vason_MAP)
          stritem.kind = vason_STR;
        List_append(&level, &stritem);
        i = next;
      } break;
      case ARR_start:
      case MAP_start: {
        mList_push(endList, t.ptr[i]);
        nullable(usize) next_nullable = find_obj_end_heap(endList, i + 1, t);
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

      case MAP_delim_ID: {
        if (mode != vason_MAP)
          goto early_return;
        i++;
      } break;
      case ARR_delim:
      case MAP_delim: {
        if (mode == vason_ARR) {
          i++;
          break;
        }
        i++;
      } break;
      default:
        i++;
        break;
    }
  }
  goto normal_return;
  // clang-format off
  early_return:
  malofrmed = true;
  normal_return:
  // clang-format on

  // try to make maps that dont make sense get processed
  if (mode == vason_MAP && level.length) {
    typedef enum { key,
                   value,
                   unknown } itemkind;
    slice(itemkind) items = (slice(itemkind)){
        level.length,
        aCreate(allocator, itemkind, level.length),
    };
    // all objects are values
    for (usize i = 0; i < level.length; i++) {
      switch ((*(vason_level *)(List_getRef(&level, i))).kind) {
        case vason_ARR:
        case vason_MAP:
          items.ptr[i] = value;
          break;
        default:
          items.ptr[i] = unknown;
          break;
      }
    }
    // value preceded by value is unknown
    for (usize i = 1; i < level.length; i++) {
      if (items.ptr[i] == items.ptr[i - 1] && items.ptr[i] == value)
        items.ptr[i] = unknown;
    }
    // first item is key if its a string
    if ((*(vason_level *)(List_getRef(&level, 0))).kind == vason_STR) {
      items.ptr[0] = key;
    };
    // all values preceded by key
    for (usize i = 1; i < items.len; i++)
      if (items.ptr[i] == value)
        items.ptr[i - 1] = key;
    // unknown preceded by key is a value
    for (usize i = 0; i + 1 < items.len; i++)
      if (items.ptr[i] == key)
        if (items.ptr[i + 1] == unknown)
          items.ptr[i + 1] = value;
    // two unkowns after a value are a key-value pair
    for (usize i = 0; i + 2 < items.len; i++)
      if (items.ptr[i] == value)
        if (items.ptr[i + 1] == unknown && items.ptr[i + 2] == unknown) {
          items.ptr[i + 1] = key;
          items.ptr[i + 2] = value;
        }
    // all others removed
    for (ssize i = (ssize)items.len - 1; i >= 0; i--) {
      if (items.ptr[i] == unknown) {
        List_remove(&level, (usize)i);
      } else if (items.ptr[i] == key) {
        vason_level *node = (vason_level *)(List_getRef(&level, (usize)i));
        node->kind = vason_ID;
      }
    }

    if (List_length(&level) % 2)
      level.length--;

    aFree(allocator, items.ptr);
  }
  List_forceResize(&level, level.length);
  slice(vason_level) listMemory =
      (slice(vason_level)){
          level.length,
          (vason_level *)level.head,
      };
  return (struct breakdown_return){
      mode | (malofrmed ? vason_INVALID : 0),
      listMemory
  };
}

vason_object bdconvert(
    mList(vason_object) containerList,
    struct breakdown_return bdr,
    // recursion for bdr
    slice(c8) str,
    slice(vason_token) t,
    AllocatorV allocator
) {
  // TODO deal with invalid types
  bool isvalid = !(bdr.kind & vason_INVALID);
  bdr.kind &= ~vason_INVALID;
  vason_object res = {.tag = bdr.kind};
  if (!isvalid)
    goto invalidLabel;
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
      // if (bdr.kind == vason_MAP)
      //   res.span.len = (res.span.len / 2) * 2;
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
  invalidLabel:
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
