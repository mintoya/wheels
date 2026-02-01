def main [ammount:int] {
  do {
    mut $filec = "\n"
    $filec = $filec + "#ifndef VARIADIC_APPLY_H\n"
    $filec = $filec + "#define VARIADIC_APPLY_H\n"
    $filec = $filec + "#define APPLY_1(M, a) M(a)\n"
    $filec = $filec + "#define APPLY_0(M, a)\n"
    for i in 2..$ammount { 
      $filec = $filec + $"#define APPLY_($i)\(M, a, ...\) M\(a\), APPLY_($i - 1)\(M, __VA_ARGS__\)\n"
    }

    $filec = $filec + "#define ARG_N("
    for i in 1..$ammount {
    $filec = $filec + $"_($i),"
    }
    $filec = $filec + "N,...) N\n"
    $filec = $filec + "#define COUNT_ARGS(...) ARG_N(__VA_ARGS__"
    for i in $ammount..0 {
    $filec = $filec + $",($i)"
    }
    $filec = $filec + ")\n"

    $filec = $filec + "#define CAT(A, B) A##B\n"
    $filec = $filec + "#define APPLY_MACRO_TO_ARGS_HELPER(N) CAT(APPLY_, N)\n"
    $filec = $filec + "#define APPLY_N(MACRO, ...)                                                    \\\n"
    $filec = $filec + "  APPLY_MACRO_TO_ARGS_HELPER(COUNT_ARGS(__VA_ARGS__))(MACRO, __VA_ARGS__)\n"
    $filec = $filec + "\n"
    $filec = $filec + "#endif // VARIADIC_APPLY_H\n"
    return $filec
  }
  | save -f variadic.h
}
