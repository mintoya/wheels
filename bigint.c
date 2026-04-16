
#include "stdckdint.h"
#include "wheels/arenaAllocator.h"
#include "wheels/debugallocator.h"
#include "wheels/fptr.h"
#include "wheels/macros.h"
#include "wheels/mytypes.h"
#include "wheels/print.h"
#include "wheels/wheels.h"
#include <cstring>
#include <stdarg.h>
#include <stddef.h>
#include <threads.h>

#if !defined(BIGINT_BITS)
  #define BIGINT_BITS 8
#endif
typedef unsigned _BitInt(BIGINT_BITS) bigint_unit;
typedef msList(bigint_unit) bigint;
typedef typeof(*((bigint)NULL)) bigint_unit;

bool bigint_ckd_add(bigint_unit *res, bigint_unit a, bigint_unit b) {
  static_assert(~(bigint_unit)0 > 0, "must be unsigned ");
  *res = a + b;
  return a > (~(bigint_unit)0) - b;
}
struct bigint_ckdt {
  bigint_unit result;
  bool flag;
};
struct bigint_ckdt bigint_ckd_add_struct(bigint_unit a, bigint_unit b) {
  typeof(bigint_ckd_add_struct(a, b)) res;
  res.flag = bigint_ckd_add(&res.result, a, b);
  return res;
}
bool bigint_negetive(bigint i) {
  return i ? !!(i[msList_len(i) - 1] & ((((bigint_unit)-1) >> 1) + 1)) : 0;
}
bigint_unit bigint_get(bigint b, usize idx) {
  bigint_unit u = bigint_negetive(b) ? -1 : 0;
  return b
             ? P$(msList_get(b, idx),
                  $ ? *$ : u;)
             : 0;
}
int bigint_cmp(bigint a, bigint b) {
  i8 neg = bigint_negetive(a);
  if (neg != bigint_negetive(b))
    return neg ? -1 : 1;
  neg = neg ? -1 : 1;

  usize len_a = a ? msList_len(a) : 0;
  usize len_b = b ? msList_len(b) : 0;
  usize max_len = len_a > len_b ? len_a : len_b;

  for (usize k = max_len; k > 0; k--) {
    usize i = k - 1;
    bigint_unit ua = bigint_get(a, i);
    bigint_unit ub = bigint_get(b, i);
    if (ua != ub)
      return (ua < ub) ? -neg : neg;
  }

  return 0;
}
void bigint_trim(bigint *b) {
  sList_header *slh = msList_header(*b);
  if (bigint_negetive(b[0])) {
    bigint_unit check = (((bigint_unit)-1) >> 1) + 1;
    bigint_unit skip = ((bigint_unit)-1);
    while (slh->length > 1 && b[0][slh->length - 1] == skip && b[0][slh->length - 2] & check)
      slh->length--;
  } else {
    bigint_unit check = (((bigint_unit)-1) >> 1) + 1;
    bigint_unit skip = 0;
    while (slh->length > 1 && b[0][slh->length - 1] == skip && !(b[0][slh->length - 2] & check))
      slh->length--;
  }
}
void bigint_expand(AllocatorV allocator, bigint *b, usize len) {
  assertMessage(len >= msList_len(*b), "%zu < %zu", len, msList_len(*b));
  bigint_unit u = 0;
  if (bigint_negetive(b[0]))
    u = (bigint_unit)-1;
  while (len >= msList_len(*b))
    msList_push(allocator, *b, u);
}
bigint bigint_copy(AllocatorV allocator, bigint b) {
  var_ r = msList_init(allocator, bigint_unit, b ? msList_len(b) : 1);
  if (b)
    msList_pushArr(allocator, r, *msList_vla(b));
  return r;
}

