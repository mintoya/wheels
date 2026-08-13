// init , next , check , cast
//
// init  : initialization structure
// next  : given the init'ed structure ,
//  produce a tuple with the structure and it's value
// check : given the init'ed structure ,
//  produce a boolean of *if* the loop should continue
// cast : given the init'ed structure ,
//  produce a variable for the user
//
// args of the foreach macro are determined based on init
//{range(start ,end)

#define REM_PAREN(...) __VA_ARGS__
#define VA_SWITCH_SEL(a, ...) REM_PAREN a
#define VA_SWITCH(first, ...) VA_SWITCH_SEL(__VA_OPT__((__VA_ARGS__), )(first))

#define FOREACH_range_init(start, end, ...) (       \
    struct {                                        \
      typeof(start + 0) _initial;                   \
      typeof(end + 0) _final;                       \
      int _started;                                 \
      typeof(start + 0) _current;                   \
      typeof(VA_SWITCH(1, __VA_ARGS__)) _increment; \
    },                                              \
    {                                               \
        ._initial = (start),                        \
        ._final = (end),                            \
        ._started = 0,                              \
        ._current = 0,                              \
        ._increment = (VA_SWITCH(1, __VA_ARGS__)),  \
    }                                               \
)

#define FOREACH_range_increase(is) \
  (is._final > is._initial ? (is._current += is._increment) : (is._current -= is._increment))
#define FOREACH_range_valid(is)                                                      \
  ({                                                                                 \
    if (!is._started)                                                                \
      is._current = is._initial;                                                     \
    is._started = 1;                                                                 \
    is._final > is._initial ? (is._current < is._final) : (is._current > is._final); \
  })
#define FOREACH_range_cast(is) (is._current)

#define FOREACH_range         \
  (                           \
      FOREACH_range_init,     \
      FOREACH_range_increase, \
      FOREACH_range_valid,    \
      FOREACH_range_cast)
//}
//{span(start,length)
#define FOREACH_span_init(start, length, ...) (     \
    struct {                                        \
      typeof(start + length) _initial;              \
      size_t _len;                                  \
      size_t _current;                              \
      typeof(VA_SWITCH(1, __VA_ARGS__)) _increment; \
    },                                              \
    {                                               \
        ._initial = (start),                        \
        ._len = (length),                           \
        ._current = 0,                              \
        ._increment = (VA_SWITCH(1, __VA_ARGS__)),  \
    }                                               \
)
#define FOREACH_span_increase(is) (is._current += is._increment)
#define FOREACH_span_valid(is) (is._current < is._len)
#define FOREACH_span_cast(is) (is._initial + is._current)

#define FOREACH_span         \
  (                          \
      FOREACH_span_init,     \
      FOREACH_span_increase, \
      FOREACH_span_valid,    \
      FOREACH_span_cast)
//}
//{vla(vla)
#define FOREACH_vla_init(vla) (           \
    struct {                              \
      typeof(({ vla[0]; })) *_ptr;        \
      size_t _length;                     \
      size_t _current;                    \
    },                                    \
    ({                                    \
      var_ _vla_eval = &(vla);            \
      (typeof(_foreach_._foreach_)){      \
          ._ptr = *_vla_eval,             \
          ._length = countof(*_vla_eval), \
          ._current = 0,                  \
      };                                  \
    })                                    \
)
#define FOREACH_vla_increase(is) (is._current++)
#define FOREACH_vla_valid(is) (is._current < is._length)
#define FOREACH_vla_cast(is) (*(is._current + is._ptr))

#define FOREACH_vla         \
  (                         \
      FOREACH_vla_init,     \
      FOREACH_vla_increase, \
      FOREACH_vla_valid,    \
      FOREACH_vla_cast)
#define FOREACH_vlap_init(vla) \
  FOREACH_vla_init(*vla)
#define FOREACH_vlap        \
  (                         \
      FOREACH_vlap_init,    \
      FOREACH_vla_increase, \
      FOREACH_vla_valid,    \
      FOREACH_vla_cast)
//}
//{vtable(val)
#define FOREACH_vtable_init(vt, ...) (    \
    struct {                              \
      typeof((vt).init(__VA_ARGS__)) _it; \
      const typeof(vt) _vt;               \
    },                                    \
    ({                                    \
      let _val = vt;                      \
      (typeof(_foreach_._foreach_)){      \
          _val.init(__VA_ARGS__),         \
          _val                            \
      };                                  \
    })                                    \
)
#define FOREACH_vtable_increase(is) ((is)._vt.increase(&(is)._it))
#define FOREACH_vtable_valid(is) ((is)._vt.valid(&(is)._it))
#define FOREACH_vtable_cast(is) ((is)._vt.cast(&(is)._it))

