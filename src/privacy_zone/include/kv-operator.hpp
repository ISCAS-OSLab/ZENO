#pragma once

#include "defs.h"
#include "rid.h"

class KVStoreOperator
{
public:
    virtual ~KVStoreOperator() {}
    virtual void init() = 0;
    virtual void flush() = 0;

    virtual RID putInt(int value) = 0;
    virtual int getInt(RID key) = 0;

    virtual RID putFloat(float value) = 0;
    virtual float getFloat(RID key) = 0;

    virtual RID putTimestamp(TIMESTAMP value) = 0;
    virtual TIMESTAMP getTimestamp(RID key) = 0;

    virtual RID putString(const char *value) = 0;
    virtual const char *getString(RID key) = 0;

    virtual RID replaceInt(RID key, int value) = 0;
    virtual RID replaceFloat(RID key, float value) = 0;
    virtual RID replaceTimestamp(RID key, TIMESTAMP value) = 0;
    virtual RID replaceString(RID key, char *value) = 0;
};

class FileMapKVStoreOperator;