void bigint_add_unit_ip(AllocatorV allocator, bigint *a, bigint_unit b) {
  assertMessage(a && *a);
  for (usize p = 0; b && p < msList_len(a); p++) {
    var_ c = bigint_ckd_add_struct(a[0][p], b);
    b = c.flag;
    a[0][p] = c.result;
  }
  msList_push(allocator, *a, b);
}
void bigint_negate_ip(AllocatorV allocator, bigint *i) {
  assertMessage(i);
  if (!*i)
    return;
  for_each_P((var_ j, msList_vla(*i)), { *j = ~*j; });
  bigint_unit ca = 1;
  for (usize c = 0; ca && c < msList_len(i[0]); c++) {
    var_ c1 = bigint_ckd_add_struct(ca, i[0][c]);
    i[0][c] = c1.result;
    ca = c1.flag;
  }
}
void bigint_add_ip_flag(AllocatorV allocator, bigint *a, bigint b, bool negate) {
  usize len = msList_len(a[0]) > msList_len(b)
                  ? msList_len(a[0])
                  : msList_len(b);
  bigint_expand(allocator, a, len);
  bigint_unit carry = 0;

  for (usize i = 0; i < len; i++) {
    var_ ra = bigint_ckd_add_struct(negate ? ~a[0][i] : a[0][i], carry);
    var_ rb = bigint_ckd_add_struct(ra.result, bigint_get(b, i));
    a[0][i] = negate ? ~rb.result : rb.result;
    carry = ra.flag + rb.flag;
  }
  msList_len(a[0]) = len;
}
void bigint_add_ip(AllocatorV allocator, bigint *a, bigint b) {
  return bigint_add_ip_flag(allocator, a, b, 0);
}
void bigint_sub_ip(AllocatorV allocator, bigint *a, bigint b1) {
  bigint_add_ip_flag(allocator, a, b1, 1);
}
bigint bigint_from(AllocatorV allocator, i64 i) {
  var_ r = msList_init(allocator, bigint_unit, 1);
  if (sizeof(bigint_unit) >= sizeof(i64)) {
    msList_push(allocator, r, (bigint_unit)i);
  } else {
    usize count = sizeof(i) / sizeof(bigint_unit);
    msList_pushArr(allocator, r, *VLAP((bigint_unit *)&i, count));
  }
  bigint_trim(&r);
  return r;
}
bigint bigint_negate(AllocatorV allocator, bigint i) {
  var_ res = bigint_copy(allocator, i);
  bigint_negate_ip(allocator, &res);
  return res;
}
bigint bigint_add(AllocatorV allocator, bigint a, bigint b) {
  bigint res = bigint_copy(allocator, a);
  bigint_add_ip(allocator, &res, b);
  return res;
}
bigint bigint_sub(AllocatorV allocator, bigint a, bigint b) {
  bigint res = bigint_copy(allocator, a);
  bigint_sub_ip(allocator, &res, b);
  return res;
}

struct bigint_mul_t {
  bigint_unit result, carry;
};

struct bigint_mul_t bigint_mul_units(bigint_unit a, bigint_unit b) {
  typedef typeof(bigint_mul_units(0, 0)) rT;
  // multiplication results in a number at most twice the digits
  // figure out carry
  //  bottom half
  bigint_unit a0 = a & (((bigint_unit)1 << (sizeof(bigint_unit) * 4)) - 1);
  bigint_unit b0 = b & (((bigint_unit)1 << (sizeof(bigint_unit) * 4)) - 1);
  // top half
  bigint_unit a1 = a >> ((sizeof(bigint_unit) * 4));
  bigint_unit b1 = b >> ((sizeof(bigint_unit) * 4));

  bigint_unit carry = 0;
  bigint_unit result = 0;
  result += a0 * b0;
  carry += bigint_ckd_add(&result, result, (bigint_unit)(a1 * b0 << ((sizeof(bigint_unit) * 4))));
  carry += bigint_ckd_add(&result, result, (bigint_unit)(a0 * b1 << ((sizeof(bigint_unit) * 4))));
  carry += a1 * b1;
  carry += a1 * b0 >> ((sizeof(bigint_unit) * 4));
  carry += a0 * b1 >> ((sizeof(bigint_unit) * 4));

