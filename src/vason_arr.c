#include "../vason_arr.h"
#include "../fptr.h"
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
static constexpr vason_token_t vason_tokens_lut[256] = {

    ['"'] = vason_STR_DELIM,
    [','] = vason_TABLE_DELIM,
    [':'] = vason_PAIR_DELIM,
    ['['] = vason_TABLE_START,
    ['\\'] = vason_ESCAPE,
    [']'] = vason_TABLE_END,
    ['{'] = vason_TABLE_START,
    ['}'] = vason_TABLE_END,

};
static_assert(vason_tokens_lut[0] == vason_STR);
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
slice(c8) vason_container_asString(vason_container c) {
  vason_span vs = c.tables_strings[c.current];
  slice(c8) res = (slice(c8)){
      (usize)(vs.end - vs.start),
      c.text.ptr + vs.start,
  };
  if (res.ptr + res.len > c.text.ptr + c.text.len)
    return nullslice(c8);
  return res;
}
bool vason_container_eq(vason_container a, vason_container b) {
  bool validA = a.current < msList_len(a.tables_strings);
  bool validB = b.current < msList_len(b.tables_strings);

  if (!validA || !validB) {
    return validA == validB;
  }

  if (a.tags[a.current] != b.tags[b.current]) {
    return false;
  }

  switch (a.tags[a.current]) {
    case vason_STRING: {
      slice(c8) as = vason_container_asString(a);
      slice(c8) bs = vason_container_asString(b);
      return fptr_eq((fptr){as.len, (u8 *)as.ptr}, (fptr){bs.len, (u8 *)bs.ptr});
    }

    case vason_INVALID:
      return true;

    case vason_PAIR:
    case vason_TABLE: {
      vason_span as = a.tables_strings[a.current];
      vason_span bs = b.tables_strings[b.current];

      usize la = as.end - as.start;
      usize lb = bs.end - bs.start;

      if (la != lb)
        return false;

      if ((u8 *)(a.tables_strings + as.end) > (u8 *)(msList_vla(a.tables_strings)[1]))
        return false;
      if ((u8 *)(b.tables_strings + bs.end) > (u8 *)(msList_vla(b.tables_strings)[1]))
        return false;

      for (each_RANGE(usize, i, 0, la)) {
        vason_container ia = a;
        vason_container ib = b;
        ia.current = as.start + i;
        ib.current = bs.start + i;
        if (!vason_container_eq(ia, ib))
          return false;
      }
      return true;
    }

    case vason_UNPARSED:
      assertMessage(false, "equality check for unparsed");
      return false;
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
  assertMessage(string.len < (usize)((vason_index)(-1)));
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
  if (msList_len(res.tags) > 1) { // something like a,b -> {a,b}, otherwise b will not be acessible
    vason_span range = (vason_span){
        .start = 1,
        .end = (vason_index)(msList_len(res.tags) + 1),
    };
    msList_ins(allocator, res.tags, 0, vason_TABLE);
    msList_ins(allocator, res.tables_strings, 0, range);
    for (vason_index i = range.start; i < range.end; i++) {
      if (res.tags[i] == vason_TABLE) {
        res.tables_strings[i].start++;
        res.tables_strings[i].end++;
      }
    }
  }
  vason_parse_level2(&res, tokens);
  res.current = 0;
  assertMessage(msList_len(res.tables_strings) == msList_len(res.tags));
  return res;
}
vason_container vason_parseString_Lazy(AllocatorV allocator, slice(c8) string) {
  assertMessage(string.len < (usize)((vason_index)(-1)));
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
  if (msList_len(res.tags) > 1) { // something like a,b
    vason_span range = (vason_span){
        .start = 1,
        .end = (vason_index)(msList_len(res.tags) + 1),
    };
    msList_ins(allocator, res.tags, 0, vason_TABLE);
    msList_ins(allocator, res.tables_strings, 0, range);
    for (vason_index i = range.start; i < range.end; i++) {
      if (res.tags[i] == vason_TABLE) {
        res.tables_strings[i].start++;
        res.tables_strings[i].end++;
      }
    }
  } else
    vason_parse_level2_it(&res, *tokens, 0);

  res.current = 0;
  res.tokens = bitcast(typeof(res.tokens), tokens);
  assertMessage(msList_len(res.tables_strings) == msList_len(res.tags));
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
        fptr cs = (fptr){(usize)key_text.end - key_text.start, (u8 *)(c->text.ptr + key_text.start)};

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
void vason_tostr_lesser(vason_container c, mList(c8) res) {
  switch (c.tags[c.current]) {
    case vason_STRING: {
      vason_span vs = c.tables_strings[c.current];
      bool escaped = false;

      vason_index count = 0;
      for (auto i = vs.start; i < vs.end; i++) {
        vason_token_t token = to_token(c.text.ptr[i]);
        if (escaped)
          escaped = false;
        else if (token == vason_ESCAPE)
          escaped = true;
        else if (token != vason_STR)
          count++;
      }

      escaped = false;
      if (count >= 2)
        mList_push(res, '"');

      for (auto i = vs.start; i < vs.end; i++) {
        vason_token_t token = to_token(c.text.ptr[i]);
        if (escaped)
          escaped = false;
        else if (token == vason_ESCAPE)
          escaped = true;
        else if (count < 2 && token != vason_STR)
          mList_push(res, '\\');
        else if (count >= 2 && token == vason_STR_DELIM)
          mList_push(res, '\\');
        mList_push(res, c.text.ptr[i]);
      }

      if (count >= 2)
        mList_push(res, '"');
    } break;
    case vason_PAIR: {
      vason_span vs = c.tables_strings[c.current];
      for (auto i = 0; i < 2; i++) {
        if (i)
          mList_push(res, ':');
        auto temp = (vason_container){
            .current = (vason_index)(i + vs.start),
            .tags = c.tags,
            .tables_strings = c.tables_strings,
            .text = c.text,
        };
        vason_tostr_lesser(temp, res);
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
        vason_tostr_lesser(temp, res);
      }
      mList_push(res, '}');
    } break;
    case vason_UNPARSED:
    case vason_INVALID: {
    } break;
  }
}
slice(c8) vason_tostr(AllocatorV allocator, vason_container c) {
  mList(c8) res = mList_init(allocator, c8);
  defer { aFree(allocator, res); };

  vason_tostr_lesser(c, res);

  slice(c8) result = (slice(c8))slice_stat(*mList_vla(res));
  return result;
}
usize vason_container_footprint(vason_container c) {
  usize res = 0;
  res += sizeof(*msList_vla(c.tags)) + sizeof(sList_header);
  res += sizeof(*msList_vla(c.tables_strings)) + sizeof(sList_header);
  if (c.tokens) {
    res += sizeof(c.tokens);
    res += sizeof(c.tokens->ptr[0]) * c.tokens->len;
  }
  return res;
}
