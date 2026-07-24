#include "../macros.h"
#include "../mytypes.h"

tu_def(
    (integer, char),
    (iu32, unsigned int),
    (ii32, int),
    (iu64, unsigned long long),
    (ii64, long long),
);

#define integer_of(i) match_type(     \
    i,                                \
    (u32, u, (integer)tu_of(u32, u)), \
    (i32, s, (integer)tu_of(i32, s)), \
    (u64, u, (integer)tu_of(u64, u)), \
    (i64, s, (integer)tu_of(i64, s)), \
)

int main(void) {
  integer f;
  tu_match_exp(
      f,
      (iu32, u, (usize)u),
      (ii32, u, (usize)u),
      (iu64, u, (usize)u),
      (ii64, u, (usize)u),
  );
}
