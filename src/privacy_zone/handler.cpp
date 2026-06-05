// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include "tee.hpp"
#include <rpc.h>
#include <cstddef>
#include <time.h>

int handle_ops(BaseRequest* base_req)
{
    // printf("\nops: %d", base_req->reqType);
    auto calc_req = (CalcRequestData *)base_req;
    auto cmp_req = (CmpRequestData *)base_req;
    auto bulk_req = (BulkRequestData *)base_req;
    auto tsey_req = (TimestampExtractYearRequestData *)base_req;
    auto subs_req = (SubstringRequestData *)base_req;
    auto put_req = (KVRequestData *)base_req;
    auto get_req = (KVRequestData *)base_req;
    auto batch_req = (BatchRequestData *)base_req;
    auto pmt_req = (PromoteRequestData *)base_req;
#ifdef USE_PG_WAL_FLUSH_HOOK
    auto wal_batch_req = (WalAppendBatchRequestData *)base_req;
    auto wal_materialize_req = (WalMaterializeBatchRequestData *)base_req;
    auto wal_replay_req = (WalReplayPayloadRequestData *)base_req;
#endif

    switch (base_req->reqType) {
    #ifdef USE_LOCAL_STORE_CLEAR
    case CMD_LOCAL_CLEAR:
        localkv_ops.clear();
        break;
    #endif

    #ifdef USE_PG_WAL_FLUSH_HOOK
    case CMD_TRUNCATE: {
        truncate();
        break;
    }
    case CMD_REPLAY:
        replay_log();
        break;
    case CMD_FLUSH: {
        flush();
        break;
    }
    case CMD_CKPT: {
        kv_ops.flush_async();
        break;
    }
    case CMD_WAL_APPEND_BATCH: {
        KVDB_CHECK(wal_batch_req->nitems >= 0 && wal_batch_req->nitems <= WAL_RID_BATCH_SIZE,
            "invalid WAL RID batch size: %d", wal_batch_req->nitems);
        wal_ops.appendBatch(wal_batch_req->items, wal_batch_req->nitems);
        break;
    }
    case CMD_WAL_MATERIALIZE_BATCH: {
        KVDB_CHECK(wal_materialize_req->nitems >= 0 &&
                   wal_materialize_req->nitems <= WAL_MATERIALIZE_RID_BATCH_SIZE,
            "invalid materialize WAL RID batch size: %d", wal_materialize_req->nitems);
        uint64_t payload_capacity = REQ_REGION_SIZE - offsetof(WalMaterializeBatchRequestData, payload);
        wal_materialize_req->payload_len = wal_ops.materializeBatch(wal_materialize_req->items,
            wal_materialize_req->nitems, wal_materialize_req->payload, payload_capacity);
        break;
    }
    case CMD_WAL_REPLAY_PAYLOAD: {
        uint64_t payload_capacity = REQ_REGION_SIZE - offsetof(WalReplayPayloadRequestData, payload);
        KVDB_CHECK(wal_replay_req->payload_len <= payload_capacity,
            "invalid WAL replay payload size: %lu capacity=%lu",
            (unsigned long)wal_replay_req->payload_len, (unsigned long)payload_capacity);
        wal_ops.replayPayload(wal_replay_req->payload, wal_replay_req->payload_len);
        break;
    }
    #else
    case CMD_TRUNCATE:
    case CMD_REPLAY:
    case CMD_FLUSH:
    case CMD_WAL_APPEND_BATCH:
    case CMD_WAL_MATERIALIZE_BATCH:
    case CMD_WAL_REPLAY_PAYLOAD:
        KVDB_FATAL("WAL request %d received but USE_PG_WAL_FLUSH_HOOK is disabled", base_req->reqType);
    #endif

    /* Calculate requests */
    case CMD_INT_PLUS:
        calc_req->res = kv_int_add(calc_req->left, calc_req->right);
        break;
    case CMD_INT_MINUS:
        calc_req->res = kv_int_sub(calc_req->left, calc_req->right);
        break;
    case CMD_INT_MULT:
        calc_req->res = kv_int_mult(calc_req->left, calc_req->right);
        break;
    case CMD_INT_DIV:
        calc_req->res = kv_int_div(calc_req->left, calc_req->right);
        break;
    case CMD_INT_POW:
        calc_req->res = kv_int_pow(calc_req->left, calc_req->right);
        break;
    case CMD_INT_MOD:
        calc_req->res = kv_int_mod(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_PLUS:
        calc_req->res = kv_float_add(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_MINUS:
        calc_req->res = kv_float_sub(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_MULT:
        calc_req->res = kv_float_mult(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_DIV:
        calc_req->res = kv_float_div(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_POW:
        calc_req->res = kv_float_pow(calc_req->left, calc_req->right);
        break;
    case CMD_FLOAT_MOD:
        calc_req->res = kv_float_mod(calc_req->left, calc_req->right);
        break;
    case CMD_TEXT_CONCAT:
        calc_req->res = kv_text_concat(calc_req->left, calc_req->right);
        break;

    /* Comparison requests */
    case CMD_INT_CMP:
        cmp_req->cmp = kv_int_cmp(cmp_req->left, cmp_req->right);
        break;
    case CMD_FLOAT_CMP:
        cmp_req->cmp = kv_float_cmp(cmp_req->left, cmp_req->right);
        break;
    case CMD_TIMESTAMP_CMP:
        cmp_req->cmp = kv_timestamp_cmp(cmp_req->left, cmp_req->right);
        break;
    case CMD_TEXT_LIKE: // like use cmp data, because return boolean value.
        cmp_req->cmp = kv_text_like(cmp_req->left, cmp_req->right);
        break;
    case CMD_TEXT_CMP:
        cmp_req->cmp = kv_text_cmp(cmp_req->left, cmp_req->right);
        break;

    /* Bulk requests */
    case CMD_INT_SUM_BULK:
        bulk_req->res = kv_int_sum_bulk(bulk_req->bulk_size, bulk_req->keys);
        break;
    case CMD_FLOAT_SUM_BULK:
        bulk_req->res = kv_float_sum_bulk(bulk_req->bulk_size, bulk_req->keys);
        break;


    /* Special requests */
    case CMD_TIMESTAMP_EXTRACT_YEAR:
        tsey_req->res = kv_timestamp_extract_year(tsey_req->in);
        break;
    case CMD_TEXT_SUBSTRING:
        subs_req->res = kv_text_substr(subs_req->str, subs_req->start, subs_req->length);
        break;


    /* Put and get requests */
    case CMD_INT_PUT_ENC: {
        auto put_enc_req = (EncIntPutEncIntoKVRequestData *)base_req;
        int value;
        decrypt_bytes((uint8_t*)&put_enc_req->enc, sizeof(put_enc_req->enc), (uint8_t *)&value, sizeof(value));
#ifdef USE_KVMAP_PARTITION
        put_enc_req->res = putInt(INVALID_RID, value);
#else
        put_enc_req->res = putInt(INVALID_RID, value, false);
#endif
        break;
    }
    case CMD_FLOAT_PUT_ENC: {
        auto put_enc_req = (EncFloatPutEncIntoKVRequestData *)base_req;
        float value;
        decrypt_bytes((uint8_t*)&put_enc_req->enc, sizeof(put_enc_req->enc), (uint8_t *)&value, sizeof(value));
#ifdef USE_KVMAP_PARTITION
        put_enc_req->res = putFloat(INVALID_RID, value);
#else
        put_enc_req->res = putFloat(INVALID_RID, value, false);
#endif
        break;
        }
    case CMD_TIMESTAMP_PUT_ENC: {
        auto put_enc_req = (EncTimestampPutEncIntoKVRequestData *)base_req;
        TIMESTAMP value;
        decrypt_bytes((uint8_t*)&put_enc_req->enc, sizeof(put_enc_req->enc), (uint8_t *)&value, sizeof(value));
#ifdef USE_KVMAP_PARTITION
        put_enc_req->res = putTimestamp(INVALID_RID, value);
#else
        put_enc_req->res = putTimestamp(INVALID_RID, value, false);
#endif
        break;
        }
    case CMD_STRING_PUT_ENC: {
        auto put_enc_req = (EncStrPutEncIntoKVRequestData *)base_req;
        Str plaintext;
        plaintext.len = put_enc_req->enc.len - IV_SIZE - TAG_SIZE;
        decrypt_bytes((uint8_t*)&put_enc_req->enc.enc_cstr, put_enc_req->enc.len,
            (uint8_t*)&plaintext.data, plaintext.len);
        plaintext.data[plaintext.len] = '\0';
#ifdef USE_KVMAP_PARTITION
        put_enc_req->res = putText(INVALID_RID, (char *)(plaintext.data));
#else
        put_enc_req->res = putText(INVALID_RID, (char *)(plaintext.data), false);
#endif
        break;
        }
    case CMD_INT_PUT: 
        put_req->key = putInt(INVALID_RID, *((int *)(put_req->value)), false);
        break;
    case CMD_FLOAT_PUT: 
        put_req->key = putFloat(INVALID_RID, *((float *)(put_req->value)), false);
        break;
    case CMD_TIMESTAMP_PUT: 
        put_req->key = putTimestamp(INVALID_RID, *((TIMESTAMP *)(put_req->value)), false);
        break;
    case CMD_TEXT_PUT: 
        put_req->key = putText(INVALID_RID, (char *)(put_req->value), false);
        break;

    case CMD_INT_GET_ENC: {
        auto get_enc_req = (EncIntGetEncFromKVRequestData *)base_req;
        int value = getInt(get_enc_req->dec);
        encrypt_bytes((uint8_t*)&value, sizeof(value),
            (uint8_t*)&get_enc_req->res, sizeof(get_enc_req->res));
        break;
        }
    case CMD_FLOAT_GET_ENC: {
        auto get_enc_req = (EncFloatGetEncFromKVRequestData *)base_req;
        float value = getFloat(get_enc_req->dec);
        encrypt_bytes((uint8_t*)&value, sizeof(value),
            (uint8_t*)&get_enc_req->res, sizeof(get_enc_req->res));
        break;
        }
    case CMD_TIMESTAMP_GET_ENC: {
        auto get_enc_req = (EncTimestampGetEncFromKVRequestData *)base_req;
        TIMESTAMP value = getTimestamp(get_enc_req->dec);
        encrypt_bytes((uint8_t*)&value, sizeof(value),
            (uint8_t*)&get_enc_req->res, sizeof(get_enc_req->res));
        break;
        }
    case CMD_STRING_GET_ENC: {
        auto get_enc_req = (EncStrGetEncFromKVRequestData *)base_req;
        Str plaintext;
        auto tmp = getText(get_enc_req->dec);
        memcpy(plaintext.data, tmp, strlen(tmp) + 1);
        plaintext.len = strlen((char *)(plaintext.data));
        get_enc_req->res.len = plaintext.len + IV_SIZE + TAG_SIZE;
        encrypt_bytes((uint8_t*)&plaintext.data, plaintext.len,
            (uint8_t*)&get_enc_req->res.enc_cstr, get_enc_req->res.len);
        break;
        }
    case CMD_INT_GET: 
        *((int *)(get_req->value)) = getInt(get_req->key);
        break;
    case CMD_FLOAT_GET: 
        *((float *)(get_req->value)) = getFloat(get_req->key);
        break;
    case CMD_TIMESTAMP_GET: 
        *((TIMESTAMP *)(get_req->value)) = getTimestamp(get_req->key);
        break;
    case CMD_TEXT_GET: 
        memcpy(get_req->value, getText(get_req->key), STRING_LENGTH);
        break;

    case CMD_BATCH_GET:
    case CMD_BATCH_PUT: {
        auto buf_p = (uint8_t *)(batch_req->buf);
        for (int i = 0; i < batch_req->nreqs; i ++) {
            auto req = (KVRequestData *)buf_p;
            KVDB_CHECK(req->size != 0, "empty request in batch at index %d", i);
            handle_ops((BaseRequest *)req);
            buf_p += req->size;
        }
        break;
    }

    case CMD_INT_ENC: {
        EncIntEncRequestData* req = (EncIntEncRequestData*)base_req;
        base_req->resp = encrypt_bytes((uint8_t*)&req->plaintext, sizeof(req->plaintext),
            (uint8_t*)&req->ciphertext, sizeof(req->ciphertext));
        break;
    }
    case CMD_INT_DEC: {
        EncIntDecRequestData* req = (EncIntDecRequestData*)base_req;
        base_req->resp = decrypt_bytes((uint8_t*)&req->ciphertext, sizeof(req->ciphertext),
            (uint8_t*)&req->plaintext, sizeof(req->plaintext));
        break;
    }
    case CMD_FLOAT_ENC: {
        EncFloatEncRequestData* req = (EncFloatEncRequestData*)base_req;
        req->common.resp = encrypt_bytes((uint8_t*)&req->plaintext, sizeof(req->plaintext),
            (uint8_t*)&req->ciphertext, sizeof(req->ciphertext));
        break;
    }
    case CMD_FLOAT_DEC: {
        EncFloatDecRequestData* req = (EncFloatDecRequestData*)base_req;
        req->common.resp = decrypt_bytes((uint8_t*)&req->ciphertext, sizeof(req->ciphertext),
            (uint8_t*)&req->plaintext, sizeof(req->plaintext));
        break;
    }
    case CMD_TIMESTAMP_ENC: {
        EncTimestampEncRequestData* req = (EncTimestampEncRequestData*)base_req;
        req->common.resp = encrypt_bytes((uint8_t*)&req->plaintext, sizeof(req->plaintext),
            (uint8_t*)&req->ciphertext, sizeof(req->ciphertext));
        break;
    }
    case CMD_TIMESTAMP_DEC: {
        EncTimestampDecRequestData* req = (EncTimestampDecRequestData*)base_req;
        req->common.resp = decrypt_bytes((uint8_t*)&req->ciphertext, sizeof(req->ciphertext),
            (uint8_t*)&req->plaintext, sizeof(req->plaintext));
        break;
    }
    case CMD_STRING_ENC: {
        EncStrEncRequestData* req = (EncStrEncRequestData*)base_req;
        KVDB_CHECK(req->plaintext.len < STRING_LENGTH,
            "plaintext string too long: %u >= %u", req->plaintext.len, STRING_LENGTH);
        req->ciphertext.len = req->plaintext.len + IV_SIZE + TAG_SIZE;
        req->common.resp = encrypt_bytes((uint8_t*)&req->plaintext.data, req->plaintext.len,
            (uint8_t*)&req->ciphertext.enc_cstr, req->ciphertext.len);
        break;
    }
    case CMD_STRING_DEC: {
        EncStrDecRequestData* req = (EncStrDecRequestData*)base_req;
        KVDB_CHECK(req->ciphertext.len >= IV_SIZE + TAG_SIZE &&
                   req->ciphertext.len < STRING_LENGTH + IV_SIZE + TAG_SIZE,
            "invalid ciphertext string length: %u", req->ciphertext.len);
        req->plaintext.len = req->ciphertext.len - IV_SIZE - TAG_SIZE;
        req->common.resp = decrypt_bytes((uint8_t*)&req->ciphertext.enc_cstr, req->ciphertext.len,
            (uint8_t*)&req->plaintext.data, req->plaintext.len);
        req->plaintext.data[req->plaintext.len] = '\0';
        break;
    }

    case CMD_PROMOTE_INT:
        pmt_req->global = putInt(pmt_req->target, getInt(pmt_req->local), false, pmt_req->partition);
#ifdef USE_LOCAL_STORE_CLEAR
        freeLocalInt(pmt_req->local);
#endif
        break;
    case CMD_PROMOTE_FLOAT:
        pmt_req->global = putFloat(pmt_req->target, getFloat(pmt_req->local), false, pmt_req->partition);
#ifdef USE_LOCAL_STORE_CLEAR
        freeLocalFloat(pmt_req->local);
#endif
        break;
    case CMD_PROMOTE_TEXT:
        pmt_req->global = putText(pmt_req->target, getText(pmt_req->local), false, pmt_req->partition);
#ifdef USE_LOCAL_STORE_CLEAR
        freeLocalText(pmt_req->local);
#endif
        break;
    case CMD_PROMOTE_TIMESTAMP:
        pmt_req->global = putTimestamp(pmt_req->target, getTimestamp(pmt_req->local), false, pmt_req->partition);
#ifdef USE_LOCAL_STORE_CLEAR
        freeLocalTimestamp(pmt_req->local);
#endif
        break;

    default:
        break;
    }

    base_req->resp = 0; // TODO validation may need to be considered
    return 0;
}
