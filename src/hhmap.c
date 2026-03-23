#include "../hhmap.h"
#include "../allocator.h"
#include "../assertMessage.h"
#include "../fptr.h"
#include <string.h>

typedef struct HMap {
  AllocatorV allocator;
  u32 keysize;
  u32 valsize;
  usize metaSize;
  struct {
    u32 length;
    u32 capacity;
    void *ptr;
  } storage[/*metasize*/];
} HMap;
usize HMap_getKeySize(const HMap *map) { return map->keysize; }
usize HMap_getValSize(const HMap *map) { return map->valsize; }
u32 HMap_getMetaSize(const HMap *map) { return map->metaSize; }
inline u32 HMap_getBucketSize(const HMap *map, u32 idx) { return map->storage[idx].length; }
typedef typeof((*((HMap *)nullptr)->storage)) HMap_LesserList;
inline void *HMap_getCoord(const HMap *map, u32 bucket, u32 index) {
  if (bucket > map->metaSize)
    return nullptr;
  HMap_LesserList ll = map->storage[bucket];
  if (index > ll.length)
    return nullptr;
  return (
      (u8 *)(map->storage[bucket].ptr) +
      (map->valsize + map->keysize) * index
  );
}

static inline umax HMap_hash(const fptr str) {
  umax hash = 5381;

  if (str.len == sizeof(umax)) {
    memcpy(&hash, str.ptr, sizeof(hash));
  } else {
    usize i = 0;
    for (; i + sizeof(umax) <= str.len; i += sizeof(umax)) {
      u64 part;
      memcpy(&part, str.ptr + i, sizeof(part));

      hash ^= part;
      hash *= 0xff51afd7ed558ccdULL;
      hash ^= hash >> 33;
    }

    for (; i < str.len; i++) {
      hash ^= str.ptr[i];
      hash *= 0x100000001b3ULL;
    }
  }

  hash ^= hash >> 33;
  hash *= 0xff51afd7ed558ccdULL;
  hash ^= hash >> 33;

  return hash;
}

HMap *HMap_new(u32 kSize, u32 vSize, AllocatorV allocator, const usize metaSize) {
  assertMessage(kSize && vSize && metaSize && allocator);
  usize totalSize = sizeof(HMap) + metaSize * sizeof(HMap_LesserList);
  HMap *hm = (typeof(hm))aAlloc(allocator, totalSize);
  *hm = (HMap){
      .allocator = allocator,
      .keysize = kSize,
      .valsize = vSize,
      .metaSize = metaSize,
  };
  memset(hm->storage, 0, metaSize * sizeof(*hm->storage));
  return hm;
}

void HMap_free(HMap *hm) {
  AllocatorV allocator = hm->allocator;
  const u32 msize = hm->metaSize;
  for (int i = 0; i < msize; i++)
    if (hm->storage[i].ptr)
      aFree(allocator, hm->storage[i].ptr);
  aFree(allocator, hm);
}

void *LesserList_getref(usize elw, const HMap_LesserList *hll, u32 idx) {
  return (u8 *)(hll->ptr) + idx * (elw);
}
void LesserList_appendGarbage(usize elw, HMap_LesserList *hll, AllocatorV allocator) {
  if (!hll->capacity) {
    hll->ptr = aAlloc(allocator, elw);
    if (allocator->size)
      hll->capacity = allocator->size(allocator, hll->ptr) / elw;
    else
      hll->capacity = 1;
  }
  if (hll->capacity < hll->length + 1) {
    hll->ptr = aResize(allocator, hll->ptr, hll->capacity * 2 * elw);
    if (allocator->size)
      hll->capacity = allocator->size(allocator, hll->ptr) / elw;
    else
      hll->capacity *= 2;
  }
  hll->length++;
}
void HMap_set(HMap *map, const void *key, const void *val) {
  usize elw = map->keysize + map->valsize;
  if (!key)
    return;
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  HMap_LesserList *hll = &(map->storage[lindex]);
  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, listindex);
    if (!memcmp(place, key, map->keysize))
      break;
  }
  if (listindex == hll->length) {
    LesserList_appendGarbage(elw, hll, map->allocator);
    place = (u8 *)LesserList_getref(elw, hll, hll->length - 1);
  }
  memcpy(place, key, map->keysize);
  if (val)
    memcpy(place + map->keysize, val, map->valsize);
  else {
    memmove(place, place + elw, (hll->length - listindex - 1) * elw);
    hll->length--;
  }
}
void HMap_transform(HMap **last, usize kSize, usize vSize, AllocatorV allocator, u32 metaSize) {
  assertMessage(last && *last);

  HMap *oldMap = *last;
  if (kSize == 0)
    kSize = oldMap->keysize;
  if (vSize == 0)
    vSize = oldMap->valsize;
  if (allocator == nullptr)
    allocator = oldMap->allocator;
  if (metaSize == 0)
    metaSize = oldMap->metaSize;
  HMap *newMap = HMap_new(kSize, vSize, allocator, metaSize);

  // u8 tempKey[newMap->keysize > oldMap->keysize ? newMap->keysize : oldMap->keysize];
  // u8 tempVal[newMap->valsize > oldMap->valsize ? newMap->valsize : oldMap->valsize];

  auto tempKey = aCreate(allocator, u8, newMap->keysize > oldMap->keysize ? newMap->keysize : oldMap->keysize);
  defer { aFree(allocator, tempKey); };
  auto tempVal = aCreate(allocator, u8, newMap->valsize > oldMap->valsize ? newMap->valsize : oldMap->valsize);
  defer { aFree(allocator, tempVal); };

  usize keyCopySize = (oldMap->keysize < kSize) ? oldMap->keysize : kSize;
  usize valCopySize = (oldMap->valsize < vSize) ? oldMap->valsize : vSize;

  for (usize i = 0; i < oldMap->metaSize; i++) {
    for (usize j = 0; j < HMap_getBucketSize(oldMap, i); j++) {
      u8 *oldKey = (u8 *)HMap_getCoord(oldMap, i, j);
      u8 *oldVal = oldKey + oldMap->keysize;
      if (!oldKey)
        continue;
      memset(tempKey, 0, kSize);
      memcpy(tempKey, oldKey, keyCopySize);
      memset(tempVal, 0, vSize);
      memcpy(tempVal, oldVal, valCopySize);

      HMap_set(newMap, tempKey, tempVal);
    }
  }

  HMap_free(oldMap);

  *last = newMap;
}

