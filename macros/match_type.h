#define match_type_ucast(type, value) \
  _Generic(                           \
      value,                          \
      type: value,                    \
      default: (type){}               \
  )

#define match_type_default 1

#define match_type_items(value, t)                              \
  REM_PAREN IF_IS1(                                             \
      ID_CONCAT(match_type_, TUPLE_EXPAND_FIRST(t)),            \
      (default : ({ TUPLE_EXPAND_REST(t); })),                  \
      (                                                         \
                                                                \
          TUPLE_EXPAND_FIRST(t) : ({                            \
            TUPLE_EXPAND_FIRST(t)                               \
            TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(t))) =        \
                match_type_ucast(TUPLE_EXPAND_FIRST(t), value); \
            TUPLE_EXPAND_REST((TUPLE_EXPAND_REST(t)));          \
          })                                                    \
      )                                                         \
  )

#define match_type(value, ...) \
  _Generic(                    \
      (typeof(value)){},       \
      APPLY_N_WITH_C(          \
          match_type_items,    \
          (value),             \
          __VA_ARGS__          \
      )                        \
  )\
