
#include "stdckdint.h"
#include "wheels/arenaAllocator.h"
#include "wheels/debugallocator.h"
#include "wheels/fptr.h"
#include "wheels/macros.h"
#include "wheels/mytypes.h"
#include "wheels/print.h"
#include "wheels/wheels.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <threads.h>

#if !defined(BIGINT_BITS)
  #define BIGINT_BITS 8
#endif
typedef unsigned _BitInt(BIGINT_BITS) bigint_unit;
// typedef u8 bigint_unit;
typedef msList(bigint_unit) bigint;
typedef typeof(*((bigint)NULL)) bigint_unit;

bool bigint_ckd_add(bigint_unit *res, bigint_unit a, bigint_unit b) {
  static_assert((bigint_unit) ~(bigint_unit)0 > 0, "must be unsigned ");
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
void bigint_sub_ip(AllocatorV allocator, bigint *a, bigint b1) {
  bigint_add_ip_flag(allocator, a, b1, 1, 0);
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
  var_ arena = arena_new_ext(allocator, (b1 ? msList_len(b1) : 1) * (b1 ? msList_len(b1) : 1) * sizeof(bigint) * 8);
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
  // bigint_shrl(arena, &b, -sh_b);

  var_ res = bigint_from(allocator, 0);
  for (each_RANGE(usize, i, 0, msList_len(b))) {
    var_ temp = bigint_mul_single(arena, &a, bigint_get(b, i + sh_b));
    bigint_add_ip(allocator, &res, temp, i);
    msList_deInit(arena, temp);
  }
  // while (msList_len(res) < msList_len(a) + msList_len(b))
  //   msList_push(allocator, res, 0);

  if (negetive)
    bigint_negate_ip(allocator, &res);

  bigint_trim(&res);
  bigint_shrl(allocator, &res, sh_a + sh_b);
  return res;
}
struct bigint_div_t {
  bigint result;
  bigint remainder;
};

// Knuth Algorithm D — operates on bigint directly, little-endian words
// u is modified in-place to become the remainder after the call

static int bigint_nlz(bigint_unit x) {
  int n = 0;
  int bits = sizeof(bigint_unit) * 8;
  if (x == 0)
    return bits;
  bigint_unit msb = (bigint_unit)1 << (bits - 1);
  while (!(x & msb)) {
    n++;
    x <<= 1;
  }
  return n;
}

static void knuth_divmod(
    AllocatorV arena,
    bigint u, // dividend in, remainder out (must have extra 0 word at top)
    bigint v, // divisor (normalized copy, untouched)
    bigint q  // quotient out (pre-allocated, length m+1)
) {
  typedef unsigned long long u64;
  const int bits = sizeof(bigint_unit) * 8;
  const u64 BASE = (u64)1 << bits;
  int n = (int)msList_len(v);
  int m = (int)msList_len(u) - n - 1; // u has m+n+1 words (extra guard word)

  int s = bigint_nlz(v[n - 1]);

  // Normalized copies on the arena
  bigint vn = msList_init(arena, bigint_unit, n);
  bigint un = msList_init(arena, bigint_unit, m + n + 1);

  // Shift v left by s into vn
  for (int i = n - 1; i > 0; i--)
    msList_push(arena, vn, (v[i] << s) | (v[i - 1] >> (bits - s)));
  msList_push(arena, vn, v[0] << s); // vn[0] last (push = append, little-endian)
  // fix: we built vn backwards above — build correctly
  msList_len(vn) = 0;
  msList_push(arena, vn, v[0] << s);
  for (int i = 1; i < n; i++)
    msList_push(arena, vn, (v[i] << s) | (v[i - 1] >> (bits - s)));

  // Shift u left by s into un (u has m+n words, un gets m+n+1)
  msList_push(arena, un, u[0] << s);
  for (int i = 1; i < m + n; i++)
    msList_push(arena, un, (u[i] << s) | (u[i - 1] >> (bits - s)));
  msList_push(arena, un, u[m + n - 1] >> (bits - s));

  for (int j = m; j >= 0; j--) {
    // Estimate q̂
    u64 num = ((u64)un[j + n] << bits) | un[j + n - 1];
    u64 qhat = num / vn[n - 1];
    u64 rhat = num % vn[n - 1];

    // Refine (at most 2 iterations, proven by Knuth)
    while (qhat >= BASE ||
           qhat * vn[n - 2] > BASE * rhat + un[j + n - 2]) {
      qhat--;
      rhat += vn[n - 1];
      if (rhat >= BASE)
        break;
    }

    // Multiply and subtract: un[j..j+n] -= qhat * vn
    long long borrow = 0;
    for (int i = 0; i < n; i++) {
      u64 prod = qhat * (u64)vn[i];
      long long sub = (long long)un[j + i] - borrow - (long long)(prod & (BASE - 1));
      un[j + i] = (bigint_unit)sub;
      borrow = (long long)(prod >> bits) - (sub >> bits);
    }
    long long top = (long long)un[j + n] - borrow;
    un[j + n] = (bigint_unit)top;

    q[j] = (bigint_unit)qhat;

    // Add back if overshot (~1/BASE chance)
    if (top < 0) {
      q[j]--;
      u64 carry = 0;
      for (int i = 0; i < n; i++) {
        carry += (u64)un[j + i] + (u64)vn[i];
        un[j + i] = (bigint_unit)carry;
        carry >>= bits;
      }
      un[j + n] += (bigint_unit)carry;
    }
  }

  // Denormalize remainder back into u (shift un right by s)
  for (int i = 0; i < n - 1; i++)
    u[i] = (un[i] >> s) | (un[i + 1] << (bits - s));
  u[n - 1] = un[n - 1] >> s;
}

struct bigint_div_t bigint_div(AllocatorV allocator, bigint a1, bigint b1) {
  typedef struct bigint_div_t div_result;
  assertMessage(bigint_cmp(b1, NULL), "Division by zero");

  if (!bigint_cmp(a1, NULL))
    return (div_result){bigint_from(allocator, 0), bigint_from(allocator, 0)};

  var_ arena = arena_new_ext(allocator, 4096);
  defer { arena_cleanup(arena); };

  bool negative = 0;
  var_ a = bigint_negetive(a1)
               ? (negative = !negative, bigint_negate(arena, a1))
               : bigint_copy(arena, a1);
  var_ b = bigint_negetive(b1)
               ? (negative = !negative, bigint_negate(arena, b1))
               : bigint_copy(arena, b1);
  bigint_trim(&a);
  bigint_trim(&b);

  if (bigint_cmp(a, b) < 0) {
    bigint rem = bigint_copy(allocator, a);
    if (negative)
      bigint_negate_ip(allocator, &rem);
    return (div_result){bigint_from(allocator, 0), rem};
  }

  typedef unsigned long long u64;
  const int bits = sizeof(bigint_unit) * 8;
  const u64 BASE = (u64)1 << bits;

  int n = (int)msList_len(b);
  int m = (int)msList_len(a) - n;

  // single-word divisor fast path
  if (n == 1) {
    bigint q = msList_init(allocator, bigint_unit, m + 1);

    // Initialize array with zeros first to establish length
    for (int i = 0; i <= m; i++)
      msList_push(allocator, q, (bigint_unit)0);

    u64 rem = 0;
    for (int i = m; i >= 0; i--) {
      u64 cur = rem * BASE + a[i];
      q[i] = (bigint_unit)(cur / b[0]);
      rem = cur % b[0];
    }

    bigint_trim(&q);
    bigint r = bigint_from(allocator, (i64)rem);
    if (negative) {
      bigint_negate_ip(allocator, &q);
      bigint_negate_ip(allocator, &r);
    }
    return (div_result){q, r};
  }
  // Knuth Algorithm D — multi-word
  int s = bigint_nlz(b[n - 1]);

  // normalized divisor vn[0..n-1]
  bigint vn = msList_init(arena, bigint_unit, n);
  for (int i = 0; i < n; i++)
    msList_push(arena, vn, (bigint_unit)0);
  vn[0] = b[0] << s;
  for (int i = 1; i < n; i++)
    vn[i] = s ? (b[i] << s) | (b[i - 1] >> (bits - s)) : b[i];

  // normalized dividend un[0..m+n], one extra guard word
  bigint un = msList_init(arena, bigint_unit, m + n + 1);
  for (int i = 0; i < m + n + 1; i++)
    msList_push(arena, un, (bigint_unit)0);
  un[m + n] = s ? a[m + n - 1] >> (bits - s) : 0;
  un[0] = a[0] << s;
  for (int i = 1; i < m + n; i++)
    un[i] = s ? (a[i] << s) | (a[i - 1] >> (bits - s)) : a[i];

  bigint q = msList_init(allocator, bigint_unit, m + 1);
  for (int i = 0; i <= m; i++)
    msList_push(allocator, q, (bigint_unit)0);

  for (int j = m; j >= 0; j--) {
    u64 num = ((u64)un[j + n] << bits) | un[j + n - 1];
    u64 qhat = num / vn[n - 1];
    u64 rhat = num % vn[n - 1];

    // refine estimate
    while (qhat >= BASE ||
           qhat * (u64)vn[n - 2] > BASE * rhat + un[j + n - 2]) {
      qhat--;
      rhat += vn[n - 1];
      if (rhat >= BASE)
        break;
    }

    // multiply and subtract
    long long borrow = 0;
    for (int i = 0; i < n; i++) {
      u64 prod = qhat * (u64)vn[i];
      long long sub = (long long)un[j + i] - borrow - (long long)(prod & (BASE - 1));
      un[j + i] = (bigint_unit)sub;
      borrow = (long long)(prod >> bits) - (sub >> bits);
    }
    long long top = (long long)un[j + n] - borrow;
    un[j + n] = (bigint_unit)top;

    q[j] = (bigint_unit)qhat;

    // add back if overshot
    if (top < 0) {
      q[j]--;
      u64 carry = 0;
      for (int i = 0; i < n; i++) {
        carry += (u64)un[j + i] + (u64)vn[i];
        un[j + i] = (bigint_unit)carry;
        carry >>= bits;
      }
      un[j + n] += (bigint_unit)carry;
    }
  }

  // denormalize remainder
  bigint r = msList_init(allocator, bigint_unit, n);
  for (int i = 0; i < n; i++)
    msList_push(allocator, r, (bigint_unit)0);
  for (int i = 0; i < n - 1; i++)
    r[i] = s ? (un[i] >> s) | (un[i + 1] << (bits - s)) : un[i];
  r[n - 1] = un[n - 1] >> s;

  bigint_trim(&q);
  bigint_trim(&r);

  if (negative) {
    bigint_negate_ip(allocator, &q);
    bigint_negate_ip(allocator, &r);
  }
  return (div_result){q, r};
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
    if (!msList_len(digits))
      msList_push(arena, digits, '0');
    put(digits, _arb, msList_len(digits), 0);
  }
});

int main(void) {
  AllocatorV debug = debugAllocator(
      allocator = stdAlloc,
      log = stdout
  );
  var_ sn = sn_print(stdAlloc, "{bigint:dbg}", bigint_from(stdAlloc, 100));
  while (!sn.ptr[sn.len - 1])
    sn.len--;
  // assertMessage(fptr_eq(*(fptr *)&sn, fp("[64]")), "%s", sn.ptr);

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
    defer { msList_deInit(debug, c); };
    println(
        "{bigint:both} * {bigint:both} = {bigint:both}",
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
  // return 0;
  bigint a = bigint_from(debug, 1);
  bigint b = bigint_from(debug, 1);
  // 1 1 2 3 5
  int i = 2;
  while (1) {
    bigint c = bigint_add(debug, a, b);
    println("{bigint}\n{} ", c, i++, );
    msList_deInit(debug, a);
    a = b;
    b = c;
  }
}
