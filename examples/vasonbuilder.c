#include "../debugallocator.h"
#include "../vason_tree.h"
int main(void) {
  println("{}", (f128)0.6767);
  AllocatorV local = debugAllocator(log = stderr);
  defer { debugAllocatorDeInit(local); };
  c8 string[] = "{hello:{{{{}}},{},world:{hello:{world}}}}";
  struct debugStats c_start = debugAllocator_stats(local);
  vason_container c = vason_parseString(local, (slice(c8))slice_stat(string));
  struct debugStats c_end = debugAllocator_stats(local);
  defer { vason_container_free(c); };
  println(
      "container creation profile - memory delta: {}, calls delta: {}",
      c_end.current_memory - c_start.current_memory,
      c_end.total_calls - c_start.total_calls
  );
  slice(c8) str = vason_tostr(local, c);
  println("input\t:{}", str);
  slice_free(local, str);
  struct debugStats n_start = debugAllocator_stats(local);
  vason_node n = vason_container_toNode(local, c);
  struct debugStats n_end = debugAllocator_stats(local);
  defer { vason_node_freeRecursive(local, n); };
  println(
      "node creation profile - memory delta: {}, calls delta: {}",
      n_end.current_memory - n_start.current_memory,
      n_end.total_calls - n_start.total_calls
  );
  println(
      "container size : {}\n"
      "nodes size: {}",
      vason_container_footprint(c) + sizeof(string),
      vason_node_footprint(n)
  );
  assertMessage(n.tag == vason_TABLE);
  vason_table_push(
      local,
      &n,
      vason_node_makePair(
          local,
          vason_node_str(local, "new"),
          vason_node_str(local, ":,[{\"\"}]")
      )
  );
  slice(c8) *strcontainer = {};
  defer { slice_free(local, *strcontainer); };
  vason_container newC = vason_node_toContainer(local, n, strcontainer);
  defer { vason_container_free(newC); };
  slice(c8) result = vason_tostr(local, newC);
  // defer { aFree(local, result.ptr); };
  println("output\t:{}", result);
}
#include "../wheels.h"
