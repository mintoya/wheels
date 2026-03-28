// Part of macros but got too long

#define TU_MK_TAG(t, n) ID_CONCAT(n, TU_TAGENUM_##t) // name of enum
#define TU_MK_UF_(t) t _##t;                         // union fields ;
#define TU_MK_TAGENUM(name, t) TU_MK_TAG(name, t),   // enum fields ,

#define TU_MK_INIT_(type, t) ._##t = TU_MK_TAGENUM(type, t)

#define TAGGED_UNION(name, tagtype, ...)           \
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
#define TU_MAKE_CASE_UNWRAP(a, b, c, d) \
  if (b.tag == TU_MK_TAG(a, c)) {       \
    var_ temp__ = ID_CONCAT(b._, c);    \
    var_ b = temp__;                    \
    d;                                  \
  } else

#define TU_MAKE_CASE(a, b) \
  TU_MAKE_CASE_UNWRAP(     \
      TUPLE_EXPAND_A(a),   \
      TUPLE_EXPAND_B(a),   \
      TUPLE_EXPAND_A(b),   \
      TUPLE_EXPAND_B(b)    \
  )

#define TU_MATCH(u_type, ...)                       \
  do {                                              \
    APPLY_N_WITH(TU_MAKE_CASE, u_type, __VA_ARGS__) \
    assert("unhandled type" && false);              \
  } while (0);
#define TU_OF(enum, type, value) \
  (enum) { .tag = TU_MK_TAG(enum, type), ._##type = value }
