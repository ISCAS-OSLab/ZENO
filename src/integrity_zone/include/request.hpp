#pragma once

extern "C" {
#include <defs.h>
#include <request_types.h>
#include <util.h>
#include "enc_types.h"
}
#include <vector>
#include <memory>

#define IS_ENCSTR(a) (sizeof(*a) == sizeof(EncStr))
#define ENCSTR_LEN(a) (((EncStr*)a)->len)
#define ENCSTR_DATA(a) (&((EncStr*)a)->enc_cstr)
#define COPY_ENC(to, from)                                          \
    {                                                               \
        ENCSTR_LEN(to) = ENCSTR_LEN(from);                          \
        memcpy(ENCSTR_DATA(to), ENCSTR_DATA(from), ENCSTR_LEN(to)); \
    }

#define IS_STR(a) (sizeof(*a) == sizeof(Str))
#define STR_LEN(a) (((Str*)a)->len)
#define STR_DATA(a) (((Str*)a)->data)
#define COPY_PLAIN(to, from)                               \
    {                                                      \
        STR_LEN(to) = STR_LEN(from);                       \
        memcpy(STR_DATA(to), STR_DATA(from), STR_LEN(to)); \
    }

#define COPY(to, from)                \
    {                                 \
        if (IS_STR(from)) {           \
            COPY_PLAIN(to, from);     \
        } else if (IS_ENCSTR(from)) { \
            COPY_ENC(to, from);       \
        } else                        \
            *to = *from;              \
    }

#define TYPESIZE(a, size)                                 \
    {                                                     \
        if (IS_STR(a)) {                                  \
            size = sizeof(STR_LEN(a)) + STR_LEN(a);       \
        } else if (IS_ENCSTR(a)) {                        \
            size = sizeof(ENCSTR_LEN(a)) + ENCSTR_LEN(a); \
        } else {                                          \
            size = sizeof(*a);                            \
        }                                                 \
    }

class Request {
public:
    virtual void serializeTo(void* buffer) const = 0;
    virtual inline void copyResultFrom(void* buffer) const = 0;
    virtual int getSize() const {
        return sizeof(BulkRequestData);
    };
};

/* DataType should be one of CMP DATA */

class CmpRequest : public Request {
public:
    CmpRequest(int reqType, RID left, RID right, int* cmp)
        : reqType(reqType)
        , left(left)
        , right(right)
        , cmp(cmp)
    {
    }

    int reqType;
    RID left;
    RID right;
    int* cmp;

    void serializeTo(void* buffer) const override
    {
        auto req = (CmpRequestData *)buffer;
        req->common.reqType = reqType;
        req->left = left;
        req->right = right;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (CmpRequestData *)buffer;
        KVDB_CHECK(cmp != nullptr, "CmpRequest result pointer is null");
        *cmp = req->cmp;
    }
};

class CalcRequest : public Request {
public:
    CalcRequest(int reqType, RID left, RID right, RID* res)
        : reqType(reqType)
        , left(left)
        , right(right)
        , res(res)
    {
    }

    int reqType;
    RID left;
    RID right;
    RID* res;

    void serializeTo(void* buffer) const override
    {
        auto req = (CalcRequestData *)buffer;
        req->common.reqType = reqType;
        req->left = left;
        req->right = right;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (CalcRequestData *)buffer;
        KVDB_CHECK(res != nullptr, "CalcRequest result pointer is null");
        *res = req->res;
    }
};

class BulkRequest : public Request {
public:
    BulkRequest(int bulkType, int bulkSize, RID *keys, RID* res)
        : bulkType(bulkType)
        , bulkSize(bulkSize)
        , keys(keys)
        , res(res)
    {
    }

    int bulkType;
    int bulkSize;
    RID *keys; // begin of keys
    RID *res;

