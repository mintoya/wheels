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
      (default : TUPLE_EXPAND_REST(t)),                         \
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
  )

#define EMPTY()
#define DEFER(id) id EMPTY()

#define PROBE(x) x, 1,
#define IS_PAREN(x) CHECK(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)
#define CHECK(...) CHECK_N(__VA_ARGS__, 0, )
#define CHECK_N(x, n, ...) n

#define MATCH_INT_LOOP_INDIRECT() MATCH_INT_LOOP

#define MATCH_INT_LOOP(i, tuple, ...) \
  CONCATS(MATCH_INT_STEP_, IS_PAREN(tuple))(i, tuple, __VA_ARGS__)

#define MATCH_INT_STEP_0(i, tuple, ...) 0
#define MATCH_INT_STEP_1(i, tuple, ...) \
  _Generic((char (*)[(TUPLE_EXPAND_FIRST(tuple)) == i])0, char (*)[1]: TUPLE_EXPAND_REST(tuple), default: DEFER(MATCH_INT_LOOP_INDIRECT)()(i, __VA_ARGS__))

#define match_int(i, ...) MACRO_EXPAND(MATCH_INT_LOOP(i, __VA_ARGS__, ~))
