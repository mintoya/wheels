#if !defined(VASON_PARSER_H)
  #define VASON_PARSER_H (1)
  #if defined(__EMSCRIPTEN__)
    #include <emscripten.h>
  #else
    #define EMSCRIPTEN_KEEPALIVE
  #endif
  #include "allocator.h"
  #include "fptr.h"
  #include "mylist.h"
  #include "mytypes.h"
  #include "shortList.h"
sliceDef(c8);
typedef usize vason_index;
typedef struct {
  vason_index start, end;
} vason_span;

typedef enum : u8 {
  vason_STRING,
  vason_TABLE,
  vason_PAIR,
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
  slice(c8) * tokens;
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

  #if defined(MY_PRINTER_H)
    #include "print.h"
REGISTER_PRINTER(vason_container, {
  if (in.current >= msList_len(in.tags) || in.tags[in.current] & vason_INVALID) {
    PUTS(U"(!)");
    return;
  } else if (in.tags[in.current] & vason_UNPARSED) {
    PUTS(U"(?");
    switch (in.tags[in.current] ^ vason_UNPARSED) {
      case vason_INVALID:
      case vason_UNPARSED:
        break;
      case vason_TABLE:
        PUTS(U"<>)");
        break;
      case vason_PAIR:
        PUTS(U":)");
        break;
      case vason_STRING:
        PUTS(U"s)");
        break;
    }
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
      for (vason_index i = vs.start; i < vs.end; i++) {
        i != vs.start ? PUTS(U",") : (void)0;
        in.current = i;
        USETYPEPRINTER(vason_container, in);
      }
      PUTS(U"}");
    } break;
  }
});
  #endif

// referance requred since lazy containers modify tehmselves
vason_index vason_get_str(vason_container *c, vason_index entry, fptr f);
vason_index vason_get_idx(vason_container *c, vason_index entry, vason_index f);
  #include "macros.h"
  #define MAKE_VASONS_GET_ARG(arg)                                                           \
    _Generic(                                                                                \
        arg,                                                                                 \
        i32: (struct vason_getArg){.is_unsigned_integer = 1, ._u32 = REF(typeof(arg), arg)}, \
        u32: (struct vason_getArg){.is_unsigned_integer = 1, ._u32 = REF(typeof(arg), arg)}, \
        fptr: (struct vason_getArg){.is_fat_pointer = 1, ._fptr = REF(typeof(arg), arg)}     \
    ),
  #define vason_get(container, entry, ...)                                                    \
    ({                                                                                        \
      vason_get_func(                                                                         \
          container, entry,                                                                   \
          (struct vason_getArg[]){                                                            \
              APPLY_N(MAKE_VASONS_GET_ARG, __VA_ARGS__)(struct vason_getArg){.terminator = 1} \
          }                                                                                   \
      );                                                                                      \
    })

vason_index vason_get_func(vason_container *c, vason_index entry, struct vason_getArg *argTypes);

vason_container vason_parseString(AllocatorV allocator, slice(c8) string);
vason_container vason_parseString_Lazy(AllocatorV allocator, slice(c8) string);
  #if defined __cplusplus
typedef struct vason {
  vason_container *origional;
  vason_index place;
  inline constexpr vason(vason_container &c) : origional(&c), place(c.current) {}
  inline constexpr vason(vason_container *c, vason_index i) : origional(c), place(i) {}
  inline operator vason_container() const {
    return (vason_container){
        .current = place,
        .tags = origional->tags,
        .tables_strings = origional->tables_strings,
        .text = origional->text,
    };
  }

  inline vason_tag tag() const {
    return origional && place < msList_len(origional->tags)
               ? origional->tags[place]
               : vason_INVALID;
  }
  inline usize countChildren() const {
    return tag() == vason_TABLE
               ? origional->tables_strings[place].end -
                     origional->tables_strings[place].start
               : 0;
  }
  bool simpleArray() {
    if (tag() != vason_TABLE)
      return false;
    for (auto i = origional->tables_strings[place].start;
         i < origional->tables_strings[place].end;
         i++) {
      if (origional->tags[i] == vason_PAIR)
        return false;
    }
    return true;
  }
  bool simpleMap() {
    if (tag() != vason_TABLE)
      return false;
    for (auto i = origional->tables_strings[place].start;
         i < origional->tables_strings[place].end;
         i++) {
      if (origional->tags[i] != vason_PAIR)
        return false;
    }
    return true;
  }
  inline vason operator[](usize idx) {
    return (vason){
        origional,
        vason_get_idx(origional, place, idx),
    };
  }
  inline vason operator[](fptr c) {
    return (vason){
        origional,
        vason_get_str(origional, place, c),
    };
  }
  inline vason operator[](const std::string &c) {
    return (vason){
        origional,
        vason_get_str(
            origional, place,
            (fptr){
                c.length(),
                (u8 *)c.c_str(),
            }
        ),
    };
  }
  // inline vason operator[](const char *c) {
  //   return (vason){
  //       origional,
  //       vason_get_str(
  //           origional, place,
  //           fptr_CS((void *)c)
  //       ),
  //   };
  // }
  // inline vason operator[](char *c) { return (*this)[c]; }
  fptr asString() const {
    return this->tag() == vason_STRING
               ? (fptr){
                     (usize)origional->tables_strings[place].end -
                         origional->tables_strings[place].start,
                     origional->tables_strings[place].start + origional->text.ptr
                 }
               : nullFptr;
  }
  explicit operator bool() const { return tag() != vason_INVALID; }
} vason;

    #if defined(__EMSCRIPTEN__)
      #include <emscripten/bind.h>
