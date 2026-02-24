#include "fptr.h"
#if !defined(VASON_PARSER_H)
  #define VASON_PARSER_H (1)
  #include "allocator.h"
  #include "my-list.h"
  #include "mytypes.h"
  #include "print.h"
  #include "shortList.h"
  #include <alloca.h>
typedef u32 vason_index;
typedef struct {
  vason_index offset, len;
} vason_span;

typedef enum : u8 {
  vason_STRING /*  */ = 1 << 0,
  vason_TABLE /*   */ = 1 << 1,
  vason_PAIR /*    */ = 1 << 2,
  vason_UNPARSED /**/ = 1 << 6,
  vason_INVALID /* */ = 1 << 7,
} vason_tag;
/**
 * string     : offset + length inside of table-stirngs
 * table      : offset + length inside of table-strings
 * key_value  : offset inside of table-strings
 *                first item is string
 *                second is table/string
 */
typedef struct {
  vason_index current;
  vason_index *indexes;
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
  #include "printer/variadic.h"
  #define MAKE_VASONS_GET_ARG(arg)                                                           \
    _Generic(                                                                                \
        arg,                                                                                 \
        i32: (struct vason_getArg){.is_unsigned_integer = 1, ._u32 = REF(typeof(arg), arg)}, \
        fptr: (struct vason_getArg){.is_fat_pointer = 1, ._fptr = REF(typeof(arg), arg)}     \
    ),
  #define vason_get(container, ...) vason_get(                                            \
      container,                                                                          \
      (struct vason_getArg[]){                                                            \
          APPLY_N(MAKE_VASONS_GET_ARG, __VA_ARGS__)(struct vason_getArg){.terminator = 1} \
      }                                                                                   \
  )

