
def main [ammount:int] {
  let amt = ([ $ammount, 10 ] | math max);
  do {
    mut $filec = "\n"
    $filec = $filec + "#ifndef GENERICS_H\n"
    $filec = $filec + "#define GENERICS_H\n"

    for i in 1..$amt {
      $filec = $filec + $" #define GENERIC_TYPE_($i)\n"
    }
    $filec = $filec + "#endif //GENERICS_H\n"
    $filec = $filec + "#define GENERIC_ELEMENT(type) type:#type,\n"
    $filec = $filec + "#define GENERIC_NAME(val) \\\n"
    $filec = $filec + "_Generic((val), \\\n"
    for i in 1..$amt {
      $filec = $filec + $" GENERIC_TYPE_($i) \\\n"
    }
    $filec = $filec + "default:\"\")\n"
    $filec = $filec + "#define EXPAND_(x) x\n"
    $filec = $filec + "#define EXPAND(x) EXPAND_(x)\n"

    $filec = $filec + "#if GENERIC_NAME_N == 0\n"
    $filec = $filec + " #undef GENERIC_NAME_N\n"
    $filec = $filec + " #define GENERIC_NAME_N 1\n"

    for i in 1..$amt {
      $filec = $filec + $" #elif GENERIC_NAME_N == ($i)\n"
      $filec = $filec + $" #undef GENERIC_NAME_N\n"
      $filec = $filec + $" #define GENERIC_NAME_N ($i + 1)\n"
    }
    $filec = $filec + "#endif\n"

    $filec = $filec + "#if GENERIC_NAME_N == 1\n"
    $filec = $filec + "#undef GENERIC_TYPE_1\n"
    $filec = $filec + "#define GENERIC_TYPE_1 GENERIC_ELEMENT(_1____tn)\n"
    for i in 2..$amt {
      $filec = $filec + $"#elif GENERIC_NAME_N == ($i)\n"
      $filec = $filec + $"#undef GENERIC_TYPE_($i)\n"
      $filec = $filec + $"#define GENERIC_TYPE_($i) GENERIC_ELEMENT\(_($i)____tn\)\n"
    }
    $filec = $filec + "#endif\n"

    $filec = $filec + "#define GEN_CAT_(a, b) a##b\n"
    $filec = $filec + "#define GEN_CAT(a, b) GEN_CAT_(a, b)\n"
    $filec = $filec + "#define GEN_CAT_3(a,b,c) GEN_CAT(a,GEN_CAT(b,c))\n"
    $filec = $filec + "#define STRINGIFY(x) #x\n"
    $filec = $filec + "#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)\n"

    $filec = $filec + "#define MAKE_NEW_TYPE_HELPER(type, newtype)\\\n"
    $filec = $filec + "typedef type newtype;\\\n"
    $filec = $filec + "REGISTER_ALIASED_PRINTER(type, newtype);\n"
    $filec = $filec + "#define MAKE_NEW_TYPE(type)\\\n"
    $filec = $filec + "MAKE_NEW_TYPE_HELPER(type, GEN_CAT_3(_,GENERIC_NAME_N,____tn))\n"

    return $filec
  }
  | save -f genericName.h
}