// TODO
using namespace emscripten;

vason vason_get_idx_wrapper(vason &v, usize idx) { return v[idx]; }
vason vason_get_str_wrapper(vason &v, const std::string &key) { return v[key]; }

std::string vason_as_js_string(const vason &v) {
  fptr f = v.asString();
  if (!f.width && f.ptr)
    return "";
  return std::string((char *)f.ptr, f.width);
}

EMSCRIPTEN_BINDINGS(vason_module) {
  enum_<vason_tag>("vason_tag")
      .value("vason_STRING", vason_STRING)
      .value("vason_TABLE", vason_TABLE)
      .value("vason_PAIR", vason_PAIR)
      .value("vason_UNPARSED", vason_UNPARSED)
      .value("vason_INVALID", vason_INVALID);

  class_<vason>("vason")
      .function("tag", &vason::tag)
      .function("countChildren", &vason::countChildren)
      .function("simpleArray", &vason::simpleArray)
      .function("simpleMap", &vason::simpleMap)
      .function("isValid", &vason::operator bool)
      .function("getIdx", &vason_get_idx_wrapper)
      .function("getStr", &vason_get_str_wrapper)
      .function("asString", &vason_as_js_string);

  function("parseString", optional_override([](const std::string &input) {
             u8 *perm_str = (u8 *)aAlloc(stdAlloc, input.length());
             memcpy(perm_str, input.c_str(), input.length());
             slice(c8) text = {(usize)input.length(), perm_str};
             vason_container *res = new vason_container(vason_parseString(stdAlloc, text));
             return vason(res, 0);
           }));
}
    #endif
  #endif
void vason_lazy_expand(vason_container *c, vason_index current);
slice(c8) vason_tostr(AllocatorV allocator, vason_container c);

#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define VASON_PARSER_C (1)
#endif
#if defined(VASON_PARSER_C)
typedef enum : c8 {
  vason_STR,
  vason_STR_DELIM,
  vason_TABLE_START,
  vason_TABLE_END,
  vason_TABLE_DELIM,
  vason_ESCAPE,
  vason_PAIR_DELIM,
} vason_token_t;
sliceDef(vason_token_t);
  // makes parsing json with this a lil easier
  #if defined(__cplusplus)

vason_token_t to_token(c8 in) {
  switch (in) {
    case '[':
      return vason_TABLE_START;
    case '{':
      return vason_TABLE_START;
    case ']':
      return vason_TABLE_END;
    case '}':
      return vason_TABLE_END;
    case ',':
      return vason_TABLE_DELIM;
    case '\\':
      return vason_ESCAPE;
    case '"':
      return vason_STR_DELIM;
    case ':':
      return vason_PAIR_DELIM;
    default:
      return vason_STR;
  }
}
  #else
DIAGNOSTIC_PUSH("-Winitializer-overrides")
static constexpr vason_token_t vason_tokens_lut[256] = {
    [0 ... 255] = vason_STR,

    ['['] = vason_TABLE_START,
    ['{'] = vason_TABLE_START,
    [']'] = vason_TABLE_END,
    ['}'] = vason_TABLE_END,
    [','] = vason_TABLE_DELIM,
    ['\\'] = vason_ESCAPE,
    ['"'] = vason_STR_DELIM,
    [':'] = vason_PAIR_DELIM,

};
DIAGNOSTIC_POP()
    #define to_token(in) vason_tokens_lut[in]
  #endif
// __attribute__((always_inline)) static inline vason_token_t(to_token)(c8 in) {
//   return vason_tokens_lut[in];
// }

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
          .tokens /*         */ = NULL,
      };
  return res;
}
void vason_container_free(vason_container container) {
  AllocatorV alocator = container.allocator;
  msList_deInit(alocator, container.tags);
  msList_deInit(alocator, container.tables_strings);
  if (container.tokens) {
    aFree(alocator, container.tokens->ptr);
    aFree(alocator, container.tokens);
  }
}

