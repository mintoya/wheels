#ifndef WHEELS_FOREACH4_H
#define WHEELS_FOREACH4_H

#if !defined let
  #define let __auto_type
#endif
#if !defined REMOVE_PARENS
  #define REM_PAREN(...) __VA_ARGS__
  #define REMOVE_PARENS(...) REM_PAREN __VA_ARGS__
#endif

#if !defined VA_SWITCH_SEL
  #define VA_SWITCH_SEL(a, ...) REM_PAREN a
  #define VA_SWITCH(first, ...) VA_SWITCH_SEL(__VA_OPT__((__VA_ARGS__), )(first))
#endif

// Iterator: ((state_type), (initial_value), (next_expression)).
// next_expression updates $iter and returns {hasvalue, value}.
#define iter_next_t(T) \
  struct {             \
    char hasvalue;     \
    T value;           \
  }

// A negative constant step produces a negative array size.
#define ITER_CHECK_INCREMENT(step) \
  ((void)sizeof(char[(step) >= 0 ? 1 : -1]), (step))

// Exclusive end; step is a positive magnitude in either direction.
#define range(start, end, ...)                                        \
  ((struct {                                                          \
     typeof((start) + 0) _initial;                                    \
     typeof((end) + 0) _final;                                        \
     typeof((start) + 0) _current;                                    \
     typeof(VA_SWITCH(1, __VA_ARGS__)) _increment;                    \
     char _started;                                                   \
   }),                                                                \
   ({                                                                 \
       ._initial = (start),                                           \
       ._final = (end),                                               \
       ._current = 0,                                                 \
       ._increment = ITER_CHECK_INCREMENT(VA_SWITCH(1, __VA_ARGS__)), \
       ._started = 0,                                                 \
   }),                                                                \
   (({                                                                \
     iter_next_t(typeof($iter._current)) $result = {0};               \
     if (!$iter._started) {                                           \
       $iter._current = $iter._initial;                               \
       $iter._started = 1;                                            \
     } else if ($iter._final > $iter._initial) {                      \
       $iter._current += $iter._increment;                            \
     } else {                                                         \
       $iter._current -= $iter._increment;                            \
     }                                                                \
     $result.hasvalue = $iter._increment > 0 &&                       \
                        ($iter._final > $iter._initial                \
                             ? $iter._current < $iter._final          \
                             : $iter._current > $iter._final);        \
     $result.value = $iter._current;                                  \
     $result;                                                         \
   })))

#define span(start, length, ...)                                                \
  ((struct {                                                                    \
     typeof(1 ? (start) : (start + length)) _initial;                           \
     __SIZE_TYPE__ _length;                                                     \
     __SIZE_TYPE__ _current;                                                    \
     typeof(VA_SWITCH(1, __VA_ARGS__)) _increment;                              \
   }),                                                                          \
   ({                                                                           \
       ._initial = (start),                                                     \
       ._length = (length),                                                     \
       ._current = 0,                                                           \
       ._increment = ITER_CHECK_INCREMENT(VA_SWITCH(1, __VA_ARGS__)),           \
   }),                                                                          \
   (({                                                                          \
     iter_next_t(typeof($iter._initial)) $result = {0};                         \
     $result.hasvalue = $iter._increment > 0 && $iter._current < $iter._length; \
     if ($result.hasvalue) {                                                    \
       $result.value = $iter._initial + $iter._current;                         \
       if ((__SIZE_TYPE__)$iter._increment >= $iter._length - $iter._current)   \
         $iter._current = $iter._length;                                        \
       else                                                                     \
         $iter._current += $iter._increment;                                    \
     }                                                                          \
     $result;                                                                   \
   })))

#define vla_iter(array)                                     \
  ((struct {                                                \
     typeof(&((array)[0])) _ptr;                            \
     __SIZE_TYPE__ _length;                                 \
     __SIZE_TYPE__ _current;                                \
   }),                                                      \
   (({                                                      \
     let $array = &(array);                                 \
     (typeof($foreach.iter)){                               \
         ._ptr = *$array,                                   \
         ._length = sizeof(*$array) / sizeof((*$array)[0]), \
         ._current = 0                                      \
     };                                                     \
   })),                                                     \
   (({                                                      \
     iter_next_t(typeof(({ *$iter._ptr; }))) $result = {0}; \
     $result.hasvalue = $iter._current < $iter._length;     \
     if ($result.hasvalue)                                  \
       $result.value = $iter._ptr[$iter._current++];        \
     $result;                                               \
   })))

// Accepts a pointer to an array, yielding its element values.
#define vlap_iter(array_ptr) vla_iter(*(array_ptr))

// table.init(args...) returns state; table.next(&state) returns
// a result with hasvalue and value fields. The table is evaluated once.
#define vtable(table, ...)                                        \
  ((struct {                                                      \
     typeof((table).init(__VA_ARGS__)) _state;                    \
     typeof(({ (table).next; })) _next;                           \
   }),                                                            \
   (({                                                            \
     let $table = (table);                                        \
     (typeof($foreach.iter)){                                     \
         ._state = $table.init(__VA_ARGS__), ._next = $table.next \
     };                                                           \
   })),                                                           \
   (($iter._next(&$iter._state))))

#define ITER_POP_H(a, ...) (__VA_ARGS__)
#define ITER_POP(tuple) ITER_POP_H tuple
#define ITER_FIRST_H(a, ...) a
#define ITER_FIRST(tuple) ITER_FIRST_H tuple
#define ITER_GET_TYPE(tuple) REMOVE_PARENS(ITER_FIRST(tuple))
#define ITER_GET_VAL(tuple) REMOVE_PARENS(ITER_FIRST(ITER_POP(tuple)))
#define ITER_GET_NEXT(tuple) REMOVE_PARENS(ITER_FIRST(ITER_POP(ITER_POP(tuple))))

#define foreach(decl, iterator)                     \
  for (struct {                                     \
         char cond;                                 \
         char hasvalue;                             \
         ITER_GET_TYPE(iterator)                    \
         iter;                                      \
       } $foreach = {1, 1, ITER_GET_VAL(iterator)}; \
       $foreach.cond && $foreach.hasvalue;          \
       $foreach.cond = !$foreach.cond)              \
    for (decl = ({                                  \
           let $iter = $foreach.iter;               \
           let $next = ITER_GET_NEXT(iterator);     \
           $foreach.hasvalue = $next.hasvalue;      \
           $foreach.iter = $iter;                   \
           $next.value;                             \
         });                                        \
         $foreach.cond && $foreach.hasvalue;        \
         $foreach.cond = !$foreach.cond)

#endif
