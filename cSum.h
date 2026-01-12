#ifndef MY_CSUM_H
#define MY_CSUM_H
#include "allocator.h"
#include "fptr.h"
#include "my-list.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#define CSUM_START_BIT ((uint8_t)0x67)
#define CSUM_END_BIT ((uint8_t)0x41)
// clang-format off
#ifndef cSum_REDUNDANCY_AMMOUNT
  #define cSum_REDUNDANCY_AMMOUNT ((unsigned int)4) // cant be 0
#endif
// clang-format on

#define CHECK_TYPE uint32_t
static inline CHECK_TYPE cSum_CHECK_EXPR(CHECK_TYPE val, uint8_t data) {
  uint16_t top = (val & 0xffff0000) >> 16;
  top += data;
  uint16_t bot = val & 0x0000ffff;
  bot ^= data;
  bot ^= top;
  return ((uint32_t)top) << 16 | bot;
}

// start_bit .. data .. data .. checksum .. end_bit
typedef struct
{
  List *checkSumScratch;
} dataChecker;

typedef struct
{
  um_fp data;
} checkData;

static dataChecker cSum_new(AllocatorV allocator) {
  printf("new csum item with  %ix redundancy\n", cSum_REDUNDANCY_AMMOUNT);
  List *l = List_new(allocator, sizeof(uint8_t));
  List_resize(l, 20);
  return (dataChecker){.checkSumScratch = l};
}
static inline void cSum_free(dataChecker a) { List_free(a.checkSumScratch); }

static inline unsigned int csum_expectedLength(int messageLength) {
  return sizeof(CSUM_START_BIT) + sizeof(CSUM_END_BIT) + sizeof(CHECK_TYPE) +
         (messageLength * cSum_REDUNDANCY_AMMOUNT);
}

static checkData cSum_toSum(dataChecker d, um_fp data) {
  CHECK_TYPE sum = 0;
  for (unsigned int i = 0; i < data.width; i++) {
    sum = cSum_CHECK_EXPR(sum, ((uint8_t *)data.ptr)[i]);
  }
  d.checkSumScratch->length = 0;
  unsigned int newWidth = sizeof(CSUM_START_BIT) + sizeof(CSUM_END_BIT) +
                          sizeof(CHECK_TYPE) +
                          (data.width * cSum_REDUNDANCY_AMMOUNT);
  if (newWidth > d.checkSumScratch->width)
    List_resize(d.checkSumScratch, newWidth);

  uint8_t tmp[1] = {(uint8_t)CSUM_START_BIT};

  List_append(d.checkSumScratch, tmp);

  for (unsigned int i = 0; i < cSum_REDUNDANCY_AMMOUNT; i++)
    List_appendFromArr(d.checkSumScratch, data.ptr, data.width);

  List_appendFromArr(d.checkSumScratch, &sum, sizeof(CHECK_TYPE));

  tmp[0] = (uint8_t)CSUM_END_BIT;
  List_append(d.checkSumScratch, tmp);

  checkData res = {.data = {
                       .ptr = d.checkSumScratch->head,
                       .width = d.checkSumScratch->length,
                   }};
  return res;
}
static um_fp cSum_fromSum(checkData data) {
  // char *ptr1, *ptr2;
  uint8_t *ptrs[cSum_REDUNDANCY_AMMOUNT];
  CHECK_TYPE check;
  CHECK_TYPE checkResult = 0;
  ptrs[0] = (uint8_t *)data.data.ptr;
  if (ptrs[0][0] != CSUM_START_BIT) {
    return nullUmf;
  }
  if (ptrs[0][data.data.width - 1] != CSUM_END_BIT) {
    return nullUmf;
  }
  unsigned int dataLength = data.data.width;
  dataLength -= (sizeof(uint8_t) + sizeof(CHECK_TYPE));

  check = *(CHECK_TYPE *)((uint8_t *)data.data.ptr + dataLength);

  dataLength -= sizeof(uint8_t);
  dataLength /= cSum_REDUNDANCY_AMMOUNT;
  ptrs[0] += sizeof(uint8_t);
  for (unsigned int i = 1; i < cSum_REDUNDANCY_AMMOUNT; i++)
    ptrs[i] = ptrs[0];

  // ptr2 = ptr1 = ptr1 + sizeof(char);

  for (unsigned int i = 1; i < cSum_REDUNDANCY_AMMOUNT; i++)
    ptrs[i] += dataLength * i;
  // ptr2 += dataLength;

  for (unsigned int i = 0; i < dataLength; i++) {
    uint8_t c = ptrs[0][i];
    for (unsigned int ii = 0; ii < cSum_REDUNDANCY_AMMOUNT; ii++)
      if (ptrs[ii][i] != c)
        return nullFptr;
    checkResult = cSum_CHECK_EXPR(checkResult, c);
  }
  if (checkResult != check) {
    return nullUmf;
  }
  return (um_fp){
      .width = dataLength,
      .ptr = ptrs[0],
  };
}
#undef CHECK_EXPR
#undef CHECK_TYPE
#undef cSum_REDUNDANCY_AMMOUNT
#endif