  return ((rT){
      .result = result,
      .carry = carry,
  });
}
void bigint_shrl(AllocatorV allocator, bigint *b, isize direction) {
  if (!bigint_cmp(b[0], NULL))
    return;
  if (direction < 0) {
    direction *= -1;
    if (direction > msList_len(b[0])) {
      msList_len(b[0]) = 0;
      return;
    }
    memmove(b[0], b[0] + direction, sizeof(b[0][0]) * (msList_len(b[0]) - direction));
    msList_len(b[0]) -= direction;
  } else if (direction == 0) {
    return;
  } else {
    msList_insArr(allocator, b[0], 0, *VLAP((bigint_unit *)NULL, direction));
  }
}
bigint bigint_mul_single(AllocatorV allocator, bigint *b, bigint_unit bu) {
  typedef typeof(bigint_mul_units(0, 0)) product;
  bigint res = msList_init(allocator, bigint_unit, msList_len(b[0]));
  bigint_unit carry = 0;
  for (each_RANGE(usize, i, 0, msList_len(b[0]))) {
    product p = bigint_mul_units(b[0][i], bu);
    var_ c2 = bigint_ckd_add_struct(p.result, carry);
    carry = c2.flag + p.carry;
    msList_push(allocator, res, c2.result);
  }
  msList_push(allocator, res, carry);
  return res;
}
bigint bigint_mul(AllocatorV allocator, bigint a1, bigint b1) {
  var_ arena = arena_new_ext(allocator, 1024);
  defer { arena_cleanup(arena); };
  if ((!bigint_cmp(a1, NULL)) || (!bigint_cmp(b1, 0)))
    return bigint_from(allocator, 0);
  bool negetive = 0;
  var_ a = bigint_negetive(a1)
               ? (negetive = !negetive, bigint_negate(arena, a1))
               : bigint_copy(arena, a1);
  var_ b = bigint_negetive(b1)
               ? (negetive = !negetive, bigint_negate(arena, b1))
               : bigint_copy(arena, b1);
  bigint_trim(&a);
  bigint_trim(&b);

  usize sh_a = 0, sh_b = 0;
  while (!a[sh_a])
    sh_a++;
  while (!b[sh_b])
    sh_b++;
  bigint_shrl(arena, &a, -sh_a);
  bigint_shrl(arena, &b, -sh_b);
  // println("\nshifted a : {} , b : {}", sh_a, sh_b);

  var_ partials = msList_init(arena, bigint, msList_len(b));
  for (each_RANGE(usize, i, 0, msList_len(b))) {
    var_ temp = bigint_mul_single(arena, &a, b[i]);
    msList_insArr(arena, temp, 0, *VLAP((bigint)NULL, i));
    msList_push(arena, partials, temp);
  }
  var_ res = bigint_from(allocator, 0);
  for_each_((var_ bi, msList_vla(partials)), {
    bigint_add_ip(allocator, &res, bi);
  });
  // while (msList_len(res) < msList_len(a) + msList_len(b))
  //   msList_push(allocator, res, 0);

  if (negetive)
    bigint_negate_ip(allocator, &res);

  bigint_trim(&res);
  bigint_shrl(allocator, &res, sh_a + sh_b);
  return res;
}
bigint_unit bigint_div_guess_under(bigint a, bigint b) {
  assertMessage(
      !bigint_negetive(a) &&
      !bigint_negetive(b)
  );
  bigint_trim(&b);
  bigint_trim(&a);
  if (!bigint_cmp(a, NULL)) { // a == 0
    return 0;
  }
  var_ cmp = bigint_cmp(a, b);
  if (cmp < 0) { // a < b
    return 0;
  } else if (cmp == 0) {
    return 1;
  } else if (msList_len(a) == 1) { //
    return bigint_get(a, 0) / bigint_get(b, 0);
  } else {
    bigint_unit a0 = bigint_get(a, 1);
    bigint_unit b0 = bigint_get(b, 1);
    bigint_unit a1 = bigint_get(a, 0);
    bigint_unit b1 = bigint_get(b, 0);

    int unit_bits = sizeof(bigint_unit) * 8;
    bigint_unit q = 0;
    bigint_unit rem_h = 0;
    bigint_unit rem_l = 0;

    for (int i = (unit_bits * 2); i > 0; i--) {
      rem_h = (rem_h << 1) | (rem_l >> (unit_bits - 1));
      rem_l <<= 1;

      bigint_unit bit = ((i - 1) >= unit_bits)
                            ? ((a0 >> ((i - 1) - unit_bits)) & 1)
                            : ((a1 >> (i - 1)) & 1);

      rem_l |= bit;

      if (rem_h > b0 || (rem_h == b0 && rem_l >= b1)) {
        if (rem_l < b1)
          rem_h -= 1;
        rem_l -= b1;
        rem_h -= b0;

        if ((i - 1) < unit_bits) {
          q |= ((bigint_unit)1 << (i - 1));
        }
      }
    }

    return q ? q - 1 : 0;
  }
}
struct bigint_div_t {
  bigint result;
  bigint remainder;
};
struct bigint_div_t
bigint_div(AllocatorV allocator, bigint a1, bigint b1) {
  typedef typeof(bigint_div(allocator, a1, b1)) div_result;
  var_ arena = arena_new_ext(allocator, 1024);
  defer { arena_cleanup(arena); };
  assertMessage(bigint_cmp(b1, NULL), "Division by zero");

  if ((!bigint_cmp(a1, NULL)))
    return (div_result){
        bigint_from(allocator, 0),
    };
  bool negetive = 0;
  var_ remainder = bigint_negetive(a1)
                       ? (negetive = !negetive, bigint_negate(arena, a1))
                       : bigint_copy(arena, a1);
  var_ b = bigint_negetive(b1)
               ? (negetive = !negetive, bigint_negate(arena, b1))
               : bigint_copy(arena, b1);
  bigint_trim(&remainder);
  bigint_trim(&b);
  var_ partials = msList_init(arena, bigint, msList_len(remainder) - msList_len(b) + 1);

  // r 00 10 00
  // b 00 10 10
  for (usize i = msList_len(remainder); i >= msList_len(b); i--) {
    var_ t = bigint_copy(arena, b);
    usize shift = i - msList_len(b);
    msList_insArr(arena, t, 0, *VLAP((bigint)NULL, shift));
    msList_push(arena, partials, t);
  }

  var_ result = msList_init(allocator, bigint_unit, msList_len(partials) + 1);
  // rs 00 00 00
  //
  // rm 00 10 00
  //
  // b0 10 10 00
  // b1 00 10 10
  var_ local = arena_new_ext(arena, 512);
  for (usize i = 0; i < msList_len(partials); i++) {
    defer { arena_clear(local); };

    bigint temp = bigint_copy(local, partials[i]);
    // bigint_unit bu = 0;
    bigint_unit bu = bigint_div_guess_under(remainder, temp);

    bigint s = bigint_mul_single(local, &temp, bu);
    while (1) {
      bigint next_s = bigint_mul_single(local, &temp, bu + 1);
      if (bigint_cmp(next_s, remainder) > 0)
        break;
      s = next_s;
      bu++;
    }

    msList_ins(allocator, result, 0, bu);
    bigint_sub_ip(arena, &remainder, s);
  }
  if (negetive) {
    bigint_negate_ip(allocator, &result);
    bigint_negate_ip(allocator, &remainder);
  }
  return (div_result){
      .result = result,
      .remainder = bigint_copy(allocator, remainder),
  };
}

