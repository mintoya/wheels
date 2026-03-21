#include "../vason_tree.h"
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
      for (each_VLAP(item, msList_vla(n.table)))
        vason_node_freeRecursive(allocator, *item);
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
      for (each_VLAP(item, msList_vla(n.table)))
        vason_node_intoContainer(c, *item, tableStart++);
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
      for (auto i = vs.start; i < vs.end; i++) {
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
      for (each_VLAP(item, msList_vla(n.table)))
        res += vason_node_footprint(*item);
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
