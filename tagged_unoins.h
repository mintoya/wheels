#if !defined(MY_TAGGED_UNIONS_H)

  #include "macros.h"
// Part of macros but got too long

  #define TU_NAM(t) _##t
  #define TU_MK_TAG(t, n) ID_CONCAT(n, ID_CONCAT(TU_TAGENUM_, t)) // name of enum
  #define TU_MK_UF_(t) t TU_NAM(t);                               // union fields ;
  #define TU_MK_TAGENUM(name, t) TU_MK_TAG(name, t),              // enum fields ,
  #define TU_MK_INIT_(type, t) .TU_NAM(t) = TU_MK_TAGENUM(type, t)

  #define TU_DEFINE(name, tagtype, ...)              \
    typedef enum : tagtype {                         \
      APPLY_N_WITH(TU_MK_TAGENUM, name, __VA_ARGS__) \
    } name##realEnum;                                \
    /* the actual onion */                           \
    typedef struct {                                 \
      name##realEnum tag;                            \
      union {                                        \
        APPLY_N(TU_MK_UF_, __VA_ARGS__)              \
      };                                             \
    } name;
  #define TU_TAG(name, type) TU_MK_INIT_(name, type)
  #define TU_MAKE_CASE_UNWRAP(a, b, c, d)                   \
    case (TU_MK_TAG(a, c)): {                               \
      __auto_type ID_CONCAT(temp, TU_NAM(c)) = b.TU_NAM(c); \
      __auto_type b = (c)ID_CONCAT(temp, TU_NAM(c));        \
      d                                                     \
    } break; // :( no fallthrough

  #define TU_MAKE_CASE_UNWRAP_default(a, b, c, d) \
    default: {                                    \
      d                                           \
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
    (TUPLE_EXPAND_A(a),                                     \
     TUPLE_EXPAND_B(a),                                     \
     TUPLE_EXPAND_A(b),                                     \
     TUPLE_EXPAND_B(b))

  #define TU_MATCH(type_val_tuple, ... /* type and statement tuples */)           \
    /* The first tuple contains the type name of the variable and the variable */ \
    /* The first arg of every tuple is the type, the second is the statement*/    \
    /* The word 'default' will place the statement in a default statement */      \
    /* Automatically casts input into correct type output   */                    \
    do {                                                                          \
      var_ local_item = TUPLE_EXPAND_B(type_val_tuple);                           \
      var_ tmp = ASSERT_EXPR(                                                     \
          types_eq(TUPLE_EXPAND_A(type_val_tuple), typeof(local_item)), ""        \
      );                                                                          \
      switch ((TUPLE_EXPAND_B(type_val_tuple)).tag) {                             \
        APPLY_N_WITH(TU_MAKE_CASE, type_val_tuple, __VA_ARGS__)                   \
      }                                                                           \
    } while (0);
  #define TU_OF_IM(enum, t, value) \
    (enum) { .tag = TU_MK_TAG(enum, t), .TU_NAM(t) = value }
  #define TU_OF(enumt, value) \
    TU_OF_IM(TUPLE_EXPAND_A(enumt), TUPLE_EXPAND_B(enumt), value)
// it doesn't have to be a tuple but might as well
  #define TU_ACCESS(type_val_tuple, variant_val_tuple)                                    \
    (TUPLE_EXPAND_B(type_val_tuple).tag ==                                                \
             TU_MK_TAG(TUPLE_EXPAND_A(type_val_tuple), TUPLE_EXPAND_A(variant_val_tuple)) \
         ? TUPLE_EXPAND_B(type_val_tuple).TU_NAM(TUPLE_EXPAND_A(variant_val_tuple))       \
         : TUPLE_EXPAND_B(variant_val_tuple))

#endif // MY_TAGGED_UNIONS_H
