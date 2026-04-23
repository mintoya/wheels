#if !defined(MY_BIGINT_H)
  #define MY_BIGINT_H (1)
  #include "print.h"
  #include "wheels/arenaAllocator.h"
  #include "wheels/macros.h"
  #include "wheels/mytypes.h"
  #include "wheels/sList.h"

typedef u8 bigint_unit;
typedef msList(bigint_unit) bigint;
typedef typeof(*((bigint)NULL)) bigint_unit;

bool bigint_ckd_add(bigint_unit *res, bigint_unit a, bigint_unit b);
struct bigint_ckdt {
  bigint_unit result;
  bool flag;
};
struct bigint_ckdt bigint_ckd_add_struct(bigint_unit a, bigint_unit b);
bool bigint_negetive(bigint i);
bigint_unit bigint_get(bigint b, usize idx);
usize bigint_digits(bigint b);
i8 bigint_cmp_sh(bigint a, bigint b, isize sha, isize shb);
i8 bigint_cmp(bigint a, bigint b);
void bigint_trim(bigint *b);
void bigint_expand(AllocatorV allocator, bigint *b, usize len);
bigint bigint_copy(AllocatorV allocator, bigint b);

void bigint_negate_ip(AllocatorV allocator, bigint *i);
void bigint_add_ip_flag(AllocatorV allocator, bigint *a, bigint b, bool negate, isize shift);

void bigint_add_ip(AllocatorV allocator, bigint *a, bigint b, isize shift);

void bigint_sub_ip(AllocatorV allocator, bigint *a, bigint b, isize shift);
bigint bigint_from(AllocatorV allocator, i64 i);
bigint bigint_negate(AllocatorV allocator, bigint i);
bigint bigint_add(AllocatorV allocator, bigint a, bigint b);
bigint bigint_sub(AllocatorV allocator, bigint a, bigint b);

struct bigint_mul_t {
  bigint_unit result, carry;
};

struct bigint_mul_t bigint_mul_units(bigint_unit a, bigint_unit b);
void bigint_shrl(AllocatorV allocator, bigint *b, isize direction);
bigint bigint_mul_single(AllocatorV allocator, bigint *b, bigint_unit bu);
bigint bigint_mul(AllocatorV allocator, bigint a1, bigint b1);
struct bigint_div_t {
  bigint div;
  bigint mod;
};

bigint_unit bigint_estimate_q(bigint rem, bigint b);
struct bigint_div_t bigint_div(AllocatorV allocator, bigint a1, bigint b1);

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
  args = printer_arg_trim(args);
  bool debug = fptr_eq(args, fp("dbg"));
  bool normal = !debug;
  normal |= fptr_eq(args, fp("both"));
  debug |= fptr_eq(args, fp("both"));

  if (debug) {
    PUTS("[");
    for (usize i = in ? msList_len(in) : 0; i > 0; i--)
      for (usize j = sizeof(*in); j > 0; j--)
        USENAMEDPRINTER("x", ((u8 *)(in + (i - 1)))[j - 1]);
    PUTS("]");
  }
  if (normal) {

    in = bigint_negetive(in)
             ? (PUTS("-"), bigint_negate(stdAlloc, in))
             : bigint_copy(stdAlloc, in);
    defer { msList_deInit(stdAlloc, in); };

    var_ base = ({
      usize dcount = 1;
      bigint_unit u = 10;
      bigint_unit max_unit = (bigint_unit)-1;
      while (max_unit / 10 >= u) {
        u *= 10;
        dcount++;
      }
      struct {
        usize digits;
        bigint_unit modu;
      } r = {
          .digits = dcount,
          .modu = u,
      };
      r;
    });

    struct {
      sList_header h[1];
      bigint_unit u[2];
    } hundred = {{2, 2}, {base.modu, 0}};

    if (bigint_cmp(in, NULL)) {
      msList(c8) digits = msList_init(stdAlloc, c8);
      defer { msList_deInit(stdAlloc, digits); };

      while (bigint_cmp(in, NULL)) {
        var_ dig_big = bigint_div(stdAlloc, in, (bigint)(hundred.h->buf));
        defer {
          msList_deInit(stdAlloc, dig_big.mod);
          msList_deInit(stdAlloc, dig_big.div);
        };
        var_ dig = bigint_get(dig_big.mod, 0);

        bool is_last = !bigint_cmp(dig_big.div, NULL);

        if (is_last) {
          while (dig > 0) {
            msList_ins(stdAlloc, digits, 0, (c8)(dig % 10 + '0'));
            dig /= 10;
          }
        } else {
          for (usize i = 0; i < base.digits; i++) {
            msList_ins(stdAlloc, digits, 0, (c8)(dig % 10 + '0'));
            dig /= 10;
          }
        }

        msList_header(in)->length = msList_header(dig_big.div)->length;
        memcpy(in, dig_big.div, sizeof(*msList_vla(dig_big.div)));
      }

      msList_push(stdAlloc, digits, 0);
      PUTS(*msList_vla(digits));
    } else
      PUTS("0");
  }
});
#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_BIGINT_C (1)
#endif
#if defined(MY_BIGINT_C)
bool bigint_ckd_add(bigint_unit *res, bigint_unit a, bigint_unit b) {
  static_assert((bigint_unit) ~(bigint_unit)0 > 0, "must be unsigned ");
  *res = a + b;
  return a > (~(bigint_unit)0) - b;
}
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
usize bigint_digits(bigint b) {
  return b ? msList_len(b) : 0;
}
  #pragma push_macro("max")
  #pragma push_macro("min")
  #define max(a, b) (((a) > (b)) ? (a) : (b))
  #define min(a, b) (((a) < (b)) ? (a) : (b))
