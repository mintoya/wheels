#if !defined(MY_MACROS_H)
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

  #define DEFER_CONCAT_1(a, b) a##b
  #define DEFER_CONCAT(a, b) DEFER_CONCAT_1(a, b)
  #define DEFER_NAME(a, b) DEFER_CONCAT(a, b)

  #if defined(__cplusplus)
    #pragma GCC warning "using cpp closure defer"
    #include <memory>
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
    #if __has_include(<stddefer.h>)
      #include <stddefer.h>
    #else
      #if defined(__clang__)
        #pragma GCC warning "using clang block defer (captures only work on pointers)"
static inline void _defer_cleanup_block(void (^*block)(void)) { (*block)(); }
        #define defer __attribute__((cleanup(_defer_cleanup_block))) void (^DEFER_CONCAT(_defer_var_, __COUNTER__))(void) = ^
      #elif defined(__GNUC__)
        #pragma GCC warning "using gnu nested function defer"
        #define _defer_helper(func_name, var_name)              \
          auto void func_name(int *);                           \
          int var_name __attribute__((cleanup(func_name))) = 0; \
          void func_name(int *_)
        #define defer _defer_helper(DEFER_CONCAT(_defer_func_, __COUNTER__), DEFER_CONCAT(_defer_var_, __COUNTER__))
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

  #define VLAP(ptr, len) ((typeof(typeof(*ptr))(*)[len])ptr)

  #define each_VLAP(name, vla)                   \
    typeof(vla[0][0]) *name = (typeof(name))vla; \
    name < vla[1];                               \
    name++

#endif