#define FOREACH_vtable         \
  (                            \
      FOREACH_vtable_init,     \
      FOREACH_vtable_increase, \
      FOREACH_vtable_valid,    \
      FOREACH_vtable_cast)
//}
//

//
//
//
//  implementation
//
//
//

// { own macro
#define MACRO_FROEACH_EXPAND1(...) \
  __VA_ARGS__
#define MACRO_FROEACH_EXPAND2(...) \
  __VA_OPT__(MACRO_FROEACH_EXPAND1(MACRO_FROEACH_EXPAND1(MACRO_FROEACH_EXPAND1(MACRO_FROEACH_EXPAND1(__VA_ARGS__)))))
#define MACRO_FROEACH_EXPAND3(...) \
  MACRO_FROEACH_EXPAND2(MACRO_FROEACH_EXPAND2(MACRO_FROEACH_EXPAND2(MACRO_FROEACH_EXPAND2(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND4(...) \
  MACRO_FROEACH_EXPAND3(MACRO_FROEACH_EXPAND3(MACRO_FROEACH_EXPAND3(MACRO_FROEACH_EXPAND3(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND5(...) \
  MACRO_FROEACH_EXPAND4(MACRO_FROEACH_EXPAND4(MACRO_FROEACH_EXPAND4(MACRO_FROEACH_EXPAND4(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND6(...) \
  MACRO_FROEACH_EXPAND5(MACRO_FROEACH_EXPAND5(MACRO_FROEACH_EXPAND5(MACRO_FROEACH_EXPAND5(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND7(...) \
  MACRO_FROEACH_EXPAND6(MACRO_FROEACH_EXPAND6(MACRO_FROEACH_EXPAND6(MACRO_FROEACH_EXPAND6(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND8(...) \
  MACRO_FROEACH_EXPAND7(MACRO_FROEACH_EXPAND7(MACRO_FROEACH_EXPAND7(MACRO_FROEACH_EXPAND7(__VA_ARGS__))))
#define MACRO_FROEACH_EXPAND(...) \
  MACRO_FROEACH_EXPAND3(__VA_ARGS__)
// }
#define FOREACH_DROP_ARGS(...)

#define FOREACH_getInit_struct(init, ...) init
#define FOREACH_getInit_set(init, ...) __VA_ARGS__

#define FOREACH_getInit(init, next, check, cast) init
#define FOREACH_getNext(init, next, check, cast) next FOREACH_DROP_ARGS
#define FOREACH_getCheck(init, next, check, cast) check FOREACH_DROP_ARGS
#define FOREACH_getCast(init, next, check, cast) cast FOREACH_DROP_ARGS

#define foreach(declaration, generator)                                                               \
  for (                                                                                               \
      struct {                                                                                        \
        char cond;                                                                                    \
        MACRO_FROEACH_EXPAND(FOREACH_getInit_struct FOREACH_getInit FOREACH_##generator)              \
        _foreach_;                                                                                    \
      } _foreach_ = {                                                                                 \
          .cond = 1,                                                                                  \
          ._foreach_ = MACRO_FROEACH_EXPAND(FOREACH_getInit_set FOREACH_getInit FOREACH_##generator), \
      };                                                                                              \
      MACRO_FROEACH_EXPAND(/**/                                                                       \
                           FOREACH_getCheck FOREACH_##generator                                       \
      )(_foreach_._foreach_) &&                                                                       \
      _foreach_.cond;                                                                                 \
      (_foreach_.cond = !_foreach_.cond, /**/                                                         \
       MACRO_FROEACH_EXPAND(             /**/                                                         \
                            FOREACH_getNext FOREACH_##generator                                       \
       )(_foreach_._foreach_)))                                                                       \
    for (                                                                                             \
        declaration =                                                                                 \
            MACRO_FROEACH_EXPAND(                                                                     \
                FOREACH_getCast FOREACH_##generator                                                   \
            )(_foreach_._foreach_);                                                                   \
        _foreach_.cond;                                                                               \
        _foreach_.cond = !_foreach_.cond)
