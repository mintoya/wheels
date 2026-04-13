#include "stdckdint.h"
#include <__stdarg_va_list.h>
#include <stdarg.h>
#include <stddef.h>
#include <stddefer.h>
#include <string.h>
#define my_ckd_add(a, b) ({                     \
  struct {                                      \
    typeof(a) carry, result;                    \
  } ___res;                                     \
  ___res.carry = ckd_add(&___res.result, a, b); \
  ___res;                                       \
})
#define my_ckd_sub(a, b) ({                    \
  struct {                                     \
    typeof(a) flag, result;                    \
  } ___res;                                    \
  ___res.flag = ckd_sub(&___res.result, a, b); \
  ___res;                                      \
})
#define my_ckd_mul(a, b) ({                    \
  struct {                                     \
    typeof(a) flag, result;                    \
  } ___res;                                    \
  ___res.flag = ckd_mul(&___res.result, a, b); \
  ___res;                                      \
})
#include "wheels/arenaAllocator.h"
#include "wheels/macros.h"
#include "wheels/mytypes.h"
#include "wheels/print.h"
#include "wheels/shmap.h"
#include "wheels/wheels.h"

typedef u8 bigint_unit;
typedef msList(bigint_unit) bigint;
typedef typeof(*((bigint)NULL)) bigint_unit;

