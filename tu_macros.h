#if !defined(MY_TAGGED_UNIONS_H)
  #define MY_TAGGED_UNIONS_H (1)

  #include "macros.h"

  #define TU_TAG(type) ID_CONCAT(type, _enum)

  #define TU_ENUM(type_d) TU_TAG(TUPLE_FIRST type_d),
  #define TU_TDEF(type_d) typedef TUPLE_REST type_d TUPLE_FIRST type_d;
  #define TU_UMEM(type_d) TUPLE_EXPAND_A(type_d) TUPLE_EXPAND_A(type_d);

  #define tu_def(tagged_union, ...)                    \
    typedef enum : TUPLE_EXPAND_B(tagged_union){       \
        APPLY_N(TU_ENUM, __VA_ARGS__)                  \
    } ID_CONCAT(_enum_, TUPLE_EXPAND_A(tagged_union)); \
    APPLY_N(TU_TDEF, __VA_ARGS__)                      \
    typedef struct TUPLE_EXPAND_A(tagged_union) {      \
      ID_CONCAT(_enum_, TUPLE_EXPAND_A(tagged_union))  \
      tag;                                             \
      union {                                          \
        APPLY_N(TU_UMEM, __VA_ARGS__)                  \
      };                                               \
    } TUPLE_EXPAND_A(tagged_union)

  #define CONCAT_CASE(...) TU_##__VA_ARGS__,

  #define TU_case(...) (TU_CASE, __VA_ARGS__)
  #define TU_default(...) (TU_DEFAULT, __VA_ARGS__)
  #define TU_of(...) (TU_CASE, __VA_ARGS__)
  #define TU_otherwise(...) (TU_DEFAULT, __VA_ARGS__)

  #define REMOVE_PARENT(...) __VA_ARGS__
  #define ADD_ARG(add, ...) (add, REMOVE_PARENT __VA_ARGS__),

  #define tu_match(item, ...)                                                                      \
    do {                                                                                           \
      __auto_type item_eval = item;                                                                \
      switch ((item_eval).tag) {                                                                   \
        APPLY_N(TU_WRAP_CASE, APPLY_N_WITH(ADD_ARG, item_eval, APPLY_N(CONCAT_CASE, __VA_ARGS__))) \
      }                                                                                            \
    } while (0)

  #define TU_WRAP_CASE_UW(item, mac, ...) mac(item, __VA_ARGS__)
  #define TU_WRAP_CASE(...) TU_WRAP_CASE_UW __VA_ARGS__
  #define TU_DEFAULT(item, ...) \
    default: {                  \
      __VA_ARGS__               \
    } break;

  #define TU_CASE(item, type, namedec, ...) \
    case TU_TAG(type): {                    \
      __auto_type namedec = item.type;      \
      __VA_ARGS__                           \
    } break;
  #define tu_of(type, ...) \
    {.tag = TU_TAG(type), .type = (type)__VA_ARGS__}
  #define tu_is(type, item) (item.tag == TU_TAG(type))
  #define tu_or(type, item, other, ...) \
    ({                                  \
      var_ item_eval = item;            \
      tu_is(type, item_eval)            \
          ? item_eval.type              \
          : (type)other __VA_ARGS__;    \
    })
  #define tu_assert(type, item) ({                       \
    var_ item_eval = item;                               \
    assert(tu_is(type, item_eval) && "unexpected type"); \
    item_eval.type;                                      \
  })
  #define tu_catch(type, item, ...) ({      \
    var_ item_eval = item;                  \
    if_unlikely (!tu_is(type, item_eval)) { \
      __VA_ARGS__;                          \
      abort();                              \
    };                                      \
    item_eval.type;                         \
  })

  #define TU_MATCH(item)                                         \
    for (struct { typeof(item) _eval; int _once; } _tu = {(item), 1}; _tu._once; _tu._once = 0) \
      switch ((int)_tu._eval.tag)

  // 2. Prepend a break to terminate the PREVIOUS case, insert the case label,
  //    and use a 2-tier decrementing loop to safely scope the inner payload.
  //    (This preserves 'namedec' directly while handling user 'break;' correctly)
  #define TU_OF(type, namedec)          \
    break;                              \
    case TU_TAG(type):                  \
      for (int _once2 = 1; _once2 > 0;) \
        for (__auto_type namedec = _tu._eval.type; _once2--;)

  // 3. Fallback case using the same prepended break trick
  #define TU_OTHER \
    break;         \
    default:
// example
/*
  #include "bigint.h"
int main(void) {
  // create tagged union of integer types
  // 'u8' first means the type of the tag thi

  //  this is the only way to pack it
  typedef struct __attribute__((packed)) integer integer;
  // after that tuples of type names and actual types
  // will be created as typedefs
  tu_def(
      (integer, u8),

      (u_8, u8),
      (u_16, u16),
      (u_32, u32),
      (u_64, u64),

      (i_8, i8),
      (i_16, i16),
      (i_32, i32),
      (i_64, i64),

      (big, bigint),
  );
  {
    int i = sizeof(integer);
  }

  integer u8 = tu_of(u_8, 5);
  integer b = tu_of(big, bigint_cs(stdAlloc, 10, "1 005 600 000"));

  tu_is(u_8, u8); // true
  tu_is(i_8, u8); // false
  // v here is the type u_8, the assert  will fail if this is false
  var_ v = tu_assert(u_8, u8);
  // v here is the type u_8, it will be 5 if u8 isnt
  var_ v2 = tu_or(u_8, u8, 5);
  // match
  tu_match(
      b,
      of(u_8, i, { i++; }),
      of(u_16, i, {}),
      otherwise({}),
  );
  tu_match(
      b,
      case (u_8, i, {}),
      case (u_16, i, {}),
      default({}), // this will execute
  );
  // same as
  TU_MATCH(b) {
    TU_OF(u_8, i) {}
    TU_OF(u_16, i) {}
    TU_OTHER {}
  }
}
*/
#endif // MY_TAGGED_UNIONS_H
