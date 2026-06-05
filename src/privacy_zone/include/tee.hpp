#pragma once

#include "kv.hpp"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <assert.h>

#include "rid.h"
#include <request_types.h>

void gcm_init(void);
int gcm_encrypt(uint8_t* in, uint64_t in_sz, uint8_t* out, uint64_t* out_sz);
int gcm_decrypt(uint8_t* in, uint64_t in_sz, uint8_t* out, uint64_t* out_sz);

#ifdef __cplusplus
extern "C" {
#endif

int encrypt_bytes(uint8_t* pSrc, uint64_t src_len, uint8_t* pDst, uint64_t exp_dst_len);
int decrypt_bytes(uint8_t* pSrc, uint64_t src_len, uint8_t* pDst, uint64_t exp_dst_len);

int handle_ops(BaseRequest* req);

RID kv_int_add(RID left, RID right);
RID kv_int_sub(RID left, RID right);
RID kv_int_mult(RID left, RID right);
RID kv_int_div(RID left, RID right);
RID kv_int_pow(RID left, RID right);
RID kv_int_mod(RID left, RID right);
RID kv_int_sum_bulk(size_t bulk_size, RID *bulk_data);
int kv_int_cmp(RID left, RID right);

RID kv_float_add(RID left, RID right);
RID kv_float_sub(RID left, RID right);
RID kv_float_mult(RID left, RID right);
RID kv_float_div(RID left, RID right);
RID kv_float_pow(RID left, RID right);
RID kv_float_mod(RID left, RID right);
RID kv_float_sum_bulk(size_t bulk_size, RID *bulk_data);
int kv_float_cmp(RID left, RID right);

RID kv_text_concat(RID left, RID right);
RID kv_text_substr(RID str, RID begin, RID length);
int kv_text_cmp(RID left, RID right);
int kv_text_like(RID str, RID pattern);

RID kv_timestamp_extract_year(RID in);
int kv_timestamp_cmp(RID left, RID right);

#ifdef __cplusplus
}
#endif
