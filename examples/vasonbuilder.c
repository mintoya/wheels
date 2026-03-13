#include "../debugallocator.h"
#include "../vason_tree.h"
#include <stddefer.h>
int i = __STDC_VERSION__;
int main(void) {
  My_allocator *local = debugAllocator(track_total = 1, log = stderr);
  defer { debugAllocatorDeInit(local); };
  c8 string[] = "{hello:{world : {hello : {world}}}}";
  println("input\t:{cstr}", (c8 *)string);

  vason_container c = vason_parseString(local, (slice(c8))slice_stat(string));
  defer { vason_container_free(c); };

  vason_node n = vason_container_toNode(local, c);
  defer { vason_node_freeRecursive(local, n); };
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

  c8 *strcontainer = NULL;
  defer { aFree(local, strcontainer); };
  vason_container newC = vason_node_toContainer(local, n, &strcontainer);
  defer { vason_container_free(newC); };

  slice(c8) result = vason_tostr(local, newC);
  defer { aFree(local, result.ptr); };

  println("output\t:{fptr}", result);
}
#include "../wheels.h"
