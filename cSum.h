#ifndef MY_CSUM_H
#define MY_CSUM_H
#include "allocator.h"
#include "fptr.h"
#include "my-list.h"
#include <stdint.h>
#include <stdio.h>
constexpr u8 CSUM_START_BIT = 0x67;
constexpr u8 CSUM_END_BIT = 0x41;
// clang-format off
#ifndef cSum_REDUNDANCY_AMMOUNT
  #define cSum_REDUNDANCY_AMMOUNT ((unsigned int)4) // cant be 0
#endif
// clang-format on

typedef u32 CHECK_TYPE;
static inline CHECK_TYPE cSum_CHECK_EXPR(CHECK_TYPE val, u8 data) {
  u16 top = (u16)(val >> 16);
  top += data;
  u16 bot = (u16)val;
  bot ^= data;
  bot ^= top;
  return ((u32)top) << 16 | bot;
}

typedef struct {
  mList(u8) checkSumScratch;
} dataChecker;

typedef struct {
  fptr data;
} checkData;

static dataChecker cSum_new(AllocatorV allocator) {
  return (dataChecker){
      .checkSumScratch = mList_init(allocator, u8, 20),
  };
}
static inline void cSum_free(dataChecker a) { mList_deInit(a.checkSumScratch); }

static inline unsigned int csum_expectedLength(int messageLength) {
  return sizeof(CSUM_START_BIT) + sizeof(CSUM_END_BIT) + sizeof(CHECK_TYPE) +
         (messageLength * cSum_REDUNDANCY_AMMOUNT);
}
static checkData cSum_toSum(dataChecker d, fptr data) {
  CHECK_TYPE sum = 0;
  for (unsigned int i = 0; i < data.width; i++) {
    sum = cSum_CHECK_EXPR(sum, ((u8 *)data.ptr)[i]);
  }
  mList_len(d.checkSumScratch) = 0;
  unsigned int newWidth = csum_expectedLength(data.width);

  if (newWidth > mList_len(d.checkSumScratch))
    mList_reserve(d.checkSumScratch, newWidth);

  u8 tmp[1] = {(uint8_t)CSUM_START_BIT};

  mList_push(d.checkSumScratch, *tmp);
  for (unsigned int i = 0; i < cSum_REDUNDANCY_AMMOUNT; i++)
    List_appendFromArr((List *)d.checkSumScratch, data.ptr, data.width);

  List_appendFromArr((List *)d.checkSumScratch, &sum, sizeof(CHECK_TYPE));

  tmp[0] = (u8)CSUM_END_BIT;
  mList_push(d.checkSumScratch, *tmp);

  checkData res = {
      .data = {
          .width = mList_len(d.checkSumScratch),
          .ptr = mList_arr(d.checkSumScratch),
      },
  };
  return res;
}
static fptr cSum_fromSum_helper(checkData data) {
  u8 *ptrs[cSum_REDUNDANCY_AMMOUNT];
  CHECK_TYPE check;
  CHECK_TYPE checkResult = 0;
  ptrs[0] = (u8 *)data.data.ptr;
  if (ptrs[0][0] != CSUM_START_BIT)
    return nullFptr;

  if (ptrs[0][data.data.width - 1] != CSUM_END_BIT)
    return nullFptr;

  unsigned int dataLength = data.data.width;
  dataLength -= (sizeof(u8) + sizeof(CHECK_TYPE));

  check = *(CHECK_TYPE *)((u8 *)data.data.ptr + dataLength);

  dataLength -= sizeof(u8);
  dataLength /= cSum_REDUNDANCY_AMMOUNT;
  ptrs[0] += sizeof(u8);
  for (unsigned int i = 1; i < cSum_REDUNDANCY_AMMOUNT; i++)
    ptrs[i] = ptrs[0];

  // ptr2 = ptr1 = ptr1 + sizeof(char);

  for (unsigned int i = 1; i < cSum_REDUNDANCY_AMMOUNT; i++)
    ptrs[i] += dataLength * i;
  // ptr2 += dataLength;

  for (unsigned int i = 0; i < dataLength; i++) {
    u8 c = ptrs[0][i];
    for (unsigned int ii = 0; ii < cSum_REDUNDANCY_AMMOUNT; ii++)
      if (ptrs[ii][i] != c)
        return nullFptr;
    checkResult = cSum_CHECK_EXPR(checkResult, c);
  }
  if (checkResult != check)
    return nullFptr;

  return (fptr){
      .width = dataLength,
      .ptr = ptrs[0],
  };
}
static fptr cSum_fromSum(checkData data) {
  for (auto i = 0; i < data.data.width; i++) {
    if (data.data.ptr[i] == CSUM_END_BIT) {
      checkData dd = data;
      dd.data.width = i + 1;
      fptr r = cSum_fromSum_helper(dd);
      if (r.ptr)
        return r;
    }
  }
  return nullFptr;
}

#undef CHECK_TYPE
#endif
