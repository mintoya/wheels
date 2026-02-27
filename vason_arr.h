#include "fptr.h"
#if !defined(VASON_PARSER_H)
  #define VASON_PARSER_H (1)
  #include "allocator.h"
  #include "my-list.h"
  #include "mytypes.h"
  #include "print.h"
  #include "shortList.h"
typedef u16 vason_index;
typedef struct {
  vason_index start, end;
} vason_span;

typedef enum : u8 {
  vason_STRING /*  */ = 1 << 0,
  vason_TABLE /*   */ = 1 << 1,
  vason_PAIR /*    */ = 1 << 2,
  vason_UNPARSED /**/ = 1 << 6,
  vason_INVALID /* */ = 1 << 7,
} vason_tag;
/**
 * string     : offset + length inside of text
 * table      : offset + length inside of table-strings
 * key_value  : offset inside of table-strings
 *                first item is string
 *                second is anything else
 * text       : origional passed in string
 * tags and tables_strings are msList_pointers, use msList_free
 */
typedef struct {
  vason_index current;
  vason_tag *tags;
  vason_span *tables_strings;
  AllocatorV allocator;
  slice(c8) text;
} vason_container;

vason_container vason_container_create(slice(c8) text, AllocatorV allocator);
void vason_container_free(vason_container container);

struct vason_getArg {
  struct {
    bool is_unsigned_integer : 1;
    bool is_fat_pointer : 1;
    bool terminator : 1;
  };
  union {
    void *_u32;
    void *_fptr;
  };
};
vason_container vason_get_str(vason_container c, fptr f);
vason_container vason_get_idx(vason_container c, vason_index f);
  #include "macros.h"
  #define MAKE_VASONS_GET_ARG(arg)                                                           \
    _Generic(                                                                                \
        arg,                                                                                 \
        i32: (struct vason_getArg){.is_unsigned_integer = 1, ._u32 = REF(typeof(arg), arg)}, \
        u32: (struct vason_getArg){.is_unsigned_integer = 1, ._u32 = REF(typeof(arg), arg)}, \
        fptr: (struct vason_getArg){.is_fat_pointer = 1, ._fptr = REF(typeof(arg), arg)}     \
    ),
  #define vason_get(container, ...) vason_get_func(                                       \
      container,                                                                          \
      (struct vason_getArg[]){                                                            \
          APPLY_N(MAKE_VASONS_GET_ARG, __VA_ARGS__)(struct vason_getArg){.terminator = 1} \
      }                                                                                   \
  )

vason_container vason_get_func(vason_container c, struct vason_getArg *argTypes);

  #if defined __cplusplus
typedef struct vason {
  vason_container self;
  inline constexpr vason(const vason_container c) : self(c) {}
  inline constexpr operator vason_container() const { return self; }

  inline void free() { vason_container_free(self); }
  inline vason_tag tag() const {
    return self.current < msList_len(self.tags) ? self.tags[self.current] : vason_INVALID;
  }
  inline usize countChildren() const {
    return self.current < msList_len(self.tags)
               ? self.tables_strings[self.current].end -
                     self.tables_strings[self.current].start
               : 0;
  }
  struct vason operator[](usize idx) {
    auto r = *this;
    r.self = vason_get_idx(self, idx);
    return r;
  }
  struct vason operator[](fptr c) {
    auto r = *this;
    r.self = vason_get_str(self, c);
    return r;
  }
  fptr asString() const {
    return this->tag() == vason_STRING
               ? (fptr){
                     (usize)self.tables_strings[self.current].end -
                         self.tables_strings[self.current].start,
                     self.tables_strings[self.current].start + self.text.ptr
                 }
               : nullFptr;
  }
  inline constexpr operator fptr() const { return this->asString(); }
  struct vason operator[](const std::string &c) { return (*this)[(fptr){c.length(), (u8 *)c.c_str()}]; }
  struct vason operator[](const char *c) { return (*this)[(fptr){strlen(c), (u8 *)c}]; }
  explicit operator bool() const { return tag() != vason_INVALID; }
} vason;
  #endif
vason_container vason_parseString(AllocatorV allocator, slice(c8) string);

#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define VASON_PARSER_C (1)
#endif
#if defined(VASON_PARSER_C)
typedef enum : c8 {
  vason_TABLE_START,
  vason_TABLE_END,
  vason_TABLE_DELIM,
  vason_STR,
  vason_STR_DELIM,
  vason_ESCAPE,
  vason_PAIR_DELIM,
} vason_token_t;
static inline vason_token_t to_token(c8 in) {
  // makes parsing json with this a lil easier
  switch (in) {
    case '[':
    case '{':
      return vason_TABLE_START;
    case ']':
    case '}':
      return vason_TABLE_END;
    case ',':
    case ';':
      return vason_TABLE_DELIM;
    case '\\':
      return vason_ESCAPE;
    case '\'':
    case '"':
      return vason_STR_DELIM;
    case ':':
      return vason_PAIR_DELIM;
    default:
      return vason_STR;
  }
}

