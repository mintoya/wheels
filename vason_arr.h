#if !defined(VASON_ARRAY_2)
  #define VASON_ARRAY_2
  #include "mytypes.h"
typedef u32 vason_index;
typedef struct {
  vason_index offset, len;
} vason_span;
enum_named(
    elementTag,
    u8,
    string,
    array,
    key_value,
    num_value,
    invalid;
) = {
    1 << 0,
    1 << 1,
    1 << 2,
    1 << 3,
    1 << 7,
};
/**
 * string     : offset + length inside of string
 * array      : offset + length inside of objects
 * key_value  : offset inside of objects, len is 2
 * num_value  : offset inside of objects
 */
typedef struct {
  vason_index num_index;
  vason_index offset;
} vason_num_value;
typedef struct {
  struct {
    /**
     * soa of tags and indexes
     * tag tells which of the slices to look inside
     * indexes tells where it acutally is
     */
    vason_index len;
    vason_index *indexes;
    elementTag_t *tags;
  };
  //
  slice(vason_index) key_values;
  slice(vason_num_value) num_values;
  union {
    slice(vason_span) strings;
    slice(vason_span) arrays;
  };
  //
  slice(c8) text; // origional string handle
                  // maybe change to a FILE* stream?
} vason_container;

#endif
