#if !defined(VASON_BUILDER_H)
  #define VASON_BUILDER_H
  #include "macros.h"
  #include "mytypes.h"
  #include "shortList.h"
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

#endif
#if defined(VASON_BUILDER_C) && VASON_BUILDER_C == 1
  #define VASON_BUILDER_C (2)
  #include "src/vason_tree.c"
#endif
