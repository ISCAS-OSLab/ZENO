// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <extension.hpp>
#include <interface.hpp>
#include <request.hpp>
#include "enc_types.h"

RID kv_timestamp_put(TIMESTAMP value)
{
    RID res;
    auto req = PutPlainIntoKVRequest<TIMESTAMP>(CMD_TIMESTAMP_PUT, value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

TIMESTAMP kv_timestamp_get(RID key)
{
    TIMESTAMP res;
    auto req = GetPlainFromKVRequest<TIMESTAMP>(CMD_TIMESTAMP_GET, key, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

RID kv_timestamp_put_enc(EncTimestamp *value)
{
    RID res;
    auto req = PutEncIntoKVRequest<EncTimestamp, CMD_TIMESTAMP_PUT_ENC>(value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

void kv_timestamp_get_enc(EncTimestamp *res, RID key)
{
    auto req = GetEncFromKVRequest<EncTimestamp, CMD_TIMESTAMP_GET_ENC>(key, res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

int kv_timestamp_cmp(RID left, RID right)
{
    int res;
    auto req = CmpRequest(CMD_TIMESTAMP_CMP, left, right, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

RID kv_timestamp_extract_year(RID in)
{
    RID res;
    auto req = TimestampExtractYearRequest(in, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

#ifdef __cplusplus
extern "C" {
#endif
PG_FUNCTION_INFO_V1(kv_timestamp_in);
PG_FUNCTION_INFO_V1(kv_timestamp_out);
PG_FUNCTION_INFO_V1(kv_timestamp_eq);
PG_FUNCTION_INFO_V1(kv_timestamp_ne);
PG_FUNCTION_INFO_V1(kv_timestamp_lt);
PG_FUNCTION_INFO_V1(kv_timestamp_le);
PG_FUNCTION_INFO_V1(kv_timestamp_gt);
PG_FUNCTION_INFO_V1(kv_timestamp_ge);
PG_FUNCTION_INFO_V1(kv_timestamp_cmp);
PG_FUNCTION_INFO_V1(date_part);
#ifdef __cplusplus
}
#endif

Timestamp pg_timestamp_in(char *str)
{
    Timestamp result;
    char workbuf[MAXDATELEN + MAXDATEFIELDS];
    char *field[MAXDATEFIELDS];
    int ftype[MAXDATEFIELDS];
    int dterr;
    int nf;
    int tz;
    int dtype;
    fsec_t fsec;
#if PG_VERSION_NUM >= 160000
    DateTimeErrorExtra extra;
#endif
    struct pg_tm tt, *tm = &tt;

    dterr = ParseDateTime(str, workbuf, sizeof(workbuf), field, ftype, MAXDATEFIELDS, &nf);

#if PG_VERSION_NUM >= 160000
    if (dterr == 0)
        dterr = DecodeDateTime(field, ftype, nf, &dtype, tm, &fsec, &tz, &extra);
    if (dterr != 0)
        DateTimeParseError(dterr, &extra, str, "timestamp", nullptr);
#else
    if (dterr == 0)
        dterr = DecodeDateTime(field, ftype, nf, &dtype, tm, &fsec, &tz);
    if (dterr != 0)
        DateTimeParseError(dterr, str, "timestamp");
#endif

    switch (dtype)
    {
    case DTK_DATE:
        if (tm2timestamp(tm, fsec, NULL, &result) != 0)
            ereport(ERROR,
                    (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                     errmsg("timestamp out of range: \"%s\"", str)));
        break;

    case DTK_EPOCH:
        result = SetEpochTimestamp();
        break;

    case DTK_LATE:
        TIMESTAMP_NOEND(result);
        break;

    case DTK_EARLY:
        TIMESTAMP_NOBEGIN(result);
        break;

    default:
        elog(ERROR, "unexpected dtype %d while parsing timestamp \"%s\"",
             dtype, str);
        TIMESTAMP_NOEND(result);
    }

    return result;
}

int enc_timestamp_encrypt(TIMESTAMP* src, EncTimestamp* dst)
{
    auto req = EncRequest<TIMESTAMP, EncTimestamp, CMD_TIMESTAMP_ENC>(src, dst);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

int enc_timestamp_decrypt(EncTimestamp* src, TIMESTAMP* dst)
{
    auto req = DecRequest<EncTimestamp, TIMESTAMP, CMD_TIMESTAMP_DEC>(src, dst);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

Datum kv_timestamp_in(PG_FUNCTION_ARGS)
{
    char* pIn = PG_GETARG_CSTRING(0);
    TIMESTAMP in = pg_timestamp_in(pIn);

    EncTimestamp* result = (EncTimestamp*)palloc0(sizeof(EncTimestamp));
    int error = enc_timestamp_encrypt(&in, result);
    if (error) print_error("%s %d", __func__, error);

    RID id = kv_timestamp_put_enc(result);
    pfree(result);

    PG_RETURN_DATUM(id);
}

Datum kv_timestamp_out(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    EncTimestamp* result = (EncTimestamp*)palloc0(sizeof(EncTimestamp));
    kv_timestamp_get_enc(result, id);

    TIMESTAMP plain;
    int error = enc_timestamp_decrypt(result, &plain);
    if (error) print_error("%s %d", __func__, error);

    pfree(result);

    char *pOut = (char *)palloc(MAXDATELEN + 1);
    struct pg_tm tt, *tm = &tt;
    fsec_t fsec;
    if (timestamp2tm(plain, NULL, tm, &fsec, NULL, NULL) == 0)
        EncodeDateTime(tm, fsec, false, 0, NULL, 1, pOut);
    PG_RETURN_POINTER(pOut);
}

Datum kv_timestamp_eq(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_BOOL(cmp == 0);
}

Datum kv_timestamp_ne(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_BOOL(cmp != 0);
}

Datum kv_timestamp_lt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_BOOL(cmp < 0);
}

Datum kv_timestamp_le(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);
    int cmp = kv_timestamp_cmp(id1, id2);
    PG_RETURN_BOOL(cmp <= 0);
}

Datum kv_timestamp_gt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_BOOL(cmp > 0);
}

Datum kv_timestamp_ge(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_BOOL(cmp >= 0);
}

Datum kv_timestamp_cmp(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_timestamp_cmp(id1, id2);

    PG_RETURN_INT32(cmp);
}

Datum date_part(PG_FUNCTION_ARGS)
{
    char *get = text_to_cstring(PG_GETARG_TEXT_P(0));
    if (strcmp(get, "year") != 0)
        print_error("Only date_part('year', kv_timestamp) is currently implemented.");

    RID id = PG_GETARG_DATUM(1);
    RID res = kv_timestamp_extract_year(id);

    PG_RETURN_DATUM(res);
}