void vason_tokenize(slice(vason_token_t) res, slice(c8) cs) {
  vason_token_t *__restrict out = res.ptr;
  const c8 *__restrict in = cs.ptr;
  const usize len = res.len;
  out[len - 1] = to_token(in[len - 1]);

  for (usize i = 0; i + 1 < len; i++) {
    out[i] = to_token(in[i]);
    if (__builtin_expect(out[i] == vason_ESCAPE, 0)) {
      out[i] = vason_STR;
      out[++i] = vason_STR;
    } else if (__builtin_expect(out[i] == vason_STR_DELIM, 0)) {
      while (i < len - 1 && to_token(in[++i]) != vason_STR_DELIM) {
        if (to_token(in[i]) == vason_ESCAPE) {
          out[i] = vason_STR;
          out[++i] = vason_STR;
        } else {
          out[i] = vason_STR;
        }
      }
      out[i] = vason_STR_DELIM;
    }
  }
}
vason_tag figureType(slice(vason_token_t) in) {
  for (usize i = 0; i < in.len; i++) {
    if (in.ptr[i] == vason_PAIR_DELIM) {
      return vason_PAIR;
    } else if (in.ptr[i] == vason_TABLE_START) {
      return vason_TABLE;
    }
  }
  return vason_STRING;
}

void vason_parse_level1(
    vason_span span,
    slice(vason_token_t) tokens,
    vason_container *parent
) {
  vason_index tstack = 0;
  vason_index i = span.start;
  vason_index j = span.start;
  for (; i < span.end; i++) {

    switch (tokens.ptr[i]) {
      case vason_TABLE_START: {
        tstack++;
      } break;
      case vason_TABLE_END: {
        if (tstack) {
          tstack--;
        } else {
          msList_push(parent->allocator, parent->tags, vason_INVALID);
          msList_push(parent->allocator, parent->tables_strings, (vason_span){});
          return;
        }
      } break;
      case vason_TABLE_DELIM: {
        if (!tstack) {
          vason_span vs = {j, i};
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
          j = i + 1;
        }
      } break;
      default:
        break;
        ;
    }
  }
  if (j < span.end) {
    vason_span vs = {j, span.end};
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
  }
}

void vason_parse_level2_it(
    vason_container *parent,
    slice(vason_token_t) tokens,
    vason_index i
) {
  if (!(parent->tags[i] & vason_UNPARSED))
    return;

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
      vason_index s = span.start;
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
void vason_parse_level2(
    vason_container *parent,
    slice(vason_token_t) tokens
) {
  for (usize i = 0; i < msList_len(parent->tags); ++i)
    vason_parse_level2_it(parent, tokens, i);
}

vason_container vason_parseString(AllocatorV allocator, slice(c8) string) {
  vason_container res = vason_container_create(string, allocator);
  slice(vason_token_t) tokens = (typeof(tokens)){
      string.len,
      (vason_token_t *)aAlloc(allocator, sizeof(vason_token_t) * string.len)
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
vason_container vason_parseString_Lazy(AllocatorV allocator, slice(c8) string) {
  vason_container res = vason_container_create(string, allocator);
  slice(vason_token_t) *tokens = aCreate(allocator, slice(vason_token_t));

  *tokens = (typeof(*tokens)){
      string.len,
      (vason_token_t *)aAlloc(allocator, sizeof(vason_token_t) * string.len),
  };
  vason_tokenize(*tokens, string);
  vason_parse_level1(
      (vason_span){
          .start = 0,
          .end = (vason_index)string.len,
      },
      *tokens,
      &res
  );
  vason_parse_level2_it(&res, *tokens, 0);
  res.current = 0;
  res.tokens = bitcast(typeof(res.tokens), tokens);
  return res;
}

vason_index vason_get_str(vason_container *c, vason_index entry, fptr f) {
  if (msList_len(c->tags) <= entry)
    return -1;
  if (
      c->tags[entry] == vason_TABLE
  ) {
    vason_span vs = c->tables_strings[entry];
    for (vason_index i = vs.start; i < vs.end; i++) {
      if (c->tags[i] & vason_PAIR && c->tags[i] & vason_UNPARSED) {
        assertMessage(c->tokens, "unparsed inside non-lazy object");
        vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, i);
      }
      if (c->tags[i] == vason_PAIR) {
        vason_span children = c->tables_strings[i];
        assertMessage(children.end - children.start == 2);

        vason_index ikey = children.start;
        vason_index ival = children.start + 1;
        if (c->tags[ikey] & vason_UNPARSED) {
          assertMessage(c->tokens, "unparsed inside non-lazy object");
          vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, ikey);
        }
        assertMessage(c->tags[ikey] == vason_STRING);
        vason_span key_text = c->tables_strings[ikey];
        fptr cs = (fptr){(usize)key_text.end - key_text.start, c->text.ptr + key_text.start};

        if (fptr_eq(cs, f)) {
          if (c->tags[ival] & vason_UNPARSED) {
            assertMessage(c->tokens, "unparsed inside non-lazy object");
            vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, ival);
          }
          return ival;
        }
      }
    }
  }
  return -1;
}
vason_index vason_get_idx(vason_container *c, vason_index entry, vason_index f) {
  if (msList_len(c->tags) <= entry)
    return -1;
  else if (
      !EQUAL_ANY(c->tags[entry], vason_TABLE, vason_PAIR) ||
      c->tables_strings[entry].end <= c->tables_strings[entry].start + f
  )
    return -1;

  vason_index res = c->tables_strings[entry].start + f;
  if (c->tags[res] & vason_UNPARSED) {
    assertMessage(c->tokens, "unparsed inside non-lazy object");
    vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, res);
  }
  return res;
}
vason_index vason_get_func(vason_container *c, vason_index entry, struct vason_getArg *argTypes) {
  while (!argTypes->terminator && c->current < msList_len(c->tags)) {
    if (argTypes->is_fat_pointer) {
      entry = vason_get_str(c, entry, *(fptr *)argTypes->_fptr);
    } else if (argTypes->is_unsigned_integer) {
      entry = vason_get_idx(c, entry, *(u32 *)argTypes->_u32);
    } else {
      __builtin_unreachable();
    }
    argTypes++;
  }
  return entry;
}