i8 bigint_cmp_sh(bigint a, bigint b, isize sha, isize shb) {
  i8 neg_a = bigint_negetive(a);

  if (neg_a != bigint_negetive(b))
    return neg_a ? -1 : 1;

  i8 sc = neg_a ? -1 : 1;

  isize top = max(
      bigint_digits(a) + sha,
      bigint_digits(b) + shb
  );
  top = top ? top - 1 : 0;
  isize bot = min(sha, shb);

  for (isize i = top; i >= bot; --i) {
    bigint_unit au = bigint_get(a, i - sha);
    bigint_unit bu = bigint_get(b, i - shb);

    if (au != bu)
      return (au > bu) ? sc : -sc;
  }

  return 0;
}
i8 bigint_cmp(bigint a, bigint b) {
  return bigint_cmp_sh(a, b, 0, 0);
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

void bigint_negate_ip(AllocatorV allocator, bigint *i) {
  assertMessage(i);
  if (!*i)
    return;
  bigint_expand(allocator, i, msList_len(*i) + 1);
  for_each_P((var_ j, msList_vla(*i)), { *j = ~*j; });
  bigint_unit ca = 1;
  for (usize c = 0; ca && c < msList_len(i[0]); c++) {
    var_ c1 = bigint_ckd_add_struct(ca, i[0][c]);
    i[0][c] = c1.result;
    ca = c1.flag;
  }
  bigint_trim(i);
}
void bigint_add_ip_flag(AllocatorV allocator, bigint *a, bigint b, bool negate, isize shift) {
  usize len = msList_len(a[0]) > msList_len(b)
                  ? msList_len(a[0])
                  : msList_len(b);
  len += 1;
  bigint_expand(allocator, a, len);
  bigint_unit carry = 0;

  for (usize i = 0; i < len; i++) {
    var_ ra = bigint_ckd_add_struct(negate ? ~a[0][i] : a[0][i], carry);
    var_ rb = bigint_ckd_add_struct(ra.result, bigint_get(b, i + shift));
    a[0][i] = negate ? ~rb.result : rb.result;
    carry = ra.flag + rb.flag;
  }
  msList_len(a[0]) = len;
  bigint_trim(a);
}

void bigint_add_ip(AllocatorV allocator, bigint *a, bigint b, isize shift) {
  return bigint_add_ip_flag(allocator, a, b, 0, shift);
}
void bigint_sub_ip(AllocatorV allocator, bigint *a, bigint b, isize shift) {
  return bigint_add_ip_flag(allocator, a, b, 1, shift);
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
  bigint_add_ip(allocator, &res, b, 0);
  return res;
}
bigint bigint_sub(AllocatorV allocator, bigint a, bigint b) {
  bigint res = bigint_copy(allocator, a);
  bigint_sub_ip(allocator, &res, b, 0);
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
  if ((!bigint_cmp(a1, NULL)) || (!bigint_cmp(b1, NULL)))
    return bigint_from(allocator, 0);
  bool negetive = 0;
  var_ a = bigint_negetive(a1)
               ? (negetive = !negetive, bigint_negate(allocator, a1))
               : bigint_copy(allocator, a1);
  var_ b = bigint_negetive(b1)
               ? (negetive = !negetive, bigint_negate(allocator, b1))
               : bigint_copy(allocator, b1);
  defer { msList_deInit(allocator, a); };
  defer { msList_deInit(allocator, b); };
  bigint_trim(&a);
  bigint_trim(&b);

  usize sh_a = 0, sh_b = 0;
  while (!a[sh_a])
    sh_a++;
  while (!b[sh_b])
    sh_b++;
  bigint_shrl(allocator, &a, -sh_a);

  var_ res = bigint_from(allocator, 0);

  {

    for (each_RANGE(usize, i, 0, msList_len(b))) {
      var_ temp = bigint_mul_single(allocator, &a, bigint_get(b, i + sh_b));
      defer { msList_deInit(allocator, temp); };
      bigint_add_ip(allocator, &res, temp, i);
    }
  }

  if (negetive)
    bigint_negate_ip(allocator, &res);

  bigint_trim(&res);
  bigint_shrl(allocator, &res, sh_a + sh_b);
  return res;
}
struct bigint_div_t {
  bigint div;
  bigint mod;
};

bigint_unit bigint_estimate_q(bigint rem, bigint b) {
  typedef unsigned _BitInt(sizeof(bigint_unit) * 16) double_unit;
  usize len_b = bigint_digits(b);
  if (len_b == 0)
    return 0;

  usize top_idx = len_b - 1;

  bigint_unit d1 = bigint_get(b, top_idx);
  bigint_unit d0 = top_idx > 0 ? bigint_get(b, top_idx - 1) : 0;

  bigint_unit r1 = bigint_get(rem, top_idx + 1);
  bigint_unit r0 = bigint_get(rem, top_idx);

  if (r1 == d1) {
    return ~(bigint_unit)0;
  }

  double_unit top_rem = ((double_unit)r1 << (sizeof(bigint_unit) * 8)) | r0;

  double_unit q_guess = top_rem / d1;
  double_unit r_guess = top_rem % d1;

  while ((q_guess * d0) > ((r_guess << (sizeof(bigint_unit) * 8)) | 0)) {
    q_guess--;
    r_guess += d1;
    if (r_guess > (bigint_unit) ~(bigint_unit)0)
      break;
  }

  return (bigint_unit)q_guess;
}
struct bigint_div_t bigint_div(AllocatorV allocator, bigint a1, bigint b1) {
  assertMessage(bigint_cmp(b1, NULL));

  var_ arena = arena_new_ext(allocator, ((a1 ? msList_len(a1) : 1) + (b1 ? msList_len(b1) : 1)) * sizeof(bigint_unit) * 16);
  defer { arena_cleanup(arena); };

  bool neg_a = bigint_negetive(a1);
  bool neg_b = bigint_negetive(b1);

  var_ a = neg_a ? bigint_negate(arena, a1) : bigint_copy(arena, a1);
  var_ b = neg_b ? bigint_negate(arena, b1) : bigint_copy(arena, b1);
  bigint_trim(&a);
  bigint_trim(&b);

  var_ quot = bigint_from(allocator, 0);
  var_ rem = bigint_from(allocator, 0);

  isize len_a = (isize)bigint_digits(a);
  for (isize i = len_a - 1; i >= 0; --i) {
    bigint_shrl(allocator, &rem, 1);
    rem[0] = a[i];
    bigint_trim(&rem);

    bigint_unit q = bigint_estimate_q(rem, b);

    var_ bq = bigint_mul_single(arena, &b, q);

    while (bigint_cmp(bq, rem) > 0) {
      q--;
      var_ bq_new = bigint_sub(arena, bq, b);
      msList_deInit(arena, bq);
      bq = bq_new;
    }

    bigint_sub_ip(allocator, &rem, bq, 0);
    msList_deInit(arena, bq);
    bigint_trim(&rem);

    bigint_shrl(allocator, &quot, 1);
    quot[0] = q;
    bigint_trim(&quot);
  }

  if (neg_a != neg_b)
    bigint_negate_ip(allocator, &quot);
  if (neg_a)
    bigint_negate_ip(allocator, &rem);

  bigint_trim(&quot);
  bigint_trim(&rem);

  return (struct bigint_div_t){.div = quot, .mod = rem};
}
#endif
