#if !defined ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H (1)
  #include "../allocator.h"

AllocatorV arena_new_ext(AllocatorV allocator, usize blocksize);
void arena_clear(AllocatorV allocator);
usize arena_countBlocks(AllocatorV allocator);
void arena_cleanup(AllocatorV allocator);
usize arena_totalMem(AllocatorV allocator);
usize arena_footprint(AllocatorV allocator);
// #include "../tests.c"
  #if defined(MAKE_TEST_FN)
MAKE_TEST_FN(arena_test, {
  var_ arena = arena_new_ext(allocator, 100);
  defer { arena_cleanup(arena); };
  var_ u8s = aCreate(arena, u8, 1000);
  var_ i = aCreate(arena, int);
  if ((uptr)u8s % alignof(myAlign)) return 1;
  if ((uptr)i % alignof(myAlign)) return 2;
  return 0;
});
  #endif
#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define ARENA_ALLOCATOR_C (1)
#endif

#if defined(ARENA_ALLOCATOR_C)

  #include "../mylist.h"
typedef struct {
  usize capacity, occupied, count;
  u8 *ptr;
} ArenaAllocator_buffer;
typedef struct {
  My_allocator vt[1];
  mList(ArenaAllocator_buffer) buffers; // stores backing allocatorr inside
} ArenaAllocator_data;
AllocatorV arena_backing_allocator(AllocatorV allocator) {
  return mList_allocator(((ArenaAllocator_data *)allocator)->buffers);
}

void *_arena_alloc(AllocatorV allocator, usize size, char *, usize);
void _arena_free(AllocatorV allocator, void *ptr, usize size, char *, usize);
AllocatorV _arena_prototype = (typeof(stdAlloc))(My_allocator[1]){{
    _arena_alloc,
    _arena_free,
    nullptr,
    nullptr,
}};
ArenaAllocator_buffer arena_newBlock(AllocatorV origional_allocator, usize size) {
  var_ ptr = aCreate(origional_allocator, u8, size);
  var_ cap =
      origional_allocator->size
          ? origional_allocator->size(origional_allocator, ptr)
          : size;
  return ((ArenaAllocator_buffer){
      .capacity = size,
      .occupied = 0,
      .count = 0,
      .ptr = ptr,
  });
}
AllocatorV arena_new_ext(AllocatorV allocator, usize blocksize) {
  var_ result = aCreate(allocator, ArenaAllocator_data);
  memcpy(result->vt, _arena_prototype, sizeof(result->vt));
  result->buffers = mList_init(allocator, ArenaAllocator_buffer);
  mList_push(result->buffers, arena_newBlock(allocator, blocksize));
  return result->vt;
}
void arena_cleanup(AllocatorV allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  var_ backing = mList_allocator(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    aFree(backing, block->ptr, block->capacity);
  mList_deinit(data->buffers);
  aFree(backing, data, sizeof(*data));
}
void arena_clear(AllocatorV allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1])) {
    block->count = 0;
    block->occupied = 0;
  }
}
usize arena_countBlocks(AllocatorV allocator) {
  return mList_len(((ArenaAllocator_data *)allocator)->buffers);
}
usize arena_totalMem(AllocatorV allocator) {
  usize size = 0;
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    size += block->capacity;
  return size;
}
usize arena_footprint(AllocatorV allocator) {
  var_ data = ((ArenaAllocator_data *)allocator);
  usize size = 0;
  var_ vla = mList_vla(data->buffers);
  foreach (var_ block, range(vla[0], vla[1]))
    size += block->capacity;
  size += sizeof(List);
  size += mList_len(data->buffers) * sizeof(mList_arr(data->buffers)[0]);
  return size;
}

void *_arena_alloc(AllocatorV allocator, usize size, char *file, usize line) {
  var_ data = ((ArenaAllocator_data *)allocator);
  size = lineup(size, alignof(myAlign));

  assertMessage(mList_len(data->buffers));
  var_ len = mList_len(data->buffers);

  var_ current = &mList_last(data->buffers);

  {
    var_ vla = mList_vla(data->buffers);
    if (current->occupied + size > current->capacity) {
      foreach (var_ block, range(vla[0], vla[1])) {
        current = block;
        assertMessage(current->occupied % alignof(myAlign) == 0);
        if (current->occupied + size <= current->capacity) goto found;
      }
      mList_push(
          data->buffers,
          arena_newBlock(
              arena_backing_allocator(allocator),
              MAX$(mList_arr(data->buffers)[0].capacity, size)
          )
      );
      current = &mList_last(data->buffers);
      {
      found:;
      }
    }
    // vla is'nt valid
  }

  void *ptr = current->ptr + current->occupied;
  current->occupied += size;
  current->count++;

  return ptr;
}
void _arena_free(AllocatorV allocator, void *ptr, usize size, char *file, usize line) {
  var_ data = ((ArenaAllocator_data *)allocator);
  var_ vla = mList_vla(data->buffers);
  assertMessage(!((uptr)ptr % alignof(myAlign)));
  foreach (var_ block, range(vla[0], vla[1])) {
    if ((u8 *)ptr >= block->ptr && (u8 *)ptr < block->ptr + block->capacity) {
      assertMessage(block->count, "double free?");
      assertMessage(block->occupied, "double free?");
      block->count--;
      if (!block->count)
        block->occupied = 0;
      else if (block->ptr + block->occupied == ptr)
        block->occupied -= size;
      return;
    }
  }
  assertMessage(false, "allocator could'nt find the pointer");
}
#endif
