#pragma once

#ifdef USE_PG_WAL_FLUSH_HOOK

#include "defs.h"
#include "rid.h"
#include "request_types.h"
#include "util.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "file-map-operator.hpp"
extern FileMapKVStoreOperator kv_ops;
int gcm_encrypt(uint8_t *in, uint64_t in_sz, uint8_t *out, uint64_t *out_sz);
int gcm_decrypt(uint8_t *in, uint64_t in_sz, uint8_t *out, uint64_t *out_sz);

#define WAL_DATA_FILE "/mnt/encrypted-ssd/kvmap/wal_log_v4"
#define WAL_PENDING_SHM_PREFIX "/zeno_wal_pending_v3_"

typedef struct {
    int type;   // 0 int, 1 float, 2 text, 3 timestamp
    RID key;
    union {
        int i;
        float f;
        char s[STRING_LENGTH];
        TIMESTAMP t;
    } value;
} WALEntry;

class WALLogOperator
{
public:
    WALLogOperator() = default;

    ~WALLogOperator() {
        if (is_init()) flush();
    }

    bool is_init() {
        return pending != nullptr && wal_fd != -1;
    }

    void init()
    {
        if (likely(is_init())) return;

        init_pending();
        wal_fd = open(WAL_DATA_FILE, O_CREAT | O_RDWR, 0600);
        KVDB_CHECK(wal_fd != -1, "failed to open WAL file %s: errno=%d", WAL_DATA_FILE, errno);
        init_wal_file();
    }

    void appendBatch(const WalRidItem *items, int nitems)
    {
        if (nitems <= 0) return;
        init();

        for (int i = 0; i < nitems; i++) {
            KVDB_CHECK(items[i].type >= 0 && items[i].type <= 3,
                "invalid WAL RID type: %d", items[i].type);
        }

        lock_fd(pending_fd);
        KVDB_CHECK(pending->nitems + (size_t)nitems <= MAX_PENDING_ITEMS,
            "pending WAL RID log is full: nitems=%zu append=%d max=%zu",
            pending->nitems, nitems, MAX_PENDING_ITEMS);
        memcpy(&pending->items[pending->nitems], items, (size_t)nitems * sizeof(WalRidItem));
        pending->nitems += (size_t)nitems;
        unlock_fd(pending_fd);
    }

    uint64_t materializeBatch(const WalRidItem *items, int nitems, uint8_t *payload, uint64_t payload_capacity)
    {
        if (nitems <= 0) return 0;
        KVDB_CHECK(nitems <= WAL_MATERIALIZE_RID_BATCH_SIZE,
            "invalid materialize WAL RID batch size: %d", nitems);

        for (int i = 0; i < nitems; i++) {
            KVDB_CHECK(items[i].type >= 0 && items[i].type <= 3,
                "invalid WAL RID type: %d", items[i].type);
        }

        std::vector<uint8_t> plaintext;
        materialize_payload(items, (size_t)nitems, plaintext);

        uint64_t plaintext_len = plaintext.size();
        uint64_t payload_len = plaintext_len + IV_SIZE + TAG_SIZE;
        KVDB_CHECK(payload_len <= payload_capacity,
            "WAL payload buffer too small: need=%lu capacity=%lu",
            (unsigned long)payload_len, (unsigned long)payload_capacity);
        KVDB_CHECK(gcm_encrypt(plaintext.data(), plaintext_len, payload, &payload_len) == 0,
            "failed to encrypt WAL payload");
        return payload_len;
    }

