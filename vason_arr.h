#include <assert.h>
#if !defined(VASON_PARSER_H)
  #define VASON_PARSER_H (1)
  #include "allocator.h"
  #include "fptr.h"
  #include "macros.h"
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
typedef struct vason_container {
  vason_index current;
  vason_tag *tags;
  vason_span *tables_strings;
  AllocatorV allocator;
  slice(c8) text;
  slice(c8) * tokens;
} vason_container;

vason_container vason_container_create(slice(c8) text, AllocatorV allocator);
void vason_container_free(vason_container container);
usize vason_container_footprint(vason_container c);

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
      USENAMEDPRINTER("slice(c8)", ((fptr){(usize)vs.end - vs.start, vs.start + in.text.ptr}));
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
    #if defined(__EMSCRIPTEN__)
  bool is_top() {
    return (origional->current == place);
  }
  void try_free() {
    // horrible terrible no-good very bad
    assertMessage(origional->current == place, "lost the plot");
    vason_container_free(*origional);
  }
    #endif
} vason;

    #if defined(__EMSCRIPTEN__)
      #include <emscripten/bind.h>
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
      .function("asString", &vason_as_js_string)
      .function("istop", &vason::is_top)
      .function("free", &vason::try_free);

  function("parseString", optional_override([](const std::string &input) {
             slice(c8) text = {
                 (usize)input.length(),
                 aCreate(stdAlloc, u8, input.length()),
             };
             memcpy(text.ptr, input.c_str(), input.length());
             vason_container *res = new vason_container(vason_parseString(stdAlloc, text));
             return vason{res, 0};
           }));
}
    #endif
  #endif
void vason_lazy_expand(vason_container *c, vason_index current);
slice(c8) vason_tostr(AllocatorV allocator, vason_container c);

#endif
#if defined(VASON_PARSER_C) && VASON_PARSER_C == (1)
  #define VASON_PARSER_C (2)
  #include "src/vason_arr.c"
#endif
