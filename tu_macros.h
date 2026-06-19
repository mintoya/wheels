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
  #define if_tu_is(variable, type, value)                                                 \
    if (tu_is(type, value))                                                            \
      for (struct {type tv;bool keep; } tu_check_ = {value.type, 1}; tu_check_.keep; tu_check_.keep = 0) \
        for (variable = tu_check_.tv; tu_check_.keep; tu_check_.keep = 0)

#endif // MY_TAGGED_UNIONS_H