    void replayPayload(uint8_t *payload, uint64_t payload_len)
    {
        if (payload_len == 0) return;
        KVDB_CHECK(payload_len >= IV_SIZE + TAG_SIZE,
            "WAL replay payload too short: len=%lu min=%d",
            (unsigned long)payload_len, IV_SIZE + TAG_SIZE);

        uint64_t plaintext_len = payload_len - IV_SIZE - TAG_SIZE;
        std::vector<uint8_t> plaintext(plaintext_len);
        uint64_t decrypt_len = plaintext_len;
        KVDB_CHECK(gcm_decrypt(payload, payload_len, plaintext.data(), &decrypt_len) == 0,
            "failed to decrypt WAL replay payload");
        KVDB_CHECK(decrypt_len == plaintext_len,
            "unexpected WAL replay plaintext size: got=%lu expected=%lu",
            (unsigned long)decrypt_len, (unsigned long)plaintext_len);

        kv_ops.init();
        replay_payload(plaintext.data(), plaintext.size());
    }

    void truncate()
    {
        init();
        lock_fd(wal_fd);
        lock_fd(pending_fd);
        pending->nitems = 0;
        unlock_fd(pending_fd);
        header.write_pos = WAL_DATA_OFFSET;
        write_header();
        KVDB_CHECK(fdatasync(wal_fd) == 0, "failed to fdatasync truncated WAL file: errno=%d", errno);
        unlock_fd(wal_fd);
    }

    void flush()
    {
        init();
        lock_fd(wal_fd);

        std::vector<WalRidItem> items = take_pending();
        if (!items.empty()) {
            std::vector<WALEntry> entries(items.size());
            materialize(items.data(), entries.data(), items.size());
            size_t size = entries.size() * sizeof(WALEntry);
            KVDB_CHECK(header.write_pos + size <= WAL_FILE_SIZE,
                "WAL file is full: write_pos=%llu size=%zu max=%llu",
                (unsigned long long)header.write_pos, size, (unsigned long long)WAL_FILE_SIZE);
            pwrite_full(wal_fd, entries.data(), size, (off_t)header.write_pos);
            header.write_pos += size;
            write_header();
            KVDB_CHECK(fdatasync(wal_fd) == 0, "failed to fdatasync WAL file: errno=%d", errno);
        }

        unlock_fd(wal_fd);
    }

    void replay()
    {
        init();
        lock_fd(wal_fd);
        KVDB_CHECK(lseek(wal_fd, 0, SEEK_SET) != (off_t)-1, "failed to seek WAL file: errno=%d", errno);

        size_t entry_num = 0;
        WALEntry entry;
        kv_ops.init();
        for (uint64_t pos = WAL_DATA_OFFSET; pos < header.write_pos; pos += sizeof(WALEntry)) {
            read_full(wal_fd, &entry, sizeof(entry), (off_t)pos);
            replay_entry(entry);
            entry_num++;
        }

        KVDB_CHECK(lseek(wal_fd, 0, SEEK_END) != (off_t)-1, "failed to seek WAL end: errno=%d", errno);
        unlock_fd(wal_fd);
        printf("Totally %zu store\n", entry_num);
    }

private:
    static constexpr uint64_t PENDING_MAGIC = 0x4645444257414c33ULL; // ZENOWAL3
    static constexpr uint64_t WAL_MAGIC = 0x4645444257414c34ULL; // ZENOWAL4
    static constexpr uint32_t WAL_PAYLOAD_MAGIC = 0x31564c57U; // WLV1
    static constexpr uint16_t WAL_PAYLOAD_VERSION = 1;
    static constexpr uint64_t WAL_FILE_SIZE = 4ULL << 30;
    static constexpr uint64_t WAL_DATA_OFFSET = PAGE_SIZE;
    static constexpr size_t MAX_PENDING_ITEMS = 1ULL << 20;

    struct WALHeader {
        uint64_t magic;
        uint64_t write_pos;
    };

    struct WALPendingLog {
        uint64_t magic;
        size_t nitems;
        WalRidItem items[MAX_PENDING_ITEMS];
    };

    struct WALPayloadHeader {
        uint32_t magic;
        uint16_t version;
        uint16_t nitems;
    };

