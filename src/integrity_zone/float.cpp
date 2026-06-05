// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <extension.hpp>
#include <interface.hpp>
#include <request.hpp>
#include "enc_types.h"

static inline RID kv_float_put(float value)
{
    RID res;
    auto req = PutPlainIntoKVRequest<float>(CMD_FLOAT_PUT, value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline float kv_float_get(RID key)
{
    float res;
    auto req = GetPlainFromKVRequest<float>(CMD_FLOAT_GET, key, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_float_put_enc(EncFloat *value)
{
    RID res;
    auto req = PutEncIntoKVRequest<EncFloat, CMD_FLOAT_PUT_ENC>(value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline void kv_float_get_enc(EncFloat *res, RID key)
{
    auto req = GetEncFromKVRequest<EncFloat, CMD_FLOAT_GET_ENC>(key, res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

static inline RID kv_float_calc(int cmd, RID left, RID right)
{
    RID res;
    auto req = CalcRequest(cmd, left, right, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_float_add(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_PLUS, left, right);
}

static inline RID kv_float_sub(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_MINUS, left, right);
}

static inline RID kv_float_mult(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_MULT, left, right);
}

static inline RID kv_float_div(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_DIV, left, right);
}

static inline RID kv_float_pow(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_POW, left, right);
}

static inline RID kv_float_mod(RID left, RID right)
{
    return kv_float_calc(CMD_FLOAT_MOD, left, right);
}

static inline int kv_float_cmp(RID left, RID right)
{
    int res;
    auto req = CmpRequest(CMD_FLOAT_CMP, left, right, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

static inline RID kv_float_sum_bulk(size_t bulk_size, RID *keys)
{
    RID res;
    auto req = BulkRequest(CMD_FLOAT_SUM_BULK, bulk_size, keys, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);


    return res;
}

#ifdef __cplusplus
extern "C" {
#endif

PG_FUNCTION_INFO_V1(float4_to_kv_float4);
PG_FUNCTION_INFO_V1(numeric_to_kv_float4);
PG_FUNCTION_INFO_V1(double_to_kv_float4);
PG_FUNCTION_INFO_V1(int8_to_kv_float4);
PG_FUNCTION_INFO_V1(int4_to_kv_float4);
PG_FUNCTION_INFO_V1(kv_float4_in);
PG_FUNCTION_INFO_V1(kv_float4_out);

PG_FUNCTION_INFO_V1(kv_float4_sum_bulk);
PG_FUNCTION_INFO_V1(kv_float4_avg_bulk);
PG_FUNCTION_INFO_V1(kv_float4_min_bulk);
PG_FUNCTION_INFO_V1(kv_float4_max_bulk);

PG_FUNCTION_INFO_V1(kv_float4_add);
PG_FUNCTION_INFO_V1(kv_float4_sub);
PG_FUNCTION_INFO_V1(kv_float4_mult);
PG_FUNCTION_INFO_V1(kv_float4_div);
PG_FUNCTION_INFO_V1(kv_float4_pow);
PG_FUNCTION_INFO_V1(kv_float4_eq);
PG_FUNCTION_INFO_V1(kv_float4_ne);
PG_FUNCTION_INFO_V1(kv_float4_lt);
PG_FUNCTION_INFO_V1(kv_float4_le);
PG_FUNCTION_INFO_V1(kv_float4_gt);
PG_FUNCTION_INFO_V1(kv_float4_ge);
PG_FUNCTION_INFO_V1(kv_float4_cmp);
PG_FUNCTION_INFO_V1(kv_float4_mod);

#ifdef __cplusplus
}
#endif

static float4 pg_float4_in(char* num)
{
    char* orig_num;
    double val;
    char* endptr;

    /*
     * endptr points to the first character _after_ the sequence we recognized
     * as a valid floating point number. orig_num points to the original input
     * string.
     */
    orig_num = num;

    /* skip leading whitespace */
    while (*num != '\0' && isspace((unsigned char)*num))
        num++;

    /*
     * Check for an empty-string input to begin with, to avoid the vagaries of
     * strtod() on different platforms.
     */
    if (*num == '\0')
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("invalid input syntax for type %s: \"%s\"",
                    "real", orig_num)));

    errno = 0;
    val = strtod(num, &endptr);

    /* did we not see anything that looks like a double? */
    if (endptr == num || errno != 0) {
        int save_errno = errno;

        /*
         * C99 requires that strtod() accept NaN, [+-]Infinity, and [+-]Inf,
         * but not all platforms support all of these (and some accept them
         * but set ERANGE anyway...)  Therefore, we check for these inputs
         * ourselves if strtod() fails.
         *
         * Note: C99 also requires hexadecimal input as well as some extended
         * forms of NaN, but we consider these forms unportable and don't try
         * to support them.  You can use 'em if your strtod() takes 'em.
         */
        if (pg_strncasecmp(num, "NaN", 3) == 0) {
            val = get_float4_nan();
            endptr = num + 3;
        } else if (pg_strncasecmp(num, "Infinity", 8) == 0) {
            val = get_float4_infinity();
            endptr = num + 8;
        } else if (pg_strncasecmp(num, "+Infinity", 9) == 0) {
            val = get_float4_infinity();
            endptr = num + 9;
        } else if (pg_strncasecmp(num, "-Infinity", 9) == 0) {
            val = -get_float4_infinity();
            endptr = num + 9;
        } else if (pg_strncasecmp(num, "inf", 3) == 0) {
            val = get_float4_infinity();
            endptr = num + 3;
        } else if (pg_strncasecmp(num, "+inf", 4) == 0) {
            val = get_float4_infinity();
            endptr = num + 4;
        } else if (pg_strncasecmp(num, "-inf", 4) == 0) {
            val = -get_float4_infinity();
            endptr = num + 4;
        } else if (save_errno == ERANGE) {
            /*
             * Some platforms return ERANGE for denormalized numbers (those
             * that are not zero, but are too close to zero to have full
             * precision).  We'd prefer not to throw error for that, so try to
             * detect whether it's a "real" out-of-range condition by checking
             * to see if the result is zero or huge.
             */
            if (val == 0.0 || val >= HUGE_VAL || val <= -HUGE_VAL)
                ereport(ERROR,
                    (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
                        errmsg("\"%s\" is out of range for type real",
                            orig_num)));
        } else
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("invalid input syntax for type %s: \"%s\"",
                        "real", orig_num)));
    }
#ifdef HAVE_BUGGY_SOLARIS_STRTOD
    else {
        /*
         * Many versions of Solaris have a bug wherein strtod sets endptr to
         * point one byte beyond the end of the string when given "inf" or
         * "infinity".
         */
        if (endptr != num && endptr[-1] == '\0')
            endptr--;
    }
#endif /* HAVE_BUGGY_SOLARIS_STRTOD */

    /* skip trailing whitespace */
    while (*endptr != '\0' && isspace((unsigned char)*endptr))
        endptr++;

    /* if there is any junk left at the end of the string, bail out */
    if (*endptr != '\0')
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("invalid input syntax for type %s: \"%s\"",
                    "real", orig_num)));

    /*
     * if we get here, we have a legal double, still need to check to see if
     * it's a legal float4
     */
    // CHECKFLOATVAL((float4) val, isinf(val), val == 0);

    return ((float4)val);
}

int enc_float_encrypt(float in, EncFloat* out)
{
    auto req = EncRequest<float, EncFloat, CMD_FLOAT_ENC>(&in, out);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

int enc_float_decrypt(EncFloat* in, float* out)
{
    auto req = DecRequest<EncFloat, float, CMD_FLOAT_DEC>(in, out);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

Datum float4_to_kv_float4(PG_FUNCTION_ARGS)
{
    float in = PG_GETARG_FLOAT4(0);
    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    enc_float_encrypt(in, result);
    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum numeric_to_kv_float4(PG_FUNCTION_ARGS)
{
    Numeric num = PG_GETARG_NUMERIC(0);
    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    float in;
    char* tmp = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(num)));

    in = pg_float4_in(tmp);
    enc_float_encrypt(in, result);
    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum double_to_kv_float4(PG_FUNCTION_ARGS)
{
    float8 num = PG_GETARG_FLOAT8(0);
    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    float in;
    char* tmp = DatumGetCString(DirectFunctionCall1(float8out, Float8GetDatum(num)));

    in = pg_float4_in(tmp);
    enc_float_encrypt(in, result);
    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum int8_to_kv_float4(PG_FUNCTION_ARGS)
{
    int8 num = PG_GETARG_INT64(0);
    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    float in;
    char* tmp = DatumGetCString(DirectFunctionCall1(int8out, Int8GetDatum(num)));

    in = pg_float4_in(tmp);
    enc_float_encrypt(in, result);
    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum int4_to_kv_float4(PG_FUNCTION_ARGS)
{
    int num = PG_GETARG_INT32(0);
    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    float in;
    char* tmp = DatumGetCString(DirectFunctionCall1(int4out, Int32GetDatum(num)));

    in = pg_float4_in(tmp);
    enc_float_encrypt(in, result);
    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

/* from plain float4 to cipher (end-to-end encryption), then to RID */
Datum kv_float4_in(PG_FUNCTION_ARGS)
{
    char* pIn = PG_GETARG_CSTRING(0);
    float in = pg_float4_in(pIn);

    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    int error = enc_float_encrypt(in, result);
    if (error) print_error("%s %d", __func__, error);

    RID id = kv_float_put_enc(result);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_out(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    EncFloat* result = (EncFloat*)palloc0(sizeof(EncFloat));
    kv_float_get_enc(result, id);

    float in;
    int error = enc_float_decrypt(result, &in);
    if (error) print_error("%s %d", __func__, error);


    pfree(result);

    char *pOut = (char *)palloc(32 * sizeof(char));
    MemSet(pOut, 0, 32 * sizeof(char));
    sprintf(pOut, "%f", in);
    PG_RETURN_POINTER(pOut);
}

Datum kv_float4_add(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_add(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_sub(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_sub(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_mult(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_mult(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_div(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_div(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_pow(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_pow(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_mod(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID id = kv_float_mod(id1, id2);


    PG_RETURN_DATUM(id);
}

Datum kv_float4_eq(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp == 0);
}

Datum kv_float4_ne(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp != 0);
}

Datum kv_float4_lt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp < 0);
}

Datum kv_float4_le(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp <= 0);
}

Datum kv_float4_gt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp > 0);
}

Datum kv_float4_ge(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);


    PG_RETURN_BOOL(cmp >= 0);
}

Datum kv_float4_cmp(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int32_t cmp = kv_float_cmp(id1, id2);

    PG_RETURN_INT32(cmp);
}

Datum kv_float4_sum_bulk(PG_FUNCTION_ARGS)
{
    ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
    bool isnull;
    Datum value;

    ArrayMetaState *my_extra = (ArrayMetaState *)fcinfo->flinfo->fn_extra;
    ArrayIterator array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID bulk_array[BULK_SIZE];
    int count = 1;
    bulk_array[0] = DatumGetUInt64(value);
    while (array_iterate(array_iterator, &value, &isnull))
    {
        if (count == BULK_SIZE)
        {
            RID tmp = kv_float_sum_bulk(BULK_SIZE, bulk_array);
            bulk_array[0] = tmp;
            count = 1;
        }
        bulk_array[count] = DatumGetUInt64(value);
        count++;
    }
    RID sum = kv_float_sum_bulk(count, bulk_array);
    PG_RETURN_DATUM(sum);
}

Datum kv_float4_avg_bulk(PG_FUNCTION_ARGS)
{
    // SAME AS kv_float4_avgfinal()
    ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
    bool isnull;
    Datum value;
    int ndims1 = ARR_NDIM(v); // array dimension
    int *dims1 = ARR_DIMS(v);
    int nitems = ArrayGetNItems(ndims1, dims1); // number of items in array

    ArrayMetaState *my_extra = (ArrayMetaState *)fcinfo->flinfo->fn_extra;
    ArrayIterator array_iterator = array_create_iterator(v, 0, my_extra);
    array_iterate(array_iterator, &value, &isnull);

    RID bulk_array[BULK_SIZE];
    int count = 1;
    bulk_array[0] = DatumGetUInt64(value);
    while (array_iterate(array_iterator, &value, &isnull))
    {
        if (count == BULK_SIZE)
        {
            RID tmp = kv_float_sum_bulk(BULK_SIZE, bulk_array);
            bulk_array[0] = tmp;
            count = 1;
        }
        bulk_array[count] = DatumGetUInt64(value);
        count++;
    }

    RID sum = kv_float_sum_bulk(count, bulk_array);
    RID div = kv_float_put(nitems); // TODO
    RID avg = kv_float_div(sum, div);

    PG_RETURN_DATUM(avg);
}

Datum kv_float4_max_bulk(PG_FUNCTION_ARGS)
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
        cmp = kv_float_cmp(max, tmp);
        if (-1 == cmp)
            max = tmp;
    }

    PG_RETURN_DATUM(max);
}

Datum kv_float4_min_bulk(PG_FUNCTION_ARGS)
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
        cmp = kv_float_cmp(min, tmp);
        if (1 == cmp)
            min = tmp;
    }

    PG_RETURN_DATUM(min);
}