    void serializeTo(void* buffer) const override
    {
        auto req = (BulkRequestData*)buffer;
        req->common.reqType = bulkType;
        req->bulk_size = bulkSize;
        memcpy(req->keys, keys, sizeof(RID) * bulkSize);
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (BulkRequestData*)buffer;
        KVDB_CHECK(res != nullptr, "BulkRequest result pointer is null");
        *res = req->res;
    }
};

// TODO: refactor
template <typename PlainType, typename EncType, int reqType>
class EncRequest : public Request {
public:
    DEFINE_ENCTYPE_ENC_ReqData(EncType, PlainType);

    PlainType* plaintext;
    EncType* res;

    EncRequest(PlainType* plaintext, EncType* res)
        : plaintext(plaintext)
        , res(res)
    {
    }

    void serializeTo(void* buffer) const override
    {
        auto* req = (EncTypeEncRequestData*)buffer;
        req->common.reqType = reqType;
        // req->plaintext = *plaintext;
        COPY(&req->plaintext, plaintext);

        // if(reqType == CMD_STRING_ENC){
        //     char ch[100];
        //     sprintf(ch, "after serialize %d", ((Str *) plaintext)->len);
        //     print_info(ch);
        // }
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto* req = (EncTypeEncRequestData*)buffer;
        COPY(res, &req->ciphertext);
    }
    inline int getSize() const override
    {
        int size1, size2;
        TYPESIZE(plaintext, size1);
        TYPESIZE(res, size2);
        return sizeof(BaseRequest) + size1 + size2;
    };
};

template <typename EncType, typename PlainType, int reqType>
class DecRequest : public Request {
public:
    DEFINE_ENCTYPE_DEC_ReqData(EncType, PlainType);
    EncType* ciphertext;
    PlainType* res;

    DecRequest(EncType* ciphertext, PlainType* res)
        : ciphertext(ciphertext)
        , res(res)
    {
    }

    void serializeTo(void* buffer) const override
    {
        auto* req = (EncTypeDecRequestData*)buffer;
        req->common.reqType = reqType;

        COPY(&req->ciphertext, ciphertext);
        // req->ciphertext = *ciphertext;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto* req = (EncTypeDecRequestData*)buffer;

        COPY(res, &req->plaintext);
        // *res = req->plaintext;
    }
    inline int getSize() const override
    {
        int size1, size2;
        TYPESIZE(ciphertext, size1);
        TYPESIZE(res, size2);
        return sizeof(BaseRequest) + size1 + size2;
    };
};

// cipher->ID
template <typename EncType, int reqType>
class PutEncIntoKVRequest : public Request {
public:
    DEFINE_ENCTYPE_PUT_ENC_ReqData(EncType);

    EncType* enc;
    RID* res;

    PutEncIntoKVRequest(EncType* enc, RID* res)
        : enc(enc)
        , res(res)
    {
    }

    void serializeTo(void* buffer) const override
    {
        auto* req = (EncTypePutEncIntoKVRequestData*)buffer;
        req->common.reqType = reqType;
        COPY(&req->enc, enc);
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto* req = (EncTypePutEncIntoKVRequestData*)buffer;
        *res = req->res;
    }
    inline int getSize() const override
    {
        int size;
        TYPESIZE(enc, size);
        return sizeof(BaseRequest) + size + sizeof(RID);
    };
};

// ID->cipher
template <typename EncType, int reqType>
class GetEncFromKVRequest : public Request {
public:
    DEFINE_ENCTYPE_GET_ENC_ReqData(EncType);
    RID dec;
    EncType* res;

    GetEncFromKVRequest(RID dec, EncType* res)
        : dec(dec)
        , res(res)
    {
    }

    void serializeTo(void* buffer) const override
    {
        auto* req = (EncTypeGetEncFromKVRequestData*)buffer;
        req->common.reqType = reqType;
        req->dec = dec;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto* req = (EncTypeGetEncFromKVRequestData*)buffer;
        COPY(res, &req->res);
    }
    inline int getSize() const override
    {
        int size;
        TYPESIZE(res, size);
        return sizeof(BaseRequest) + size + sizeof(RID);
    };
};