    WALPendingLog *pending = nullptr;
    WALHeader header = {};
    int pending_fd = -1;
    int wal_fd = -1;
    char pending_shm_name[64] = {};

    void init_wal_file()
    {
        struct stat st;
        KVDB_CHECK(fstat(wal_fd, &st) == 0, "failed to stat WAL file: errno=%d", errno);
        bool need_prealloc = (uint64_t)st.st_size != WAL_FILE_SIZE;
        if (need_prealloc) {
            KVDB_CHECK(ftruncate(wal_fd, (off_t)WAL_FILE_SIZE) == 0,
                "failed to size WAL file: errno=%d", errno);
            int ret = posix_fallocate(wal_fd, 0, (off_t)WAL_FILE_SIZE);
            KVDB_CHECK(ret == 0, "failed to preallocate WAL file: errno=%d", ret);
        }

        lock_fd(wal_fd);
        load_header();
        if (header.magic != WAL_MAGIC || header.write_pos < WAL_DATA_OFFSET ||
            header.write_pos > WAL_FILE_SIZE ||
            (header.write_pos - WAL_DATA_OFFSET) % sizeof(WALEntry) != 0) {
            header.magic = WAL_MAGIC;
            header.write_pos = WAL_DATA_OFFSET;
            write_header();
            KVDB_CHECK(fdatasync(wal_fd) == 0, "failed to fdatasync initialized WAL file: errno=%d", errno);
        }
        unlock_fd(wal_fd);
    }

    void init_pending()
    {
        int n = snprintf(pending_shm_name, sizeof(pending_shm_name), "%s%ld",
            WAL_PENDING_SHM_PREFIX, (long)getppid());
        KVDB_CHECK(n > 0 && (size_t)n < sizeof(pending_shm_name),
            "failed to build WAL pending shm name");

        pending_fd = shm_open(pending_shm_name, O_CREAT | O_RDWR, 0600);
        KVDB_CHECK(pending_fd != -1, "failed to open WAL pending shm %s: errno=%d", pending_shm_name, errno);
        KVDB_CHECK(ftruncate(pending_fd, sizeof(WALPendingLog)) == 0,
            "failed to size WAL pending shm: errno=%d", errno);

        void *addr = mmap(nullptr, sizeof(WALPendingLog), PROT_READ | PROT_WRITE, MAP_SHARED, pending_fd, 0);
        KVDB_CHECK(addr != MAP_FAILED, "failed to mmap WAL pending shm: errno=%d", errno);
        pending = (WALPendingLog *)addr;

        lock_fd(pending_fd);
        if (pending->magic != PENDING_MAGIC) {
            memset(pending, 0, sizeof(WALPendingLog));
            pending->magic = PENDING_MAGIC;
        }
        unlock_fd(pending_fd);
    }

    std::vector<WalRidItem> take_pending()
    {
        lock_fd(pending_fd);
        std::vector<WalRidItem> items(pending->nitems);
        if (!items.empty()) {
            memcpy(items.data(), pending->items, items.size() * sizeof(WalRidItem));
            pending->nitems = 0;
        }
        unlock_fd(pending_fd);
        return items;
    }

    void materialize(const WalRidItem *items, WALEntry *entries, size_t nitems)
    {
        kv_ops.init();
        for (size_t i = 0; i < nitems; i++) {
            entries[i].type = items[i].type;
            entries[i].key = items[i].rid;
            switch (items[i].type) {
            case 0:
                entries[i].value.i = kv_ops.getInt(items[i].rid);
                break;
            case 1:
                entries[i].value.f = kv_ops.getFloat(items[i].rid);
                break;
            case 2:
                memcpy(entries[i].value.s, kv_ops.getString(items[i].rid), STRING_LENGTH);
                break;
            case 3:
                entries[i].value.t = kv_ops.getTimestamp(items[i].rid);
                break;
            default:
                KVDB_FATAL("invalid WAL entry type: %d", items[i].type);
            }
        }
    }

