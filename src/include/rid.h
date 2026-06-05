#pragma once

#include "defs.h"

typedef uint64_t RID;

#ifdef USE_KVMAP_PARTITION
#ifndef KVMAP_PARTITION_BITS
#define KVMAP_PARTITION_BITS 16
#endif

#define RID_PARTITION_SHIFT (64 - KVMAP_PARTITION_BITS)
#define RID_PARTITION_MASK (~((1ULL << RID_PARTITION_SHIFT) - 1ULL))
#define RID_MASK ((1ULL << RID_PARTITION_SHIFT) - 1ULL)
#define RID_LOCAL_PARTITION ((1ULL << KVMAP_PARTITION_BITS) - 1ULL)
#define RID_DEFAULT_PARTITION 0ULL
#else
#define RID_MASK ((1ULL << 63) - 1)
#define RID_DEFAULT_PARTITION 0ULL
#endif

#define INVALID_RID UINT64_MAX

#ifdef USE_KVMAP_PARTITION
inline RID rid_partition(RID id) {
    return id >> RID_PARTITION_SHIFT;
}

inline RID rid_index(RID id) {
    return id & RID_MASK;
}

inline RID make_partitioned_rid(RID partition, RID idx) {
    return (partition << RID_PARTITION_SHIFT) | (idx & RID_MASK);
}

inline bool islocal(RID id) {
    return rid_partition(id) == RID_LOCAL_PARTITION;
}
#else
inline bool islocal(RID id) {
    return 0ULL != (id & (~RID_MASK)) ? true : false;
}
#endif
