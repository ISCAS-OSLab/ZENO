// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include "defs.h"
#include "util.h"

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/gcm.h>

#define KEY_SIZE 32 // AES-256
#define IV_SIZE 12
#define TAG_SIZE 16

#include <assert.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <stdbool.h>
#include <string.h>

/* FIXME: remember to replace this with your real key */
const uint8_t enc_key[KEY_SIZE] = {
    0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12
}; // TA_DERIVED_KEY_MAX_SIZE

static __thread mbedtls_gcm_context aes;
static __thread uint8_t iv[IV_SIZE] = {0};

static inline void randombytes(uint8_t *buf, size_t sz)
{
    int res = 0;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    res = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0);
    if (res != 0)
        fprintf(stderr, "%s: %d\n", __func__, res);

    res = mbedtls_ctr_drbg_random(&ctr_drbg, buf, sz);
    if (res != 0)
        fprintf(stderr, "%s: %d\n", __func__, res);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

static inline void increment(unsigned char *iv, int size)
{
    for (int i = size - 1; i >= 0; i--) {
        if (iv[i] == 0xFF) {
            iv[i] = 0x00;
        } else {
            iv[i]++;
            break;
        }
    }
}

void gcm_init(void)
{
    int res = 0;

    // init the context...
    mbedtls_gcm_init(&aes);

    // an alternative is to use salt + counter per session
    randombytes(iv, IV_SIZE);

    // Set the key. This next line could have CAMELLIA or ARIA as our GCM mode cipher...
    res = mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES, (const unsigned char *)enc_key, sizeof(enc_key) * 8);
    if (res != 0)
        fprintf(stderr, "%s: %d\n", __func__, res);
}

/* encryption should gurantee that ciphertext locate at the end of out_text.
to allow for cutting the data size short. */
/* IV + TAG + cihpertext */
int gcm_encrypt(uint8_t *in, uint64_t in_sz, uint8_t *out, uint64_t *out_sz)
{
    int res = 0;

    uint8_t *iv_pos = out;
    uint8_t *tag_pos = out + IV_SIZE;
    uint8_t *data_pos = out + IV_SIZE + TAG_SIZE;

    increment(iv, IV_SIZE);
    memcpy(iv_pos, iv, IV_SIZE);

    // Initialise the GCM cipher...
    res = mbedtls_gcm_crypt_and_tag(&aes, MBEDTLS_GCM_ENCRYPT, in_sz, iv_pos, IV_SIZE, NULL, 0, in, data_pos, TAG_SIZE, tag_pos);
    if (res != 0)
        fprintf(stderr, "%s: %d\n", __func__, res);
    *out_sz = in_sz + IV_SIZE + TAG_SIZE;
    return res;
}

int gcm_decrypt(uint8_t *in, uint64_t in_sz, uint8_t *out, uint64_t *out_sz)
{
    int res = 0;

    uint8_t *iv_pos = in;
    uint8_t *tag_pos = in + IV_SIZE;
    uint8_t *data_pos = in + IV_SIZE + TAG_SIZE;

    uint32_t data_sz = in_sz - IV_SIZE - TAG_SIZE;
    *out_sz = data_sz;

    res = mbedtls_gcm_auth_decrypt(&aes, data_sz, iv_pos, IV_SIZE, NULL, 0, tag_pos, TAG_SIZE, data_pos, out);
    if (res != 0)
        fprintf(stderr, "%s: %d\n", __func__, res);
    return res;
}

extern "C"
int encrypt_bytes(uint8_t *pSrc, uint64_t src_len, uint8_t *pDst, uint64_t exp_dst_len)
{
    uint64_t dst_len = exp_dst_len;
    int resp = 0;

    resp = gcm_encrypt(pSrc, src_len, pDst, &dst_len);
    KVDB_CHECK(dst_len == exp_dst_len && resp == 0,
        "encryption failed: resp=%d dst_len=%lu expected=%lu",
        resp, (unsigned long)dst_len, (unsigned long)exp_dst_len);
    return resp;
}

extern "C"
int decrypt_bytes(uint8_t *pSrc, uint64_t src_len, uint8_t *pDst, uint64_t exp_dst_len)
{
    uint64_t dst_len = exp_dst_len;
    int resp = 0;

    KVDB_CHECK(src_len >= IV_SIZE + TAG_SIZE,
        "ciphertext too short: src_len=%lu min=%d",
        (unsigned long)src_len, IV_SIZE + TAG_SIZE);
    resp = gcm_decrypt(pSrc, src_len, pDst, &dst_len);
    KVDB_CHECK(dst_len == exp_dst_len && resp == 0,
        "decryption failed: resp=%d dst_len=%lu expected=%lu",
        resp, (unsigned long)dst_len, (unsigned long)exp_dst_len);
    return resp;
}
