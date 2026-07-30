#if !defined ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H (1)
  #include "../allocator.h"

allocfn arena_new_ext(allocfn allocator, usize blocksize);
void arena_clear(allocfn allocator);
usize arena_countBlocks(allocfn allocator);
void arena_cleanup(allocfn allocator);
usize arena_totalMem(allocfn allocator);
usize arena_footprint(allocfn allocator);
  #include "../tests.h"
test_fn(arena_test) {
  var_ arena = arena_new_ext(allocator, 100);
  defer { arena_cleanup(arena); };
  var_ u8s = acreate(arena, u8[200]);
  var_ i32s = acreate(arena, int[100]);
  foreach (var_ i32, vla(*u8s))
    i32 *= i32;
  foreach (var_ i32, vla(*i32s))
    i32 *= i32;
  test_assert(!((uptr)u8s % alignof(myAlign)));
  test_assert(!((uptr)i32s % alignof(myAlign)));
}

#endif
#if (defined ARENA_ALLOCATOR_C && ARENA_ALLOCATOR_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef ARENA_ALLOCATOR_C
  #define ARENA_ALLOCATOR_C (2)

  #include "../assertMessage.h"
  #include "../mylist.h"
typedef struct ArenaAllocator_buffer {
  usize capacity, occupied, count;
  u8 *ptr;
} ArenaAllocator_buffer;
typedef struct ArenaAllocator_data {
  void *(*fn)(void *, void *, usize, usize, char *, usize);
  mList(ArenaAllocator_buffer) buffers; // stores backing allocatorr inside
} ArenaAllocator_data;
allocfn arena_backing_allocator(allocfn allocator) {
  return mList_allocator(((ArenaAllocator_data *)allocator)->buffers);
}

void *_arena_fn(void *allocator, void *ptr, usize oldsize, usize newsize, char *file, usize line);

ArenaAllocator_buffer arena_newBlock(allocfn origional_allocator, usize size) {
  var_ ptr = (u8 *)acreate(origional_allocator, u8[size]);
  return ((ArenaAllocator_buffer){
      .capacity = size,
      .occupied = 0,
      .count = 0,
      .ptr = ptr,
  });
}
allocfn arena_new_ext(allocfn allocator, usize blocksize) {
  var_ result = acreate(allocator, ArenaAllocator_data);
  result->fn = _arena_fn;
  result->buffers = mList_init(allocator, ArenaAllocator_buffer);
  mList_push(result->buffers, arena_newBlock(allocator, blocksize));
  return (allocfn)result;
}
void arena_cleanup(allocfn allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  var_ backing = mList_allocator(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    vcall(backing, fn, (block->ptr, block->capacity, 0, nullptr, 0));
  mList_deinit(data->buffers);
  adestroy(backing, data);
}
void arena_clear(allocfn allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1])) {
    block->count = 0;
    block->occupied = 0;
  }
}
usize arena_countBlocks(allocfn allocator) {
  return mList_len(((ArenaAllocator_data *)allocator)->buffers);
}
usize arena_totalMem(allocfn allocator) {
  usize size = 0;
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    size += block->capacity;
  return size;
}
usize arena_footprint(allocfn allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  usize size = 0;
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    size += block->capacity;
  size += sizeof(List);
  size += mList_len(data->buffers) * sizeof(mList_arr(data->buffers)[0]);
  return size;
}

void *_arena_fn(void *allocator, void *ptr, usize oldsize, usize newsize, char *file, usize line) {
  var_ data = ((ArenaAllocator_data *)allocator);
  oldsize = lineup(oldsize, alignof(myAlign));
  newsize = lineup(newsize, alignof(myAlign));

  if (!newsize) {
    if (!ptr) return nullptr;
    var_ vla = mList_vla(data->buffers);
    assertMessage(!((uptr)ptr % alignof(myAlign)));
    foreach (var_ block, range(vla[0], vla[1])) {
      if ((u8 *)ptr >= block->ptr && (u8 *)ptr < block->ptr + block->capacity) {
        assertMessage(block->count, "double free?");
        assertMessage(block->occupied, "double free?");
        block->count--;
        if (!block->count)
          block->occupied = 0;
        else if (block->ptr + block->occupied == (u8 *)ptr + oldsize)
          block->occupied -= oldsize;
        return nullptr;
      }
    }
    assertMessage(false, "allocator could'nt find the pointer");
    return nullptr;
  }

  assertMessage(mList_len(data->buffers));
  var_ len = mList_len(data->buffers);

  var_ current = &mList_last(data->buffers);

  {
    var_ vla = mList_vla(data->buffers);
    if (current->occupied + newsize > current->capacity) {
      foreach (var_ block, range(vla[0], vla[1])) {
        current = block;
        assertMessage(current->occupied % alignof(myAlign) == 0);
        if (current->occupied + newsize <= current->capacity) goto found;
      }
      mList_push(
          data->buffers,
          arena_newBlock(
              arena_backing_allocator(allocator),
              MAX$(mList_arr(data->buffers)[0].capacity, newsize)
          )
      );
      current = &mList_last(data->buffers);
      {
      found:;
      }
    }
  }

  void *res = current->ptr + current->occupied;
  current->occupied += newsize;
  current->count++;

  if (ptr && oldsize) {
    memcpy(res, ptr, MIN$(oldsize, newsize));
    _arena_fn(allocator, ptr, oldsize, 0, file, line);
  }

  return res;
}
#endif
