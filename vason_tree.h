#if !defined(VASON_BUILDER_H)
  #define VASON_BUILDER_H
  #include "macros.h"
  #include "mylist.h"
  #include "mytypes.h"
  #include "sList.h"
  #include "vason_arr.h"
/* once something like a pair is placed into a table,
 * the memory is "owned" by the table
 * the recursive free should work for all items
 *
 * mostlybecause msList needs a modifiable pointer
 * TODO do something about vason_INVALID
 */
typedef struct vason_node vason_node;
typedef struct vason_node {
  vason_tag tag;
  union {
    struct vason_node *table;
    struct vason_node *pair;
    struct {
      usize len;
      c8 buffer[];
    } *string;
  };
} vason_node;
void vason_node_free(AllocatorV allocator, vason_node n);
void vason_node_freeRecursive(AllocatorV allocator, vason_node n);
usize vason_node_footprint(vason_node n);
vason_node vason_node_newPair(AllocatorV a);
vason_node vason_node_makePair(AllocatorV a, vason_node key, vason_node val);
vason_node vason_node_newTable(AllocatorV a);
vason_node vason_node_newStr(AllocatorV a, slice(c8) str);
void vason_table_push(AllocatorV a, vason_node *table, vason_node item);
vason_node vason_container_toNode(AllocatorV allocator, vason_container c);
vason_container vason_node_toContainer(AllocatorV allocator, vason_node n, c8 **strContainer);
vason_node vason_node_str(AllocatorV a, const char *c);
vason_node vason_node_deepCopy(AllocatorV allocator, vason_node n);
slice(c8) vason_node_toStr(AllocatorV allocator, vason_node n);
  #if defined(MAKE_TEST_FN)

MAKE_TEST_FN(vason_exact_memcmp_match, {
  vason_node root = vason_node_newTable(allocator);
  defer { vason_node_freeRecursive(allocator, root); };

  vason_node p_key = vason_node_str(allocator, "hello");
  vason_node p_val = vason_node_str(allocator, "world");
  vason_table_push(allocator, &root, vason_node_makePair(allocator, p_key, p_val));

  vason_node s1 = vason_node_str(allocator, "hello");
  vason_table_push(allocator, &root, s1);

  vason_node s2 = vason_node_str(allocator, "world");
  vason_table_push(allocator, &root, s2);

  vason_table_push(allocator, &root, vason_node_deepCopy(allocator, root));

  slice(c8) result = vason_node_toStr(allocator, root);
  defer { aFree(allocator, result.ptr); };

  const char expected[] = "{hello:world,hello,world,{hello:world,hello,world}}";
  usize expected_len = strlen(expected);

  printf(
      "input\t: %s \noutput\t: %.*s\n",
      expected, result.len, result.ptr
  );
  if (!fptr_eq(fp(expected), (fptr){result.len, result.ptr}))
    return 1;

  return 0;
});

  #endif
#endif

#if defined(VASON_BUILDER_C) && VASON_BUILDER_C == 1
  #define VASON_BUILDER_C (2)
  #include "src/vason_tree.c"
#endif