static inline bool isIgnored(c8 in) { return in <= ' ' && in; }
static inline vason_span basic_trim(slice(c8) in, vason_span span) {
  while (span.start < span.end &&
         isIgnored(in.ptr[span.start]))
    span.start++;
  while (span.end > span.start &&
         isIgnored(in.ptr[span.end - 1]))
    span.end--;
  return span;
}
static inline vason_span string_reduce(slice(c8) str, vason_span span) {
  span = basic_trim(str, span);
  if (
      span.end - span.start >= 2 &&
      to_token(str.ptr[span.start]) == vason_STR_DELIM &&
      to_token(str.ptr[span.end - 1]) == vason_STR_DELIM &&
      str.ptr[span.end - 1] == str.ptr[span.start]
  ) {
    span.end--;
    span.start++;
  }
  return span;
}
static inline vason_span table_reduce(slice(vason_token_t) str, vason_span span) {
  while (span.start < span.end && str.ptr[span.start] != vason_TABLE_START)
    span.start++;
  if (span.start < span.end && str.ptr[span.start] == vason_TABLE_START)
    span.start++;
  while (span.end > span.start && str.ptr[span.end - 1] != vason_TABLE_END)
    span.end--;
  if (span.end > span.start && str.ptr[span.end - 1] == vason_TABLE_END)
    span.end--;

  return span;
}
vason_container vason_container_create(slice(c8) text, AllocatorV allocator) {
  vason_container res =
      (vason_container){
          .tags /*           */ = msList_init(allocator, vason_tag),
          .tables_strings /* */ = msList_init(allocator, vason_span),
          .allocator /*      */ = allocator,
          .text /*           */ = text,
      };
  return res;
}
void vason_container_free(vason_container container) {
  AllocatorV alocator = container.allocator;
  msList_deInit(alocator, container.tags);
  msList_deInit(alocator, container.tables_strings);
}
void vason_tokenize(slice(vason_token_t) res, slice(c8) cs) {
  for (auto i = 0; i < res.len; i++)
    res.ptr[i] = to_token(cs.ptr[i]);

  for (auto i = 0; i + 1 < res.len; i++)
    if (res.ptr[i] == vason_ESCAPE) {
      res.ptr[i] = vason_STR;
      res.ptr[i + 1] = vason_STR;
      i++;
    }

  for (auto i = 0; i < res.len; i++)
    if (res.ptr[i] == vason_STR_DELIM) {
      i++;
      while (i < res.len && res.ptr[i] != vason_STR_DELIM) {
        res.ptr[i] = vason_STR;
        i++;
      }
    }
}
vason_tag figureType(slice(vason_token_t) in) {
  for (auto i = 0; i < in.len; i++) {
    if (in.ptr[i] == vason_PAIR_DELIM) {
      return vason_PAIR;
    } else if (in.ptr[i] == vason_TABLE_START) {
      return vason_TABLE;
    }
  }
  return vason_STRING;
}

// REGISTER_PRINTER(vason_token_t, {
//   switch (in) {
//     case vason_ESCAPE:
//       PUTC('/');
//       break;
//     case vason_PAIR_DELIM:
//       PUTC(':');
//       break;
//     case vason_TABLE_DELIM:
//       PUTC(',');
//       break;
//     case vason_STR_DELIM:
//       PUTC('"');
//       break;
//     case vason_STR:
//       PUTC('_');
//       break;
//     case vason_TABLE_START:
//       PUTC('{');
//       break;
//     case vason_TABLE_END:
//       PUTC('}');
//       break;
//   }
// });
//   #include "printer/genericName.h"
// MAKE_PRINT_ARG_TYPE(vason_token_t);

