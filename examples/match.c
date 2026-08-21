#include "../macros.h"
#include "../mytypes.h"
#include <stdlib.h>

tu_def(
    (integer, char),
    (iu32, unsigned int),
    (ii32, int),
    (iu64, unsigned long long),
    (ii64, long long),
);

#define integer_of(i) match_type(      \
    i,                                 \
    (u32, u, (integer)tu_of(iu32, u)), \
    (i32, s, (integer)tu_of(ii32, s)), \
    (u64, u, (integer)tu_of(iu64, u)), \
    (i64, s, (integer)tu_of(ii64, s)), \
)

integer five() { return integer_of(5); }
int main(void) {
  integer f = integer_of(5);
  return tu_catch(iu32, f) = 6;
  return tu_catch(iu32, f, abort()) = 6;
  return tu_catchr(iu32, five());

  return tu_match_exp(
      f,
      (iu32, u, (usize)u),
      (ii32, u, (usize)u),
      (iu64, u, (usize)u),
      (ii64, u, (usize)u),
  );
  tu_match_void(
      f,
      (iu32, u, return u),
      (ii32, u, return u),
      (iu64, u, return u),
      (ii64, u, return u),
  );
  switch_exp(
      f.tag,
      (iu32_enum, 1),
      (ii32_enum, 1),
  );
}
