#include "my-list.h"
#include "print.h"
#include "types.h"
#include <stdio.h>

struct vason_string {
  usize len, offset;
};
typedef struct vason_object {
  enum : u8 {
    vason_STR = 0b00,
    vason_ARR = 0b01,
    vason_MAP = 0b10,
  } tag;
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
void vason_combineStrings(slice(u8) string, slice(vason_token) t) {
  for (usize i = 0; i < t.len; i++) {
    if (t.ptr[i] == ESCAPE) {
      t.ptr[i] = STR_;
      if (i + 1 < t.len) {
        t.ptr[i + 1] = STR_;
        i++;
      }
    }
  }
}
typedef struct vason_level {
  enum {
    vason_level_STR, // value , between , or :;
    vason_level_ID,  // right before an list|object(when inside object) between ,:
    vason_level_ARR, //
    vason_level_MAP, //
  } kind;
  struct vason_string pos;
} vason_level;

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
nullable(usize) stack_resolve_breakdown(usize start, slice(vason_token) t, vason_token query) {
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
            nullable(usize) next = stack_resolve_breakdown(i + 1, t, ARR_end);
            if (next.isnull)
              return nullable_null(usize);
            i = next.data;
          } break;
          case MAP_start: {
            nullable(usize) next = stack_resolve_breakdown(i + 1, t, MAP_end);
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
}

slice(vason_level) breakdown(usize start, slice(vason_token) t, AllocatorV allocator, slice(u8) str) {
  List level;
  List_makeNew(allocator, &level, sizeof(vason_level), 5);
  enum { arr,
         map,
         undefined } mode = undefined;

  for (usize i = start; i < t.len; i++) {
    if (t.ptr[i] == ARR_start) {
      mode = arr;
      start = i;
      break;
    }
    if (t.ptr[i] == MAP_start) {
      mode = map;
      start = i;
      break;
    }
  }
  assertMessage(mode != undefined, "couldnt find ");
  start++;

  nullable(usize) end_nullable = stack_resolve_breakdown(start, t, mode == arr ? ARR_end : MAP_end);
  assertMessage(!end_nullable.isnull);
  usize end = end_nullable.data;

  // println("starting at {}", start);
  // for (usize i = start; i < end; i++)
  //   print("{u8},", t.ptr[i]);
  // println();
  // for (usize i = start; i < end; i++)
  //   print("{char},", str.ptr[i]);
  // println();

  usize i = start;
  enum { index,
         value,
  } mapMode = index;
  while (i < end) {
    switch (t.ptr[i]) {
      case STR_: {
        usize next = i + 1;

        while (next < end && t.ptr[next] == STR_)
          next++;

        usize istart = i;
        usize inext = next;
        while (istart < inext && vason_skip(str.ptr[istart]))
          istart++;
        while (inext > istart && vason_skip(str.ptr[inext - 1]))
          inext--;

        vason_level stritem = {
            .kind = vason_level_STR,
            .pos = {
                .len = inext - istart,
                .offset = istart,
            },
        };
        if (mode == map) {
          stritem.kind = mapMode == value ? vason_level_STR : vason_level_ID;
        }
        if (mode == arr || (inext != istart))
          List_append(&level, &stritem);
        i = next;
      } break;

      case ARR_start:
      case MAP_start: {
        nullable(usize) next_nullable = stack_resolve_breakdown(
            i + 1, t,
            t.ptr[i] == ARR_start ? ARR_end : MAP_end
        );
        assertMessage(!next_nullable.isnull);
        usize next = next_nullable.data;

        List_append(
            &level,
            &(vason_level){
                .kind = t.ptr[i] == ARR_start ? vason_level_ARR : vason_level_MAP,
                .pos = {
                    .len = next - i + 1,
                    .offset = i,
                },
            }
        );
        i = next + 1;
      } break;

      case ARR_delim:
        assertMessage(mode == arr);
        i++;
        break;
      case MAP_delim_ID: {
        assertMessage(mode == map);
        assertMessage(mapMode == index);
        mapMode = value;
        i++;
      } break;
      case MAP_delim: {
        assertMessage(mode == map);
        assertMessage(mapMode == value);
        mapMode = index;
        i++;
      } break;
      default:
        i++;
        break;
    }
  }

  slice(vason_level) res = {level.length, (vason_level *)level.head};
  for (each_slice(res, e)) {
    switch (e[0].kind) {
        // clang-format off
      case vason_level_STR: print("S"); break;
      case vason_level_ID: print("I"); break;
      case vason_level_ARR: print("A"); break;
      case vason_level_MAP: print("M"); break;
        // clang-format on
    }
    println(
        ":{}.{} ({fptr})",
        e[0].pos.offset,
        e[0].pos.len,
        ((slice(u8)){e[0].pos.len, e[0].pos.offset + str.ptr})
    );
  }
  return res;
}

struct vason_result {
  vason_object object;
  List *strings;
  List *objects;
  AllocatorV allocator;
};
