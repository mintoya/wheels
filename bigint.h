#if !defined(MY_BIGINT_H)
  #define MY_BIGINT_H (1)

  #include "allocators/fbafallbackAllocator.h"
  #include "fptr.h"
  #include "macros.h"
  #include "mytypes.h"
  #include "print/print_pre.h"
  #include "sList.h"

typedef unsigned int bigint_unit;
typedef msList(bigint_unit) bigint;
typedef ptrstype(bigint) bigint_unit;

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
void bigint_expand(allocfn allocator, bigint *b, usize len);
bigint bigint_copy(allocfn allocator, bigint b);

void bigint_negate_ip(allocfn allocator, bigint *i);

void bigint_add_ip_flag(allocfn allocator, bigint *a, bigint b, bool negate, isize shift);
void bigint_add_ip(allocfn allocator, bigint *a, bigint b, isize shift);
void bigint_sub_ip(allocfn allocator, bigint *a, bigint b, isize shift);

bigint bigint_from(allocfn allocator, i64 i);
bigint bigint_fromBits(allocfn alloc, void *ptr, usize bitcount, bool signmask);
bigint bigint_cs(allocfn allocator, u8 base, cstr str);
bigint bigint_fptr(allocfn allocator, u8 base, fptr str);
bigint bigint_negate(allocfn allocator, bigint i);
bigint bigint_add(allocfn allocator, bigint a, bigint b);
bigint bigint_sub(allocfn allocator, bigint a, bigint b);

struct bigint_mul_t {
  bigint_unit result, carry;
};

struct bigint_mul_t bigint_mul_units(bigint_unit a, bigint_unit b);
void bigint_shrl(allocfn allocator, bigint *b, isize direction);
bigint bigint_mul_single(allocfn allocator, bigint *b, bigint_unit bu);
bigint bigint_mul(allocfn allocator, bigint a1, bigint b1);
struct bigint_div_t {
  bigint div;
  bigint mod;
};

bigint_unit bigint_estimate_q(bigint rem, bigint b);
struct bigint_div_t bigint_div(allocfn allocator, bigint a1, bigint b1);

NAMESPACE_STRUCT(
    BInt_from,
    (cstr, &bigint_cs),
    (fptr, &bigint_fptr),
    (i64, &bigint_from),
    (bits, &bigint_fromBits),
);
NAMESPACE_STRUCT(
    BInt_advanced,
    (add_ip, &bigint_add_ip),
    (sub_ip, &bigint_sub_ip),
);
NAMESPACE_STRUCT(
    BInt,
    (advanced, BInt_advanced),
    (cmp, &bigint_cmp),
    (add, &bigint_add),
    (sub, &bigint_sub),
    (mul, &bigint_mul),
    (div, &bigint_div),
    (trim, &bigint_trim),
    (expand, &bigint_expand),
    (from, BInt_from),
    (negate, &bigint_negate),
    (negetive, &bigint_negetive),
);

