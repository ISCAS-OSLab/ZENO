#pragma once

#include "kv-operator.hpp"
#include "util.h"

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <scoped_allocator>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>

#include <sys/mman.h>

using namespace boost::interprocess;

#define KV_MMAP_FILE_PREFIX "/tmp/kvmap_"
#define KV_MMAP_SEGMENT_NAME_PREFIX "kvdb_"
#define KV_MMAP_SIZE (1ULL << 40)
#define LRU_MMAP_SIZE (1500 * PAGE_SIZE)

#define PAGE_ALIGN_UP(v) (((v) + (PAGE_SIZE - 1ULL)) & (~(PAGE_SIZE - 1ULL)))

// 2% dirty pages
#define DRITY_PAGE_LIMITS (((8ULL * 1024 * 1024 * 1024) / PAGE_SIZE) / 50)

using segment_manager_t = managed_mapped_file::segment_manager;
using void_allocator = allocator<void, managed_mapped_file::segment_manager>;

template <typename ValueType>
class FileMapKVStore
{
public:
    FileMapKVStore(const void_allocator &alloc) {
        fprintf(stderr, "%llx %lx %llx\n", (PAGE_ALIGN_DOWN((uint64_t)store)), (uint64_t)store, (uint64_t)store + KV_MMAP_SIZE);
    }

    FileMapKVStore() = default;

    RID put(ValueType value)
    {
        RID key = cur_rid.fetch_add(1, std::memory_order_seq_cst);
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        store[key] = value;
        return key;
    }
    ValueType get(RID key)
    {
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        return store[key];
    }

    void replace(RID key, ValueType value) {
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        store[key] = value;
    }

    void flush_async() const
    {
        size_t used = cur_rid.load(std::memory_order_relaxed) * sizeof(ValueType);
        if (used == 0)
            return;
        uintptr_t start = (uintptr_t)store;
        uintptr_t aligned_start = PAGE_ALIGN_DOWN(start);
        uintptr_t end = start + used;
        uintptr_t aligned_end = PAGE_ALIGN_UP(end);
        msync((void *)aligned_start, aligned_end - aligned_start, MS_ASYNC);
    }

private:
    std::atomic<RID> cur_rid{0};
    const static size_t MAX_ARRAY_ELE_NUM = (KV_MMAP_SIZE - PAGE_SIZE) / sizeof(ValueType);
    ValueType store[MAX_ARRAY_ELE_NUM];
};

template <>
class FileMapKVStore<char *>
{
public:
    FileMapKVStore(const void_allocator &alloc) {
#ifdef USE_VARLEN_STRING
        fprintf(stderr,
                "dirty=%lld base=%llx store=%lx store_bytes=%zu offset_bytes=%zu\n",
                (long long)DRITY_PAGE_LIMITS,
                (unsigned long long)(PAGE_ALIGN_DOWN((uint64_t)store)),
                (uint64_t)store,
                (size_t)STORE_SIZE,
                (size_t)OFFSET_ARRAY_BYTES);
#else
        fprintf(stderr, "dirty=%lld %llx %lx %llx\n", DRITY_PAGE_LIMITS, (PAGE_ALIGN_DOWN((uint64_t)store)), (uint64_t)store, (uint64_t)store + KV_MMAP_SIZE);
#endif
    }
    FileMapKVStore() = default;

    RID put(const char *value)
    {
#ifdef USE_VARLEN_STRING
        RID key = cur_rid.fetch_add(1, std::memory_order_seq_cst);
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);

        size_t len = strlen(value);
        size_t write_len = len + 1; // include trailing null
        size_t write_pos = cur_store_offset.fetch_add(write_len, std::memory_order_seq_cst);

        KVDB_CHECK(write_pos + write_len <= STORE_SIZE,
            "text kvmap store out of range: %zu + %zu > %zu",
            write_pos, write_len, STORE_SIZE);

        offsets[key] = write_pos;
        memcpy(store + write_pos, value, len);
        store[write_pos + len] = '\0';

        return key;
#else
        RID key = cur_rid.fetch_add(1, std::memory_order_seq_cst);
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        memcpy(store[key], value, STRING_LENGTH);
        return key;
#endif
    }
    const char * get(RID key)
    {
#ifdef USE_VARLEN_STRING
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        return store + offsets[key];
#else
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        return store[key];
#endif
    }

    void replace(RID key, const char *value)
    {
#ifdef USE_VARLEN_STRING
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);

        size_t len = strlen(value);
        size_t write_len = len + 1;
        size_t write_pos = cur_store_offset.fetch_add(write_len, std::memory_order_seq_cst);

        KVDB_CHECK(write_pos + write_len <= STORE_SIZE,
            "text kvmap store out of range: %zu + %zu > %zu",
            write_pos, write_len, STORE_SIZE);

        offsets[key] = write_pos;
        memcpy(store + write_pos, value, len);
        store[write_pos + len] = '\0';
