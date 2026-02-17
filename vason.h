#ifndef VASON_PARSER_H
#define VASON_PARSER_H
#include "allocator.h"
#include "assertMessage.h"
#include "fptr.h"
#include "my-list.h"
#include "mytypes.h"
#include "omap.h"
#include "print.h"
#include "shortList.h"
#include "stringList.h"
#include <stdarg.h>
#include <stddefer.h>
#include <string.h>
#include <time.h>

typedef struct vason_span {
  u16 len, offset;
} vason_span;

/**
 * if an invalid item needs to be extracted, with bit operations
 */
typedef enum : u8 {
  vason_STR /*    */ = 1 << 0,
  vason_ARR /*    */ = 1 << 1,
  vason_MAP /*    */ = 1 << 2,
  vason_ID /*     */ = 1 << 3,
  vason_INVALID /**/ = 1 << 7,
  // vason_UNPARSED  = 1 << 6,
} vason_tag;
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
  // MAP_delim_ID = '=', // also handled
  MAP_delim = ';',
} vason_token;

/**
 * data based on `tag`
 * !the string is the same one passed into the parser,
 *  it doesnt edit it
 * `span`:
 *  `vason_STR`: len&offset inside of vason_container.string
 *  `vason_ARR`: len&offset inside of vason_container.objects
 *  `vason_ID` : len&offset inside of vason_container.string
 *       - only within a map @ the even indexes
 *  `vason_MAP`: len&offset inside of vason_container.objects
 *       - len must be even,
 *       - even indexes: keys (`vason_ID`)
 *       - odd indexes: keys !(`vason_ID`)
 *  `vason_INVALID`: no data contained
 *       - result of failed map or array parse
 *       - cannot exist in place o f `vason_ID`
 */
typedef struct {
  vason_tag tag;
  struct vason_span span;
} vason_object;
// object to make getting easier
// inneficient to have multiple of them for one parse
typedef struct {
  slice(c8) string;
  slice(vason_object) objects;
  vason_object top;
} vason_container;
slice(vason_object) vason_getChildren(vason_object obj, vason_container c);
fptr vason_getString(vason_object obj, vason_container c);
vason_object vason_mapGet(vason_object o, vason_container c, fptr k);
vason_object vason_arrGet(vason_object o, vason_container c, u32 key);
static inline vason_container vason_mapGetContainer(vason_container o, fptr k) {
  o.top = vason_mapGet(o.top, o, k);
  return o;
}
static inline vason_container vason_arrGetContainer(vason_container o, u32 k) {
  o.top = vason_arrGet(o.top, o, k);
  return o;
}
REGISTER_PRINTER(vason_container, {
  switch (in.top.tag) {
    case vason_ID:
    case vason_STR: {
      if (in.top.tag == vason_ID)
        PUTS(U"(:)");
      else
        PUTS(U"(S)");
      USENAMEDPRINTER("fptr", vason_getString(in.top, in));
    }; break;
    case vason_ARR: {
      PUTS(U"(A");
      USETYPEPRINTER(usize, (usize)(in.top.span.len));
      PUTS(U")[");
      for (usize i = 0; i < in.top.span.len; i++) {
        vason_object swap = in.objects.ptr[i + in.top.span.offset];
        vason_object origional = in.top;
        in.top = swap;
        USETYPEPRINTER(vason_container, in);
        in.top = origional;
      }
      PUTS(U"]");
    }; break;
    case vason_MAP: {
      PUTS(U"(M");
      USETYPEPRINTER(usize, (usize)(in.top.span.len / 2));
      PUTS(U"){");
      for (usize i = 0; i < in.top.span.len; i += 2) {
        vason_object origional = in.top;
        vason_object name = in.objects.ptr[i + in.top.span.offset];
        vason_object object = in.objects.ptr[i + in.top.span.offset + 1];
        in.top = name;
        USETYPEPRINTER(vason_container, in);
        in.top = object;
        USETYPEPRINTER(vason_container, in);
        in.top = origional;
      }
      PUTS(U"}");
    }; break;
    case vason_INVALID: {
      PUTS(U"(!)");
    }; break;
  }
});
typedef struct [[gnu::packed]] {
  vason_tag tag;
  union {
    stringList list[1];
    OMap map[1];
  };
} vason_live;
typedef struct [[gnu::packed]] {
  vason_tag tag;
  union {
    vason_live *next;
    u8 string[];
  };
} vason_ref;
REGISTER_SPECIAL_PRINTER("vason_live*", vason_live *, {
  if (!in) {
    PUTS(U"null");
    return;
  }

  if (in->tag == vason_ARR) {
    PUTC((c32)ARR_start);
    stringList *sl = in->list;
    usize len = stringList_len(sl);

    for (usize i = 0; i < len; i++) {
      if (i > 0)
        PUTC((c32)ARR_delim);

      fptr item = stringList_get(sl, i);
      vason_ref *ref = (vason_ref *)item.ptr;

      if (ref->tag == vason_STR || ref->tag == vason_ID) {
        PUTC((c32)STR_start);
        usize str_len = item.width - sizeof(vason_tag);
        for (usize c = 0; c < str_len; c++) {
          u8 ch = ref->string[c];
          if (ch == STR_end || ch == ESCAPE)
            PUTC((c32)ESCAPE);
          PUTC((c32)ch);
        }
        PUTC((c32)STR_end);
      } else {
        USENAMEDPRINTER("vason_live*", ref->next);
      }
    }
    PUTC((c32)ARR_end);

  } else if (in->tag == vason_MAP) {
    PUTC((c32)MAP_start);
    stringList *sl = in->map->data;
    usize len = stringList_len(sl);

    for (usize i = 0; i < len; i += 2) {
      if (i > 0)
        PUTC((c32)MAP_delim);

      fptr key = stringList_get(sl, i);
      PUTC((c32)STR_start);
      for (usize c = 0; c < key.width; c++) {
        u8 ch = key.ptr[c];
        if (ch == STR_end || ch == ESCAPE)
          PUTC((c32)ESCAPE);
        PUTC((c32)ch);
      }
      PUTC((c32)STR_end);

      PUTC((c32)MAP_delim_ID);

      fptr val = stringList_get(sl, i + 1);
      vason_ref *ref = (vason_ref *)val.ptr;

      if (ref->tag == vason_STR || ref->tag == vason_ID) {
        PUTC((c32)STR_start);
        usize str_len = val.width - sizeof(vason_tag);
        for (usize c = 0; c < str_len; c++) {
          u8 ch = ref->string[c];
          if (ch == STR_end || ch == ESCAPE)
            PUTC((c32)ESCAPE);
          PUTC((c32)ch);
        }
        PUTC((c32)STR_end);
      } else {
        USENAMEDPRINTER("vason_live*", ref->next)
      }
    }
    PUTC((c32)MAP_end);
  }
});

