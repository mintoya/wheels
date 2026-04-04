#include "../print.h"
#include "../shmap.h"
#include <time.h>

#define N_ITERS 100000

const char keys[][8] = {
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "ten",
    "eleven",
    "twelve",
    "thirten",
    "osne",
    "asltwo",
    "thriee",
    "sfour",
    "oene",
    "aslelev",
    "twelee",
    "enur",
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

  for (each_RANGE(int, it, 0, N_ITERS))
    for (each_RANGE(int, i, 0, countof(keys)))
      msHmap_set(sm, keys[i], i);

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

  msHmap_deinit(sm);
  return r;
}
typedef struct {
  char str[countof(keys[0])];
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

  for (each_RANGE(int, iters, 0, N_ITERS))
    for (each_RANGE(int, i, 0, countof(keys)))
      mHmap_set(hm, convert(keys[i]), i);

  double t1 = now_sec();

  for (each_RANGE(int, iters, 0, N_ITERS))
    for (each_RANGE(int, i, 0, countof(keys))) {
      int *v = mHmap_get(hm, convert(keys[i]));
      if (!v)
        abort();
    }

  double t2 = now_sec();

  r.set_time = t1 - t0;
  r.get_time = t2 - t1;

  mHmap_deinit(hm);
  return r;
}
BenchResult bench_mHmapo(AllocatorV allocator) {
  BenchResult r = {0};

  mHmap(key, int) hm = mHmap_init(allocator, key, int, 0, 32);

  double t0 = now_sec();

  for (each_RANGE(int, iters, 0, N_ITERS))
    for (each_RANGE(int, i, 0, countof(keys)))
      mHmap_set(hm, convert(keys[i]), i);

  double t1 = now_sec();

  for (each_RANGE(int, iters, 0, N_ITERS))
    for (each_RANGE(int, i, 0, countof(keys))) {
      int *v = mHmap_get(hm, convert(keys[i]));
      if (!v)
        abort();
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
  BenchResult o = bench_mHmapo(allocator);

  print(
      "sHmap  SET: {:3} sec\n"
      "mHmap  SET: {:3} sec\n"
      "oHmap  SET: {:3} sec\n",
      s.set_time, m.set_time, o.set_time
  );
  print(
      "sHmap  GET: {:3} sec\n"
      "mHmap  GET: {:3} sec\n"
      "oHmap  GET: {:3} sec\n",
      s.get_time, m.get_time, o.get_time
  );
}
#include "../wheels.h"