class PromoteRequest : public Request {
public:
    PromoteRequest(int reqType, RID local, RID target, RID partition, RID* global)
        : reqType(reqType)
        , local(local)
        , target(target)
        , partition(partition)
        , global(global)
    {
    }

    int reqType;
    RID local;
    RID target;
    RID partition;
    RID* global;

    void serializeTo(void* buffer) const override
    {
        auto req = (PromoteRequestData *)buffer;
        req->common.reqType = reqType;
        req->local = local;
        req->target = target;
        req->partition = partition;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (PromoteRequestData *)buffer;
        KVDB_CHECK(global != nullptr, "PromoteRequest result pointer is null");
        *global = req->global;
    }

    inline int getSize() const override
    {
        struct FakeData {
            BaseRequest common;
            RID global;
            RID partition;
            RID target;
            RID local;
        };
        return sizeof(FakeData);
    }
};

template <typename ValueType>
class PutPlainIntoKVRequest : public Request {
public:
    PutPlainIntoKVRequest(int reqType, ValueType value, RID* key)
        : reqType(reqType)
        , value(value)
        , key(key)
    {
    }

    int reqType;
    ValueType value;
    RID* key;

    void serializeTo(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        req->common.reqType = reqType;
        req->size = getSize();
        memcpy(req->value, &value, sizeof(value));
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        KVDB_CHECK(key != nullptr, "PutPlainIntoKVRequest key pointer is null");
        *key = req->key;
    }

    inline int getSize() const override
    {
        struct FakeData {
            BaseRequest common;
            uint64_t size;
            RID key;
            ValueType value;
        };
        return sizeof(FakeData);
    }
};

template <typename T, std::size_t N>
class PutPlainIntoKVRequest<T[N]> : public Request {
public:
    PutPlainIntoKVRequest(int reqType, T *value, RID* key)
        : reqType(reqType)
        , key(key)
    {
        memcpy(this->value, value, sizeof(T[N]));
    }

    int reqType;
    T value[N];
    RID* key;

    void serializeTo(void* buffer) const override
    {
        auto req = (KVRequestData*)buffer;
        req->common.reqType = reqType;
        req->size = getSize();
        memcpy(req->value, value, sizeof(T[N]));
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        KVDB_CHECK(key != nullptr, "PutPlainIntoKVRequest array key pointer is null");
        *key = req->key;
    }

    inline int getSize() const override
    {
        struct FakeData {
            BaseRequest common;
            uint64_t size;
            RID key;
            T value[N];
        };
        return sizeof(FakeData);
    }
};

template <typename ValueType>
class GetPlainFromKVRequest : public Request {
public:
    GetPlainFromKVRequest(int reqType, RID key, ValueType *value)
        : reqType(reqType)
        , key(key)
        , value(value)
    {
    }

    int reqType;
    RID key;
    ValueType *value;

    void serializeTo(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        req->common.reqType = reqType;
        req->key = key;
        req->size = getSize();
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        KVDB_CHECK(value != nullptr, "GetPlainFromKVRequest value pointer is null");
        memcpy(value, req->value, sizeof(*value));
    }

    inline int getSize() const override
    {
        struct FakeData {
            BaseRequest common;
            uint64_t size;
            RID key;
            ValueType value;
        };
        return sizeof(FakeData);
    }
};

template <typename T, std::size_t N>
class GetPlainFromKVRequest<T[N]> : public Request {
public:
    GetPlainFromKVRequest(int reqType, RID key, T *value)
        : reqType(reqType)
        , key(key)
        , value(value)
    {
    }

    int reqType;
    RID key;
    T *value;

