#pragma once

#include "defs.h"
#include "rid.h"
#include <vector>
#include <string>
#include <cstring>

#ifdef USE_LOCAL_STORE_CLEAR
template <typename ValueType>
class LocalKVStore
{
public:
    LocalKVStore() = default;

    RID put(ValueType value)
    {
        RID key = store.size();
        store.push_back(value);
        return key;
    }
    ValueType get(RID key)
    {
        return store[key];
    }

    void replace(RID key, ValueType value) {
        if (key >= store.size()) {
            store.resize(key + 1);
        }
        store[key] = value;
    }

    void clear()
    {
        store.clear();
        store.shrink_to_fit();
    }

private:
    std::vector<ValueType> store;
};

template <typename ValueType>
class ReusableLocalKVStore
{
public:
    ReusableLocalKVStore() = default;

    RID put(ValueType value)
    {
        RID key;
        if (!free_list.empty()) {
            key = free_list.back();
            free_list.pop_back();
            store[key] = value;
            live[key] = 1;
            return key;
        }

        key = store.size();
        store.push_back(value);
        live.push_back(1);
        return key;
    }

    ValueType get(RID key)
    {
        return store[key];
    }

    void replace(RID key, ValueType value)
    {
        if (key >= store.size()) {
            store.resize(key + 1);
            live.resize(key + 1);
        }
        store[key] = value;
        live[key] = 1;
    }

    void erase(RID key)
    {
        if (key >= live.size() || !live[key])
            return;
        live[key] = 0;
        store[key] = ValueType();
        free_list.push_back(key);
    }

    void clear()
    {
        store.clear();
        live.clear();
        free_list.clear();
        store.shrink_to_fit();
        live.shrink_to_fit();
        free_list.shrink_to_fit();
    }

private:
    std::vector<ValueType> store;
    std::vector<uint8_t> live;
    std::vector<RID> free_list;
};

template <>
class ReusableLocalKVStore<char *>
{
public:
    ReusableLocalKVStore() = default;

    RID put(const char *value)
    {
        RID key;
        if (!free_list.empty()) {
            key = free_list.back();
            free_list.pop_back();
            store[key] = std::string(value);
            live[key] = 1;
            return key;
        }

        key = store.size();
        store.push_back(std::string(value));
        live.push_back(1);
        return key;
    }

    const char *get(RID key)
    {
        return store[key].c_str();
    }

    void replace(RID key, const char *value)
    {
        if (key >= store.size()) {
            store.resize(key + 1);
            live.resize(key + 1);
        }
        store[key] = std::string(value);
        live[key] = 1;
    }

    void erase(RID key)
    {
        if (key >= live.size() || !live[key])
            return;
        live[key] = 0;
        store[key].clear();
        free_list.push_back(key);
    }

    void clear()
    {
        store.clear();
        live.clear();
        free_list.clear();
        store.shrink_to_fit();
        live.shrink_to_fit();
        free_list.shrink_to_fit();
    }

private:
    std::vector<std::string> store;
    std::vector<uint8_t> live;
    std::vector<RID> free_list;
};

#ifdef USE_VARLEN_STRING
template <>
class LocalKVStore<char *>
{
public:
    LocalKVStore() = default;

    RID put(const char *value)
    {
        RID key = store.size();
        store.push_back(std::string(value));
        return key;
    }
    const char * get(RID key)
    {
        return store[key].c_str();
    }

    void replace(RID key, const char *value)
    {
        if (key >= store.size()) {
            store.resize(key + 1);
        }
        store[key] = std::string(value);
    }

    void clear()
    {
        store.clear();
        store.shrink_to_fit();
    }

private:
    std::vector<std::string> store;
};
#else
template <>
class LocalKVStore<char *>
{
public:
    LocalKVStore() = default;

    RID put(const char *value)
    {
        RID key = store.size();
        store.push_back(std::string(value, strnlen(value, STRING_LENGTH)));
        return key;
    }
    const char * get(RID key)
    {
        return store[key].c_str();
    }

    void replace(RID key, const char *value)
    {
        if (key >= store.size()) {
            store.resize(key + 1);
        }
        store[key] = std::string(value, strnlen(value, STRING_LENGTH));
    }

    void clear()
    {
        store.clear();
        store.shrink_to_fit();
    }

private:
    std::vector<std::string> store;
};
#endif

#else
template <typename ValueType>
class LocalKVStore
{
public:
    LocalKVStore() = default;