    void materialize_payload(const WalRidItem *items, size_t nitems, std::vector<uint8_t> &out)
    {
        KVDB_CHECK(nitems <= UINT16_MAX, "too many WAL payload items: %zu", nitems);

        kv_ops.init();
        WALPayloadHeader header = {WAL_PAYLOAD_MAGIC, WAL_PAYLOAD_VERSION, (uint16_t)nitems};
        append_bytes(out, &header, sizeof(header));

        for (size_t i = 0; i < nitems; i++) {
            uint8_t type = (uint8_t)items[i].type;
            append_bytes(out, &type, sizeof(type));

            switch (items[i].type) {
            case 0: {
                int value = kv_ops.getInt(items[i].rid);
                append_record_value(out, items[i].rid, &value, sizeof(value));
                break;
            }
            case 1: {
                float value = kv_ops.getFloat(items[i].rid);
                append_record_value(out, items[i].rid, &value, sizeof(value));
                break;
            }
            case 2: {
                const char *value = kv_ops.getString(items[i].rid);
                size_t len = strnlen(value, STRING_LENGTH - 1) + 1;
                append_record_value(out, items[i].rid, value, len);
                break;
            }
            case 3: {
                TIMESTAMP value = kv_ops.getTimestamp(items[i].rid);
                append_record_value(out, items[i].rid, &value, sizeof(value));
                break;
            }
            default:
                KVDB_FATAL("invalid WAL entry type: %d", items[i].type);
            }
        }
    }

    void replay_payload(const uint8_t *data, size_t size)
    {
        const uint8_t *ptr = data;
        const uint8_t *end = data + size;
        WALPayloadHeader header;
        read_bytes(ptr, end, &header, sizeof(header));
        KVDB_CHECK(header.magic == WAL_PAYLOAD_MAGIC && header.version == WAL_PAYLOAD_VERSION,
            "invalid WAL payload header: magic=0x%x version=%u",
            header.magic, header.version);

        for (uint16_t i = 0; i < header.nitems; i++) {
            uint8_t type;
            uint16_t len;
            RID key;

            read_bytes(ptr, end, &type, sizeof(type));
            read_bytes(ptr, end, &len, sizeof(len));
            read_bytes(ptr, end, &key, sizeof(key));
            KVDB_CHECK(ptr + len <= end,
                "truncated WAL payload record: len=%u remaining=%zu",
                len, (size_t)(end - ptr));

            switch (type) {
            case 0: {
                int value;
                KVDB_CHECK(len == sizeof(value), "invalid int WAL value length: %u", len);
                memcpy(&value, ptr, sizeof(value));
                kv_ops.replaceInt(key, value);
                break;
            }
            case 1: {
                float value;
                KVDB_CHECK(len == sizeof(value), "invalid float WAL value length: %u", len);
                memcpy(&value, ptr, sizeof(value));
                kv_ops.replaceFloat(key, value);
                break;
            }
            case 2: {
                KVDB_CHECK(len > 0 && len <= STRING_LENGTH,
                    "invalid text WAL value length: %u", len);
                char value[STRING_LENGTH] = {};
                memcpy(value, ptr, len);
                value[STRING_LENGTH - 1] = '\0';
                kv_ops.replaceString(key, value);
                break;
            }
            case 3: {
                TIMESTAMP value;
                KVDB_CHECK(len == sizeof(value), "invalid timestamp WAL value length: %u", len);
                memcpy(&value, ptr, sizeof(value));
                kv_ops.replaceTimestamp(key, value);
                break;
            }
            default:
                KVDB_FATAL("invalid WAL payload type: %u", type);
            }
            ptr += len;
        }
        KVDB_CHECK(ptr == end, "trailing bytes in WAL payload: %zu", (size_t)(end - ptr));
    }

