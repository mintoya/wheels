
#ifndef VARIADIC_APPLY_H
#define VARIADIC_APPLY_H
#define APPLY_1(M, a) M(a)
#define APPLY_0(M, a)
#define APPLY_2(M, a, ...) M(a), APPLY_1(M, __VA_ARGS__)
#define APPLY_3(M, a, ...) M(a), APPLY_2(M, __VA_ARGS__)
#define APPLY_4(M, a, ...) M(a), APPLY_3(M, __VA_ARGS__)
#define APPLY_5(M, a, ...) M(a), APPLY_4(M, __VA_ARGS__)
#define APPLY_6(M, a, ...) M(a), APPLY_5(M, __VA_ARGS__)
#define APPLY_7(M, a, ...) M(a), APPLY_6(M, __VA_ARGS__)
#define APPLY_8(M, a, ...) M(a), APPLY_7(M, __VA_ARGS__)
#define APPLY_9(M, a, ...) M(a), APPLY_8(M, __VA_ARGS__)
#define APPLY_10(M, a, ...) M(a), APPLY_9(M, __VA_ARGS__)
#define APPLY_11(M, a, ...) M(a), APPLY_10(M, __VA_ARGS__)
#define APPLY_12(M, a, ...) M(a), APPLY_11(M, __VA_ARGS__)
#define APPLY_13(M, a, ...) M(a), APPLY_12(M, __VA_ARGS__)
#define APPLY_14(M, a, ...) M(a), APPLY_13(M, __VA_ARGS__)
#define APPLY_15(M, a, ...) M(a), APPLY_14(M, __VA_ARGS__)
#define APPLY_16(M, a, ...) M(a), APPLY_15(M, __VA_ARGS__)
#define APPLY_17(M, a, ...) M(a), APPLY_16(M, __VA_ARGS__)
#define APPLY_18(M, a, ...) M(a), APPLY_17(M, __VA_ARGS__)
#define APPLY_19(M, a, ...) M(a), APPLY_18(M, __VA_ARGS__)
#define APPLY_20(M, a, ...) M(a), APPLY_19(M, __VA_ARGS__)
#define APPLY_21(M, a, ...) M(a), APPLY_20(M, __VA_ARGS__)
#define APPLY_22(M, a, ...) M(a), APPLY_21(M, __VA_ARGS__)
#define APPLY_23(M, a, ...) M(a), APPLY_22(M, __VA_ARGS__)
#define APPLY_24(M, a, ...) M(a), APPLY_23(M, __VA_ARGS__)
#define APPLY_25(M, a, ...) M(a), APPLY_24(M, __VA_ARGS__)
#define APPLY_26(M, a, ...) M(a), APPLY_25(M, __VA_ARGS__)
#define APPLY_27(M, a, ...) M(a), APPLY_26(M, __VA_ARGS__)
#define APPLY_28(M, a, ...) M(a), APPLY_27(M, __VA_ARGS__)
#define APPLY_29(M, a, ...) M(a), APPLY_28(M, __VA_ARGS__)
#define APPLY_30(M, a, ...) M(a), APPLY_29(M, __VA_ARGS__)
#define APPLY_31(M, a, ...) M(a), APPLY_30(M, __VA_ARGS__)
#define APPLY_32(M, a, ...) M(a), APPLY_31(M, __VA_ARGS__)
#define ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,N,...) N
#define COUNT_ARGS_HELPER(...) ARG_N(__VA_ARGS__,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define SELECT_FIRST_ARG(a,...) a
#define COUNT_ARGS(...) SELECT_FIRST_ARG(__VA_OPT__(COUNT_ARGS_HELPER(__VA_ARGS__), ) 0)
#define CAT(A, B) A##B
#define APPLY_MACRO_TO_ARGS_HELPER(N) CAT(APPLY_, N)
#define APPLY_N(MACRO, ...)                                                    \
  APPLY_MACRO_TO_ARGS_HELPER(COUNT_ARGS(__VA_ARGS__))(MACRO, __VA_ARGS__)

#endif // VARIADIC_APPLY_H
