#define TU_TAG(type) ID_CONCAT(type, _enum)

#define TU_ENUM(type_d) TU_TAG(TUPLE_FIRST type_d),
#define TU_TDEF(type_d) typedef TUPLE_EXPAND_REST(type_d) TUPLE_EXPAND_FIRST(type_d);
#define TU_UMEM(type_d) TUPLE_EXPAND_FIRST(type_d) TUPLE_EXPAND_FIRST(type_d);

#define tu_def(tagged_union, ...)                        \
  typedef enum : TUPLE_EXPAND_B(tagged_union){           \
      APPLY_N(TU_ENUM, __VA_ARGS__)                      \
  } ID_CONCAT(_enum_, TUPLE_EXPAND_FIRST(tagged_union)); \
  APPLY_N(TU_TDEF, __VA_ARGS__)                          \
  typedef struct TUPLE_EXPAND_FIRST(tagged_union) {      \
    ID_CONCAT(_enum_, TUPLE_EXPAND_FIRST(tagged_union))  \
    tag;                                                 \
    union {                                              \
      APPLY_N(TU_UMEM, __VA_ARGS__)                      \
    };                                                   \
  } TUPLE_EXPAND_FIRST(tagged_union)

#define tu_void_toi(...)                         \
  _Generic(                                      \
      (typeof(({ __VA_ARGS__; })) *)0,     \
      void *: ((({ __VA_ARGS__; }), nothing_v)), \
      default: ({ __VA_ARGS__; })                \
  )
#define tu_match_case_type(variable, tuple)                         \
  typeof(IF_IS1(                                                    \
      ID_CONCAT(tu_match_, TUPLE_EXPAND_FIRST(tuple)),              \
      ({ tu_void_toi(TUPLE_EXPAND_REST(tuple)); }),                 \
      ({                                                            \
        tu_void_toi(                                                \
            var_                                                    \
                TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(tuple))) /**/ \
            = variable.TUPLE_EXPAND_FIRST(tuple);                   \
            TUPLE_EXPAND_REST((TUPLE_EXPAND_REST(tuple)))           \
        );                                                          \
      })                                                            \
  ))

#define tu_ignore_assign(variable, value)                \
  do {                                                   \
    var_ _eval = value;                                  \
    _Static_assert(                                      \
        types_eq(                                        \
            typeof(_eval), typeof(variable)              \
        ),                                               \
        "result types in match statements have to match" \
    );                                                   \
    variable = _Generic(                                 \
        variable,                                        \
        typeof(_eval): _eval,                            \
        default: (typeof(variable)){}                    \
    );                                                   \
  } while (0)

#define tu_match_default 1
#define tu_match_case(variable, tuple)                                  \
  REM_PAREN IF_IS1(                                                     \
      ID_CONCAT(tu_match_, TUPLE_EXPAND_FIRST(tuple)),                  \
      (default : {                                                      \
        tu_ignore_assign(                                               \
            _result,                                                    \
            tu_void_toi(TUPLE_EXPAND_REST(tuple))                       \
        );                                                              \
      } break;),                                                        \
      (case (TU_TAG(TUPLE_EXPAND_FIRST(tuple))) : {                     \
        tu_ignore_assign(                                               \
            _result,                                                    \
            tu_void_toi(                                                \
                var_                                                    \
                    TUPLE_EXPAND_FIRST((TUPLE_EXPAND_REST(tuple))) /**/ \
                = variable.TUPLE_EXPAND_FIRST(tuple);                   \
                TUPLE_EXPAND_REST((TUPLE_EXPAND_REST(tuple)))           \
            )                                                           \
        );                                                              \
      } break;)                                                         \
  )
#define tu_match(variable, first, ...)                                  \
  ({                                                                    \
    var_ _variable = variable;                                          \
    tu_match_case_type(_variable, first) _result = (typeof(_result)){}; \
    switch (variable.tag) {                                             \
      MACRO_EXPAND(tu_match_case(_variable, first))                     \
      APPLY_N_WITH(tu_match_case, _variable, __VA_ARGS__)               \
    }                                                                   \
    _result;                                                            \
  })

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
#define if_tu_is(variable, type, value)                                              \
  if (tu_is(type, value))                                                            \
    for (struct {type tv;bool keep; } tu_check_ = {value.type, 1}; tu_check_.keep; tu_check_.keep = 0) \
      for (variable = tu_check_.tv; tu_check_.keep; tu_check_.keep = 0)
