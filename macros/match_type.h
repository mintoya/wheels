#define match_type_ucast(type, value) \
  _Generic(                           \
      value,                          \
      type: value,                    \
      default: (type){}               \
  )
#define urcast(type, value)              \
  _Generic(                              \
      value,                             \
      type: value,                       \
      default: (unreachable(), (type){}) \
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

#define switch_exp_default 1
#define switch_exp_item(res, number_expr)                                                       \
  REMOVE_PARENS(IF_IS1(                                                                         \
      ID_CONCAT(switch_exp_, TUPLE_EXPAND_FIRST(number_expr)),                                  \
      (default : { res = TUPLE_EXPAND_REST(number_expr); } break;),                             \
      (case TUPLE_EXPAND_FIRST(number_expr) : { res = TUPLE_EXPAND_REST(number_expr); } break;) \
  ))

#define switch_exp(num, first, ...) ({                        \
  let _swresult = (typeof(TUPLE_EXPAND_REST(first))){};       \
  switch (num) {                                              \
    switch_exp_item(_swresult, first)                         \
        APPLY_N_WITH(switch_exp_item, _swresult, __VA_ARGS__) \
  }                                                           \
  _swresult;                                                  \
})

#define switch_tuple_item(number_expr)                                                    \
  REMOVE_PARENS(IF_IS1(                                                                   \
      ID_CONCAT(switch_exp_, TUPLE_EXPAND_FIRST(number_expr)),                            \
      (default : { TUPLE_EXPAND_REST(number_expr); } break;),                             \
      (case TUPLE_EXPAND_FIRST(number_expr) : { TUPLE_EXPAND_REST(number_expr); } break;) \
  ))

#define switch_tuple(num, ...) ({           \
  switch (num) {                            \
    APPLY_N(switch_tuple_item, __VA_ARGS__) \
  }                                         \
})

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

#define MATCH_INT_STEP_0(i, tuple, ...) (void)0
#define MATCH_INT_EXPR(tuple, i) TUPLE_EXPAND_FIRST(tuple)(i TUPLE_EXPAND_REST(tuple))
#define MATCH_INT_STEP_1(i, tuple, ...) \
  _Generic((char (*)[MATCH_INT_EXPR(TUPLE_EXPAND_FIRST(tuple), (i))])0, char (*)[1]: TUPLE_EXPAND_REST(tuple), default: DEFER(MATCH_INT_LOOP_INDIRECT)()(i, __VA_ARGS__))

#define match_int(i, ...) MACRO_EXPAND(MATCH_INT_LOOP(i, __VA_ARGS__, ~))
