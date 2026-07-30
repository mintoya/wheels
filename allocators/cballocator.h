#if !defined CBA_ALLOCATOR_H
  #define CBA_ALLOCATOR_H (1)
  #include "../allocator.h"
typedef struct cbhandle {
  struct cballocator *cbself;
  void *inptr;
  usize insize;
  usize outsize;
  const char *filename;
  uint linenumber;
  void *udata;
} callbackallocatorhandle;
typedef struct cballocator {
  struct allocfns dt[1];
  allocfn allocator;
  fnptrof((const callbackallocatorhandle *), void) cba;
  fnptrof((const callbackallocatorhandle *, void *), void) cbb;
  void *udata;
} callbackallocatorbuffer;
allocfn cba_init(allocfn fn, itypeof(struct cballocator, cba) cba, itypeof(struct cballocator, cbb) cbb, void *);
void cba_deinit(allocfn fn);

  #include "../tests.h"
[[maybe_unused]] static void _test_cba_fn(const callbackallocatorhandle *h) {
  struct {
    int calls, total;
  } *x = (typeof(x))h->cbself->udata;
  x->calls++;
}
[[maybe_unused]] static void _test_cbb_fn(const callbackallocatorhandle *h, void *res) {
  struct {
    int calls, total;
  } *x = (typeof(x))h->cbself->udata;
  x->total += (h->outsize - h->insize);
}
test_fn(cba_test_fn) {
  struct {
    int calls;
    int total;
  } p = {};
  allocfn wrapper = cba_init(allocator, _test_cba_fn, _test_cbb_fn, &p);
  defer { cba_deinit(wrapper); };

  u8 *ptr = *acreate(wrapper, u8[128]);
  ptr = (u8 *)aresize(wrapper, (u8(*)[128])ptr, u8[256]);
  adestroy(wrapper, (u8(*)[256])ptr);
  test_inteq(p.calls, 3);
  test_inteq(p.total, 0);
}

#endif

#if (defined CBA_ALLOCATOR_C && CBA_ALLOCATOR_C == 1) || (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)
  #undef CBA_ALLOCATOR_C
  #define CBA_ALLOCATOR_C (2)
void *_cba_alloc(allocfn slf, void *op, usize in, usize out, const char *f, uint l) {
  let selff = (struct cballocator *)slf;
  const struct cbhandle here[1] = {{selff, op, in, out, f, l}};
  selff->cba(here);
  let result = vcall(selff->allocator, fn, (op, in, out, f, l));
  selff->cbb(here, result);
  return result;
}
allocfn cba_init(
    allocfn fn,
    itypeof(struct cballocator, cba) cba,
    itypeof(struct cballocator, cbb) cbb,
    void *udata
) {
  return avalue(fn, ((struct cballocator){{_cba_alloc}, fn, cba, cbb, udata}))->dt;
}
void cba_deinit(allocfn fn) {
  let selff = (struct cballocator *)fn;
  adestroy(selff->allocator, selff);
}
#endif
