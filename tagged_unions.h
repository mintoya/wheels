#if !defined(MY_TAGGED_UNIONS_H)
  #define MY_TAGGED_UNIONS_H (1)

  #include "macros.h"
// Part of macros but got too long

  #define TU_NAM(t) _##t
  #define TU_MK_TAG(t, n) ID_CONCAT(n, ID_CONCAT(TU_TAGENUM_, t)) // name of enum
  #define TU_MK_TAGEN(t, n) TU_MK_TAG(t, n),                      // name of enum
  #define TU_MK_UF_(t) t TU_NAM(t);                               // union fields ;
  #define TU_MK_INIT_(type, t) .TU_NAM(t) = TU_MK_TAG(type, t)

  #define TU_DEFINE(name_tatt, ...)                                       \
    typedef enum : TUPLE_EXPAND_B(name_tatt){                             \
        APPLY_N_WITH(TU_MK_TAGEN, TUPLE_EXPAND_A(name_tatt), __VA_ARGS__) \
    } ID_CONCAT(TUPLE_EXPAND_A(name_tatt), realEnum);                     \
    /* the actual onion */                                                \
    typedef struct TUPLE_EXPAND_A(name_tatt) {                            \
      ID_CONCAT(TUPLE_EXPAND_A(name_tatt), realEnum)                      \
      tag;                                                                \
      union {                                                             \
        APPLY_N(TU_MK_UF_, __VA_ARGS__)                                   \
      };                                                                  \
    } TUPLE_EXPAND_A(name_tatt);
  #define TU_TAG(name, type) TU_MK_INIT_(name, type)

  #define TU_MAKE_CASE_UNWRAP(a, type, block)                                     \
    case (TU_MK_TAG(TUPLE_EXPAND_A(a), type)): {                                  \
      __auto_type ID_CONCAT(temp, TU_NAM(type)) = TUPLE_EXPAND_B(a).TU_NAM(type); \
      __auto_type $in = ID_CONCAT(temp, TU_NAM(type));                            \
      block                                                                       \
    } break; // :( no fallthrough

  #define TU_MAKE_CASE_UNWRAP_default(a, c, d) \
    default: {                                 \
      d                                        \
    } break;

  #define TU_ISMATCH_default placeholder, 1
  #define TU_ISMATCH_SECOND(a, b, ...) b
  #define TU_ISMATCH_EXPAND(...) TU_ISMATCH_SECOND(__VA_ARGS__, 0)
  #define TU_ISMATCH(x) TU_ISMATCH_EXPAND(ID_CONCAT(TU_ISMATCH_, x))

  #define TU_IF_default(n) ID_CONCAT(TU_IF_default, n)
  #define TU_IF_default0 TU_MAKE_CASE_UNWRAP
  #define TU_IF_default1 TU_MAKE_CASE_UNWRAP_default

  #define TU_MAKE_CASE(a, b)                                \
    ID_CONCAT(TU_IF_default, TU_ISMATCH(TUPLE_EXPAND_A(b))) \
    (a,                                                     \
     TUPLE_EXPAND_A(b),                                     \
     TUPLE_EXPAND_B(b))

  #define TU_MATCH(type_val_tuple, ... /* type and statement tuples */)           \
    /* The first tuple contains the type name of the variable and the variable */ \
    /* The first arg of every tuple is the type, the second is the statement   */ \
    /* The word 'default' will place the statement in a default statement      */ \
    /* Automatically casts input into correct type output as a pointer         */ \
    do {                                                                          \
      var_ local_item = TUPLE_EXPAND_B(type_val_tuple);                           \
      ASSERT_EXPR(                                                                \
          types_eq(TUPLE_EXPAND_A(type_val_tuple), typeof(local_item)), ""        \
      );                                                                          \
      switch ((TUPLE_EXPAND_B(type_val_tuple)).tag) {                             \
        APPLY_N_WITH(TU_MAKE_CASE, type_val_tuple, __VA_ARGS__)                   \
      }                                                                           \
    } while (0);
  #define TU_OF_IM(enum, t, value) \
    (enum) { .tag = TU_MK_TAG(enum, t), .TU_NAM(t) = ((t)value) }
  #define TU_OF(enumt, value) \
    TU_OF_IM(TUPLE_EXPAND_A(enumt), TUPLE_EXPAND_B(enumt), value)
  #define TU_IS(enum_t, a) (a.tag == TU_MK_TAG(TUPLE_EXPAND_A(enum_t), TUPLE_EXPAND_B(enum_t)))

#endif // MY_TAGGED_UNIONS_H