    RID put(ValueType value)
    {
        RID key = cur_rid++;
        store[key] = value;
        if (cur_rid >= MAX_ARRAY_ELE_NUM) {
            cur_rid = 0;
        }
        return key;
    }
    ValueType get(RID key)
    {
        return store[key];
    }

    void replace(RID key, ValueType value) {
        store[key] = value;
    }

private:
    RID cur_rid = 0;
    const static uint64_t MAX_ARRAY_ELE_NUM = 10 * 1024;
    ValueType store[MAX_ARRAY_ELE_NUM];
};

#ifdef USE_VARLEN_STRING
template <>
class LocalKVStore<char *>
{
public:
    LocalKVStore() = default;

    RID put(const char *value)
    {
        RID key = cur_rid++;
        if (key >= store.size()) {
            store.resize(key + 1);
        }
        store[key] = std::string(value);
        if (cur_rid >= MAX_ARRAY_ELE_NUM) {
            cur_rid = 0;
        }
        return key;
    }
    const char * get(RID key)
    {
        return store[key].c_str();
    }

    void replace(RID key, const char *value)
    {
        if (key >= store.size()) {
            store.resize(key + 1);
        }
        store[key] = std::string(value);
    }

private:
    RID cur_rid = 0;
    const static uint64_t MAX_ARRAY_ELE_NUM = 10 * 1024 * 1024 / STRING_LENGTH;
    std::vector<std::string> store;
};
#else
template <>
class LocalKVStore<char *>
{
public:
    LocalKVStore() = default;

    RID put(const char *value)
    {
        RID key = cur_rid++;
        memcpy(store[key], value, STRING_LENGTH);
        if (cur_rid >= MAX_ARRAY_ELE_NUM) {
            cur_rid = 0;
        }
        return key;
    }
    const char * get(RID key)
    {
        return store[key];
    }

    void replace(RID key, const char *value)
    {
        memcpy(store[key], value, STRING_LENGTH);
    }

private:
    RID cur_rid = 0;
    const static uint64_t MAX_ARRAY_ELE_NUM = 10 * 1024 / STRING_LENGTH;
    char store[MAX_ARRAY_ELE_NUM][STRING_LENGTH];
};
#endif
#endif

class LocalKVStoreOperator
{
public:
    LocalKVStoreOperator() = default;

    RID putInt(int value)
    {
        return int_store.put(value);
    }
    int getInt(RID key)
    {
        return int_store.get(key);
    }

    RID putFloat(float value)
    {
        return float_store.put(value);
    }
    float getFloat(RID key)
    {
        return float_store.get(key);
    }

    RID putTimestamp(TIMESTAMP value)
    {
        return timestamp_store.put(value);
    }
    TIMESTAMP getTimestamp(RID key)
    {
        return timestamp_store.get(key);
    }

    RID putString(const char *value)
    {
        return text_store.put(value);
    }
    const char *getString(RID key)
    {
        return text_store.get(key);
    }

    void replaceInt(RID key, int value)
    {
        int_store.replace(key, value);
    }
    void replaceFloat(RID key, float value)
    {
        float_store.replace(key, value);
    }
    void replaceTimestamp(RID key, TIMESTAMP value)
    {
        timestamp_store.replace(key, value);
    }
    void replaceString(RID key, char *value)
    {
        text_store.replace(key, value);
    }

#ifdef USE_LOCAL_STORE_CLEAR
    void eraseInt(RID key)
    {
        int_store.erase(key);
    }
    void eraseFloat(RID key)
    {
        float_store.erase(key);
    }
    void eraseTimestamp(RID key)
    {
        timestamp_store.erase(key);
    }
    void eraseString(RID key)
    {
        text_store.erase(key);
    }

    void clear()
    {
        int_store.clear();
        float_store.clear();
        timestamp_store.clear();
        text_store.clear();
    }
#endif

private:
#ifdef USE_LOCAL_STORE_CLEAR
    ReusableLocalKVStore<int> int_store;
    ReusableLocalKVStore<float> float_store;
    ReusableLocalKVStore<TIMESTAMP> timestamp_store;
    ReusableLocalKVStore<char *> text_store;
#else
    LocalKVStore<int> int_store;
    LocalKVStore<float> float_store;
    LocalKVStore<TIMESTAMP> timestamp_store;
    LocalKVStore<char *> text_store;
#endif
};