#else
        KVDB_CHECK(key < MAX_ARRAY_ELE_NUM,
            "text kvmap key out of range: %llu >= %zu",
            (unsigned long long)key, MAX_ARRAY_ELE_NUM);
        memcpy(store[key], value, STRING_LENGTH);
#endif
    }

    void flush_async() const
    {
#ifdef USE_VARLEN_STRING
        size_t store_used = cur_store_offset.load(std::memory_order_relaxed);
        if (store_used == 0)
            return;
        uintptr_t start = (uintptr_t)offsets;
        uintptr_t aligned_start = PAGE_ALIGN_DOWN(start);
        uintptr_t end = (uintptr_t)store + store_used;
        uintptr_t aligned_end = PAGE_ALIGN_UP(end);
        msync((void *)aligned_start, aligned_end - aligned_start, MS_ASYNC);
#else
        size_t used = cur_rid.load(std::memory_order_relaxed) * sizeof(char[STRING_LENGTH]);
        if (used == 0)
            return;
        uintptr_t start = (uintptr_t)store;
        uintptr_t aligned_start = PAGE_ALIGN_DOWN(start);
        uintptr_t end = start + used;
        uintptr_t aligned_end = PAGE_ALIGN_UP(end);
        msync((void *)aligned_start, aligned_end - aligned_start, MS_ASYNC);
#endif
    }

private:
#ifdef USE_VARLEN_STRING
    std::atomic<RID> cur_rid{0};
    std::atomic<size_t> cur_store_offset{0};
    const static size_t MAX_ARRAY_ELE_NUM = 512ULL * 1024 * 1024;
    const static size_t OFFSET_ARRAY_BYTES = MAX_ARRAY_ELE_NUM * sizeof(size_t);
    const static size_t STORE_SIZE = (KV_MMAP_SIZE - PAGE_SIZE) - OFFSET_ARRAY_BYTES;
    size_t offsets[MAX_ARRAY_ELE_NUM];
    char store[STORE_SIZE];
#else
    std::atomic<RID> cur_rid{0};
    const static size_t MAX_ARRAY_ELE_NUM = (KV_MMAP_SIZE - PAGE_SIZE)/ sizeof(char[STRING_LENGTH]);
    char store[MAX_ARRAY_ELE_NUM][STRING_LENGTH];
#endif
};

class FileMapKVStoreOperator : public KVStoreOperator
{
public:
    FileMapKVStoreOperator() = default;

    ~FileMapKVStoreOperator()
    {
#ifdef USE_KVMAP_PARTITION
        for (auto &[_, segment] : int_segments) segment.flush();
        for (auto &[_, segment] : float_segments) segment.flush();
        for (auto &[_, segment] : timestamp_segments) segment.flush();
        for (auto &[_, segment] : text_segments) segment.flush();
#else
        int_segment.flush();
        float_segment.flush();
        timestamp_segment.flush();
        text_segment.flush();
#endif
    }

    void init() override
    {
#ifdef USE_KVMAP_PARTITION
        return;
#else
        if (likely(int_store != nullptr)) return;

        init_store<int>(&int_store, &int_segment, KV_MMAP_FILE_PREFIX "int", KV_MMAP_SEGMENT_NAME_PREFIX "int");
        init_store<float>(&float_store, &float_segment, KV_MMAP_FILE_PREFIX "float", KV_MMAP_SEGMENT_NAME_PREFIX "float");
        init_store<TIMESTAMP>(&timestamp_store, &timestamp_segment, KV_MMAP_FILE_PREFIX "timestamp", KV_MMAP_SEGMENT_NAME_PREFIX "timestamp");
        init_store<char *>(&text_store, &text_segment, KV_MMAP_FILE_PREFIX "text", KV_MMAP_SEGMENT_NAME_PREFIX "text");

        KVDB_CHECK(int_store != nullptr, "FileMapKVStore not found: int");
        KVDB_CHECK(float_store != nullptr, "FileMapKVStore not found: float");
        KVDB_CHECK(timestamp_store != nullptr, "FileMapKVStore not found: timestamp");
        KVDB_CHECK(text_store != nullptr, "FileMapKVStore not found: text");
#endif
    }

