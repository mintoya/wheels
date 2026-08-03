#if !defined ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H (1)
  #include "../allocator.h"
  #include "../assertMessage.h"
  #include "../mylist.h"
  #include <string.h>

allocfn arena_new_ext(allocfn allocator, usize blocksize);
void arena_clear(allocfn allocator);
usize arena_countBlocks(allocfn allocator);
void arena_cleanup(allocfn allocator);
usize arena_totalMem(allocfn allocator);
usize arena_footprint(allocfn allocator);
  #include "../tests.h"
test_fn(arena_test) {
  let arena = arena_new_ext(allocator, 100);
  defer { arena_cleanup(arena); };
  let u8s = acreate(arena, u8[200]);
  let i32s = acreate(arena, int[100]);
  u8s = aresize(arena, u8s, u8[200]);
  i32s = aresize(arena, i32s, int[100]);
  foreach (let i32, vla(*u8s))
    i32 *= i32;
  foreach (let i32, vla(*i32s))
    i32 *= i32;
  test_assert(!((uptr)u8s % alignof(myAlign)));
  test_assert(!((uptr)i32s % alignof(myAlign)));
  printf("total : %zu\n", arena_totalMem(arena));
  printf("footprint : %zu\n", arena_footprint(arena));
}

#endif
#if (defined ARENA_ALLOCATOR_C && ARENA_ALLOCATOR_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef ARENA_ALLOCATOR_C
  #define ARENA_ALLOCATOR_C (2)

typedef struct ArenaAllocator_buffer {
  usize capacity, occupied, count;
  u8 *ptr;
} ArenaAllocator_buffer;
typedef struct ArenaAllocator_data {
  struct allocfns fn[1];
  mList(ArenaAllocator_buffer) buffers; // stores backing allocatorr inside
} ArenaAllocator_data;
allocfn arena_backing_allocator(allocfn allocator) {
  return mList_allocator(((ArenaAllocator_data *)allocator)->buffers);
}

// void *_arena_fn(allocfn allocator, void *ptr, usize oldsize, usize newsize, const char *file, uint line);

ArenaAllocator_buffer arena_newBlock(allocfn origional_allocator, usize size) {
  let ptr = (u8 *)acreate(origional_allocator, u8[size]);
  return ((ArenaAllocator_buffer){
      .capacity = size,
      .occupied = 0,
      .count = 0,
      .ptr = ptr,
  });
}
void *_arena_fn(allocfn allocator, void *ptr, usize os, usize ns, const char *fn, uint ln) {
  os = lineup(os, alignof(myAlign));
  ns = lineup(ns, alignof(myAlign));
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;
  let data = ((ArenaAllocator_data *)allocator);
  switch (((!!os) << 1) | ((!!ns) << 0)) {
    case ALLOC: {
      foreach (let it, span(mList_arr(data->buffers), mList_len(data->buffers))) {
        if (it->occupied + ns < it->capacity) {
          let res = it->ptr + it->occupied;
          it->occupied += ns;
          it->count++;
          return res;
        }
      }
      mList_push(data->buffers, arena_newBlock(arena_backing_allocator(allocator), MAX$(ns, mList_arr(data->buffers)->capacity)));
      mList_last(data->buffers).count++;
      mList_last(data->buffers).occupied += ns;
      return mList_last(data->buffers).ptr;
    } break;
    case FREE: {
      foreach (let it, span(mList_arr(data->buffers), mList_len(data->buffers))) {
        if (it->count && ((u8 *)ptr >= it->ptr && (u8 *)ptr < it->ptr + it->occupied)) {
          it->count--;
          it->occupied *= !!it->count;
          return nullptr;
        }
      }
      assertMessage(false, "free of pointer not in arena");
    } break;
    case RESIZE: {
      let result = acreate(allocator, u8[ns]);
      memcpy(result, ptr, MIN$(ns, os));
      adestroy(allocator, (u8(*)[os])ptr);
      return result;
    } break;
    default:
      assertMessage(false, "invalid call from %s line %u: (%p , %zu , %zu)", fn, ln, ptr, os, ns);
  }
}

allocfn arena_new_ext(allocfn allocator, usize blocksize) {
  let result = avalue(allocator, ((ArenaAllocator_data){{_arena_fn}, mList_init(allocator, ArenaAllocator_buffer)}));
  mList_push(result->buffers, arena_newBlock(allocator, blocksize));
  return result->fn;
}
void arena_cleanup(allocfn allocator) {
  let data = ((ArenaAllocator_data *)allocator);
  let vla = mList_vla(data->buffers);
  let backing = mList_allocator(data->buffers);
  foreach (let block, range(vla[0], vla[1]))
    vcall(backing, fn, (block->ptr, block->capacity, 0, nullptr, 0));
  mList_deinit(data->buffers);
  adestroy(backing, data);
}
void arena_clear(allocfn allocator) {
  let data = ((ArenaAllocator_data *)allocator);
  foreach (let block, span(mList_arr(data->buffers), mList_len(data->buffers))) {
    block->count = 0;
    block->occupied = 0;
  }
}
usize arena_countBlocks(allocfn allocator) {
  return mList_len(((ArenaAllocator_data *)allocator)->buffers);
}
usize arena_totalMem(allocfn allocator) {
  let data = ((ArenaAllocator_data *)allocator);
  usize size = 0;
  foreach (let i, mList_iter(data->buffers))
    size += i.capacity;
  return size;
}
usize arena_footprint(allocfn allocator) {
  let data = ((ArenaAllocator_data *)allocator);
  usize size = sizeof(*mList_vla(data->buffers)) + sizeof(List);
  foreach (let block, mList_iter(data->buffers))
    size += block.capacity;
  return size;
}
#endif
