#if !defined(MY_MACROS_H)

  #if !defined(__cplusplus)
    #define REF(type, value) ((type[1]){value})
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #else
      #define nullptr ((void *)0)
    #endif
    #define bitcast(to, from) ((typeof(union {typeof(to)a;typeof(from)b; })){.b = from}.a)
  #else
template <typename T>
static T *TEMPORARY_REF_UB(T &&v) { return &v; }
    #define REF(type, value) TEMPORARY_REF_UB((type){value})
template <class To, class From>
To bit_cast_func(const From &src) noexcept {
  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
    #define bitcast(to, from) (bit_cast_func<to>(from))
    #define typeof(...) __typeof__(__VA_ARGS__)
  #endif
  #define ID_CONCAT_IM(a, b) a##b
  #define ID_CONCAT(a, b) ID_CONCAT_IM(a, b)

  #define MY_MACROS_H
  #ifdef __cplusplus
    #define EXTERN_C_START extern "C" {
    #define EXTERN_C_END }
  #else
    #define EXTERN_C_START
    #define EXTERN_C_END
  #endif
  #define PARENTHESIS_HELPER ()
  #define MACRO_EXPAND1(...) \
    __VA_ARGS__
  #define MACRO_EXPAND2(...) \
    __VA_OPT__(MACRO_EXPAND1(MACRO_EXPAND1(MACRO_EXPAND1(MACRO_EXPAND1(__VA_ARGS__)))))
  #define MACRO_EXPAND3(...) \
    MACRO_EXPAND2(MACRO_EXPAND2(MACRO_EXPAND2(MACRO_EXPAND2(__VA_ARGS__))))
  #define MACRO_EXPAND4(...) \
    MACRO_EXPAND3(MACRO_EXPAND3(MACRO_EXPAND3(MACRO_EXPAND3(__VA_ARGS__))))
  #define MACRO_EXPAND5(...) \
    MACRO_EXPAND4(MACRO_EXPAND4(MACRO_EXPAND4(MACRO_EXPAND4(__VA_ARGS__))))
  #define MACRO_EXPAND6(...) \
    MACRO_EXPAND5(MACRO_EXPAND5(MACRO_EXPAND5(MACRO_EXPAND5(__VA_ARGS__))))
  #define MACRO_EXPAND7(...) \
    MACRO_EXPAND6(MACRO_EXPAND6(MACRO_EXPAND6(MACRO_EXPAND6(__VA_ARGS__))))
  #define MACRO_EXPAND(...) \
    MACRO_EXPAND3(__VA_ARGS__)

  #define DEFER_NAME(a, b) ID_CONCAT(a, b)

  #if defined(__cplusplus)
    #pragma GCC warning "using cpp closure defer"
    #include <utility>
template <typename F>
struct Deferrer {
  F fn;
  inline ~Deferrer() { fn(); }
};

struct DeferHelper {
  template <typename F>
  Deferrer<F> operator+(F &&f) { return {std::forward<F>(f)}; }
};

    #define defer auto DEFER_NAME(_defer_, __LINE__) = DeferHelper() + [&]()
  #else
    #if __has_include(<stddefer.h>)
      #include <stddefer.h>
    #else
      #if defined(__clang__)
        #pragma GCC warning "using clang block defer (captures only work on pointers)"
static void _defer_cleanup_block(void (^*block)(void)) { (*block)(); }
        #define defer __attribute__((cleanup(_defer_cleanup_block))) void (^ID_CONCAT(_defer_var__, __COUNTER__))(void) = ^
      #elif defined(__GNUC__)
        #pragma GCC warning "using gnu nested function defer"
        #define _defer_helper(func_name, var__name)              \
          auto void func_name(int *);                            \
          int var__name __attribute__((cleanup(func_name))) = 0; \
          void func_name(int *_)
        #define defer _defer_helper(ID_CONCAT(_defer_func_, __COUNTER__), ID_CONCAT(_defer_var__, __COUNTER__))
      #endif
    #endif
  #endif

  #define APPLY_N(macro, ...) MACRO_EXPAND(APPLY_N_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER(macro, arg, ...) macro(arg) \
      __VA_OPT__(APPLY_N_HELPER_INVOKE PARENTHESIS_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER_INVOKE() APPLY_N_HELPER

  #define APPLY_N_WITH(macro, captured, ...) \
    MACRO_EXPAND(APPLY_N_WITH_HELPER(macro, captured, __VA_ARGS__))
  #define APPLY_N_WITH_HELPER(macro, captured, arg, ...) macro(captured, arg) \
      __VA_OPT__(APPLY_N_WITH_HELPER_INVOKE PARENTHESIS_HELPER(macro, captured, __VA_ARGS__))
  #define APPLY_N_WITH_HELPER_INVOKE() APPLY_N_WITH_HELPER

//
// pragmas
//

  #if !defined(__GNUC__)
    #define PRAGMA_MAKE_STR(...) #__VA_ARGS__
    #define MAKE_PRAGMA(warning) _Pragma(PRAGMA_MAKE_STR(GCC diagnostic ignored warning))
    #define DIAGNOSTIC_PUSH(...)     \
      _Pragma("GCC diagnostic push") \
          APPLY_N(MAKE_PRAGMA, __VA_ARGS__)
    #define DIAGNOSTIC_POP() \
      _Pragma("GCC diagnostic pop")
  #else
    #define DIAGNOSTIC_PUSH(...)
    #define DIAGNOSTIC_POP()
  #endif

//
// (,) stuff
//
  #define TUPLE_A(name, func) name
  #define TUPLE_B(name, func) func
  #define TUPLE_EXPAND_A(tuple) TUPLE_A tuple
  #define TUPLE_EXPAND_B(tuple) TUPLE_B tuple

//
// constexpr
//
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define CONST_EXPR constexpr
  #elif defined(__cpp_constexpr) // C++ fallback
    #define CONST_EXPR constexpr
  #else
    #define CONST_EXPR static const
  #endif

//
// static constant struct based namesapce
//

  #define NAMESPACEN_H(tuple) const typeof(TUPLE_EXPAND_B(tuple)) TUPLE_EXPAND_A(tuple);
  #define NAMESPACEN(...) APPLY_N(NAMESPACEN_H, __VA_ARGS__)
  #define NAMESPACEF_H(tuple) TUPLE_EXPAND_B(tuple),
  #define NAMESPACEF(...) APPLY_N(NAMESPACEF_H, __VA_ARGS__)
  #define NAMESPACE_STRUCT(name, ...) \
    CONST_EXPR struct {               \
      NAMESPACEN(__VA_ARGS__)         \
    } name = (typeof(name)){          \
        NAMESPACEF(__VA_ARGS__)       \
    };

//
// utility
//

  #define COUNT_ONE_MACRO(x) +1
  #define COUNT_ARGS(...) (__VA_OPT__(APPLY_N(COUNT_ONE_MACRO, __VA_ARGS__)) + 0)
  #define EQUAL_ANY_HELPER(a) a ||
  #define EQUAL_ANY(expr, ...) (APPLY_N((expr) == EQUAL_ANY_HELPER, __VA_ARGS__) 0)
  #define EQUAL_ALL_HELPER(a) a &&
  #define EQUAL_ALL(expr, ...) (APPLY_N((expr) == EQUAL_ALL_HELPER, __VA_ARGS__) 1)
  #define ASSERT_EXPR(cond, msg) \
    ((void)((int)sizeof(char[1 - 2 * !(cond)])))
  #define STR_H(...) #__VA_ARGS__
  #define VLAP(ptr, len) ((typeof(typeof(*ptr))(*)[len])ptr)

  #define VA_SWITCH_FIRST(first, ...) first
  #define VA_SWITCH_EMPTY(...) VA_SWITCH_FIRST(__VA_OPT__(0, ) 1, ~)
  #define VA_SWITCH_IMPL_0(default_val, ...) __VA_ARGS__
  #define VA_SWITCH_IMPL_1(default_val) default_val
  #define VA_SWITCH(default_val, ...) \
    ID_CONCAT(VA_SWITCH_IMPL_, VA_SWITCH_EMPTY(__VA_ARGS__))(default_val __VA_OPT__(, ) __VA_ARGS__)

  #define types_eq(T1, T2) \
    _Generic((*((T1 *)NULL)), T2: true, default: false)

//
// loops
//

  #define _each_vla(vla, decl)   \
    (size_t _i = 0, _keep = 1;   \
     _keep && _i < countof(vla); \
     _keep = !_keep, _i++) /**/  \
        for (decl = (vla)[_i]; _keep; _keep = !_keep)

  #define _each_range_3(start, end, decl)                           \
    (                                                               \
        struct {                                                    \
          typeof((start) + (end)) val, last;                        \
          int change;                                               \
          int keep;                                                 \
        } _s = {                                                    \
            .val = (start),                                         \
            .last = (end),                                          \
            .change = (_s.val < _s.last ? 1 : -1),                  \
            .keep = 1,                                              \
        };                                                          \
        _s.keep &&                                             /**/ \
        (_s.change < 0 ? _s.val > _s.last : _s.val < _s.last); /**/ \
        _s.keep = !_s.keep, _s.val += _s.change)               /**/ \
        for (decl = _s.val; _s.keep; _s.keep = !_s.keep)

  #define _each_range_4(start, end, inc, decl)                      \
    (                                                               \
        struct {                                                    \
          typeof((start) + (end)) val, last;                        \
          typeof(inc) change;                                       \
          int keep;                                                 \
        } _s = {                                                    \
            .val = (start),                                         \
            .last = (end),                                          \
            .change = (inc),                                        \
            .keep = 1,                                              \
        };                                                          \
        _s.keep &&                                             /**/ \
        (_s.change < 0 ? _s.val > _s.last : _s.val < _s.last); /**/ \
        _s.keep = !_s.keep, _s.val += _s.change)               /**/ \
        for (decl = _s.val; _s.keep; _s.keep = !_s.keep)

  #define _each_span(start, len, decl)  \
    (struct {                           \
          typeof((start) + (len)) val, last;     \
          int keep; } _s = {                  \
         (start),                       \
         (start) + len,                 \
         1,                             \
     };                                 \
     _s.keep &&                    /**/ \
     _s.val < _s.last;             /**/ \
     _s.keep = !_s.keep, _s.val++) /**/ \
        for (decl = _s.val; _s.keep; _s.keep = !_s.keep)

  #define _each_iter_2(init, decl)         \
    (                                      \
        struct { typeof(init) x; int keep; } _s = {(init), 1};       \
        _s.keep && _s.x.valid(_s.x.state); \
        _s.keep = !_s.keep, _s.x.next(_s.x.state)) for (decl = _s.x.state->current; _s.keep; _s.keep = !_s.keep)
  #define _each_iter_3(init, cast, decl)   \
    (                                      \
        struct { typeof(init) x; int keep; } _s = {(init), 1};       \
        _s.keep && _s.x.valid(_s.x.state); \
        _s.keep = !_s.keep, _s.x.next(_s.x.state)) for (decl = (cast)_s.x.state->current; _s.keep; _s.keep = !_s.keep)

  #define _each_iter_sel(_1, _2, _3, NAME, ...) NAME
  #define _each_iter(...) \
    _each_iter_sel(__VA_ARGS__, _each_iter_3, _each_iter_2)(__VA_ARGS__)

  #define _each_range_sel(_1, _2, _3, _4, NAME, ...) NAME
  #define _each_range(...) \
    _each_range_sel(__VA_ARGS__, _each_range_4, _each_range_3)(__VA_ARGS__)

  #define _im_each_vla(...)  _each_vla(__VA_ARGS__,
  #define _im_each_span(...) _each_span(__VA_ARGS__ ,
  #define _im_each_range(...)_each_range(__VA_ARGS__ ,
  #define _im_each_iter(...) _each_iter(__VA_ARGS__ ,

  #define each(decl, generator)                          \
    /**/                                                 \
    /* generators*/                                      \
    /*  vla(array) takes array and uses countof*/        \
    /*  range(start,end,inc?) adds inc to start*/        \
    /*  iter(iterator struct)*/                          \
    /*    layout :*/                                     \
    /*    {*/                                            \
    /*      state [1] / ptr : state of iterator*/        \
    /*        current : used to deduce decl*/            \
    /*      valid : function pointer called with state*/ \
    /*      next  : used to increment*/                  \
    /*    }*/                                            \
    /**/                                                 \
_im_each_ ## generator decl )
  #define foreach(decl, generator)                       \
    /**/                                                 \
    /* generators*/                                      \
    /*  vla(array) takes array and uses countof*/        \
    /*  range(start,end,inc?) adds inc to start*/        \
    /*  iter(iterator struct)*/                          \
    /*    layout :*/                                     \
    /*    {*/                                            \
    /*      state [1] / ptr : state of iterator*/        \
    /*        current : used to deduce decl*/            \
    /*      valid : function pointer called with state*/ \
    /*      next  : used to increment*/                  \
    /*    }*/                                            \
    /**/                                                 \
for _im_each_ ## generator decl )

//
// var
//

  #define var_ __auto_type

//
// expect
//

  #define if_likely(...) if (__builtin_expect(!!(__VA_ARGS__), 1))
  #define if_unlikely(...) if (__builtin_expect(!!(__VA_ARGS__), 0))

//
// pipe
//

  #define P$_EMPTY()
  #define P$_DEFER(m) m P$_EMPTY()
  #define P$_FOLD_INDIRECT() P$_FOLD
  #define P$_EAT(...)

  #define P$_FOLD(state, arg, ...)                                        \
    ({                                                                    \
      var_ _state = (state);                                              \
      ({                                                                  \
        var_ $ = _state;                                                  \
        var_ _res = arg;                                                  \
        __VA_OPT__(P$_DEFER(P$_FOLD_INDIRECT)()(_res, __VA_ARGS__)P$_EAT) \
        (_res);                                                           \
      });                                                                 \
    })

  #define P$_ONE(in, ...) MACRO_EXPAND(P$_FOLD(in, __VA_ARGS__))
  // #define P$(in, ...) MACRO_EXPAND(P$_FOLD(in, __VA_ARGS__))
  #define P$(in, ...) P$_ONE(in __VA_OPT__(, __VA_ARGS__), $)
#endif