void *HMap_get(const HMap *map, const void *key) {
  if (!key) {
    return nullptr;
  }
  u32 lindex = (u32)(HMap_hash(fptr_fromPL(key, map->keysize)) % map->metaSize);
  usize elw = map->keysize + map->valsize;
  const HMap_LesserList *hll = &(map->storage[lindex]);

  if (!hll->ptr)
    return nullptr;
  u32 listindex = 0;
  u8 *place = nullptr;
  for (; listindex < hll->length; listindex++) {
    place = (u8 *)LesserList_getref(elw, hll, listindex);
    if (!memcmp(place, key, map->keysize)) {
      break;
    }
  }
  if (listindex == hll->length)
    return nullptr;
  return place + map->keysize;
}

inline void *HMap_getKey(const HMap *map, u32 n) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize && i <= n;) {
    if (i + map->storage[lindex].length > n) {
      usize s = map->keysize + map->valsize;
      usize i2 = n - i;
      return (u8 *)map->storage[lindex].ptr + s * i2;
    }
    i += map->storage[lindex].length;
    lindex++;
  }
  return nullptr;
}

inline void *HMap_getVal(const HMap *map, u32 n) {
  u8 *place = (u8 *)HMap_getKey(map, n);
  if (place)
    return place + map->keysize;
  return nullptr;
}

u32 HMap_count(const HMap *map) {
  u32 i = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    i += map->storage[lindex].length;
    lindex++;
  }
  return i;
}

u32 HMap_countCollisions(const HMap *map) {
  u32 collisions = 0;
  u32 lindex = 0;
  for (; lindex < map->metaSize;) {
    if (map->storage[lindex].length)
      collisions += map->storage[lindex].length - 1;
    lindex++;
  }
  return collisions;
}

usize HMap_footprint(const HMap *map) {
  usize res = 0;
  usize elementSize = map->keysize + map->valsize;

  res += sizeof(HMap);
  res += sizeof(*(map->storage)) * map->metaSize;
  res += elementSize;
  for (u32 lindex = 0; lindex < map->metaSize; lindex++)
    if (map->storage[lindex].length)
      res += (elementSize)*map->storage[lindex].capacity;
  return res;
}
bool HMap_getSet(HMap *map, const void *key, void *val) {
  assertMessage(key && val);

  void *result = HMap_get(map, key);
  if (result) {
    memcpy(val, result, map->valsize);
    return true;
  }
  return false;
}
void HMap_fset(HMap *map, const fptr key, void *val) {
  assertMessage(key.len <= HMap_getKeySize(map));
  if (key.len == map->keysize)
    return HMap_set(map, key.ptr, val);
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  return HMap_set(map, nname, val);
}
bool HMap_fget(HMap *map, const fptr key, void *val) {
  assertMessage(key.len <= HMap_getKeySize(map));
  if (key.len == map->keysize)
    return HMap_get(map, key.ptr);
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  void *res = HMap_get(map, nname);
  if (!res)
    return false;
  memcpy(val, res, map->valsize);
  return true;
}
void *HMap_fget_ns(HMap *map, const fptr key) {
  assertMessage(key.len <= HMap_getKeySize(map));
  u8 nname[map->keysize];
  memcpy(nname, key.ptr, key.len);
  memset(nname + key.len, 0, HMap_getKeySize(map) - key.len);
  return HMap_get(map, nname);
}
HMap_both HMap_getBoth(HMap *map, const void *key) {
  u8 *place = (u8 *)HMap_get(map, key);
  if (!place)
    return (HMap_both){nullptr, nullptr};
  return (HMap_both){place - map->keysize, place};
}

void HMap_clear(HMap *map) {
  for (u32 i = 0; i < map->metaSize; i++)
    map->storage[i].length = 0;
}
