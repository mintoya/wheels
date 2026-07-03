// match_type(
//  (int)i,
//  (int , printint),
//  (char , printchar),
//  (default , printf("unknown")),
// );

#define match_type_ucast(type, value) \
  _Generic(value, type: value, default: (__builtin_unreachable(), bitcast(type, value)))

#define SECOND(a, b, ...) b
#define IS_DEFAULT_default ~, 1
#define EVAL(...) SECOND(__VA_ARGS__)
#define IS_DEFAULT(type) EVAL(ID_CONCAT(IS_DEFAULT_, type), 0, ~)
#define MATCH_EXPAND_1(action, input, type) action
#define MATCH_EXPAND_0(action, input, type) action(match_type_ucast(type, input))
#define match_expand(type, action, input) ID_CONCAT(MATCH_EXPAND_, IS_DEFAULT(type))(action, input, type)

#define match_type_case(value, type_action_tuple) \
  TUPLE_EXPAND_A(type_action_tuple)               \
      : match_expand(TUPLE_EXPAND_A(type_action_tuple), TUPLE_EXPAND_B(type_action_tuple), value)

#define match_type(value, ...)            \
  _Generic(                               \
      (value),                            \
      APPLY_N_WITH_C(                     \
          match_type_case,                \
          value __VA_OPT__(, __VA_ARGS__) \
      )                                   \
  )