    void flush() override
    {
#ifdef USE_KVMAP_PARTITION
        for (auto &[_, segment] : int_segments) segment.flush();
        for (auto &[_, segment] : float_segments) segment.flush();
        for (auto &[_, segment] : timestamp_segments) segment.flush();
        for (auto &[_, segment] : text_segments) segment.flush();
#else
        int_segment.flush();
        float_segment.flush();
        timestamp_segment.flush();
        text_segment.flush();
#endif
    }

    void flush_async()
    {
#ifdef USE_KVMAP_PARTITION
        for (auto &[_, store] : int_stores) store->flush_async();
        for (auto &[_, store] : float_stores) store->flush_async();
        for (auto &[_, store] : timestamp_stores) store->flush_async();
        for (auto &[_, store] : text_stores) store->flush_async();
#else
        if (int_store != nullptr) int_store->flush_async();
        if (float_store != nullptr) float_store->flush_async();
        if (timestamp_store != nullptr) timestamp_store->flush_async();
        if (text_store != nullptr) text_store->flush_async();
#endif
    }

    RID putInt(int value) override
    {
        return putInt(value, RID_DEFAULT_PARTITION);
    }

    RID putInt(int value, RID partition)
    {
#ifdef USE_KVMAP_PARTITION
        init_partition(partition);
        RID key = int_stores[partition]->put(value);
        KVDB_CHECK(key <= RID_MASK,
            "partitioned int kvmap key out of range: partition=%llu key=%llu mask=%llu",
            (unsigned long long)partition, (unsigned long long)key, (unsigned long long)RID_MASK);
        return make_partitioned_rid(partition, key);
#else
        (void)partition;
        RID key = int_store->put(value);
        return key;
#endif
    }
    int getInt(RID key) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        return int_stores[partition]->get(rid_index(key));
#else
        return int_store->get(key);
#endif
    }

    RID putFloat(float value) override
    {
        return putFloat(value, RID_DEFAULT_PARTITION);
    }

    RID putFloat(float value, RID partition)
    {
#ifdef USE_KVMAP_PARTITION
        init_partition(partition);
        RID key = float_stores[partition]->put(value);
        KVDB_CHECK(key <= RID_MASK,
            "partitioned float kvmap key out of range: partition=%llu key=%llu mask=%llu",
            (unsigned long long)partition, (unsigned long long)key, (unsigned long long)RID_MASK);
        return make_partitioned_rid(partition, key);
#else
        (void)partition;
        RID key = float_store->put(value);
        return key;
#endif
    }
    float getFloat(RID key) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        return float_stores[partition]->get(rid_index(key));
#else
        return float_store->get(key);
#endif
    }

    RID putTimestamp(TIMESTAMP value) override
    {
        return putTimestamp(value, RID_DEFAULT_PARTITION);
    }

    RID putTimestamp(TIMESTAMP value, RID partition)
    {
#ifdef USE_KVMAP_PARTITION
        init_partition(partition);
        RID key = timestamp_stores[partition]->put(value);
        KVDB_CHECK(key <= RID_MASK,
            "partitioned timestamp kvmap key out of range: partition=%llu key=%llu mask=%llu",
            (unsigned long long)partition, (unsigned long long)key, (unsigned long long)RID_MASK);
        return make_partitioned_rid(partition, key);
#else
        (void)partition;
        RID key = timestamp_store->put(value);
        return key;
#endif
    }
    TIMESTAMP getTimestamp(RID key) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        return timestamp_stores[partition]->get(rid_index(key));
#else
        return timestamp_store->get(key);
#endif
    }

    RID putString(const char *value) override
    {
        return putString(value, RID_DEFAULT_PARTITION);
    }

    RID putString(const char *value, RID partition)
    {
#ifdef USE_KVMAP_PARTITION
        init_partition(partition);
        RID key = text_stores[partition]->put(value);
        KVDB_CHECK(key <= RID_MASK,
            "partitioned text kvmap key out of range: partition=%llu key=%llu mask=%llu",
            (unsigned long long)partition, (unsigned long long)key, (unsigned long long)RID_MASK);
        return make_partitioned_rid(partition, key);
#else
        (void)partition;
        RID key = text_store->put(value);
        return key;
#endif
    }
    const char *getString(RID key) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        return text_stores[partition]->get(rid_index(key));
#else
        return text_store->get(key);
#endif
    }

    RID replaceInt(RID key, int value) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        int_stores[partition]->replace(rid_index(key), value);
#else
        int_store->replace(key, value);
#endif
        return key;
    }
    RID replaceFloat(RID key, float value) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        float_stores[partition]->replace(rid_index(key), value);
#else
        float_store->replace(key, value);
#endif
        return key;
    }
    RID replaceTimestamp(RID key, TIMESTAMP value) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        timestamp_stores[partition]->replace(rid_index(key), value);