void vason_parse_level1(
    vason_span span,
    slice(vason_token_t) tokens,
    vason_container *parent
) {
  // print("in : ");
  // for (auto i = span.start; i < span.end; i++)
  //   print("{}", tokens.ptr[i]);
  // println(" | {}", ((fptr){(usize)span.end - span.start, span.start + parent->text.ptr}));
  mList(vason_token_t) tstack = mList_init(parent->allocator, vason_token_t);
  defer { mList_deInit(tstack); };
  mList(vason_span) parts = mList_init(parent->allocator, vason_span);
  defer { mList_deInit(parts); };
  auto i = span.start;
  auto j = span.start;
  for (; i < span.end; i++) {

    switch (tokens.ptr[i]) {
      case vason_TABLE_START: {
        mList_push(tstack, vason_TABLE_START);
      } break;
      case vason_TABLE_END: {
        if (mList_len(tstack)) {
          mList_pop(tstack);
        } else {
          msList_push(parent->allocator, parent->tags, vason_INVALID);
          msList_push(parent->allocator, parent->tables_strings, (vason_span){});
          return;
        }
      } break;
      case vason_TABLE_DELIM: {
        if (!mList_len(tstack)) {
          mList_push(
              parts,
              ((vason_span){.start = j, .end = i})
          );
          j = i + 1;
        }
      } break;
      default:
        break;
        ;
    }
  }
  if (j < span.end)
    mList_push(
        parts,
        ((vason_span){
            .start = j,
            .end = span.end,
        })
    );
  mList_foreach(parts, vason_span, vs, {
    // println("element : {}", ((fptr){vs.len, parent->text.ptr + vs.offset}));
    msList_push(parent->allocator, parent->tables_strings, vs);
    msList_push(
        parent->allocator,
        parent->tags,
        (vason_tag)(figureType((slice(vason_token_t)){
                        .len = (usize)vs.end - vs.start,
                        .ptr = tokens.ptr + vs.start,
                    }) |
                    vason_UNPARSED)
    );
  });
}

void vason_parse_level2(
    vason_container *parent,
    slice(vason_token_t) tokens
) {
  for (usize i = 0; i < msList_len(parent->tags); ++i) {
    if (!(parent->tags[i] & vason_UNPARSED))
      continue;

    switch (parent->tags[i] ^ vason_UNPARSED) {
      case vason_STRING: {
        vason_span span = parent->tables_strings[i];
        span = string_reduce(parent->text, span);
        parent->tables_strings[i] = span;
        parent->tags[i] = vason_STRING;
      } break;

      case vason_TABLE: {
        vason_span span = parent->tables_strings[i];
        span = table_reduce(tokens, basic_trim(parent->text, basic_trim(parent->text, span)));
        vason_index start = msList_len(parent->tables_strings);
        vason_parse_level1(span, tokens, parent);
        vason_index end = msList_len(parent->tables_strings);
        parent->tables_strings[i] = (vason_span){
            .start = start,
            .end = end,
        };
        parent->tags[i] = vason_TABLE;
      } break;

      case vason_PAIR: {
        vason_span span = parent->tables_strings[i];
        auto s = span.start;
        for (; s < span.end; s++)
          if (tokens.ptr[s] == vason_PAIR_DELIM)
            break;
        vason_span key = {
            .start = span.start,
            .end = s,
        };
        vason_span val = {
            .start = (vason_index)(s + 1),
            .end = span.end,
        };
        // println(
        //     "total:/{{}/}\nkey :/{{}/}, val :/{{}/}",
        //     ((fptr){span.len, span.start + parent->text.ptr}),
        //     ((fptr){key.len, key.offset + parent->text.ptr}),
        //     ((fptr){val.len, val.offset + parent->text.ptr})
        // );
        parent->tables_strings[i].start = (vason_index)(msList_len(parent->tables_strings));
        parent->tables_strings[i].end = (vason_index)(msList_len(parent->tables_strings) + 2);

        msList_push(parent->allocator, parent->tables_strings, key);
        msList_push(
            parent->allocator,
            parent->tags,
            (vason_tag)(vason_STRING | vason_UNPARSED)
        );
        msList_push(parent->allocator, parent->tables_strings, val);
        msList_push(
            parent->allocator,
            parent->tags,
            (vason_tag)(figureType((slice(vason_token_t)){
                            .len = (usize)val.end - val.start,
                            .ptr = tokens.ptr + val.start,
                        }) |
                        vason_UNPARSED)
        );

        parent->tags[i] = vason_PAIR;
      } break;
    }
  }
}

REGISTER_PRINTER(vason_container, {
  if (in.current >= msList_len(in.tags) || in.tags[in.current] & vason_INVALID) {
    PUTS(U"(!)");
    return;
  } else if (in.tags[in.current] & vason_UNPARSED) {
    PUTS(U"(?)");
    return;
  }
  switch (in.tags[in.current]) {
    case vason_INVALID:
    case vason_UNPARSED:
      __builtin_unreachable();
      break;
    case vason_PAIR: {
      vason_span vs = in.tables_strings[in.current];
      PUTS(U"(:)<");
      in.current = vs.start;
      USETYPEPRINTER(vason_container, in);
      PUTS(U",");
      in.current++;
      USETYPEPRINTER(vason_container, in);
      PUTS(U">");
    } break;
    case vason_STRING: {
      vason_span vs = in.tables_strings[in.current];
      USETYPEPRINTER(fptr, ((fptr){(usize)vs.end - vs.start, vs.start + in.text.ptr}));
    } break;
    case vason_TABLE: {
      vason_span vs = in.tables_strings[in.current];
      PUTS(U"(<");
      USETYPEPRINTER(usize, (usize)(vs.end - vs.start));
      PUTS(U">){");
      for (auto i = vs.start; i < vs.end; i++) {
        i != vs.start ? PUTS(U",") : (void)0;
        in.current = i;
        USETYPEPRINTER(vason_container, in);
      }
      PUTS(U"}");
    } break;
  }
});