    void replay_entry(WALEntry &entry)
    {
        switch (entry.type) {
        case 0:
            kv_ops.replaceInt(entry.key, entry.value.i);
            break;
        case 1:
            kv_ops.replaceFloat(entry.key, entry.value.f);
            break;
        case 2:
            kv_ops.replaceString(entry.key, entry.value.s);
            break;
        case 3:
            kv_ops.replaceTimestamp(entry.key, entry.value.t);
            break;
        default:
            KVDB_FATAL("invalid WAL entry type: %d", entry.type);
        }
    }

    static void append_bytes(std::vector<uint8_t> &out, const void *data, size_t size)
    {
        const uint8_t *ptr = (const uint8_t *)data;
        out.insert(out.end(), ptr, ptr + size);
    }

    static void append_record_value(std::vector<uint8_t> &out, RID key, const void *value, size_t len)
    {
        KVDB_CHECK(len <= UINT16_MAX, "WAL record value too large: %zu", len);
        uint16_t value_len = (uint16_t)len;
        append_bytes(out, &value_len, sizeof(value_len));
        append_bytes(out, &key, sizeof(key));
        append_bytes(out, value, len);
    }

    static void read_bytes(const uint8_t *&ptr, const uint8_t *end, void *dst, size_t size)
    {
        KVDB_CHECK(ptr + size <= end,
            "truncated WAL payload: need=%zu remaining=%zu", size, (size_t)(end - ptr));
        memcpy(dst, ptr, size);
        ptr += size;
    }

    static void lock_fd(int fd)
    {
        struct flock lock;
        memset(&lock, 0, sizeof(lock));
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        while (fcntl(fd, F_SETLKW, &lock) == -1) {
            if (errno == EINTR) continue;
            KVDB_FATAL("failed to lock WAL fd: errno=%d", errno);
        }
    }

    static void unlock_fd(int fd)
    {
        struct flock lock;
        memset(&lock, 0, sizeof(lock));
        lock.l_type = F_UNLCK;
        lock.l_whence = SEEK_SET;
        KVDB_CHECK(fcntl(fd, F_SETLK, &lock) != -1, "failed to unlock WAL fd: errno=%d", errno);
    }

    static void write_full(int fd, const void *buf, size_t size)
    {
        const char *ptr = (const char *)buf;
        while (size > 0) {
            ssize_t n = write(fd, ptr, size);
            if (n == -1) {
                if (errno == EINTR) continue;
                KVDB_FATAL("failed to write WAL file: errno=%d", errno);
            }
            ptr += n;
            size -= (size_t)n;
        }
    }

    void load_header()
    {
        ssize_t n = pread(wal_fd, &header, sizeof(header), 0);
        KVDB_CHECK(n == (ssize_t)sizeof(header) || n == 0,
            "failed to read WAL header: size=%zd errno=%d", n, errno);
    }

    void write_header()
    {
        header.magic = WAL_MAGIC;
        pwrite_full(wal_fd, &header, sizeof(header), 0);
    }

    static void read_full(int fd, void *buf, size_t size, off_t offset)
    {
        char *ptr = (char *)buf;
        while (size > 0) {
            ssize_t n = pread(fd, ptr, size, offset);
            if (n == -1) {
                if (errno == EINTR) continue;
                KVDB_FATAL("failed to read WAL file: errno=%d", errno);
            }
            KVDB_CHECK(n > 0, "unexpected EOF while reading WAL file");
            ptr += n;
            offset += n;
            size -= (size_t)n;
        }
    }

    static void pwrite_full(int fd, const void *buf, size_t size, off_t offset)
    {
        const char *ptr = (const char *)buf;
        while (size > 0) {
            ssize_t n = pwrite(fd, ptr, size, offset);
            if (n == -1) {
                if (errno == EINTR) continue;
                KVDB_FATAL("failed to write WAL file: errno=%d", errno);
            }
            ptr += n;
            offset += n;
            size -= (size_t)n;
        }
    }
};

#endif