bool bigint_negetive(bigint i) {
  return i[msList_len(i) - 1] & 0x80;
}
void bigint_trim(bigint i) {
  if (msList_len(i) < 2)
    return;
  const bigint_unit sign_bit = (bigint_unit)1 << (sizeof(bigint_unit) * 8 - 1);
  if (bigint_negetive(i)) {
    while (
        msList_len(i) > 1 &&
        i[msList_len(i) - 1] == (bigint_unit)-1 &&
        (i[msList_len(i) - 2] & sign_bit)) {
      msList_len(i)--;
    }
  } else {
    while (
        msList_len(i) > 1 &&
        i[msList_len(i) - 1] == 0 &&
        !(i[msList_len(i) - 2] & sign_bit)) {
      msList_len(i)--;
    }
  }
}
bigint bigint_from(AllocatorV allocator, usize len, u8 *buffer) {
  usize l2 =
      (len / sizeof(bigint_unit)) * sizeof(bigint_unit) + 1;
  bigint res = msList_init(allocator, bigint_unit, l2);
  memcpy(res, buffer, len);
  msList_len(res) = l2;
  bigint_trim(res);
  return res;
}
void bigint_expand(AllocatorV allocator, usize len, bigint *i) {
  bigint_unit n = bigint_negetive(i[0]) ? -1 : 0;
  while (msList_len(i[0]) < len)
    msList_push(allocator, *i, n);
}
bigint bigint_copy(AllocatorV allocator, bigint b) {
  bigint res = bigint_from(allocator, 0, 0);
  msList_pushArr(allocator, res, *msList_vla(b));
  return res;
}
bigint bigint_add(AllocatorV allocator, bigint a, bigint b) {
  bigint res = bigint_from(allocator, 0, 0);

  usize len = msList_len(a) > msList_len(b)
                  ? msList_len(a)
                  : msList_len(b);

  typeof(*((bigint)NULL)) carry = 0;

  for (each_RANGE(usize, i, 0, len)) {
    typeof(*((bigint)NULL)) ai = P$(msList_get(a, i), $ ? *$ : 0);
    typeof(*((bigint)NULL)) bi = P$(msList_get(b, i), $ ? *$ : 0);
    var_ ckd = my_ckd_add(ai, bi);
    var_ ckd2 = my_ckd_add(ckd.result, carry);
    carry = ckd2.carry | ckd.carry;
    msList_push(allocator, res, ckd2.result);
  }
  msList_push(allocator, res, carry);
  bigint_trim(res);
  return res;
}
void bigint_add_ip(AllocatorV allocator, bigint *a, bigint br) {
  bigint b = bigint_copy(allocator, br);
  defer { msList_deInit(allocator, b); };

  usize len = msList_len(a[0]) > msList_len(b)
                  ? msList_len(a[0])
                  : msList_len(b);
  bigint_expand(allocator, len, a);
  bigint_expand(allocator, len, &b);

  bigint_unit carry = 0;

  for (each_RANGE(usize, i, 0, len)) {
    typeof(*((bigint)NULL)) ai = P$(msList_get(a[0], i), $ ? *$ : 0);
    typeof(*((bigint)NULL)) bi = P$(msList_get(b, i), $ ? *$ : 0);
    var_ ckd = my_ckd_add(ai, bi);
    var_ ckd2 = my_ckd_add(ckd.result, carry);
    carry = ckd2.carry | ckd.carry;
    a[0][i] = ckd2.result;
    // msList_push(allocator, res, ckd2.result);
  }
  msList_push(allocator, a[0], carry);
  bigint_trim(*a);
}
i8 bigint_cmp(bigint a, bigint b) {
  bigint_trim(a);
  bigint_trim(b);
  bool negetiveMode;
  if (bigint_negetive(a)) {
    if (bigint_negetive(b)) {
      negetiveMode = true;
    } else {
      return 1;
    }
  } else if (bigint_negetive(b)) {
    return -1;
  }
  usize la = msList_len(a);
  usize lb = msList_len(b);
  if (la != lb)
    return la > lb ? -1 : 1;
  for (usize i = la; i > 0; i--) {
    if (a[i - 1] > b[i - 1])
      return -1;
    else if (a[i - 1] < b[i - 1])
      return 1;
  }
  return 0;
}
bigint bigint_mul_i(AllocatorV allocator, bigint a, bigint_unit b) {
  bigint res = bigint_from(allocator, 0, 0);

  const int half = sizeof(bigint_unit) * 4;
  const bigint_unit mask = ((bigint_unit)1 << half) - 1;

  bigint_unit carry = 0;

  for_each_((var_ v, msList_vla(a)), {
    bigint_unit a0 = v & mask;
    bigint_unit a1 = v >> half;

    bigint_unit b0 = b & mask;
    bigint_unit b1 = b >> half;

    bigint_unit p0 = a0 * b0;
    bigint_unit p1 = a0 * b1;
    bigint_unit p2 = a1 * b0;
    bigint_unit p3 = a1 * b1;

    bigint_unit mid =
        (p0 >> half) +
        (p1 & mask) +
        (p2 & mask) +
        carry;

    msList_push(allocator, res, (p0 & mask) | (mid << half));
    carry = (p1 >> half) + (p2 >> half) + (p3) + (mid >> half);
  });

  if (carry)
    msList_push(allocator, res, carry);

  return res;
}
bigint bigint_mul(AllocatorV allocator, bigint a, bigint b) {
  bigint_trim(a);
  bigint_trim(b);
  AllocatorV arena = arena_new_ext(allocator, 1024);
  defer { arena_cleanup(arena); };

  mList(bigint) resultsList = mList_init(arena, bigint, msList_len(b) ?: 1);

  for (each_RANGE(usize, curr, 0, msList_len(b))) {
    var_ tmp = bigint_mul_i(arena, a, b[curr]);
    msList_insArr(arena, tmp, 0, *VLAP((bigint)NULL, curr));
    mList_push(resultsList, tmp);
  }

  bigint res = P$(
      msList_len(a) + msList_len(b),
      bigint_from(allocator, $, NULL)
  );
  for_each_((bigint bi, mList_vla(resultsList)), {
    bigint_add_ip(allocator, &res, bi);
  });
  return res;
}

NAMESPACE_STRUCT(
    BInt,
    (add, &bigint_add),
    (mul, &bigint_mul),
    (cmp, &bigint_cmp),
    (copy, &bigint_copy),
    (from, &bigint_from),
    (trim, &bigint_trim),
    (expand, &bigint_expand),
    (negetive, &bigint_negetive),
);
REGISTER_PRINTER(bigint, {
  for (usize i = msList_len(in); i > 0; i--)
    for (usize j = sizeof(*in); j > 0; j--)
      USENAMEDPRINTER("x", ((u8 *)(in + (i - 1)))[j - 1]);
});
int main(void) {
  bigint b = BInt.from(stdAlloc, 2, (u8[]){0xff, 0xff});
  bigint a = BInt.from(stdAlloc, 1, (u8[]){0xff});
  bigint c = BInt.mul(stdAlloc, a, b);
  println("{bigint} * {bigint} = {bigint}", a, b, c);
}
