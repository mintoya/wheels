#include "arenaAllocator.h"
#include "debugallocator.h"
#include "printer/variadic.h"
#include "vason.h"

void buffer_append(const c32 *chars, mList(u8) ctx_void, unsigned int count, bool is_final) {

  for (unsigned int i = 0; i < count; i++) {
    // Simple downcast for JSON/ASCII
    if (chars[i] < 128) {
      mList_push(ctx_void, (u8)chars[i]);
    }
  }

  if (is_final) {
    mList_push(ctx_void, 0); // Null terminator
  }
}
int main(void) {
  // My_allocator *local = debugAllocatorInit(
  //         .log = stdout,
  //         .track_total = 1,
  //         .track_trace = 1,
  // );
  // defer_(debugAllocatorDeInit(local););

  Arena_scoped *local = arena_new_ext(stdAlloc, 2048);
  vason_live *root = vason_new(local, vason_MAP);

  vason_mapSetStr(root, fp("project"), fp("va'so/n_builder"));
  vason_mapSetStr(root, fp("version"), fp("0.1.0"));

  vason_live *tags = vason_new(local, vason_ARR);
  vason_arrPushStr(tags, fp("c"));
  vason_arrPushStr(tags, fp("json"));
  vason_arrPushStr(tags, fp("fast"));

  vason_mapSet(root, fp("tags"), tags);

  vason_live *meta = vason_new(local, vason_MAP);
  vason_mapSetStr(meta, fp("author"), fp("user"));
  vason_mapSetStr(meta, fp("id"), fp("9942"));
  vason_mapSet(root, fp("metadata"), meta);
  mList(u8) chars = mList_init(local, u8);
  print_wfO((outputFunction)buffer_append, chars, "{vason_live*}", root);
  println("{fptr}", ((slice(c8))mList_slice(chars)));
  println("{vason_container}", parseStr(local, (slice(c8))mList_slice(chars)));

  return 0;
}
#include "wheels.h"
