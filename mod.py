import cog
def h_start(n):
    cog.outl(f"#if !defined {n}_H\n  #define {n}_H (1)")

def h_end():
    cog.outl("#endif")

def c_start(n):
    cog.outl(f"#if (defined {n}_C && {n}_C == 1) || (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)")
    cog.outl(f" #undef  {n}_C")
    cog.outl(f" #define {n}_C (2)")

def c_end():
    cog.outl("#endif")