void vason_lazy_expand(vason_container *c, vason_index current) {
  assertMessage(!(c->tags[current] & vason_UNPARSED));
  switch (c->tags[current]) {
    case vason_STRING:
      break;
    case vason_TABLE:
    case vason_PAIR: {
      vason_span vs = c->tables_strings[current];
      usize len = msList_len(c->tables_strings);
      for (vason_index i = vs.start; i < vs.end; i++) {
        if (c->tags[i] & vason_UNPARSED) {
          assertMessage(c->tokens, "unparsed inside non-lazy object");
          vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, i);
        }
      }
      for (vason_index i = len; i < msList_len(c->tables_strings); i++) {
        if (c->tags[i] & vason_UNPARSED) {
          assertMessage(c->tokens, "unparsed inside non-lazy object");
          vason_parse_level2_it(c, *(slice(vason_token_t) *)c->tokens, i);
        }
      }
      break;
    }
    case vason_INVALID:
      c->tags[current] = vason_INVALID;
      break;
    case vason_UNPARSED:
      __builtin_unreachable();
      break;
  }
}
slice(c8) vason_tostr(AllocatorV allocator, vason_container c) {
  mList(c8) res = mList_init(allocator, c8);
  defer { aFree(allocator, res); };

  switch (c.tags[c.current]) {
    case vason_STRING: {
      vason_span vs = c.tables_strings[c.current];
      bool escaped = false;
      for (auto i = vs.start; i < vs.end; i++) {
        if (escaped) {
          escaped = false;
        } else if (to_token(c.text.ptr[i]) == vason_ESCAPE) {
          escaped = true;
        } else if (to_token(c.text.ptr[i]) != vason_STR) {
          mList_push(res, '\\');
        }
        mList_push(res, c.text.ptr[i]);
      }
    } break;
    case vason_PAIR: {
      vason_span vs = c.tables_strings[c.current];
      for (auto i = 0; i < 2; i++) {
        if (i)
          mList_push(res, ':');
        auto temp = (vason_container){
            .current = i + vs.start,
            .tags = c.tags,
            .tables_strings = c.tables_strings,
            .text = c.text,
        };
        slice(c8) that = vason_tostr(allocator, temp);
        defer { aFree(allocator, that.ptr); };
        mList_pushVla(res, *slice_vla(that));
      }
    } break;
    case vason_TABLE: {
      vason_span vs = c.tables_strings[c.current];
      mList_push(res, '{');
      for (auto i = vs.start; i < vs.end; i++) {
        if (i - vs.start)
          mList_push(res, ',');
        auto temp = (vason_container){
            .current = i,
            .tags = c.tags,
            .tables_strings = c.tables_strings,
            .text = c.text,
        };
        slice(c8) that = vason_tostr(allocator, temp);
        defer { aFree(allocator, that.ptr); };
        mList_pushVla(res, *slice_vla(that));
      }
      mList_push(res, '}');
    } break;
    case vason_UNPARSED:
    case vason_INVALID: {
    } break;
  }

  slice(c8) result = (slice(c8))slice_stat(*mList_vla(res));
  return result;
}

#endif
