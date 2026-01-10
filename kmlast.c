#include "my-list.h"
#include "print.h"
#include <dlgs.h>

struct vason_string {
  usize len, offset;
};
typedef struct vason_object {
  enum : u8 {
    vason_STR = 0b00,
    vason_LST = 0b01,
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
  STR_start,
  STR_end,
  SKIP,
  STR_continue,
  ESCAPE = '/',
  ARR_start = '[',
  ARR_end = ']',
  ARR_delim = ',',
  OBJ_start = '{',
  OBJ_end = '}',
  OBJ_delim_0 = ':',
  OBJ_delim_1 = ';',
} vason_token;

vason_token *vason_tokenize(AllocatorV allocator, fptr string) {
  vason_token *t = aAlloc(allocator, sizeof(vason_token) * string.width);
  for (usize i = 0; i < string.width; i++) {
    t[i] = string.ptr[i];
    switch (t[i]) {
      case ESCAPE:
      case ARR_start:
      case ARR_end:
      case ARR_delim:
      case OBJ_start:
      case OBJ_end:
      case OBJ_delim_0:
      case OBJ_delim_1:
        break;
      default: {
        t[i] = STR_start;
      }
    }
  }
  return t;
}
void vason_handleEscape(fptr string, vason_token *t) {
  for (usize i = 0; i < string.width; i++) {
    if (t[i] == ESCAPE) {
      t[i] = SKIP;
      if (i + 1 < string.width) {
        t[i + 1] = STR_start;
        i++;
      }
    }
  }
}
void vason_combineStrings(fptr string, vason_token *t) {
  for (usize i = 0; i < string.width; i++) {
    if (t[i] == STR_start) {
      usize j = i + 1;
      while (j < string.width && t[j] == STR_start || t[j] == SKIP) {
        t[j] = STR_continue;
        j++;
      }
      t[j - 1] = STR_end;
      i = j;
      continue;
    }
  }

  for (usize i = 0; i < string.width; i++) {
    switch (t[i]) {
      case STR_start: {
        if (vason_skip(string.ptr[i])) {
          t[i] = SKIP;
          if (i + 1 < string.width && vason_skip(t[i + 1])) {
            if (t[i + 1] == STR_continue) {
              t[i + 1] = STR_start;
            } else if (t[i + 1] == STR_end) {
              t[i + 1] = STR_start;
            }
          }
        }
      } break;
      case STR_end: {
        if (vason_skip(string.ptr[i])) {
          t[i] = SKIP;
          if (i - 1 != ((usize)-1) && vason_skip(t[i - 1])) {
            if (t[i - 1] == STR_continue) {
              t[i - 1] = STR_end;
            }
            i -= 2;
          }
        }
      } break;
      default:;
    }
  }
}

struct vason_result {
  vason_object object;
  List *strings;
  List *objects;
  AllocatorV allocator;
};

nullable(usize) vason_list_count(vason_token *t, usize place, usize len) {
  nullable(usize) res;
  return res;
}

vason_object vason_parse_list(struct vason_result pools, fptr string, vason_token *t, usize place) {
  vason_object res = (vason_object){vason_LST};
}
struct vason_result vason_parse(AllocatorV allocator, fptr string, vason_token *t) {
  struct vason_result res = (struct vason_result){
      .object = (vason_object){0},
      .objects = mList(allocator, struct vason_object),
      .strings = mList(allocator, struct vason_string),
      .allocator = allocator,
  };
  List_resize(res.strings, 10);
  List_resize(res.objects, 10);
  usize place = 0;
  while (place < string.width) {
    switch (string.ptr[place]) {
      case ARR_start: {
        place++;
        res.object = vason_parse_list(res, string, t, place);
      } break;
      case OBJ_start: {
        place++;
        assertMessage(false, "unimplemented");
      } break;
    }
    place++;
  }
  return res;
}
