#include "../shmap.h"
#include <stdcountof.h>
#include <stdio.h>
#include <time.h>

#define N_ITERS 1000000
#define N_KEYS 8

constexpr char keys[N_KEYS][8] = {
    "one", "two", "three", "four",
    "five", "six", "seven", "eight"
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
    for (int i = 0; i < N_KEYS; ++i) {
      msHmap_set(sm, keys[i], i);
    }
  }

  double t1 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < N_KEYS; ++i) {
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
    for (int i = 0; i < N_KEYS; ++i) {
      mHmap_set(hm, convert(keys[i]), i);
    }
  }

  double t1 = now_sec();

  for (int it = 0; it < N_ITERS; ++it) {
    for (int i = 0; i < N_KEYS; ++i) {
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

  printf("=== Results (8 keys, %d iterations) ===\n", N_ITERS);

  printf("sHmap  SET: %.3f sec\n", s.set_time);
  printf("mHmap  SET: %.3f sec\n", m.set_time);

  printf("sHmap  GET: %.3f sec\n", s.get_time);
  printf("mHmap  GET: %.3f sec\n", m.get_time);

  printf("\nRatios:\n");
  printf("SET  s/m: %.2fx\n", s.set_time / m.set_time);
  printf("GET  s/m: %.2fx\n", s.get_time / m.get_time);
}
#include "../wheels.h"
