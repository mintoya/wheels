#include "../print.h"
#include "../shmap.h"
#include <stdcountof.h>
#include <stdio.h>
#include <time.h>

#define N_ITERS 100000

constexpr char keys[][8] = {
    "one", "two", "three", "four",
    "five", "six", "seven", "eight",
    "ten", "eleven", "twelve", "thirten"
};

typedef struct {
  double set_time;
  double get_time;
} BenchResult;

static double now_sec() {
  return (double)clock() / CLOCKS_PER_SEC;
}
BenchResult bench_sHmap(AllocatorV allocator) {
  BenchResult r = {0};

  msHmap(int) sm = msHmap_init(allocator, int, 8);

  double t0 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < countof(keys); ++i) {
      msHmap_set(sm, keys[i], i);
    }
  }

  double t1 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < countof(keys); ++i) {
      int *v = msHmap_get(sm, keys[i]);
      if (!v)
        abort();
    }
  }

  double t2 = now_sec();

  r.set_time = t1 - t0;
  r.get_time = t2 - t1;

  msHmap_deinit(allocator, sm);
  return r;
}
typedef struct {
  char str[countof(keys[0])]
} key;
static inline key convert(const char src[8]) {
  union {
    typeof(src) a;
    key b;
  } t = {.a = src};
  return t.b;
}
BenchResult bench_mHmap(AllocatorV allocator) {
  BenchResult r = {0};

  mHmap(key, int) hm = mHmap_init(allocator, key, int, 8);

  double t0 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < countof(keys); ++i) {
      mHmap_set(hm, convert(keys[i]), i);
    }
  }

  double t1 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < countof(keys); ++i) {
      int *v = mHmap_get(hm, convert(keys[i]));
      if (!v)
        abort();
    }
  }

  double t2 = now_sec();

  r.set_time = t1 - t0;
  r.get_time = t2 - t1;

  mHmap_deinit(hm);
  return r;
}
int main() {
  AllocatorV allocator = stdAlloc;
  BenchResult s = bench_sHmap(allocator);
  BenchResult m = bench_mHmap(allocator);

  println("=== Results (8 keys, {} iterations) ===", (usize)N_ITERS);

  print(
      "sHmap  SET: {:3} sec\n"
      "mHmap  SET: {:3} sec\n",
      s.set_time, m.set_time
  );
  print(
      "sHmap  GET: {:3} sec\n"
      "mHmap  GET: {:3} sec\n",
      s.get_time, m.get_time
  );
  print(
      "Ratios\n"
      "SET  s/m: {:2}X\n"
      "GET  s/m: {:2}X\n",
      s.set_time / m.set_time,
      s.get_time / m.get_time
  );
}
#include "../wheels.h"
