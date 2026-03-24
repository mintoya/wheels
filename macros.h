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
static inline T *TEMPORARY_REF_UB(T &&v) { return &v; }
    #define REF(type, value) TEMPORARY_REF_UB((type){value})
template <class To, class From>
inline To bit_cast_func(const From &src) noexcept {
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
    MACRO_EXPAND1(MACRO_EXPAND1(MACRO_EXPAND1(MACRO_EXPAND1(__VA_ARGS__))))
  #define MACRO_EXPAND3(...) \
    MACRO_EXPAND2(MACRO_EXPAND2(MACRO_EXPAND2(MACRO_EXPAND2(__VA_ARGS__))))
  #define MACRO_EXPAND4(...) \
    MACRO_EXPAND3(MACRO_EXPAND3(MACRO_EXPAND3(MACRO_EXPAND3(__VA_ARGS__))))
  #define MACRO_EXPAND5(...) \
    MACRO_EXPAND4(MACRO_EXPAND4(MACRO_EXPAND4(MACRO_EXPAND4(__VA_ARGS__))))
  #define MACRO_EXPAND(...) \
    MACRO_EXPAND4(__VA_ARGS__)

  #define DEFER_NAME(a, b) ID_CONCAT(a, b)

  #if defined(__cplusplus)
    #pragma GCC warning "using cpp closure defer"
    #include <utility>
template <typename F>
struct Deferrer {
  F fn;
  ~Deferrer() { fn(); }
};

struct DeferHelper {
  template <typename F>
  Deferrer<F> operator+(F &&f) { return {std::forward<F>(f)}; }
};

    #define defer auto DEFER_NAME(_defer_, __LINE__) = DeferHelper() + [&]()
  #else
    #if __has_include(<stddefer.h>) && __STDC_VERSION__ >= 202311L
      #include <stddefer.h>
    #else
      #if defined(__clang__)
        #pragma GCC warning "using clang block defer (captures only work on pointers)"
static inline void _defer_cleanup_block(void (^*block)(void)) { (*block)(); }
        #define defer __attribute__((cleanup(_defer_cleanup_block))) void (^ID_CONCAT(_defer_var_, __COUNTER__))(void) = ^
      #elif defined(__GNUC__)
        #pragma GCC warning "using gnu nested function defer"
        #define _defer_helper(func_name, var_name)              \
          auto void func_name(int *);                           \
          int var_name __attribute__((cleanup(func_name))) = 0; \
          void func_name(int *_)
        #define defer _defer_helper(ID_CONCAT(_defer_func_, __COUNTER__), ID_CONCAT(_defer_var_, __COUNTER__))
      #endif
    #endif
  #endif
  #define PRAGMA_MAKE_STR(...) #__VA_ARGS__
  #define MAKE_PRAGMA(warning) _Pragma(PRAGMA_MAKE_STR(clang diagnostic ignored warning))
  #define DIAGNOSTIC_PUSH(...) \
    APPLY_N(MAKE_PRAGMA, __VA_ARGS__)
  #define DIAGNOSTIC_POP() \
    _Pragma("clang diagnostic pop")

  #define APPLY_N(macro, ...) MACRO_EXPAND(APPLY_N_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER(macro, arg, ...) macro(arg) \
      __VA_OPT__(APPLY_N_HELPER_INVOKE PARENTHESIS_HELPER(macro, __VA_ARGS__))
  #define APPLY_N_HELPER_INVOKE() APPLY_N_HELPER

  #define COUNT_ONE_MACRO(x) +1
  #define COUNT_ARGS(...) (__VA_OPT__(APPLY_N(COUNT_ONE_MACRO, __VA_ARGS__)) + 0)
  #define EQUAL_ANY_HELPER(a) a ||
  #define EQUAL_ANY(expr, ...) (APPLY_N((expr) == EQUAL_ANY_HELPER, __VA_ARGS__) 0)
  #define EQUAL_ALL_HELPER(a) a &&
  #define EQUAL_ALL(expr, ...) (APPLY_N((expr) == EQUAL_ALL_HELPER, __VA_ARGS__) 1)

  #define ASSERT_EXPR(cond, msg) \
    (0 * (int)sizeof(char[1 - 2 * !(cond)]))

  #define STR_H(...) #__VA_ARGS__
  #define VLAP(ptr, len) ((typeof(typeof(*ptr))(*)[len])ptr)

  #define VA_SWITCH_FIRST(first, ...) first

  #define VA_SWITCH_EMPTY(...) VA_SWITCH_FIRST(__VA_OPT__(0, ) 1, ~)
  #define VA_SWITCH_IMPL_0(default_val, ...) __VA_ARGS__
  #define VA_SWITCH_IMPL_1(default_val) default_val
  #define VA_SWITCH(default_val, ...) \
    ID_CONCAT(VA_SWITCH_IMPL_, VA_SWITCH_EMPTY(__VA_ARGS__))(default_val __VA_OPT__(, ) __VA_ARGS__)

  #define _RANGE_NAME(prefix) ID_CONCAT(_range_val_, ID_CONCAT(prefix, __LINE__))

  #define each_RANGE(type, name, start, end, ...)                            \
    type name = (start),                                                     \
         *const _RANGE_NAME(s) = (typeof(_RANGE_NAME(s)))REF(type, (start)), \
         *const _RANGE_NAME(e) = (typeof(_RANGE_NAME(e)))REF(type, (end));   \
    (*(type const *)_RANGE_NAME(s) <= *(type const *)_RANGE_NAME(e)          \
         ? name < *(type *)_RANGE_NAME(e)                                    \
         : name > *(type *)_RANGE_NAME(e));                                  \
    name += VA_SWITCH(*_RANGE_NAME(s) > *_RANGE_NAME(e) ? -1 : 1, __VA_ARGS__)

  #define types_eq(T1, T2) \
    _Generic((T1){0}, T2: true, default: false)

  #define SCOPED_DECLARATION_LOOPS(decl)                                       \
    for (char _RANGE_NAME(once) = 1; _RANGE_NAME(once); _RANGE_NAME(once) = 0) \
      for (decl; _RANGE_NAME(once); _RANGE_NAME(once) = 0)

  #define each_VLAP(type, name, vla)                                    \
    each_RANGE(                                                         \
        type,                                                           \
        name,                                                           \
        (vla)[0] + ASSERT_EXPR(types_eq(type, typeof(&vla[0][0])), ""), \
        (vla)[1],                                                       \
        1                                                               \
    )

  #define FOREACH_SPLIT_LABEL(label, ...) label
  #define FOREACH_SPLIT_ARGS(label, ...) __VA_ARGS__

  #define foreach_ptr(decl, vla, ...)                                 \
    /* as long as VA_ARGS must starts with a label(: not included )*/ \
    for (each_VLAP(typeof(&(vla)[0][0]), _RANGE_NAME(item), vla))     \
      if (false) {                                                    \
        FOREACH_SPLIT_LABEL(__VA_ARGS__)                              \
        decl = (typeof(typeof((vla)[0][0]))(*)[1])_RANGE_NAME(item);  \
        {                                                             \
          FOREACH_SPLIT_ARGS(__VA_ARGS__)                             \
        }                                                             \
      } else                                                          \
        SCOPED_DECLARATION_LOOPS(decl = (typeof(typeof((vla)[0][0]))(*)[1])_RANGE_NAME(item))

  #define foreach(decl, vla, ...)                                 \
    /* assuming VA_ARGS starts with a label*/                     \
    for (each_VLAP(typeof(&(vla)[0][0]), _RANGE_NAME(item), vla)) \
      if (false) {                                                \
        FOREACH_SPLIT_LABEL(__VA_ARGS__)                          \
        decl = *_RANGE_NAME(item);                                \
        FOREACH_SPLIT_ARGS(__VA_ARGS__)                           \
        break;                                                    \
      } else                                                      \
        SCOPED_DECLARATION_LOOPS(decl = *_RANGE_NAME(item))

  #define var __auto_type
#endif
