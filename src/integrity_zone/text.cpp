// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <extension.hpp>
#include <interface.hpp>
#include <request.hpp>
#include "enc_types.h"

RID kv_text_put(char* value)
{
    RID res;
    auto req = PutPlainIntoKVRequest<char[STRING_LENGTH]>(CMD_TEXT_PUT, value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

void kv_text_get(RID key, char *res_buf)
{
    auto req = GetPlainFromKVRequest<char[STRING_LENGTH]>(CMD_TEXT_GET, key, res_buf);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

RID kv_text_put_enc(EncStr *value)
{
    RID res;
    auto req = PutEncIntoKVRequest<EncStr, CMD_STRING_PUT_ENC>(value, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

void kv_text_get_enc(EncStr *res, RID key)
{
    auto req = GetEncFromKVRequest<EncStr, CMD_STRING_GET_ENC>(key, res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

int kv_text_cmp(RID id1, RID id2)
{
    int res;
    auto req = CmpRequest(CMD_TEXT_CMP, id1, id2, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

RID kv_text_concatenate(RID id1, RID id2)
{
    RID res;
    auto req = CalcRequest(CMD_TEXT_CONCAT, id1, id2, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

RID kv_text_substring(RID str, RID start, RID length)
{
    RID res;
    auto req = SubstringRequest(str, start, length, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

int kv_text_like(RID str, RID pattern)
{
    int res;
    auto req = CmpRequest(CMD_TEXT_LIKE, str, pattern, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

#ifdef __cplusplus
extern "C" {
#endif

PG_FUNCTION_INFO_V1(kv_text_out);
PG_FUNCTION_INFO_V1(kv_text_in);
PG_FUNCTION_INFO_V1(kv_text_recv);
PG_FUNCTION_INFO_V1(kv_text_send);
PG_FUNCTION_INFO_V1(kv_text_cmp);
PG_FUNCTION_INFO_V1(kv_text_gt);
PG_FUNCTION_INFO_V1(kv_text_ge);
PG_FUNCTION_INFO_V1(kv_text_lt);
PG_FUNCTION_INFO_V1(kv_text_ne);
PG_FUNCTION_INFO_V1(kv_text_le);
PG_FUNCTION_INFO_V1(kv_text_eq);
PG_FUNCTION_INFO_V1(kv_text_like);
PG_FUNCTION_INFO_V1(kv_text_notlike);
PG_FUNCTION_INFO_V1(kv_text_concatenate);
PG_FUNCTION_INFO_V1(substring);

#ifdef __cplusplus
}
#endif

int enc_text_encrypt(Str* in, EncStr* out)
{
    auto req = EncRequest<Str, EncStr, CMD_STRING_ENC>(in, out);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

int enc_text_decrypt(EncStr* in, Str* out)
{
    auto req = DecRequest<EncStr, Str, CMD_STRING_DEC>(in, out);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int resp = invoker->sendRequest(&req);
    return resp;
}

static EncText* cstring_to_enctext_with_len(const char* s, uint32_t len)
{
    if (len >= STRING_LENGTH)
        ereport(ERROR,
            (errmsg("kv_text input too long: %u bytes, maximum is %u",
                len, STRING_LENGTH - 1)));

    EncText* result = (EncText*)palloc0(ENCSTRLEN(len) + VARHDRSZ);

    Str str;
    str.len = len;
    memcpy(str.data, s, len);

    EncStr* estr = (EncStr*)VARDATA(result);
    int error = enc_text_encrypt(&str, estr);
    if (error) print_error("%s %d", __func__, error);

    SET_VARSIZE(result, ENCSTRLEN(len) + VARHDRSZ);
    return result;
}

/* from plain text to cipher (end-to-end encryption), then to RID */
Datum kv_text_in(PG_FUNCTION_ARGS)
{
    char *sIn = PG_GETARG_CSTRING(0);

    EncText* result = (EncText*)cstring_to_enctext_with_len(sIn, strlen(sIn));
    EncStr* estr = (EncStr*)VARDATA(result);
    RID id = kv_text_put_enc(estr);
    pfree(result);


    PG_RETURN_DATUM(id);
}

Datum kv_text_out(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    EncStr* estr = (EncStr*)palloc0(sizeof(EncStr));
    kv_text_get_enc(estr, id);

    Str str;
    int error = enc_text_decrypt(estr, &str);
    if (error) print_error("%s %d", __func__, error);

    pfree(estr);
    char *pOut = (char *)palloc0(STRING_LENGTH);
    memset(pOut, 0, STRING_LENGTH);
    memcpy(pOut, str.data, str.len);


    PG_RETURN_POINTER(pOut);
}

Datum kv_text_recv(PG_FUNCTION_ARGS)
{
    StringInfo  buf = (StringInfo) PG_GETARG_POINTER(0);
    char *sIn = (char *)(pq_getmsgstring(buf));

    EncText* result = (EncText*)cstring_to_enctext_with_len(sIn, strlen(sIn));
    EncStr* estr = (EncStr*)VARDATA(result);
    RID id = kv_text_put_enc(estr);
    pfree(result);
    PG_RETURN_DATUM(id);
}

Datum kv_text_send(PG_FUNCTION_ARGS)
{
    RID id = PG_GETARG_DATUM(0);

    EncStr* estr = (EncStr*)palloc0(sizeof(EncStr));
    kv_text_get_enc(estr, id);

    Str str;
    int error = enc_text_decrypt(estr, &str);
    if (error) print_error("%s %d", __func__, error);

    pfree(estr);
    char *pOut = (char *)palloc(STRING_LENGTH);
    memset(pOut, 0, STRING_LENGTH);
    memcpy(pOut, str.data, str.len);

    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendstring(&buf, pOut);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum kv_text_eq(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 == cmp);
}

Datum kv_text_ne(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 != cmp);
}

Datum kv_text_le(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 >= cmp);
}

Datum kv_text_lt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 > cmp);
}

Datum kv_text_ge(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 <= cmp);
}

Datum kv_text_gt(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);


    PG_RETURN_BOOL(0 < cmp);
}

Datum kv_text_cmp(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int cmp = kv_text_cmp(id1, id2);

    PG_RETURN_INT32(cmp);
}

/* Warning: this method is dangerous in the privacy setting */
Datum kv_text_concatenate(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    RID res = kv_text_concatenate(id1, id2);


    PG_RETURN_DATUM(res);
}

/* Warning: this method is dangerous in the privacy setting */
Datum kv_text_like(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int res = kv_text_like(id1, id2);


    PG_RETURN_BOOL(res);
}

/* Warning: this method is dangerous in the privacy setting */
Datum kv_text_notlike(PG_FUNCTION_ARGS)
{
    RID id1 = PG_GETARG_DATUM(0);
    RID id2 = PG_GETARG_DATUM(1);

    int res = kv_text_like(id1, id2);


    PG_RETURN_BOOL(res == false);
}

/* Warning: this method is dangerous in the privacy setting */
Datum substring(PG_FUNCTION_ARGS)
{
    RID str = PG_GETARG_DATUM(0);

    int32_t from = PG_GETARG_INT32(1);
    int32_t len = PG_GETARG_INT32(2);

    RID res = kv_text_substring(str, from, len);


    PG_RETURN_DATUM(res);
}
