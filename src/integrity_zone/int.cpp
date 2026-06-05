// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <extension.hpp>
#include <interface.hpp>
#include <request.hpp>
#include "enc_types.h"

static inline RID kv_int_put(int value)
{
    RID res;
    auto req = PutPlainIntoKVRequest<int>(CMD_INT_PUT, value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline int kv_int_get(RID key)
{
    int res;
    auto req = GetPlainFromKVRequest<int>(CMD_INT_GET, key, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_int_put_enc(EncInt *value)
{
    RID res;
    auto req = PutEncIntoKVRequest<EncInt, CMD_INT_PUT_ENC>(value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline void kv_int_get_enc(EncInt *res, RID key)
{
    auto req = GetEncFromKVRequest<EncInt, CMD_INT_GET_ENC>(key, res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

static inline RID kv_int_calc(int cmd, RID i1, RID i2)
{
    RID res;
    auto req = CalcRequest(cmd, i1, i2, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_int_add(RID i1, RID i2)
{
    return kv_int_calc(CMD_INT_PLUS, i1, i2);
}

static inline RID kv_int_sub(RID i1, RID i2)
{
    RID resp = kv_int_calc(CMD_INT_MINUS, i1, i2);
    return resp;
}

static inline RID kv_int_mult(RID i1, RID i2)
{
    RID resp = kv_int_calc(CMD_INT_MULT, i1, i2);
    return resp;
}

static inline RID kv_int_div(RID i1, RID i2)
{
    RID resp = kv_int_calc(CMD_INT_DIV, i1, i2);
    return resp;
}

static inline RID kv_int_pow(RID i1, RID i2)
{
    RID resp = kv_int_calc(CMD_INT_POW, i1, i2);
    return resp;
}

static inline RID kv_int_mod(RID i1, RID i2)
{
    RID resp = kv_int_calc(CMD_INT_MOD, i1, i2);
    return resp;
}

static inline int kv_int_cmp(RID i1, RID i2)
{
    int res;
    auto req = CmpRequest(CMD_INT_CMP, i1, i2, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_int_sum_bulk(size_t bulk_size, RID *keys)
{
    RID res;
    auto req = BulkRequest(CMD_INT_SUM_BULK, bulk_size, keys, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);


    return res;
}

#ifdef __cplusplus
extern "C" {
#endif

PG_FUNCTION_INFO_V1(int4_to_kv_int4);
PG_FUNCTION_INFO_V1(int8_to_kv_int4);
PG_FUNCTION_INFO_V1(kv_int4_in);
PG_FUNCTION_INFO_V1(kv_int4_out);
PG_FUNCTION_INFO_V1(kv_int4_recv);
PG_FUNCTION_INFO_V1(kv_int4_send);

PG_FUNCTION_INFO_V1(kv_int4_add);
PG_FUNCTION_INFO_V1(kv_int4_sub);
PG_FUNCTION_INFO_V1(kv_int4_mult);
PG_FUNCTION_INFO_V1(kv_int4_div);
PG_FUNCTION_INFO_V1(kv_int4_pow);
PG_FUNCTION_INFO_V1(kv_int4_mod);
PG_FUNCTION_INFO_V1(kv_int4_cmp);
PG_FUNCTION_INFO_V1(kv_int4_eq);
PG_FUNCTION_INFO_V1(kv_int4_ne);
PG_FUNCTION_INFO_V1(kv_int4_lt);
PG_FUNCTION_INFO_V1(kv_int4_le);
PG_FUNCTION_INFO_V1(kv_int4_gt);
PG_FUNCTION_INFO_V1(kv_int4_ge);
PG_FUNCTION_INFO_V1(kv_int4_sum_bulk);
PG_FUNCTION_INFO_V1(kv_int4_avg_bulk);
PG_FUNCTION_INFO_V1(kv_int4_max_bulk);
PG_FUNCTION_INFO_V1(kv_int4_min_bulk);

#ifdef __cplusplus
}
#endif

int enc_int_encrypt(int pSrc, EncInt* pDst)
{
    auto req = EncRequest<int, EncInt, CMD_INT_ENC>(&pSrc, pDst);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

int enc_int_decrypt(EncInt* pSrc, int* pDst)
{
    auto req = DecRequest<EncInt, int, CMD_INT_DEC>(pSrc, pDst);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

Datum int4_to_kv_int4(PG_FUNCTION_ARGS)
{
    int in = PG_GETARG_INT32(0);
    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));

    int error = enc_int_encrypt(in, result);
    if (error) print_error("%s %d", __func__, error);
    RID id = kv_int_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum int8_to_kv_int4(PG_FUNCTION_ARGS)
{
    int in = PG_GETARG_INT64(0);
    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));

    int error = enc_int_encrypt(in, result);
    if (error) print_error("%s %d", __func__, error);

    RID id = kv_int_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

/* from plain int4 to cipher (end-to-end encryption), then to RID */
Datum kv_int4_in(PG_FUNCTION_ARGS)
{
    char* pIn = PG_GETARG_CSTRING(0);
    int in = atoi(pIn);

    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));
    int error = enc_int_encrypt(in, result);
    if (error) print_error("%s %d", __func__, error);

    RID id = kv_int_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_out(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));
    kv_int_get_enc(result, id);

    int in;
    int error = enc_int_decrypt(result, &in);
    if (error) print_error("%s %d", __func__, error);


    pfree(result);

    char *pOut = (char *)palloc(sizeof(int));
    sprintf(pOut, "%d", in);
    PG_RETURN_POINTER(pOut);
}

Datum kv_int4_recv(PG_FUNCTION_ARGS)
{
    StringInfo  buf = (StringInfo) PG_GETARG_POINTER(0);
    int in = pq_getmsgint(buf, 4);

    // simulate the end-to-end encryption
    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));
    int error = enc_int_encrypt(in, result);
    if (error) print_error("%s %d", __func__, error);

    // transform the cipher to rid (internel representation in the DB)
    // new overhead compare to HEDB
    RID id = kv_int_put_enc(result);

    pfree(result);
    PG_RETURN_DATUM(id);
}

Datum kv_int4_send(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    // transform the cipher to rid (internel representation in the DB)
    // new overhead compare to HEDB
    EncInt* result = (EncInt*)palloc0(sizeof(EncInt));
    kv_int_get_enc(result, id);

    // simulate the end-to-end descrption
    int in;
    int error = enc_int_decrypt(result, &in);
    if (error) print_error("%s %d", __func__, error);

    pfree(result);

    char *pOut = (char *)palloc(32 * sizeof(char));
    sprintf(pOut, "%d", in);

    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendstring(&buf, pOut);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum kv_int4_add(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_add(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_sub(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_sub(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_mult(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_mult(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_div(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_div(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_pow(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_pow(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_mod(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_int_mod(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_int4_cmp(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_INT32(cmp);
}

Datum kv_int4_eq(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
        PG_RETURN_BOOL(cmp == 0);
}

Datum kv_int4_ne(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_BOOL(cmp != 0);
}

Datum kv_int4_lt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_BOOL(cmp < 0);
}

Datum kv_int4_le(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_BOOL(cmp <= 0);
}

Datum kv_int4_gt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_BOOL(cmp > 0);
}

Datum kv_int4_ge(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_int_cmp(id1, id2);
    PG_RETURN_BOOL(cmp >= 0);
}

Datum kv_int4_sum_bulk(PG_FUNCTION_ARGS)
{
    ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
    ArrayIterator array_iterator;
    ArrayMetaState *my_extra = (ArrayMetaState *)fcinfo->flinfo->fn_extra;
    bool isnull;
    Datum value;

    array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID bulk_array[BULK_SIZE];
    int count = 1;
    bulk_array[0] = DatumGetUInt64(value);
    while (array_iterate(array_iterator, &value, &isnull))
    {
        if (count == BULK_SIZE)
        {
            RID tmp = kv_int_sum_bulk(BULK_SIZE, bulk_array);
            bulk_array[0] = tmp;
            count = 1;
        }
        bulk_array[count] = DatumGetUInt64(value);
        count++;
    }
    RID sum = kv_int_sum_bulk(count, bulk_array);
    PG_RETURN_DATUM(sum);
}

Datum kv_int4_avg_bulk(PG_FUNCTION_ARGS)
{
    ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
    ArrayIterator array_iterator;
    ArrayMetaState *my_extra = (ArrayMetaState *)fcinfo->flinfo->fn_extra;
    bool isnull;
    Datum value;
    int ndims1 = ARR_NDIM(v); // array dimension
    int *dims1 = ARR_DIMS(v);
    int nitems = ArrayGetNItems(ndims1, dims1); // number of items in array

    array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID bulk_array[BULK_SIZE];
    int count = 1;
    bulk_array[0] = DatumGetUInt64(value);
    while (array_iterate(array_iterator, &value, &isnull))
    {
        if (count == BULK_SIZE)
        {
            RID tmp = kv_int_sum_bulk(BULK_SIZE, bulk_array);
            bulk_array[0] = tmp;
            count = 1;
        }
        bulk_array[count] = DatumGetUInt64(value);
        count++;
    }

    RID sum = kv_int_sum_bulk(count, bulk_array);
    RID div = kv_int_put(nitems); // TODO
    RID avg = kv_int_div(sum, div);

    PG_RETURN_DATUM(avg);
}

Datum kv_int4_max_bulk(PG_FUNCTION_ARGS)
{
    ArrayType* v = PG_GETARG_ARRAYTYPE_P(0);
    ArrayIterator array_iterator;
    ArrayMetaState* my_extra = (ArrayMetaState*)fcinfo->flinfo->fn_extra;
    bool isnull;
    Datum value;

    array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID max = DatumGetUInt64(value), tmp;
    int cmp;
    while (array_iterate(array_iterator, &value, &isnull))
    {
        tmp = DatumGetUInt64(value);
        cmp = kv_int_cmp(max, tmp);
        if (-1 == cmp)
            max = tmp;
    }

    PG_RETURN_DATUM(max);
}

Datum kv_int4_min_bulk(PG_FUNCTION_ARGS)
{
    ArrayType* v = PG_GETARG_ARRAYTYPE_P(0);
    ArrayIterator array_iterator;
    ArrayMetaState* my_extra = (ArrayMetaState*)fcinfo->flinfo->fn_extra;
    bool isnull;
    Datum value;

    array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID min = DatumGetUInt64(value), tmp;
    int cmp;
    while (array_iterate(array_iterator, &value, &isnull))
    {
        tmp = DatumGetUInt64(value);
        cmp = kv_int_cmp(min, tmp);
        if (1 == cmp)
            min = tmp;
    }

    PG_RETURN_DATUM(min);
}
