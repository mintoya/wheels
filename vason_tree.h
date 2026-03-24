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

  printf(
      "input\t: %s \noutput\t: %.*s\n",
      expected, result.len, result.ptr
  );
  slice(c8) fromExpected = (slice(c8)){strlen(expected), (c8 *)expected};
  vason_container a = vason_parseString(
      allocator,
      fromExpected
  );
  defer { vason_container_free(a); };
  vason_container b = vason_parseString(
      allocator,
      result
  );
  defer { vason_container_free(b); };
  if (!vason_container_eq(a, b))
    return 1;
  return 0;
});

  #endif
#endif

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define VASON_BUILDER_C (1)
#endif

#if defined(VASON_BUILDER_C)
vason_node vason_node_newPair(AllocatorV a) {
  return (vason_node){
      .tag = vason_PAIR,
      .pair = aCreate(a, vason_node, 2),
  };
}
void vason_node_freeRecursive(AllocatorV allocator, vason_node n) {
  switch (n.tag) {
    case vason_PAIR: {
      vason_node_freeRecursive(allocator, n.pair[0]);
      vason_node_freeRecursive(allocator, n.pair[1]);
    } break;
    case vason_TABLE: {
      foreach (var item, msList_vla(n.table))
        vason_node_freeRecursive(allocator, item);
    } break;
    case vason_STRING:
      break;
    default:
      assertMessage(false, "no free for this node ");
  }
  vason_node_free(allocator, n);
}
void vason_node_free(AllocatorV allocator, vason_node n) {
  switch (n.tag) {
    case vason_PAIR: {
      aFree(allocator, n.pair);
    } break;
    case vason_TABLE: {
      msList_deInit(allocator, n.table);
    } break;
    case vason_STRING: {
      aFree(allocator, n.string);
    } break;
    default:
      assertMessage(false, "no free for this node ");
  }
}
vason_node vason_node_makePair(AllocatorV a, vason_node key, vason_node val) {
  assertMessage(key.tag == vason_STRING);
  vason_node res = vason_node_newPair(a);
  res.pair[0] = key;
  res.pair[1] = val;
  return res;
}
vason_node vason_node_newTable(AllocatorV a) {
  return (vason_node){
      .tag = vason_TABLE,
      .table = msList_init(a, vason_node)
  };
}
vason_node vason_node_newStr(AllocatorV a, slice(c8) str) {
  vason_node res = (vason_node){
      .tag = vason_STRING,
      .string = (typeof(res.string))aAlloc(a, sizeof(res.string[0]) + str.len)
  };
  memcpy(res.string->buffer, str.ptr, str.len);
  res.string->len = str.len;
  return res;
}
vason_node vason_node_str(AllocatorV a, const char *c) {
  return vason_node_newStr(a, (slice(c8)){strlen((char *)c), (c8 *)c});
}
void vason_table_push(AllocatorV a, vason_node *table, vason_node item) {
  assertMessage(table->tag == vason_TABLE);
  msList_push(a, table->table, item);
}
void vason_node_intoContainer(vason_container *c, vason_node n, vason_index i) {
  c->tags[i] = n.tag;
  switch (n.tag) {
    case vason_TABLE: {
      vason_index tableStart = msList_len(c->tables_strings);
      c->tables_strings[i] = (vason_span){
          .start = tableStart,
          .end = (vason_index)(tableStart + msList_len(n.table)),
      };
      msList_pushVla(c->allocator, c->tables_strings, VLAP((vason_span *)NULL, msList_len(n.table)));
      msList_pushVla(c->allocator, c->tags, VLAP((vason_tag *)NULL, msList_len(n.table)));
      foreach (var item, msList_vla(n.table))
        vason_node_intoContainer(c, item, tableStart++);
    } break;
    case vason_PAIR: {
      vason_index tableStart = msList_len(c->tables_strings);
      c->tables_strings[i] = (vason_span){
          .start = tableStart,
          .end = (vason_index)(tableStart + 2),
      };
      msList_pushVla(c->allocator, c->tables_strings, VLAP((vason_span *)NULL, 2));
      msList_pushVla(c->allocator, c->tags, VLAP((vason_tag *)NULL, 2));
      vason_node_intoContainer(c, n.pair[0], tableStart);
      vason_node_intoContainer(c, n.pair[1], tableStart + 1);
    } break;
    case vason_STRING: {
      vason_index strStart = msList_len(c->text.ptr);
      c->tables_strings[i] = (vason_span){
          .start = strStart,
          .end = (vason_index)(strStart + n.string->len),
      };
      if (n.string->len)
        msList_pushArr(
            c->allocator,
            c->text.ptr,
            *VLAP(n.string->buffer, n.string->len)
        );
    } break;
    default:
      assertMessage(false, "unreachable?");
  }
}
vason_container vason_node_toContainer(AllocatorV allocator, vason_node n, c8 **strContainer) {
  assertMessage(strContainer && !*strContainer);
  vason_container res = (vason_container){
      .current = 0,
      .tags = msList_init(allocator, vason_tag),
      .tables_strings = msList_init(allocator, vason_span),
      .allocator = allocator,
      .text.ptr = msList_init(allocator, c8),
      .tokens = NULL,
  };
  msList_push(allocator, res.tags, n.tag);
  msList_push(allocator, res.tables_strings, (vason_span){});
  vason_node_intoContainer(&res, n, 0);

  slice(c8) resStr = {msList_len(res.text.ptr), aCreate(allocator, c8, msList_len(res.text.ptr))};
  memcpy(resStr.ptr, res.text.ptr, msList_len(res.text.ptr));
  msList_deInit(allocator, res.text.ptr);
  *strContainer = resStr.ptr;
  res.text = resStr;
  return res;
}
vason_node vason_container_toNode(AllocatorV allocator, vason_container c) {
  assertMessage(!c.tokens, "no lazy containers");
  switch (c.tags[c.current]) {
    case vason_STRING: {
      vason_span vs = c.tables_strings[c.current];
      vason_node res;
      res = vason_node_newStr(allocator, (slice(c8)){(vason_index)(vs.end - vs.start), c.text.ptr + vs.start});
      return res;
    } break;
    case vason_TABLE: {
      vason_node res;
      vason_span vs = c.tables_strings[c.current];
      res = vason_node_newTable(allocator);
      msList_reserve(allocator, res.table, vs.end - vs.start);
      for (var i = vs.start; i < vs.end; i++) {
        c.current = i;
        msList_push(allocator, res.table, vason_container_toNode(allocator, c));
      }
      return res;
    } break;
    case vason_PAIR: {
      vason_node res;
      vason_span vs = c.tables_strings[c.current];
      res = vason_node_newPair(allocator);
      c.current = vs.start;
      res.pair[0] = vason_container_toNode(allocator, c);
      c.current = vs.start + 1;
      res.pair[1] = vason_container_toNode(allocator, c);
      return res;
    } break;
    default:
      assertMessage(false, "unreachable?");
  }
  assertMessage(false, "unreachable?");
  return (vason_node){};
}
usize vason_node_footprint(vason_node n) {
  switch (n.tag) {
    case vason_PAIR: {
      return vason_node_footprint(n.pair[0]) + vason_node_footprint(n.pair[1]) + sizeof(n.pair[0]) * 2;
    } break;
    case vason_TABLE: {
      usize res = 0;
      res += sizeof(*msList_vla(n.table)) + sizeof(sList_header);
      foreach (var item, msList_vla(n.table))
        res += vason_node_footprint(item);
      return res;
    } break;
    case vason_STRING:
      return sizeof(n.string[0]) + n.string->len;
      break;
    default:
      assertMessage(false, "no free for this node ");
      return 0;
  }
}
vason_node vason_node_deepCopy(AllocatorV allocator, vason_node n) {
  vason_node res;
  res.tag = n.tag;
  switch (n.tag) {
    case vason_TABLE: {
      res.table = msList_init(allocator, typeof(*n.table), msList_len(n.table));
      foreach (var node, msList_vla(n.table))
        msList_push(
            allocator,
            res.table,
            vason_node_deepCopy(allocator, node)
        );
    } break;
    case vason_PAIR: {
      res.pair = aCreate(allocator, vason_node, 2);
      res.pair[0] = vason_node_deepCopy(allocator, n.pair[0]);
      res.pair[1] = vason_node_deepCopy(allocator, n.pair[1]);
    } break;
    case vason_STRING: {
      res.string = (typeof(res.string))aAlloc(allocator, sizeof(*n.string) + n.string->len);
      res.string->len = n.string->len;
      __builtin_memcpy(res.string->buffer, n.string->buffer, n.string->len);
    } break;
  }
  return res;
}
slice(c8) vason_node_toStr(AllocatorV allocator, vason_node n) {
  c8 *strp = NULL;
  defer { aFree(allocator, strp); };
  vason_container c = vason_node_toContainer(allocator, n, &strp);
  defer { vason_container_free(c); };
  return vason_tostr(allocator, c);
}
#endif
