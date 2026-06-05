#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "enc_types.h"
#include "rid.h"

#define WAL_RID_BATCH_SIZE 4096
#define WAL_MATERIALIZE_RID_BATCH_SIZE 3000

typedef enum ReqStatus {
    NONE,
    SENT,
    DONE,
    EXIT
} ReqStat;

typedef struct {
    volatile ReqStat status;
    int reqType;
    int resp;
} BaseRequest;

/* enc and dec req are for debug purpose. */
#define DEFINE_ENCTYPE_ENC_ReqData(enc_type, plain_type) \
    typedef struct {                                     \
        BaseRequest common;                              \
        plain_type plaintext;                            \
        enc_type ciphertext;                             \
    } enc_type##EncRequestData;

DEFINE_ENCTYPE_ENC_ReqData(EncInt, int);
DEFINE_ENCTYPE_ENC_ReqData(EncFloat, float);
DEFINE_ENCTYPE_ENC_ReqData(EncTimestamp, int64_t);
DEFINE_ENCTYPE_ENC_ReqData(EncStr, Str);

#define DEFINE_ENCTYPE_DEC_ReqData(enc_type, plain_type) \
    typedef struct {                                     \
        BaseRequest common;                              \
        enc_type ciphertext;                             \
        plain_type plaintext;                            \
    } enc_type##DecRequestData;

DEFINE_ENCTYPE_DEC_ReqData(EncInt, int);
DEFINE_ENCTYPE_DEC_ReqData(EncFloat, float);
DEFINE_ENCTYPE_DEC_ReqData(EncTimestamp, int64_t);
DEFINE_ENCTYPE_DEC_ReqData(EncStr, Str);

#define DEFINE_ENCTYPE_PUT_ENC_ReqData(enc_type) \
    typedef struct {                                     \
        BaseRequest common;                              \
        RID res;                            \
        enc_type enc;                             \
    } enc_type##PutEncIntoKVRequestData;

DEFINE_ENCTYPE_PUT_ENC_ReqData(EncInt);
DEFINE_ENCTYPE_PUT_ENC_ReqData(EncFloat);
DEFINE_ENCTYPE_PUT_ENC_ReqData(EncTimestamp);
DEFINE_ENCTYPE_PUT_ENC_ReqData(EncStr);

#define DEFINE_ENCTYPE_GET_ENC_ReqData(enc_type) \
    typedef struct {                                     \
        BaseRequest common;                              \
        enc_type res;                             \
        RID dec;                            \
    } enc_type##GetEncFromKVRequestData;

DEFINE_ENCTYPE_GET_ENC_ReqData(EncInt);
DEFINE_ENCTYPE_GET_ENC_ReqData(EncFloat);
DEFINE_ENCTYPE_GET_ENC_ReqData(EncTimestamp);
DEFINE_ENCTYPE_GET_ENC_ReqData(EncStr);

typedef struct {
    BaseRequest common;
    RID local;
    RID target;
    RID partition;
    RID global;
} PromoteRequestData;

typedef struct {
    BaseRequest common;
    uint64_t size;
    int nreqs;
    uint8_t buf[];
} BatchRequestData;

typedef struct {
    BaseRequest common;
    uint64_t size;
    RID key;
    uint8_t value[];
} KVRequestData;

typedef struct {
    int type; /* 0 int, 1 float, 2 text, 3 timestamp */
    RID rid;
} WalRidItem;

typedef struct {
    BaseRequest common;
    int nitems;
    WalRidItem items[WAL_RID_BATCH_SIZE];
} WalAppendBatchRequestData;

typedef struct {
    BaseRequest common;
    int nitems;
    uint64_t payload_len;
    WalRidItem items[WAL_MATERIALIZE_RID_BATCH_SIZE];
    uint8_t payload[];
} WalMaterializeBatchRequestData;

typedef struct {
    BaseRequest common;
    uint64_t payload_len;
    uint8_t payload[];
} WalReplayPayloadRequestData;

typedef struct {
    BaseRequest common;
    RID left;
    RID right;
    int cmp;
} CmpRequestData;

typedef struct {
    BaseRequest common;
    RID left;
    RID right;
    RID res;
} CalcRequestData;

typedef struct {
    BaseRequest common;
    int bulk_size;
    RID keys[BULK_SIZE];
    RID res;
} BulkRequestData;

typedef struct {
    BaseRequest common;
    RID in;
    RID res;
} TimestampExtractYearRequestData;

typedef struct {
    BaseRequest common;
    RID str;
    RID start;
    RID length;
    RID res;
} SubstringRequestData;

typedef struct {
    BaseRequest common;
} SingleArgRequestData;

#ifdef __cplusplus
}
#endif