vason_container vason_get_func(vason_container c, vason_tag *tags, ...);

#include "printer/variadic.h" // APPLY_N
#define vason_get_makeArg(j) ( \
    (vason_tag) _Generic(      \
        (j),                   \
        i32: vason_ARR,        \
        u32: vason_ARR,        \
        fptr: vason_MAP,       \
        default: vason_ID      \
    )                          \
)
#define vason_get_argsArr(...) ( \
    (vason_tag[]){               \
        APPLY_N(                 \
            vason_get_makeArg,   \
            __VA_ARGS__          \
        ),                       \
        vason_INVALID            \
    }                            \
)
#define vason_get(container, ...)     \
  vason_get_func(                     \
      container,                      \
      vason_get_argsArr(__VA_ARGS__), \
      __VA_ARGS__                     \
  )
vason_container parseStr(AllocatorV allocator, slice(c8) string);

vason_live *vason_new /* */ (AllocatorV allocator, vason_tag tag);
void vason_mapSet /*     */ (vason_live *map, fptr key, vason_live *val);
void vason_mapSetStr /*  */ (vason_live *map, fptr key, fptr str);
void vason_arrPush /*    */ (vason_live *arr, vason_live *val);
void vason_arrPushStr /* */ (vason_live *arr, fptr str);
#if defined __cplusplus
typedef struct vason {
  vason_container self;
  inline vason_tag tag() const { return self.top.tag; }
  inline usize countChildren() const {
    return self.top.span.len;
  }
  struct vason operator[](usize idx) {
    auto r = *this;
    r.self.top = vason_arrGet(self.top, self, (u32)idx);
    return r;
  }
  struct vason operator[](fptr c) {
    auto r = *this;
    r.self.top = vason_mapGet(self.top, self, c);
    return r;
  }
  fptr asString() {
    switch (self.top.tag) {
      case vason_STR:
      case vason_ID:
        return vason_getString(self.top, self);
      default:
        return nullFptr;
    }
  }
  struct vason operator[](const std::string &c) { return (*this)[(fptr){c.length(), (u8 *)c.c_str()}]; }
  struct vason operator[](const char *c) { return (*this)[(fptr){strlen(c), (u8 *)c}]; }
  explicit operator bool() const { return tag() != vason_INVALID; }

} vason;
#endif
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
bool vason_skip(char c) { return c <= ' ' && c != 0; }
bool isSkip_vason(u8 in) {
  return in > 0 && in < 33;
}
vason_token vason_toToken(c8 c) {
  switch ((vason_token)c) {
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
      return (vason_token)c;
    case '=': {
      return MAP_delim_ID;
    } break;
    case '\\': {
      return ESCAPE;
    } break;
    default: {
      return STR_;
    }
  }
}
slice(vason_token) vason_tokenize(AllocatorV allocator, slice(c8) string) {
  slice(vason_token) t = {
      string.len,
      aCreate(allocator, vason_token, string.len)
  };
  for (usize i = 0; i < t.len; i++)
    t.ptr[i] = vason_toToken(string.ptr[i]);
  // escape escape'd characters
  for (typeof(t.len) i = 0; i + 1 < t.len; i++)
    if (t.ptr[i] == ESCAPE) {
      t.ptr[i] = STR_;
      t.ptr[i + 1] = STR_;
      i++;
    }
  // force strings inside delimiters i
  for (typeof(t.len) i = 0; i + 1 < t.len; i++) {
    if (t.ptr[i] == STR_start) {
      i++;
      while (i < t.len && t.ptr[i] != STR_end) {
        t.ptr[i] = STR_;
        i++;
      }
    }
  }
  // force strings inside delimiters ii
  for (isize i = t.len; i > 0; i--) {
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
            // t.ptr[ii] = ESCAPE;
            ;
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
  for (isize i = t.len; i > 0; i--) {
    while (i > 0 && t.ptr[i - 1] == STR_ && isSkip_vason(string.ptr[i - 1])) {
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
    while (i < t.len && t.ptr[i] == STR_ && isSkip_vason(string.ptr[i])) {
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
  nullable(usize) end_nullable = nullable_null(usize);
  usize i = 0;
  enum {
    vason_MAP = vason_MAP,
    vason_ARR = vason_ARR
  } mode = vason_ARR;
  typeof(mode) startMode;
  usize end;
  // clang-format off
  for (; start < t.len; start++) {
    switch (t.ptr[start]) {
      case ARR_start:startMode = vason_ARR;goto end_loop;
      case MAP_start:startMode = vason_MAP;goto end_loop;
      default: break;
    }
  }
  goto early_return;
  end_loop:
  // clang-format on

  start++;

  mList_push(endList, t.ptr[start - 1]);
  end_nullable = find_obj_end_heap(endList, start, t);
  if (end_nullable.isnull)
    goto early_return;
  end = end_nullable.data;
  i = start;

  while (i < end) {
    switch (t.ptr[i]) {
      case STR_: {
        usize next = i + 1;
        while (next < end && t.ptr[next] == STR_)
          next++;

        vason_level e = (vason_level){
            .kind = vason_STR,
            .pos = {
                .len = (typeof(e.pos.len))(next - i),
                .offset = (typeof(e.pos.offset))i,
            },

        };
        List_append(&level, &e);
        i = next;
      } break;
      case ARR_start:
      case MAP_start: {
        mList_push(endList, t.ptr[i]);
        nullable(usize) next_nullable = find_obj_end_heap(endList, i + 1, t);
        if (next_nullable.isnull)
          goto early_return;
        usize next = next_nullable.data;

        vason_level l =
            (vason_level){
                .kind = (vason_tag)(t.ptr[i] == ARR_start ? vason_ARR : vason_MAP),
                .pos = {
                    .len = (typeof(l.pos.len))(next - i + 1),
                    .offset = (typeof(l.pos.offset))i,
                },
            };
        List_append(&level, &l);
        i = next + 1;
      } break;

      case MAP_delim_ID: {
        if (level.length) {
          ((vason_level *)List_getRef(&level, level.length - 1))->kind = vason_ID;
          mode = vason_MAP;
        }
        i++;
      } break;
      case ARR_delim:
      case MAP_delim: {
        i++;
      } break;
      case ESCAPE: {
        usize next = i + 1;
        while (next < end && t.ptr[next] == ESCAPE)
          next++;
        // skips surrounded by commas get added as an empty string
        if (
            i > 0 && next < t.len && // mode == vason_ARR &&
            t.ptr[next] == ARR_delim && t.ptr[i - 1] == ARR_delim
        )
          List_append(&level, REF(vason_level, ((vason_level){.kind = vason_STR, .pos = {}})));
        i = next;
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
  if (mode != vason_MAP) {
    for (usize i = 0; i < level.length; i++) {
      if ((*(vason_level *)(List_getRef(&level, i))).kind == vason_ID) {
        mode = vason_MAP;
        break;
      }
    }
  }

  if (mode == vason_MAP && level.length) {
    typedef enum { key,
                   value,
                   unknown } itemkind;
    vason_level *arr = (typeof(arr))level.head;
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
        case vason_ID:
          items.ptr[i] = key;
          break;
        default:
          items.ptr[i] = unknown;
          break;
      }
    }

    usize kstart = 0;
    // make sure first thing is a key;
    while (kstart < items.len && items.ptr[kstart] != key) {
      if (arr[kstart].kind != vason_STR) {
        items.ptr[kstart] = unknown;
        kstart++;
      } else {
        items.ptr[kstart] = key;
        break;
      }
    }
    // string after key is a value
    for (usize i = kstart; i < items.len; i++) {
      if (items.ptr[i] == unknown && arr[i].kind == vason_STR && items.ptr[i - 1] == key)
        items.ptr[i] = value;
    }
    // string before value is key
    for (usize i = kstart; i < items.len - 1; i++) {
      if (items.ptr[i] == unknown && arr[i].kind == vason_STR && items.ptr[i + 1] == value)
        items.ptr[i] = key;
    }
    // make pairs
    typeof(*items.ptr) expected = key;
    for (usize i = kstart; i < items.len; i++) {
      if (items.ptr[i] != expected)
        items.ptr[i] = unknown;
      else
        expected = expected == key ? value : key;
    }

    // all others removed
    {
      usize i = 0;
      usize j = 0;
      while (i < level.length) {
        if (items.ptr[j] == unknown)
          List_remove(&level, i);
        else
          i++;
        j++;
      }
    }

    if (List_length(&level) % 2)
      level.length--;

    aFree(allocator, items.ptr);
  }
  List_forceResize(&level, level.length ?: 1);
  if (level.length == 0)
    mode = startMode;
  slice(vason_level) listMemory =
      (slice(vason_level)){
          level.length,
          (vason_level *)level.head,
      };
  return (struct breakdown_return){
      (vason_tag)(mode | (malofrmed ? vason_INVALID : 0)),
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
  bdr.kind = (typeof(bdr.kind))(bdr.kind & ~vason_INVALID);
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
          .len = (typeof(res.span.len))bdr.items.len,
          .offset = (typeof(res.span.len))mList_len(containerList),
      };
      // if (bdr.kind == vason_MAP)
      //   res.span.len = (res.span.len / 2) * 2;
      //   bdr only returns even length maps
      List_appendFromArr((List *)containerList, NULL, res.span.len);
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
vason_container parseStr(AllocatorV allocator, slice(c8) str) {
  assertMessage(
      str.len < ntype_max_u(typeof(((vason_span *)NULL)->len)),
      "vason_span isnt big enough to refer to string"
  );
  slice(vason_token) t = vason_tokenize(allocator, str);
  mList(vason_object) bucket = mList_init(allocator, vason_object);

  struct breakdown_return bdr = breakdown(0, t, allocator, str);
  vason_object bdc = bdconvert(bucket, bdr, str, t, allocator);

  aFree(allocator, bdr.items.ptr);
  aFree(allocator, t.ptr);

  vason_container res = {
      .string = str,
      .objects = (typeof(res.objects)){
          .len = (typeof(res.objects.len))mList_len(bucket),
          .ptr = (vason_object *)((List *)bucket)->head,
      },
      .top = bdc,
  };
  aFree(allocator, bucket);
  return res;
}
#include <stdarg.h>

slice(vason_object) vason_getChildren(vason_object obj, vason_container c) {
  typedef typeof(vason_getChildren((vason_object){}, (vason_container){})) T_res;
  if (!(obj.tag == vason_MAP || obj.tag == vason_ARR))
    return (T_res){};
  if (!(obj.span.len + obj.span.offset <= c.objects.len))
    return (T_res){};
  return (T_res){
      obj.span.len,
      c.objects.ptr + obj.span.offset,
  };
}
fptr vason_getString(vason_object obj, vason_container c) {
  if (!(obj.tag == vason_STR || obj.tag == vason_ID))
    return nullFptr;
  if (!(obj.span.len + obj.span.offset <= c.string.len))
    return nullFptr;
  return (fptr){
      obj.span.len,
      c.string.ptr + obj.span.offset,
  };
}
vason_object vason_mapGet(vason_object o, vason_container c, fptr k) {
  if (o.tag != vason_MAP)
    return (vason_object){vason_INVALID};
  typeof(vason_getChildren(o, c)) children = vason_getChildren(o, c);
  for (i32 i = 0; i + 1 < children.len; i += 2) {
    if (fptr_eq(k, vason_getString(children.ptr[i], c)))
      return children.ptr[i + 1];
  }
  return (vason_object){vason_INVALID};
}
vason_object vason_arrGet(vason_object o, vason_container c, u32 key) {
  if (!(o.tag == vason_ARR))
    return (vason_object){vason_INVALID};
  typeof(vason_getChildren(o, c)) children = vason_getChildren(o, c);
  if (children.len < key)
    return (vason_object){vason_INVALID};
  return children.ptr[key];
}
vason_container vason_get_func(vason_container c, vason_tag *tags, ...) {

  va_list ap;
  va_start(ap, tags);
  while (c.top.tag != vason_INVALID) {
    vason_tag t = *tags++;
    switch (t) {
      case vason_MAP:
      case vason_ARR: {
        if (t != c.top.tag) {
          c.top.tag = vason_INVALID;
        } else {
          if (t == vason_MAP)
            c.top = vason_mapGet(c.top, c, va_arg(ap, fptr));
          if (t == vason_ARR)
            c.top = vason_arrGet(c.top, c, va_arg(ap, u32));
        }
      } break;
      case vason_INVALID:
        return c;
      case vason_ID:
        assertMessage(false, "arg of wrong type? (u32|i32|fptr)");
      default:
        c.top.tag = vason_INVALID;
    }
  }
  va_end(ap);
  c.top.tag = vason_INVALID;
  return c;
}
//
//
//
// code version
// dont really care how fast this is
//
//
//
vason_live *vason_new(AllocatorV allocator, vason_tag tag) {
  assertMessage(tag == vason_MAP || tag == vason_ARR, "only MAP,ARR supported");
  vason_live *res = aCreate(allocator, vason_live);
  res->tag = tag;
  stringList *sl = stringList_new(allocator, 1);
  defer_(aFree(allocator, sl););
  *res->list = *sl;
  return res;
}
void vason_mapSet(vason_live *map, fptr key, vason_live *val) {
  vason_ref ref = {.tag = val->tag, .next = val};
  fptr ref_fptr = {sizeof(ref), (u8 *)&ref};
  OMap_set(map->map, key, ref_fptr);
}
void vason_arrPush(vason_live *arr, vason_live *val) {
  vason_ref ref = {.tag = val->tag, .next = val};
  fptr ref_fptr = {sizeof(ref), (u8 *)&ref};
  stringList_append(arr->list, ref_fptr);
}
void vason_arrPushStr(vason_live *arr, fptr str) {
  usize size = sizeof(vason_tag) + str.width;
  vason_ref *ref = (typeof(ref))aAlloc(arr->list->allocator, size);
  defer_({ aFree(arr->list->allocator, ref); });

  ref->tag = vason_STR;
  memcpy(ref->string, str.ptr, str.width);
  fptr ref_fptr = {size, (u8 *)ref};
  stringList_append(arr->list, ref_fptr);
}
void vason_mapSetStr(vason_live *map, fptr key, fptr str) {
  usize size = sizeof(vason_tag) + str.width;
  vason_ref *ref = (typeof(ref))aAlloc(map->list->allocator, size);
  defer_({ aFree(map->list->allocator, ref); });
  ref->tag = vason_STR;
  memcpy(ref->string, str.ptr, str.width);
  fptr ref_fptr = {size, (u8 *)ref};
  OMap_set(map->map, key, ref_fptr);
}
#endif
