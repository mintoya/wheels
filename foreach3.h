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
        ._current = 0,                              \
        ._started = 0,                              \
        ._increment = (VA_SWITCH(1, __VA_ARGS__)),  \
    }                                               \
)

#define FOREACH_range_increase(is) (is._current += is._increment)
#define FOREACH_range_valid(is)  \
  ({                             \
    if (!is._started)            \
      is._current = is._initial; \
    is._started = 1;             \
    is._current < is._final;     \
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
      typeof(start + 0) _initial;                   \
      size_t _len;                                  \
      size_t _current;                              \
      typeof(VA_SWITCH(1, __VA_ARGS__)) _increment; \
    },                                              \
    {                                               \
        ._initial = (start),                        \
        ._current = 0,                              \
        ._len = (length),                           \
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
      typeof(vla[0]) *_ptr;               \
      size_t _length;                     \
      size_t _current;                    \
    },                                    \
    ({                                    \
      var_ _vla_eval = &vla;              \
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
//{iter(vla)
#define FOREACH_iter_init(vla) (          \
    struct {                              \
      typeof(vla[0]) *_ptr;               \
      size_t _length;                     \
      size_t _current;                    \
    },                                    \
    ({                                    \
      var_ _vla_eval = &vla;              \
      (typeof(_foreach_._foreach_)){      \
          ._ptr = *_vla_eval,             \
          ._length = countof(*_vla_eval), \
          ._current = 0,                  \
      };                                  \
    })                                    \
)
#define FOREACH_iter_increase(is) (is._current++)
#define FOREACH_iter_valid(is) (is._current < is._length)
#define FOREACH_iter_cast(is) (*(is._current + is._ptr))

#define FOREACH_iter         \
  (                          \
      FOREACH_iter_init,     \
      FOREACH_iter_increase, \
      FOREACH_iter_valid,    \
      FOREACH_iter_cast)
//}

//
//
//
//  implementation
//
//
//

#define FOREACH_DROP_ARGS(...)

#define FOREACH_getInit_struct(init, ...) init
#define FOREACH_getInit_set(init, ...) __VA_ARGS__

#define FOREACH_getInit(init, next, check, cast) init
#define FOREACH_getNext(init, next, check, cast) next FOREACH_DROP_ARGS
#define FOREACH_getCheck(init, next, check, cast) check FOREACH_DROP_ARGS
#define FOREACH_getCast(init, next, check, cast) cast FOREACH_DROP_ARGS

#define FOREACH(declaration, generator)                                                       \
  for (                                                                                       \
      struct {                                                                                \
        char cond;                                                                            \
        MACRO_EXPAND(FOREACH_getInit_struct FOREACH_getInit FOREACH_##generator)              \
        _foreach_;                                                                            \
      } _foreach_ = {                                                                         \
          ._foreach_ = MACRO_EXPAND(FOREACH_getInit_set FOREACH_getInit FOREACH_##generator), \
          .cond = 1                                                                           \
      };                                                                                      \
      MACRO_EXPAND(/**/                                                                       \
                   FOREACH_getCheck FOREACH_##generator                                       \
      )(_foreach_._foreach_) &&                                                               \
      _foreach_.cond;                                                                         \
      (_foreach_.cond = !_foreach_.cond, /**/                                                 \
       MACRO_EXPAND(                     /**/                                                 \
                    FOREACH_getNext FOREACH_##generator                                       \
       )(_foreach_._foreach_)))                                                               \
    for (                                                                                     \
        declaration =                                                                         \
            MACRO_EXPAND(                                                                     \
                FOREACH_getCast FOREACH_##generator                                           \
            )(_foreach_._foreach_);                                                           \
        _foreach_.cond;                                                                       \
        _foreach_.cond = !_foreach_.cond)
#define foreach(decl, generator) FOREACH(decl, generator)
#define EACH3(declaration, generator)                                                         \
  (                                                                                           \
      struct {                                                                                \
        char cond;                                                                            \
        MACRO_EXPAND(FOREACH_getInit_struct FOREACH_getInit FOREACH_##generator)              \
        _foreach_;                                                                            \
      } _foreach_ = {                                                                         \
          ._foreach_ = MACRO_EXPAND(FOREACH_getInit_set FOREACH_getInit FOREACH_##generator), \
          .cond = 1                                                                           \
      };                                                                                      \
      MACRO_EXPAND(/**/                                                                       \
                   FOREACH_getCheck FOREACH_##generator                                       \
      )(_foreach_._foreach_) &&                                                               \
      _foreach_.cond;                                                                         \
      (_foreach_.cond = !_foreach_.cond, /**/                                                 \
       MACRO_EXPAND(                     /**/                                                 \
                    FOREACH_getNext FOREACH_##generator                                       \
       )(_foreach_._foreach_))) for (declaration = MACRO_EXPAND(FOREACH_getCast FOREACH_##generator)(_foreach_._foreach_); _foreach_.cond; _foreach_.cond = !_foreach_.cond)