vason_container(vason_get)(vason_container c, struct vason_getArg *argTypes);
vason_container vason_get_str(vason_container c, fptr f);
vason_container vason_get_idx(vason_container c, vason_index f);

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
    return self.current < msList_len(self.tags) ? self.tables_strings[self.current].len : 0;
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
  fptr asString() {

    switch (this->tag()) {
      case vason_STRING: {
        vason_span s = self.tables_strings[self.current];
        return ((fptr){s.len, s.offset + self.text.ptr});
      } break;
      default:
        return nullFptr;
    }
  }
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
  while (
      span.len &&
      isIgnored(in.ptr[span.offset])
  ) {
    span.offset++;
    span.len--;
  }
  while (
      span.len &&
      isIgnored(in.ptr[span.offset + span.len - 1])
  )
    span.len--;
  return span;
}
static inline vason_span string_reduce(slice(c8) str, vason_span span) {
  span = basic_trim(str, span);
  if (
      span.len >= 2 &&
      to_token(str.ptr[span.offset]) == vason_STR_DELIM &&
      to_token(str.ptr[span.offset + span.len - 1]) == vason_STR_DELIM &&
      str.ptr[span.offset + span.len - 1] == str.ptr[span.offset]
  ) {
    span.offset++;
    span.len -= 2;
  }
  return span;
}
static inline vason_span table_reduce(slice(c8) str, vason_span span) {
  while (span.len && to_token(str.ptr[span.offset]) != vason_TABLE_START) {
    if (span.len && to_token(str.ptr[span.offset]) == vason_ESCAPE) {
      span.len--;
      span.offset++;
    }
    if (span.len) {
      span.offset++;
      span.len--;
    }
  }
  if (span.len) {
    span.offset++;
    span.len--;
  }
  while (span.len && to_token(str.ptr[span.offset + span.len - 1]) != vason_TABLE_END) {
    if (span.len && to_token(str.ptr[span.offset + span.len - 1]) == vason_ESCAPE)
      span.len--;
    span.len = span.len ? span.len - 1 : 0;
  }
  span.len = span.len ? span.len - 1 : 0;
  return span;
}
vason_container vason_container_create(slice(c8) text, AllocatorV allocator) {
  vason_container res =
      (vason_container){
          .indexes /*        */ = msList_init(allocator, vason_index),
          .tags /*           */ = msList_init(allocator, vason_tag),
          .tables_strings /* */ = msList_init(allocator, vason_span),
          .allocator /*      */ = allocator,
          .text /*           */ = text,
      };
  return res;
}
void vason_container_free(vason_container container) {
  AllocatorV alocator = container.allocator;
  msList_deInit(alocator, container.indexes);
  msList_deInit(alocator, container.tags);
  msList_deInit(alocator, container.tables_strings);
}
slice(vason_token_t) vason_tokenize(AllocatorV allocator, slice(c8) cs) {
  slice(vason_token_t) res = (slice(vason_token_t)){
      cs.len,
      aCreate(allocator, vason_token_t, cs.len),
  };
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

  return res;
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
REGISTER_PRINTER(vason_token_t, {
  switch (in) {
    case vason_ESCAPE:
      PUTC('/');
      break;
    case vason_PAIR_DELIM:
      PUTC(':');
      break;
    case vason_TABLE_DELIM:
      PUTC(',');
      break;
    case vason_STR_DELIM:
      PUTC('"');
      break;
    case vason_STR:
      PUTC('_');
      break;
    case vason_TABLE_START:
      PUTC('{');
      break;
    case vason_TABLE_END:
      PUTC('}');
      break;
  }
});
  #include "printer/genericName.h"
MAKE_PRINT_ARG_TYPE(vason_token_t);

void vason_parse_level1(
    vason_span span,
    slice(vason_token_t) tokens,
    vason_container *parent
) {
  // print("in : ");
  // for (auto i = span.offset; i < span.offset + span.len; i++)
  //   print("{}", tokens.ptr[i]);
  // println(" | {}", ((fptr){span.len, span.offset + parent->text.ptr}));
  mList(vason_token_t) tstack = mList_init(parent->allocator, vason_token_t);
  defer { mList_deInit(tstack); };
  mList(vason_span) parts = mList_init(parent->allocator, vason_span);
  defer { mList_deInit(parts); };
  auto i = span.offset;
  auto j = span.offset;
  for (; i < span.offset + span.len; i++) {

    switch (tokens.ptr[i]) {
      case vason_TABLE_START: {
        mList_push(tstack, vason_TABLE_START);
      } break;
      case vason_TABLE_END: {
        if (mList_len(tstack)) {
          mList_pop(tstack);
        } else {
          msList_push(parent->allocator, parent->indexes, -1);
          msList_push(parent->allocator, parent->tags, vason_INVALID);
          return;
        }
      } break;
      case vason_TABLE_DELIM: {
        if (!mList_len(tstack)) {
          mList_push(
              parts,
              ((vason_span){.offset = j, .len = (vason_index)(i - j)})
          );
          j = i + 1;
        }
      } break;
      default:
        break;
        ;
    }
  }
  if (j < span.offset + span.len)
    mList_push(
        parts,
        ((vason_span){
            .offset = j,
            .len = (vason_index)(span.offset + span.len - j)
        })
    );
  mList_foreach(parts, vason_span, vs, {
    // println("element : {}", ((fptr){vs.len, parent->text.ptr + vs.offset}));
    msList_push(parent->allocator, parent->indexes, msList_len(parent->tables_strings));
    msList_push(parent->allocator, parent->tables_strings, vs);
    msList_push(
        parent->allocator,
        parent->tags,
        (vason_tag)(figureType((slice(vason_token_t)){
                        .len = vs.len,
                        .ptr = tokens.ptr + vs.offset,
                    }) |
                    vason_UNPARSED)
    );
  });
}

void vason_print(vason_container c, usize indentLevel) {
  fptr tabs = (fptr){
      indentLevel * 2,
      (c8 *)alloca(indentLevel * 2),
  };
  memset(tabs.ptr, ' ', tabs.width);

  if (c.current >= msList_len(c.tags)) {
    println("{}(!)", tabs);
    return;
  }
  if (c.tags[c.current] & vason_UNPARSED) {

    vason_span s = c.tables_strings[c.indexes[c.current]];
    fptr fpart = {s.len, s.offset + c.text.ptr};
    println("{}( unparsed ) /{{}/}", tabs, fpart);

  } else

    switch (c.tags[c.current]) {
      case vason_UNPARSED:
        println("{}(?)", tabs);
        break;
      case vason_INVALID:
        println("{}(!)", tabs);
        break;

      case vason_PAIR: {

        println("{}(:)", tabs);
        vason_span s = c.tables_strings[c.indexes[c.current]];
        for (auto j = s.offset; j < s.len + s.offset; j++) {
          c.current = j;
          vason_print(c, indentLevel + 1);
        }

      } break;

      case vason_STRING: {

        vason_span s = c.tables_strings[c.indexes[c.current]];
        fptr fpart = {s.len, s.offset + c.text.ptr};
        println("{}(') {}", tabs, fpart);

      } break;

      case vason_TABLE: {

        println("{}(<)", tabs);
        vason_span s = c.tables_strings[c.indexes[c.current]];
        for (auto j = s.offset; j < s.len + s.offset; j++) {
          c.current = j;
          vason_print(c, indentLevel + 1);
        }

      } break;
    }
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
        vason_span span = parent->tables_strings[parent->indexes[i]];
        span = string_reduce(parent->text, span);
        parent->tables_strings[parent->indexes[i]] = span;
        parent->tags[i] = vason_STRING;
      } break;

      case vason_TABLE: {
        vason_span span = parent->tables_strings[parent->indexes[i]];
        span = table_reduce(parent->text, basic_trim(parent->text, basic_trim(parent->text, span)));
        usize offset = msList_len(parent->tables_strings);
        vason_parse_level1(span, tokens, parent);
        usize len = msList_len(parent->tables_strings) - offset;
        parent->tables_strings[parent->indexes[i]] = (vason_span){
            .offset = (vason_index)(offset),
            .len = (vason_index)(len),
        };
        parent->tags[i] = vason_TABLE;
      } break;

      case vason_PAIR: {
        vason_span span = parent->tables_strings[parent->indexes[i]];
        auto s = span.offset;
        for (; s < span.offset + span.len; s++)
          if (tokens.ptr[s] == vason_PAIR_DELIM)
            break;
        vason_span key = {
            .offset = span.offset,
            .len = (vason_index)(s - span.offset),
        };
        vason_span val = {
            .offset = (vason_index)(s + 1),
            .len = (vason_index)(span.len - (s - span.offset) - 1),
        };
        // println(
        //     "total:/{{}/}\nkey :/{{}/}, val :/{{}/}",
        //     ((fptr){span.len, span.offset + parent->text.ptr}),
        //     ((fptr){key.len, key.offset + parent->text.ptr}),
        //     ((fptr){val.len, val.offset + parent->text.ptr})
        // );

        parent->tables_strings[parent->indexes[i]] = (vason_span){
            .offset = (vason_index)(msList_len(parent->tables_strings)),
            .len = 2,
        };
        msList_push(parent->allocator, parent->indexes, msList_len(parent->tables_strings));
        msList_push(parent->allocator, parent->tables_strings, key);
        msList_push(
            parent->allocator,
            parent->tags,
            (vason_tag)(vason_STRING | vason_UNPARSED)
        );
        msList_push(parent->allocator, parent->indexes, msList_len(parent->tables_strings));
        msList_push(parent->allocator, parent->tables_strings, val);
        msList_push(
            parent->allocator,
            parent->tags,
            (vason_tag)(figureType((slice(vason_token_t)){
                            .len = val.len,
                            .ptr = tokens.ptr + val.offset,
                        }) |
                        vason_UNPARSED)
        );

        parent->tags[i] = vason_PAIR;
      } break;
    }
  }
}