vason_container vason_parseString(AllocatorV allocator, slice(c8) string) {
  auto res = vason_container_create(string, allocator);
  slice(vason_token_t) tokens = (typeof(tokens)){
      string.len,
      aCreate(allocator, vason_token_t, string.len)
  };
  vason_tokenize(tokens, string);
  defer { aFree(allocator, tokens.ptr); };
  vason_parse_level1(
      (vason_span){
          .start = 0,
          .end = (vason_index)string.len,
      },
      tokens,
      &res
  );
  vason_parse_level2(&res, tokens);
  res.current = 0;
  return res;
}
// vason_container
// will overload it with a macro to generate types
vason_container vason_get_str(vason_container c, fptr f) {
  if (msList_len(c.tags) <= c.current) {
    return c;
  }
  if (
      c.tags[c.current] != vason_TABLE
  ) {
    goto not_found;
  }
  {
    vason_span vs = c.tables_strings[c.current];
    for (auto i = vs.start; i < vs.end; i++) {
      if (c.tags[i] == vason_PAIR) {
        vason_span children = c.tables_strings[i];
        assertMessage(children.end - children.start == 2);

        auto ikey = children.start;
        auto ival = children.start + 1;
        assertMessage(c.tags[ikey] == vason_STRING);
        vason_span key_text = c.tables_strings[ikey];
        fptr cs = (fptr){(usize)key_text.end - key_text.start, c.text.ptr + key_text.start};

        if (fptr_eq(cs, f)) {
          c.current = ival;
          return c;
        }
      }
    }
  }
  {
  not_found:
    c.current = msList_len(c.tags);
    return c;
  }
}
vason_container vason_get_idx(vason_container c, vason_index f) {
  if (
      msList_len(c.tags) <= c.current ||
      (c.tags[c.current] != vason_TABLE &&
       c.tags[c.current] != vason_PAIR) ||
      c.tables_strings[c.current].end < c.tables_strings[c.current].start + f
  ) {
    c.current = msList_len(c.tags);
    return c;
  }
  c.current = c.tables_strings[c.current].start + f;
  return c;
}
vason_container vason_get_func(vason_container c, struct vason_getArg *argTypes) {
  while (!argTypes->terminator && c.current < msList_len(c.tags)) {
    if (argTypes->is_fat_pointer) {
      c = vason_get_str(c, *(fptr *)argTypes->_fptr);
    } else if (argTypes->is_unsigned_integer) {
      c = vason_get_idx(c, *(u32 *)argTypes->_u32);
    } else {
      __builtin_unreachable();
    }
    argTypes++;
  }
  return c;
}

// typedef struct vason_node {
//   vason_tag tag;
//   union {
//     struct vason_node *children; // vason_TABLE
//     fptr string;                 // vason_STRING
//     struct {
//       fptr key;
//       struct vason_node *child;
//     } pair; // vason_PAIR
//   };
// } vason_node;
// vason_node vason_makeTable(AllocatorV allocator) {
//   return (vason_node){
//       .tag = vason_TABLE,
//       .children = msList_init(allocator, vason_node),
//   };
// }
// vason_node vason_makeStr(fptr str) {
//   return (vason_node){
//       .tag = vason_STRING,
//       .string = str,
//   };
// }
// void vason_push(AllocatorV allocator, vason_node parent, vason_node child) {
//   assertMessage(parent.tag == vason_TABLE);
//   msList_push(allocator, parent.children, child);
// }
// void vason_pushKV(AllocatorV allocator, vason_node parent, fptr key, vason_node *val) {
//   vason_push(
//       allocator,
//       parent,
//       (vason_node){
//           .tag = vason_PAIR,
//           .pair = {key, val}
//       }
//   );
// }
// vason_container vason_toContainer(AllocatorV allocator, vason_node top) {
//   assertMessage(top.tag == vason_TABLE);
//   vason_container res = vason_container_create(
//       (slice(c8)){0, msList_init(allocator, c8)},
//       allocator
//   );
//   msList_push(allocator, res.tags, top.tag);
//   msList_push(
//       allocator,
//       res.tables_strings,
//       ((vason_span){(vason_index)msList_len(top.children), 1})
//   );
//   return res;
// }
#endif