NAMESPACE_STRUCT(
    BInt,
    (add, &bigint_add),
    (sub, &bigint_sub),
    (mul, &bigint_mul),
    (div, &bigint_div),
    (from, &bigint_from),
    (negate, &bigint_negate),
    (negetive, &bigint_negetive),
);
REGISTER_PRINTER(bigint, {
  bool debug = P$(
      args,
      printer_arg_trim($),
      fptr_eq($, fp("debug")) | fptr_eq($, fp("dbg"))
  );

  if (debug) {
    PUTS("[");
    for (usize i = in ? msList_len(in) : 0; i > 0; i--)
      for (usize j = sizeof(*in); j > 0; j--)
        USENAMEDPRINTER("x", ((u8 *)(in + (i - 1)))[j - 1]);
    PUTS("]");
  } else {
    var_ arena = arena_new_ext(stdAlloc, 512); // heap operations for printing
    defer { arena_cleanup(arena); };
    in = bigint_negetive(in)
             ? (PUTS("-"), bigint_negate(arena, in))
             : bigint_copy(arena, in);
    bigint ten = bigint_from(arena, 10);
    var_ digits = msList_init(arena, c8, 10);

    while (bigint_cmp(in, NULL)) {
      var_ dig_big = bigint_div(arena, in, ten);
      var_ dig = bigint_get(dig_big.remainder, 0);
      msList_ins(arena, digits, 0, dig + '0');
      in = dig_big.result;
    }
    put(digits, _arb, msList_len(digits), 0);
  }
});

int main(void) {
  AllocatorV debug = debugAllocator(
      allocator = stdAlloc,
      log = stdout
  );
  defer { debugAllocatorDeInit(debug); };
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from(debug, 1024);
    defer { msList_deInit(debug, a); };
    bigint b = BInt.negate(debug, a);
    defer { msList_deInit(debug, b); };
    println(
        "{bigint:dbg}{bigint} * -1 = {bigint:dbg}{bigint}",
        a, a, b, b
    );
  }
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from(debug, -10);
    defer { msList_deInit(debug, a); };
    bigint b = BInt.from(debug, -252);
    defer { msList_deInit(debug, b); };
    bigint c = BInt.sub(debug, a, b);
    defer { msList_deInit(debug, c); };
    println(
        "{bigint} - {bigint} = {bigint}",
        a, b, c
    );
  }
  {
    // defer {
    //   debugAllocatorDeInit(debug);
    //   debug = debugAllocator(allocator = stdAlloc, log = stdout);
    // };
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from(debug, -(2 << 2));
    defer { msList_deInit(debug, a); };
    bigint b = BInt.from(debug, -(2 << 8));
    defer { msList_deInit(debug, b); };
    bigint c = BInt.mul(debug, a, b);
    println(
        "{bigint} * {bigint} = {bigint}",
        a, b, c
    );
  }
  {
    defer { println("{dbga-stats}", debugAllocator_clear(debug)); };
    bigint a = BInt.from(debug, -(2 << 8));
    defer { msList_deInit(debug, a); };
    bigint b = BInt.from(debug, -(2 << 2));
    defer { msList_deInit(debug, b); };
    var_ q = BInt.div(debug, a, b);
    defer {
      msList_deInit(debug, q.remainder);
      msList_deInit(debug, q.result);
    };
    println(
        "{bigint} // {bigint} = {bigint} , {bigint}",
        a, b, q.result, q.remainder
    );
  }
}
