#if !defined(MY_MACROS_H)
  #define MY_MACROS_H
  #if defined(__cplusplus)
    #include <memory>
    #define DEFER_CONCAT(a, b) a##b
    #define DEFER_NAME(a, b) DEFER_CONCAT(a, b)
    #define defer_(...) \
      std::shared_ptr<void> DEFER_NAME(deferptr, __LINE__)(nullptr, [&](void *) -> void { __VA_ARGS__ })
template <typename F>
struct Deferrer {
  F fn;
  ~Deferrer() { fn(); }
};

struct DeferHelper {
  template <typename F>
  Deferrer<F> operator+(F &&f) { return {std::forward<F>(f)}; }
};
  // idk

    #define DEFER_CONCAT(a, b) a##b
    #define DEFER_NAME(a, b) DEFER_CONCAT(a, b)
    #define defer auto DEFER_NAME(_defer_, __LINE__) = DeferHelper() + [&]()
  #else
    #if __has_include(<stddefer.h>)
      #include <stddefer.h>
      #define defer_(...) defer{__VA_ARGS__};
    #else
      #ifdef __clang__
static inline void _defer_cleanup_block(void (^*block)(void)) { (*block)(); }
        #define _DEFER_CONCAT_IMPL(x, y) x##y
        #define _DEFER_CONCAT(x, y) _DEFER_CONCAT_IMPL(x, y)
        #define defer __attribute__((cleanup(_defer_cleanup_block))) void (^_DEFER_CONCAT(_defer_var_, __COUNTER__))(void) = ^
      #endif
    #endif
  #endif

  #define PRAGMA_MAKE_STR(...) #__VA_ARGS__
  #define MAKE_PRAGMA(warning) _Pragma(PRAGMA_MAKE_STR(clang diagnostic ignored warning))
  #define DIAGNOSTIC_PUSH(...) \
    APPLY_N(MAKE_PRAGMA, __VA_ARGS__)
  #define DIAGNOSTIC_POP() \
    _Pragma("clangd diagnostic pop")

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
  MACRO_EXPAND5(__VA_ARGS__)

#define APPLY_N(macro, ...) MACRO_EXPAND(APPLY_N_HELPER(macro, __VA_ARGS__))
#define APPLY_N_HELPER(macro, arg, ...) macro(arg) \
    __VA_OPT__(APPLY_N_HELPER_INVOKE PARENTHESIS_HELPER(macro, __VA_ARGS__))
#define APPLY_N_HELPER_INVOKE() APPLY_N_HELPER

#define COUNT_ONE_MACRO(x) +1
#define COUNT_ARGS(...) (__VA_OPT__(APPLY_N(COUNT_ONE_MACRO, __VA_ARGS__)) + 0)
#endif