    void serializeTo(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        req->common.reqType = reqType;
        req->key = key;
        req->size = getSize();
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (KVRequestData *)buffer;
        KVDB_CHECK(value != nullptr, "GetPlainFromKVRequest array value pointer is null");
        memcpy((void *)value, req->value, sizeof(T[N]));
    }

    inline int getSize() const override
    {
        struct FakeData {
            BaseRequest common;
            uint64_t size;
            RID key;
            T value[N];
        };
        return sizeof(FakeData);
    }
};

class KVBatchRequest : public Request {
public:
    KVBatchRequest(int reqType, int nreqs)
        : reqType(reqType)
        , nreqs(nreqs)
        , reqs_size(0)
    {
    }

    int reqType;
    int nreqs;
    std::vector<std::shared_ptr<Request>> reqs;
    mutable size_t reqs_size;

    void serializeTo(void* buffer) const override
    {
        auto req = (BatchRequestData *)buffer;
        req->common.reqType = reqType;
        req->nreqs = nreqs;
        auto buf_p = (uint8_t *)(req->buf);
        for (int i = 0; i < nreqs; i ++) {
            reqs[i]->serializeTo(buf_p);
            auto req_sz = reqs[i]->getSize();
            KVDB_CHECK(req_sz > 0, "invalid batch request size %d at index %d", req_sz, i);
            buf_p += req_sz;
            reqs_size += req_sz;
        }
        req->size = this->getSize();
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (BatchRequestData *)buffer;
        KVDB_CHECK(req->common.reqType == reqType,
            "batch request type mismatch: got %d expected %d", req->common.reqType, reqType);
        KVDB_CHECK(req->nreqs == nreqs,
            "batch request count mismatch: got %d expected %d", req->nreqs, nreqs);
        KVDB_CHECK(req->size == (uint64_t)this->getSize(),
            "batch request size mismatch: got %lu expected %d",
            (unsigned long)req->size, this->getSize());
        auto buf_p = (uint8_t *)(req->buf);
        for (int i = 0; i < req->nreqs; i ++) {
            reqs[i]->copyResultFrom(buf_p);
            buf_p += reqs[i]->getSize();
        }
    }

    inline int getSize() const override
    {
        return sizeof(BatchRequestData) + reqs_size;
    }
};

class TimestampExtractYearRequest : public Request {
public:
    TimestampExtractYearRequest(RID in, RID* res)
        : in(in)
        , res(res)
    {
    }

    RID in;
    RID* res;

    void serializeTo(void* buffer) const override
    {
        auto req = (TimestampExtractYearRequestData*)buffer;
        req->common.reqType = CMD_TIMESTAMP_EXTRACT_YEAR;
        req->in = in;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (TimestampExtractYearRequestData*)buffer;
        KVDB_CHECK(res != nullptr, "TimestampExtractYearRequest result pointer is null");
        *res = req->res;
    }
};

class SubstringRequest : public Request {
public:
    SubstringRequest(RID str, RID start, RID length, RID* res)
        : str(str)
        , start(start)
        , length(length)
        , res(res)
    {
    }

    RID str;
    RID start;
    RID length;
    RID* res;

    void serializeTo(void* buffer) const override
    {
        auto req = (SubstringRequestData *)buffer;
        req->common.reqType = CMD_TEXT_SUBSTRING;
        req->str = str;
        req->start = start;
        req->length = length;
    }

    inline void copyResultFrom(void* buffer) const override
    {
        auto req = (SubstringRequestData *)buffer;
        KVDB_CHECK(res != nullptr, "SubstringRequest result pointer is null");
        *res = req->res;
    }
};

class SingleArgRequest : public Request {
public:
    SingleArgRequest(int reqType)
        : reqType(reqType)
    {
    }

    int reqType;

    void serializeTo(void* buffer) const override
    {
        auto req = (SingleArgRequestData *)buffer;
        req->common.reqType = reqType;
    }

    inline void copyResultFrom(void* buffer) const override
    {
    }
};
