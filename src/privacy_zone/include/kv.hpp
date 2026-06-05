#pragma once

#include "kv-local.hpp"
#include "wal.hpp"

#include "file-map-operator.hpp"
extern FileMapKVStoreOperator kv_ops;

extern LocalKVStoreOperator localkv_ops;
inline RID idx2local(RID idx) {
#ifdef USE_KVMAP_PARTITION
    return make_partitioned_rid(RID_LOCAL_PARTITION, idx);
#else
    return (idx | (~RID_MASK));
#endif
}

inline RID local2idx(RID id) {
#ifdef USE_KVMAP_PARTITION
    return rid_index(id);
#else
    return (id & (RID_MASK));
#endif
}

#ifdef USE_PG_WAL_FLUSH_HOOK
extern WALLogOperator wal_ops;
inline void replay_log() 
{
    wal_ops.init();
    wal_ops.replay();
}

inline void truncate() {
    wal_ops.init();
    wal_ops.truncate();
}
#endif

inline void flush() {
#ifdef USE_PG_WAL_FLUSH_HOOK
    wal_ops.init();
    wal_ops.flush();
#endif
}

inline RID putInt(RID key, int value, bool local = true, RID partition = RID_DEFAULT_PARTITION)
{
    if (local) {
        return idx2local(localkv_ops.putInt(value));
    }

    RID res;
    if (key != INVALID_RID) {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        res = kv_ops.replaceInt(key, value);
    } else {
#ifdef USE_KVMAP_PARTITION
        res = kv_ops.putInt(value, partition);
#else
        kv_ops.init();
        res = kv_ops.putInt(value);
#endif
    }

    return res;
}

inline RID putFloat(RID key, float value, bool local = true, RID partition = RID_DEFAULT_PARTITION)
{
    if (local) {
        return idx2local(localkv_ops.putFloat(value));
    }

    RID res;
    if (key != INVALID_RID) {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        res = kv_ops.replaceFloat(key, value);
    } else {
#ifdef USE_KVMAP_PARTITION
        res = kv_ops.putFloat(value, partition);
#else
        kv_ops.init();
        res = kv_ops.putFloat(value);
#endif
    }

    return res;
}

inline RID putTimestamp(RID key, TIMESTAMP value, bool local = true, RID partition = RID_DEFAULT_PARTITION)
{
    if (local) {
        return idx2local(localkv_ops.putTimestamp(value));
    }

    RID res;
    if (key != INVALID_RID) {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        res = kv_ops.replaceTimestamp(key, value);
    } else {
#ifdef USE_KVMAP_PARTITION
        res = kv_ops.putTimestamp(value, partition);
#else
        kv_ops.init();
        res = kv_ops.putTimestamp(value);
#endif
    }

    return res;
}

inline RID putText(RID key, const char *value, bool local = true, RID partition = RID_DEFAULT_PARTITION)
{
    if (local) {
        return idx2local(localkv_ops.putString(value));
    }

    RID res;
    if (key != INVALID_RID) {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        res = kv_ops.replaceString(key, (char *)value);
    } else {
#ifdef USE_KVMAP_PARTITION
        res = kv_ops.putString(value, partition);
#else
        kv_ops.init();
        res = kv_ops.putString(value);
#endif
    }

    return res;
}

inline int getInt(RID key)
{
    if (islocal(key)) {
        return localkv_ops.getInt(local2idx(key));
    } else {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        return kv_ops.getInt(key);
    }

}

inline float getFloat(RID key)
{
    if (islocal(key)) {
        return localkv_ops.getFloat(local2idx(key));
    } else {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        return kv_ops.getFloat(key);
    }

}

inline TIMESTAMP getTimestamp(RID key)
{
    if (islocal(key)) {
        return localkv_ops.getTimestamp(local2idx(key));
    } else {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        return kv_ops.getTimestamp(key);
    }

}

inline const char *getText(RID key)
{
    if (islocal(key)) {
        return localkv_ops.getString(local2idx(key));
    } else {
#ifndef USE_KVMAP_PARTITION
        kv_ops.init();
#endif
        return kv_ops.getString(key);
    }

}

#ifdef USE_LOCAL_STORE_CLEAR
inline void freeLocalInt(RID key)
{
    if (islocal(key))
        localkv_ops.eraseInt(local2idx(key));
}

inline void freeLocalFloat(RID key)
{
    if (islocal(key))
        localkv_ops.eraseFloat(local2idx(key));
}

inline void freeLocalTimestamp(RID key)
{
    if (islocal(key))
        localkv_ops.eraseTimestamp(local2idx(key));
}

inline void freeLocalText(RID key)
{
    if (islocal(key))
        localkv_ops.eraseString(local2idx(key));
}
#endif