#else
        timestamp_store->replace(key, value);
#endif
        return key;
    }
    RID replaceString(RID key, char *value) override
    {
#ifdef USE_KVMAP_PARTITION
        RID partition = rid_partition(key);
        init_partition(partition);
        text_stores[partition]->replace(rid_index(key), value);
#else
        text_store->replace(key, value);
#endif
        return key;
    }

private:
#ifdef USE_KVMAP_PARTITION
    std::unordered_map<RID, managed_mapped_file> int_segments;
    std::unordered_map<RID, managed_mapped_file> float_segments;
    std::unordered_map<RID, managed_mapped_file> timestamp_segments;
    std::unordered_map<RID, managed_mapped_file> text_segments;

    std::unordered_map<RID, FileMapKVStore<int> *> int_stores;
    std::unordered_map<RID, FileMapKVStore<float> *> float_stores;
    std::unordered_map<RID, FileMapKVStore<TIMESTAMP> *> timestamp_stores;
    std::unordered_map<RID, FileMapKVStore<char *> *> text_stores;
#else
    FileMapKVStore<int> *int_store = nullptr;
    FileMapKVStore<float> *float_store = nullptr;
    FileMapKVStore<TIMESTAMP> *timestamp_store = nullptr;
    FileMapKVStore<char *> *text_store = nullptr;

    managed_mapped_file int_segment;
    managed_mapped_file float_segment;
    managed_mapped_file timestamp_segment;
    managed_mapped_file text_segment;
#endif

#ifdef USE_KVMAP_PARTITION
    void init_partition(RID partition)
    {
        KVDB_CHECK(partition != RID_LOCAL_PARTITION,
            "local RID partition cannot be used as kvmap partition: %llu",
            (unsigned long long)partition);

        if (int_stores.find(partition) != int_stores.end()) {
            return;
        }

        std::string suffix = "_" + std::to_string((unsigned long long)partition);
        init_store<int>(&int_stores[partition], &int_segments[partition], KV_MMAP_FILE_PREFIX "int" + suffix, KV_MMAP_SEGMENT_NAME_PREFIX "int" + suffix);
        init_store<float>(&float_stores[partition], &float_segments[partition], KV_MMAP_FILE_PREFIX "float" + suffix, KV_MMAP_SEGMENT_NAME_PREFIX "float" + suffix);
        init_store<TIMESTAMP>(&timestamp_stores[partition], &timestamp_segments[partition], KV_MMAP_FILE_PREFIX "timestamp" + suffix, KV_MMAP_SEGMENT_NAME_PREFIX "timestamp" + suffix);
        init_store<char *>(&text_stores[partition], &text_segments[partition], KV_MMAP_FILE_PREFIX "text" + suffix, KV_MMAP_SEGMENT_NAME_PREFIX "text" + suffix);

        KVDB_CHECK(int_stores[partition] != nullptr, "FileMapKVStore not found: int partition=%llu", (unsigned long long)partition);
        KVDB_CHECK(float_stores[partition] != nullptr, "FileMapKVStore not found: float partition=%llu", (unsigned long long)partition);
        KVDB_CHECK(timestamp_stores[partition] != nullptr, "FileMapKVStore not found: timestamp partition=%llu", (unsigned long long)partition);
        KVDB_CHECK(text_stores[partition] != nullptr, "FileMapKVStore not found: text partition=%llu", (unsigned long long)partition);
    }
#endif

    template <typename ValueType>
#ifdef USE_KVMAP_PARTITION
    void init_store(FileMapKVStore<ValueType> **store, managed_mapped_file *g_segment, const std::string &mmap_file_name, const std::string &mmap_seg_name) {
#else
    void init_store(FileMapKVStore<ValueType> **store, managed_mapped_file *g_segment, const char *mmap_file_name, const char *mmap_seg_name) {
#endif
        if (*store == nullptr)
        {
#ifdef USE_KVMAP_PARTITION
            const char *mmap_file = mmap_file_name.c_str();
            const char *mmap_seg = mmap_seg_name.c_str();
#else
            const char *mmap_file = mmap_file_name;
            const char *mmap_seg = mmap_seg_name;
#endif

            *g_segment = managed_mapped_file(open_or_create, mmap_file, KV_MMAP_SIZE);
            void_allocator void_alloc_inst(g_segment->get_segment_manager());
            *store = g_segment->find_or_construct<FileMapKVStore<ValueType>>(mmap_seg)(void_alloc_inst);
            g_segment->flush();
        }

        if (unlikely(*store == nullptr))
            fprintf(stderr, "FileMapKVStore not found\n");
    }
};
