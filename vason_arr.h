#if !defined(VASON_PARSER_H)
  #define VASON_PARSER_H (1)
  #include "allocator.h"
  #include "my-list.h"
  #include "mytypes.h"
  #include "print.h"
  #include "shortList.h"
typedef u16 vason_index;
typedef struct {
  vason_index offset, len;
} vason_span;

typedef enum : u8 {
  vason_STRING /*  */ = 1 << 0,
  vason_TABLE /*   */ = 1 << 1,
  vason_PAIR /*    */ = 1 << 2,
  vason_UNPARSED /**/ = 1 << 7,
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
void vason_container_free(vason_container *container);

  #if defined __cplusplus
typedef struct vason {
  vason_container self;
  // inline vason_tag tag() const { return self.top.tag; }
  // inline usize countChildren() const {
  //   return self.top.span.len;
  // }
  // struct vason operator[](usize idx) {
  //   auto r = *this;
  //   r.self.top = vason_arrGet(self.top, self, (u32)idx);
  //   return r;
  // }
  // struct vason operator[](fptr c) {
  //   auto r = *this;
  //   r.self.top = vason_mapGet(self.top, self, c);
  //   return r;
  // }
  // fptr asString() {
  //   switch (self.top.tag) {
  //     case vason_STR:
  //     case vason_ID:
  //       return vason_getString(self.top, self);
  //     default:
  //       return nullFptr;
  //   }
  // }
  // struct vason operator[](const std::string &c) { return (*this)[(fptr){c.length(), (u8 *)c.c_str()}]; }
  // struct vason operator[](const char *c) { return (*this)[(fptr){strlen(c), (u8 *)c}]; }
  // explicit operator bool() const { return tag() != vason_INVALID; }
} vason;
  #endif
vason_container vason_parseString(AllocatorV allocator, slice(c8) string);
#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define VASON_PARSER_C (1)
#endif
#if defined(VASON_PARSER_C)
typedef enum : c8 {
  vason_TABLE_START = '<',
  vason_TABLE_END = '>',
  vason_TABLE_DELIM = ',',
  vason_STR = ' ',
  vason_STR_DELIM = ':',
  vason_ESCAPE = '\\',
  vason_PAIR_DELIM = ':',
} vason_token_t;
constexpr struct {
  vason_tag string, table, pair, unparsed;
} vason_tags = {
    vason_STRING, vason_TABLE, vason_PAIR, vason_UNPARSED
};
constexpr struct {
  vason_token_t table_start, table_end, table_delim, str, str_delim, escape, pair_delim;
} vason_token = {
    vason_TABLE_START, vason_TABLE_END, vason_TABLE_DELIM, vason_STR, vason_STR_DELIM, vason_ESCAPE, vason_PAIR_DELIM
};
static inline vason_token_t to_token(c8 in) {
  switch (in) {
    case '<':
    case '{':
      return vason_token.table_start;
    case '>':
    case '}':
      return vason_token.table_end;
    case ',':
    case ';':
      return vason_token.table_delim;
    case '\\':
      return vason_token.escape;
    case '\'':
      return vason_token.str_delim;
    case ':':
      return vason_token.pair_delim;
    default:
      return vason_token.str;
  }
}

inline bool isIgnored(c8 in) { return in <= ' ' && in; }
inline vason_span basic_trim(slice(c8) in, vason_span span) {
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
inline vason_span string_reduce(slice(c8) str, vason_span span) {
  span = basic_trim(str, span);
  if (to_token(str.ptr[span.offset]) == vason_token.str_delim) {
    span.offset++;
    span.len -= 2;
  }
  return span;
}
inline vason_span table_reduce(slice(c8) str, vason_span span) {
  while (span.len && to_token(str.ptr[span.offset]) != vason_token.table_start) {
    if (span.len && to_token(str.ptr[span.offset]) == vason_token.escape) {
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
  while (span.len && to_token(str.ptr[span.offset + span.len - 1]) != vason_token.table_end) {
    if (span.len && to_token(str.ptr[span.offset + span.len - 1]) == vason_token.escape)
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
void vason_container_free(vason_container container, slice(vason_token_t) tokens) {
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
    if (res.ptr[i] == vason_token.escape) {
      res.ptr[i] = vason_token.str;
      res.ptr[i + 1] = vason_token.str;
      i++;
    }

  for (auto i = 0; i < res.len; i++)
    if (res.ptr[i] == vason_token.str_delim) {
      i++;
      while (i < res.len && res.ptr[i] != vason_token.str_delim) {
        res.ptr[i] = vason_token.str;
        i++;
      }
    }

  return res;
}
void vason_parse_level1(
    vason_span span,
    slice(vason_token_t) tokens,
    vason_container *parent
) {
  mList_scoped(vason_token_t) tstack = mList_init(parent->allocator, vason_token_t);
  mList_scoped(vason_span) parts = mList_init(parent->allocator, vason_span);
  auto i = span.offset;
  auto j = span.offset;
  // for (; i < span.offset + span.len; i++) {
  //   if (tokens.ptr[i] == vason_token.table_start) {
  //   }
  // }
}

void vason_print(vason_container *c, usize currentIndex, usize indentLevel) {
  fptr tabs = (fptr){
      indentLevel * 2,
      (c8 *)alloca(indentLevel * 2),
  };
  memset(tabs.ptr, ' ', tabs.width);

  if (c->tags[currentIndex] & vason_tags.unparsed) {

    vason_span s = c->tables_strings[c->indexes[currentIndex]];
    fptr fpart = {s.len, s.offset + c->text.ptr};
    println("{}( unparsed ) /{{}/}", tabs, fpart);

  } else

    switch (c->tags[currentIndex]) {
      case vason_tags.unparsed:
        __builtin_unreachable();

      case vason_tags.pair: {

        println("{}(s:)", tabs);
        vason_span s = c->tables_strings[c->indexes[currentIndex]];
        for (auto j = s.offset; j < s.len + s.offset; j++)
          vason_print(c, j, indentLevel + 1);

      } break;

      case vason_tags.string: {

        vason_span s = c->tables_strings[c->indexes[currentIndex]];
        fptr fpart = {s.len, s.offset + c->text.ptr};
        println("{}('') {}", tabs, fpart);

      } break;

      case vason_tags.table: {

        println("{}(<>)", tabs);
        vason_span s = c->tables_strings[c->indexes[currentIndex]];
        for (auto j = s.offset; j < s.len + s.offset; j++)
          vason_print(c, j, indentLevel + 1);

      } break;
    }
}
void vason_parse_level2(
    vason_container *parent
) {
  vason_print(parent, 0, 0);
  for (usize i = 0; i < msList_len(parent->tags); ++i) {
    if (!(parent->tags[i] & vason_tags.unparsed))
      continue;

    switch (parent->tags[i] ^ vason_tags.unparsed) {
      case vason_tags.string: {
        vason_span span = parent->tables_strings[parent->indexes[i]];
        span = string_reduce(parent->text, span);
        parent->tables_strings[parent->indexes[i]] = span;
        parent->tags[i] = vason_tags.string;
      } break;

      case vason_tags.table: {
        vason_span span = parent->tables_strings[parent->indexes[i]];
        span = table_reduce(parent->text, basic_trim(parent->text, basic_trim(parent->text, span)));
        usize offset = msList_len(parent->tables_strings);
        vason_parse_level1(span, parent);
        usize len = msList_len(parent->tables_strings) - offset;
        parent->tables_strings[parent->indexes[i]] = (vason_span){
            .offset = (vason_index)(offset),
            .len = (vason_index)(len),
        };
        parent->tags[i] = vason_tags.table;
      } break;

      case vason_tags.pair: {
        assertMessage(false, "unimplemented");
      } break;
    }
  }
}

vason_container vason_parseString(AllocatorV allocator, slice(c8) string) {
  auto res = vason_container_create(string, allocator);
  vason_parse_level1(
      (vason_span){
          .offset = 0,
          .len = (vason_index)string.len,
      },
      &res
  );
  vason_parse_level2(&res);
  res.current = 0;
  return res;
}
vason_container(vason_get)(vason_container c, vason_tag *argTypes, ...) {
  assertMessage(false, "unimplemented");
  return c;
}
#endif