typePrinter(bigint) {
  let args = PRINTARGS();
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
    let allocator = PRINTARGS_ALLOCATOR();
    in = bigint_negetive(in)
             ? (PUTS("-"), bigint_negate(allocator, in))
             : bigint_copy(allocator, in);
    defer { msList_deInit(allocator, in); };

    let base = ({
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

    let hb = msList_stackBuffer(bigint_unit[2]);
    let hundred = msList_initBuffer(hb);
    defer { msList_deInit(allocator, hundred); };
    msList_push(allocator, hundred, base.modu);

    if (bigint_cmp(in, NULL)) {
      msList(c8) digits = msList_init(allocator, c8);
      defer { msList_deInit(allocator, digits); };

      while (bigint_cmp(in, NULL)) {
        let dig_big = bigint_div(allocator, in, (bigint)(hundred));
        defer {
          msList_deInit(allocator, dig_big.mod);
          msList_deInit(allocator, dig_big.div);
        };
        let dig = bigint_get(dig_big.mod, 0);
        bool is_last = !bigint_cmp(dig_big.div, NULL);
        if (is_last)
          while (dig > 0) {
            msList_push(allocator, digits, (c8)(dig % 10 + '0'));
            dig /= 10;
          }
        else
          for (usize i = 0; i < base.digits; i++) {
            msList_push(allocator, digits, (c8)(dig % 10 + '0'));
            dig /= 10;
          }

        msList_header(in)->length = msList_header(dig_big.div)->length;
        memcpy(in, dig_big.div, sizeof(*msList_vla(dig_big.div)));
      }

      usize len = msList_len(digits);
      c8 *str = *msList_vla(digits);
      for (usize i = 0; i < len / 2; i++) {
        c8 tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
      }

      msList_push(allocator, digits, 0);
      PUTS(*msList_vla(digits));
    } else
      PUTS("0");
  }
}

test_fn(bigint_bits) {
  let a = BInt.from.bits(allocator, (u32[]){(u32)1 << 31}, 32, 0);
  defer { msList_deInit(allocator, a); };
  let b = BInt.from.bits(allocator, (u32[]){(u32)1 << 31}, 32, 1);
  defer { msList_deInit(allocator, b); };
  test_assert(BInt.cmp(a, b) > 0);
  let c = bigint_add(allocator, a, b);
  defer { msList_deInit(allocator, c); };
  test_assert(!BInt.cmp(nullptr, c));
}
test_fn(bigint_multiplication) {
  let a = BInt.from.cstr(allocator, 10, "-1 000 000 000 000");
  defer { msList_deInit(allocator, a); };
  let b = BInt.from.cstr(allocator, 10, "1 000 000 000 000");
  defer { msList_deInit(allocator, b); };
  let c = BInt.mul(allocator, a, b);
  defer { msList_deInit(allocator, c); };
  let str = snprint(allocator, "{bigint}{c8}", c, (c8)0);
  defer { slice_free(allocator, str); };
  test_streq(str.ptr, "-1000000000000000000000000");
}
test_fn(bigint_addition_subtraction) {
  let a = BInt.from.cstr(allocator, 10, "999 999 999");
  defer { msList_deInit(allocator, a); };
  let b = BInt.from.cstr(allocator, 10, "2");
  defer { msList_deInit(allocator, b); };

  let sum = bigint_add(allocator, a, b);
  defer { msList_deInit(allocator, sum); };
  let sum_str = snprint(allocator, "{bigint}{c8}", sum, (c8)0);
  defer { slice_free(allocator, sum_str); };
  test_streq(sum_str.ptr, "1000000001");

  let diff = bigint_sub(allocator, b, a);
  defer { msList_deInit(allocator, diff); };
  let diff_str = snprint(allocator, "{bigint}{c8}", diff, (c8)0);
  defer { slice_free(allocator, diff_str); };
  test_streq(diff_str.ptr, "-999999997");
}
test_fn(bigint_division_and_modulo) {
  let a = BInt.from.cstr(allocator, 10, "-1 000 000 000");
  defer { msList_deInit(allocator, a); };
  let b = BInt.from.cstr(allocator, 10, "3");
  defer { msList_deInit(allocator, b); };

  let res = bigint_div(allocator, a, b);
  defer {
    msList_deInit(allocator, res.div);
    msList_deInit(allocator, res.mod);
  };

  let div_str = snprint(allocator, "{bigint}{c8}", res.div, (c8)0);
  defer { slice_free(allocator, div_str); };
  test_streq(div_str.ptr, "-333333333");

  let mod_str = snprint(allocator, "{bigint}{c8}", res.mod, (c8)0);
  defer { slice_free(allocator, mod_str); };
  test_streq(mod_str.ptr, "-1");
}
test_fn(bigint_base_16_parsing) {
  let a = BInt.from.cstr(allocator, 16, "ff ff ff ff");
  defer { msList_deInit(allocator, a); };

  let a_str = snprint(allocator, "{bigint}{c8}", a, (c8)0);
  defer { slice_free(allocator, a_str); };
  test_streq(a_str.ptr, "4294967295");
}
test_fn(bigint_comparison) {
  let a = BInt.from.cstr(allocator, 10, "42");
  defer { msList_deInit(allocator, a); };
  let b = BInt.from.cstr(allocator, 10, "42");
  defer { msList_deInit(allocator, b); };
  let c = BInt.from.cstr(allocator, 10, "-42");
  defer { msList_deInit(allocator, c); };

  test_assert(!BInt.cmp(a, b));
  test_assert(BInt.cmp(a, c) > 0);
  test_assert(BInt.cmp(c, a) < 0);

  let c_negated = bigint_negate(allocator, c);
  defer { msList_deInit(allocator, c_negated); };
  test_assert(!BInt.cmp(a, c_negated));
}
#endif

#if (defined MY_BIGINT_C && MY_BIGINT_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef MY_BIGINT_C
  #define MY_BIGINT_C (2)

  #include "allocators/arenaAllocator.h"

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
  if (!i) return 0;
  if (!msList_len(i)) return 0;
  return !!(i[msList_len(i) - 1] & ((((bigint_unit)-1) >> 1) + 1));
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
i8 bigint_cmp_sh(bigint a, bigint b, isize sha, isize shb) {
  i8 neg_a = bigint_negetive(a);

  if (neg_a != bigint_negetive(b))
    return neg_a ? -1 : 1;

  i8 sc = neg_a ? -1 : 1;

  isize top = MAX$(
      bigint_digits(a) + sha,
      bigint_digits(b) + shb
  );
  top = top ? top - 1 : 0;
  isize bot = MIN$(sha, shb);

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
void bigint_expand(allocfn allocator, bigint *b, usize len) {
  assertMessage(len >= msList_len(*b), "%zu < %zu", len, msList_len(*b));
  bigint_unit u = 0;
  if (bigint_negetive(b[0]))
    u = (bigint_unit)-1;
  while (len > msList_len(*b))
    msList_push(allocator, *b, u);
}
bigint bigint_copy(allocfn allocator, bigint b) {
  let r = msList_init(allocator, bigint_unit, b ? msList_len(b) : 1);
  if (b)
    msList_pushArr(allocator, r, *msList_vla(b));
  return r;
}

void bigint_negate_ip(allocfn allocator, bigint *i) {
  assertMessage(i);
  if (!*i)
    return;
  bigint_expand(allocator, i, msList_len(*i) + 1);
  foreach (usize j, span(0, msList_len(*i)))
    i[0][j] = ~i[0][j];
  bigint_unit ca = 1;
  for (usize c = 0; ca && c < msList_len(i[0]); c++) {
    let c1 = bigint_ckd_add_struct(ca, i[0][c]);
    i[0][c] = c1.result;
    ca = c1.flag;
  }
  bigint_trim(i);
}
void bigint_add_ip_flag(allocfn allocator, bigint *a, bigint b, bool negate, isize shift) {
  isize b_len_signed = (isize)msList_len(b) + shift;
  usize b_len = b_len_signed > 0 ? (usize)b_len_signed : 0;
  usize len = msList_len(a[0]) > b_len
                  ? msList_len(a[0])
                  : b_len;
  len += 1;
  bigint_expand(allocator, a, len);
  bigint_unit carry = 0;

  for (usize i = 0; i < len; i++) {
    let ra = bigint_ckd_add_struct(negate ? ~a[0][i] : a[0][i], carry);
    let rb = bigint_ckd_add_struct(ra.result, bigint_get(b, i - shift));
    a[0][i] = negate ? ~rb.result : rb.result;
    carry = ra.flag + rb.flag;
  }
  msList_len(a[0]) = len;
  bigint_trim(a);
}

void bigint_add_ip(allocfn allocator, bigint *a, bigint b, isize shift) {
  return bigint_add_ip_flag(allocator, a, b, 0, shift);
}
void bigint_sub_ip(allocfn allocator, bigint *a, bigint b, isize shift) {
  return bigint_add_ip_flag(allocator, a, b, 1, shift);
}
bigint bigint_from(allocfn allocator, i64 i) {
  let r = msList_init(allocator, bigint_unit, 1);
  if (sizeof(bigint_unit) >= sizeof(i64)) {
    msList_push(allocator, r, (bigint_unit)i);
  } else {
    usize count = sizeof(i) / sizeof(bigint_unit);
    msList_pushArr(allocator, r, *VLAP((bigint_unit *)&i, count));
  }
  bigint_trim(&r);
  return r;
}
bigint bigint_negate(allocfn allocator, bigint i) {
  let res = bigint_copy(allocator, i);
  bigint_negate_ip(allocator, &res);
  return res;
}
bigint bigint_add(allocfn allocator, bigint a, bigint b) {
  bigint res = bigint_copy(allocator, a);
  bigint_add_ip(allocator, &res, b, 0);
  return res;
}
bigint bigint_sub(allocfn allocator, bigint a, bigint b) {
  bigint res = bigint_copy(allocator, a);
  bigint_sub_ip(allocator, &res, b, 0);
  return res;
}

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
void bigint_shrl(allocfn allocator, bigint *b, isize direction) {
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
bigint bigint_mul_single(allocfn allocator, bigint *b, bigint_unit bu) {
  typedef typeof(bigint_mul_units(0, 0)) product;
  bigint res = msList_init(allocator, bigint_unit, msList_len(b[0]));
  bigint_unit carry = 0;
  foreach (usize i, range(0, msList_len(b[0]))) {
    product p = bigint_mul_units(b[0][i], bu);
    let c2 = bigint_ckd_add_struct(p.result, carry);
    carry = c2.flag + p.carry;
    msList_push(allocator, res, c2.result);
  }
  msList_push(allocator, res, carry);
  return res;
}
bigint bigint_mul(allocfn allocator, bigint a1, bigint b1) {
  if ((!bigint_cmp(a1, NULL)) || (!bigint_cmp(b1, NULL)))
    return bigint_from(allocator, 0);
  bool negetive = 0;
  let a = bigint_negetive(a1)
               ? (negetive = !negetive, bigint_negate(allocator, a1))
               : bigint_copy(allocator, a1);
  let b = bigint_negetive(b1)
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

  let res = bigint_from(allocator, 0);

  foreach (usize i, range(0, msList_len(b))) {
    let temp = bigint_mul_single(allocator, &a, bigint_get(b, i + sh_b));
    defer { msList_deInit(allocator, temp); };
    bigint_add_ip(allocator, &res, temp, i);
  }

  if (negetive)
    bigint_negate_ip(allocator, &res);

  bigint_trim(&res);
  bigint_shrl(allocator, &res, sh_a + sh_b);
  return res;
}


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
struct bigint_div_t bigint_div(allocfn allocator, bigint a1, bigint b1) {
  assertMessage(bigint_cmp(b1, NULL), "division by zero");

  let buff = (fbafb_buffer(bigint_unit[20])){};
  let act = (struct {allocfn a ; usize s; }){allocator, ((a1 ? msList_len(a1) : 1) + (b1 ? msList_len(b1) : 1)) * sizeof(bigint_unit) * 16};
  let arena = fbafb_initBuffer(
      buff,
      &act,
      initarena,
      deinitarena
  );
  defer { fbafb_deinit(arena); };

  bool neg_a = bigint_negetive(a1);
  bool neg_b = bigint_negetive(b1);

  let a = neg_a ? bigint_negate(arena, a1) : bigint_copy(arena, a1);
  let b = neg_b ? bigint_negate(arena, b1) : bigint_copy(arena, b1);
  bigint_trim(&a);
  bigint_trim(&b);

  let quot = bigint_from(allocator, 0);
  let rem = bigint_from(allocator, 0);

  isize len_a = (isize)bigint_digits(a);
  foreach (isize i, range(len_a - 1, -1)) {
    bigint_shrl(allocator, &rem, 1);
    rem[0] = a[i];
    bigint_trim(&rem);

    bigint_unit q = bigint_estimate_q(rem, b);

    let bq = bigint_mul_single(arena, &b, q);

    while (bigint_cmp(bq, rem) > 0) {
      q--;
      let bq_new = bigint_sub(arena, bq, b);
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
bigint bigint_cs(allocfn allocator, const u8 base, char *str) {
  return bigint_fptr(allocator, base, fp(str));
}
bigint bigint_fptr(allocfn allocator, const u8 base, fptr str) {
  assertMessage(base <= 32);

  bool negetive = str.len > 0 && str.ptr[0] == '-';
  str = negetive ? slice_split(str, (1, -1))[0] : str;
  bigint b = bigint_from(allocator, 0);

  let sb = msList_stackBuffer(bigint_unit[1]);
  bigint add = msList_initBuffer(sb);

  while (str.len) {
    u8 nm = 0;
    bool skip = 0;
    switch (*str.ptr) {
      case '0' ... '9': {
        nm = (*str.ptr) - '0';
      } break;
      case 'a' ... 'z': {
        nm = (*str.ptr) - 'a' + 10;
      } break;
      case ' ':
      case '\'': // delimiters
        skip = 1;
        break;
      default:
        assertMessage(false, "character not supported for conversion: %c", *str.ptr);
    }

    if (!skip) {
      assertMessage(nm < base, "char %c out of range for base %i", *str.ptr, (int)base);
      let prod = bigint_mul_single(allocator, &b, base);
      add[0] = nm;
      msList_len(add) = 1;
      bigint_add_ip(allocator, &prod, add, 0);

      msList_deInit(allocator, b);

      b = prod;
    }
    str = slice_split(str, (1, -1))[0];
  }
  if (negetive)
    bigint_negate_ip(allocator, &b);
  return b;
}
// create a bigint from a specified bitcount
// assumes that a byte is 8 bits
bigint bigint_fromBits(allocfn alloc, void *ptr, const usize bitcount, bool sigmask) {
  const usize unit_bits = sizeof(bigint_unit) * 8;
  const usize units = (bitcount + unit_bits - 1) / unit_bits;

  let res = msList_init(alloc, bigint_unit, units ?: 1);
  if (!units) return res;

  memset(res, 0, units * sizeof(bigint_unit));

  usize full_bytes = bitcount / 8;
  usize rem_bits = bitcount % 8;

  memcpy(res, ptr, full_bytes);

  if (rem_bits)
    ((u8 *)res)[full_bytes] = ((u8 *)ptr)[full_bytes] & ((1u << rem_bits) - 1);

  if (sigmask && (((u8 *)ptr)[(bitcount - 1) / 8] & (1u << ((bitcount - 1) % 8)))) {
    usize mod = bitcount % unit_bits;
    if (mod)
      res[units - 1] |= ~(((bigint_unit)1 << mod) - 1);
  }

  msList_len(res) = units;
  if (!sigmask) msList_push(alloc, res, {});
  return res;
}
#endif