vason_container vason_parseString(AllocatorV allocator, slice(c8) string) {
  auto res = vason_container_create(string, allocator);

  slice(vason_token_t) tokens = vason_tokenize(allocator, string);
  defer { aFree(allocator, tokens.ptr); };
  vason_parse_level1(
      (vason_span){
          .offset = 0,
          .len = (vason_index)string.len,
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
    vason_span vs = c.tables_strings[c.indexes[c.current]];
    for (auto i = vs.offset; i < vs.offset + vs.len; i++) {
      if (c.tags[i] == vason_PAIR) {
        vason_span children = c.tables_strings[c.indexes[i]];
        assertMessage(children.len == 2);

        auto ikey = children.offset;
        auto ival = children.offset + 1;
        assertMessage(c.tags[ikey] == vason_STRING);
        vason_span key_text = c.tables_strings[c.indexes[ikey]];
        fptr cs = (fptr){key_text.len, c.text.ptr + key_text.offset};

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
  if (msList_len(c.tags) <= c.current) {
    return c;
  }
  if (
      c.tags[c.current] != vason_TABLE ||
      c.tables_strings[c.indexes[c.current]].len <= f
  ) {
    c.current = msList_len(c.tags);
    return c;
  }
  c.current = c.tables_strings[c.indexes[c.current]].offset + f;
  return c;
}
vason_container(vason_get)(vason_container c, struct vason_getArg *argTypes) {
  while (!argTypes->terminator && c.current < msList_len(c.tags)) {
    if (argTypes->is_fat_pointer) {
      c = vason_get_str(c, *(fptr *)argTypes->_fptr);
    } else if (argTypes->is_unsigned_integer) {
      c = vason_get_idx(c, *(u32 *)argTypes->_u32);
    } else {
      assertMessage(false, "wrong type in vason get?");
    }
    argTypes++;
  }
  return c;
}
#endif
