#if !defined(MY_MACROS_H)
  // #include <stdint.h>
  #if defined __cplusplus
    #include <cstdint>
    #include <iterator>
using size_t = std::size_t;
  #endif
  #if !defined(__cplusplus)
    #define REF_1(type, ...) ((type[1]){__VA_ARGS__})
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #else
      #define nullptr ((void *)0)
    #endif
    #define bitcast(to, from) ((typeof(union {typeof(to)a;typeof(from)b; })){.b = from}.a)
  #else
template <typename T>
static T *TEMPORARY_REF_UB(T &&v) { return &v; }
    #define REF_1(type, value) TEMPORARY_REF_UB((type){value})
template <class To, class From>
To bit_cast_func(const From &src) noexcept {
  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}
    #define bitcast(to, from) (bit_cast_func<to>(from))
  #endif

  #define REF_INDIRECT(one, ...) REF_1 one
  #define REF(x, ...) REF_INDIRECT(__VA_OPT__((x, __VA_ARGS__), )(typeof(x), x))

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
  #define MACRO_EXPAND8(...) \
    MACRO_EXPAND7(MACRO_EXPAND7(MACRO_EXPAND7(MACRO_EXPAND7(__VA_ARGS__))))
  #define MACRO_EXPAND(...) \
    MACRO_EXPAND4(__VA_ARGS__)

  #define LPAREN (
  #define RPAREN )

  #define CONCATS1(a, b, ...)   \
    __VA_OPT__(CONCATS2 LPAREN) \
    ID_CONCAT(a, b)             \
    __VA_OPT__(, __VA_ARGS__ RPAREN)

  #define CONCATS2(a, b, ...)   \
    __VA_OPT__(CONCATS1 LPAREN) \
    ID_CONCAT(a, b)             \
    __VA_OPT__(, __VA_ARGS__ RPAREN)

  #define CONCATS(...) MACRO_EXPAND(CONCATS1(__VA_ARGS__))

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

    #define defer auto CONCATS(_defer_, __LINE__) = DeferHelper() + [&]()
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

  #define APPLY_N(macro, ...) \
    __VA_OPT__(MACRO_EXPAND(APPLY_N_HELPER(macro, __VA_ARGS__)))
  #define APPLY_N_HELPER(macro, arg, ...) macro(arg) \
      __VA_OPT__(APPLY_N_HELPER_INVOKE PARENTHESIS_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER_INVOKE() APPLY_N_HELPER

  #define APPLY_N_C(macro, ...) \
    __VA_OPT__(MACRO_EXPAND(APPLY_N_HELPER_C(macro, __VA_ARGS__)))
  #define APPLY_N_HELPER_C(macro, arg, ...) macro(arg) \
      __VA_OPT__(, APPLY_N_HELPER_INVOKE_C PARENTHESIS_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER_INVOKE_C() APPLY_N_HELPER_C

  #define APPLY_N_WITH(macro, captured, ...) \
    __VA_OPT__(MACRO_EXPAND(APPLY_N_WITH_HELPER(macro, captured, __VA_ARGS__)))
  #define APPLY_N_WITH_HELPER(macro, captured, arg, ...) macro(captured, arg) \
      __VA_OPT__(APPLY_N_WITH_HELPER_INVOKE PARENTHESIS_HELPER(macro, captured, __VA_ARGS__))
  #define APPLY_N_WITH_HELPER_INVOKE() APPLY_N_WITH_HELPER

  #define APPLY_N_WITH_C(macro, captured, ...) \
    __VA_OPT__(MACRO_EXPAND(APPLY_N_WITH_HELPER_C(macro, captured, __VA_ARGS__)))
  #define APPLY_N_WITH_HELPER_C(macro, captured, arg, ...) macro(captured, arg) \
      __VA_OPT__(, APPLY_N_WITH_HELPER_INVOKE_C PARENTHESIS_HELPER(macro, captured, __VA_ARGS__))
  #define APPLY_N_WITH_HELPER_INVOKE_C() APPLY_N_WITH_HELPER_C

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
  #define TUPLE_FIRST(a, ...) a
  #define TUPLE_REST(a, ...) __VA_ARGS__
  #define TUPLE_EXPAND_FIRST(t) TUPLE_FIRST t
  #define TUPLE_EXPAND_REST(t) TUPLE_REST t
  #define TUPLE_EXPAND_A(tuple) TUPLE_A tuple
  #define TUPLE_EXPAND_B(tuple) TUPLE_B tuple
  #define REM_PAREN(...) __VA_ARGS__
  #define REMOVE_PARENS(...) REM_PAREN __VA_ARGS__
  #define TUPLE_PUSH_TRAILING_COMMA(...) (__VA_OPT__(__VA_ARGS__, ))
  #define TUPLE_PUSH(tuple, a) (MACRO_EXPAND(REM_PAREN TUPLE_PUSH_TRAILING_COMMA tuple a))

//
// constexpr
//

  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define CONST_EXPR constexpr
  #elif defined(__cpp_constexpr)
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
    typedef struct name##_t {         \
      NAMESPACEN(__VA_ARGS__)         \
    } name##_t;                       \
    CONST_EXPR name##_t name = (name##_t){NAMESPACEF(__VA_ARGS__)};

//
// utility
//

  #define COUNT_ONE_MACRO(x) +1
  #define COUNT_ARGS(...) (__VA_OPT__(APPLY_N(COUNT_ONE_MACRO, __VA_ARGS__)) + 0)
  #define EQUAL_ANY_HELPER(a) a ||
  #define EQUAL_ANY(expr, ...) (APPLY_N((expr) == EQUAL_ANY_HELPER, __VA_ARGS__) 0)
  #define EQUAL_ALL_HELPER(a) a &&
  #define EQUAL_ALL(expr, ...) (APPLY_N((expr) == EQUAL_ALL_HELPER, __VA_ARGS__) 1)
  #define ASSERT_EXPR(cond, ...) \
    ((void)((int)sizeof(char[1 - 2 * !(cond)])))
  #define STR_H(...) #__VA_ARGS__

  #define VA_SWITCH_SEL(a, ...) REM_PAREN a
  #define VA_SWITCH(first, ...) VA_SWITCH_SEL(__VA_OPT__((__VA_ARGS__), )(first))

  #define VA_SWITCH_REMP_HELPER(...) REM_PAREN __VA_ARGS__
  #define VA_SWITCH_REMP(first, ...) VA_SWITCH_REMP_HELPER(VA_SWITCH_SEL(__VA_OPT__((__VA_ARGS__), )(first)))

  #define IF_IS1_HELP_1 a, b
  #define IF_ISL2(a, b, c, d, ...) c
  #define IF_ISL1(tok, then, ...) IF_ISL2(tok, then, __VA_ARGS__, a, b, c, d)

  #define IF_IS1(tok, then, otherwise) \
    IF_ISL1(ID_CONCAT(IF_IS1_HELP_, tok), then, otherwise)
//
// loops
//
  #include "macros/foreach3.h"

//
// var
//

  #define var_ __auto_type
  #if defined __cplusplus
    #define __auto_type auto
  #endif
  #define Var var_
  #define let var_

//
// expect
//

  #define if_likely(...) if (__builtin_expect(!!(__VA_ARGS__), 1))
  #define if_unlikely(...) if (__builtin_expect(!!(__VA_ARGS__), 0))
  #define if_decl(decl, init)         \
    for (struct { int keep; __typeof__(init) val; } _s = {1, (init)}; \
         _s.keep;                     \
         _s.keep = 0)                 \
      if (_s.val)                     \
        for (decl = _s.val; _s.keep; _s.keep = 0)

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
  #define P$(in, ...) P$_ONE(in __VA_OPT__(, __VA_ARGS__), $)

  #define MAX$_HELP(b)           \
    ({                           \
      var_ b_eval = b;           \
      (b_eval < $ ? $ : b_eval); \
    })
  #define MIN$_HELP(b)           \
    ({                           \
      var_ b_eval = b;           \
      (b_eval > $ ? $ : b_eval); \
    })

  #define MIN$(first, ...) P$(first, APPLY_N_C(MIN$_HELP __VA_OPT__(, __VA_ARGS__)))
  #define MAX$(first, ...) P$(first, APPLY_N_C(MAX$_HELP __VA_OPT__(, __VA_ARGS__)))

//
// spiral rule
// just so i dont have to read types in a circle
//

  #define fnptrof(in, out) typeof(typeof(out)(*) in)
  #define arrof(T, ...) typeof(typeof(typeof((T){}))[__VA_ARGS__])
  #define ptrof(T) typeof((typeof(void (*)(T)))0, (typeof(T) *)0)

//
// type stuff
//

  #if defined(__cplusplus)
    #include <type_traits>

    #ifndef typeof
      #define typeof(...) __typeof__(__VA_ARGS__)
    #endif

    #ifndef typeof_unqual
      #if __cplusplus >= 202002L
        #define typeof_unqual(...) std::remove_cvref_t<__typeof__(__VA_ARGS__)>

      #elif __cplusplus >= 201402L
        #define typeof_unqual(...) std::remove_cv_t<std::remove_reference_t<__typeof__(__VA_ARGS__)>>

      #else
        #define typeof_unqual(...) typename std::remove_cv<typename std::remove_reference<__typeof__(__VA_ARGS__)>::type>::type
      #endif
    #endif

  #else
    // C23 / GNU C extensions
    #ifndef typeof
      #define typeof(...) __typeof__(__VA_ARGS__)
    #endif

    #ifndef typeof_unqual
      #define typeof_unqual(...) __typeof_unqual__(__VA_ARGS__)
    #endif
  #endif
  #define types_eq(T1, T2) \
    _Generic((T1 *)0, T2 *: 1, default: 0)
  #define UNQUAL(...) __typeof__(1 ? (__VA_ARGS__) : (__VA_ARGS__))
  #define itypeof(struct, member) typeof(((struct *)0)->member)
  #define ptrstype(ptr) typeof(typeof(*(typeof(ptr))nullptr))
  #define arrstype(arr) typeof((*(typeof(arr) *)nullptr)[0])

  #define IS_CTARRAY(x) \
    (!types_eq(typeof(x), typeof(1 ? (x) : (x))))
  #if defined __cplusplus
    #define IS_CTARRAY(x) \
      (!types_eq(typeof(x), std::decay_t<typeof(x)>))
  #endif
_Static_assert(IS_CTARRAY("hello"));
_Static_assert(!IS_CTARRAY((char *)"hello"));
  #define isArray(a) IS_CTARRAY(a)
  #define VLAP(ptr, len) ((typeof(typeof(*ptr))(*)[len])ptr)

  #define asU8Vla(x) *VLAP((u8 *)&x, sizeof(x))

  #define mcmp(a, b) ({                                       \
    var_ _a = &a;                                             \
    var_ _b = &b;                                             \
    typedef typeof(({ *_a; })) _da;                           \
    typedef typeof(({ *_b; })) _db;                           \
    _Static_assert(types_eq(_da, _db), "not the same type");  \
    __builtin_memcmp(_a, _b, MIN$(sizeof(*_b), sizeof(*_a))); \
  })
  #define mcpy(a, b) ({                                                                   \
    let _a = &a;                                                                          \
    let _b = &b;                                                                          \
    typedef typeof(({ *_a; })) _da;                                                       \
    typedef typeof(({ *_b; })) _db;                                                       \
    _Static_assert(types_eq(_da, _db), "not the same type");                              \
    (typeof(_a))__builtin_memcpy((void *)_a, (void *)_b, MIN$(sizeof(*_b), sizeof(*_a))); \
  })
  #define mset(mem, v) ({                                                                         \
    var_ _m = &mem;                                                                               \
    var_ _v = v;                                                                                  \
    _Static_assert(types_eq(typeof((*_m)[0]), typeof(_v)), #mem "[0] must be of typeof(" #v ")"); \
    foreach (var_ i, span(*_m, countof(*_m)))                                                     \
      *i = _v;                                                                                    \
  })
  #define struct_imm_type(x) typeof(TUPLE_EXPAND_REST(x)) TUPLE_EXPAND_FIRST(x);
  #define struct_imm_value(x) (TUPLE_EXPAND_REST(x)),
  #define struct_imm(...) \
    ((struct {APPLY_N(struct_imm_type, __VA_ARGS__)}){APPLY_N(struct_imm_value, __VA_ARGS__)})

  #include "macros/match_tu.h"
  #include "macros/match_type.h"
#endif
