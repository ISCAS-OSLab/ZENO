#pragma once

#include <util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_NAME "/dev/shm/ipcshm"
#define SHM_SIZE (540 * 1024 * 1024)
#define META_REQ_SIZE 1024
#define REQ_REGION_SIZE (1024 * 1024)
#define REGION_N_OFFSET(n) (META_REQ_SIZE + REQ_REGION_SIZE * (n))

#define MAX_REGION_NUM ((SHM_SIZE - META_REQ_SIZE) / REQ_REGION_SIZE)
#define BITMAP_BITS 64
#define BITMAP_SIZE ((MAX_REGION_NUM + BITMAP_BITS - 1) / BITMAP_BITS)

typedef enum ShmReqStatus {
    SHM_NONE,
    SHM_GET,
    SHM_FREE,
    SHM_DONE,
    SHM_EXIT
} ShmReqStat;

#define BITMAP_WORD(n) ((n) / BITMAP_BITS)
#define BITMAP_MASK(n) (1ULL << ((n) % BITMAP_BITS))
#define GET(a, n) ((a[BITMAP_WORD(n)] & BITMAP_MASK(n)) != 0)
#define SET(a, n) { a[BITMAP_WORD(n)] |= BITMAP_MASK(n); }
#define CLEAR(a, n) { a[BITMAP_WORD(n)] &= ~BITMAP_MASK(n); }

typedef struct {
    int volatile lock;
    int __res; // reserved filed, avoid cache false sharing.
    ShmReqStat volatile status;
    int free_id; // free region id
    int ret_id;  // return region id
    unsigned long long bitmap[BITMAP_SIZE];
} OpsReq;

static inline int claim_shm_region(OpsReq* req)
{
    for (int region_id = 0; region_id < MAX_REGION_NUM; region_id++) {
        unsigned long long mask = BITMAP_MASK(region_id);
        unsigned long long* word = &req->bitmap[BITMAP_WORD(region_id)];

        while (1) {
            unsigned long long old = __atomic_load_n(word, __ATOMIC_RELAXED);
            if ((old & mask) != 0) {
                break;
            }
            if (__sync_bool_compare_and_swap(word, old, old | mask)) {
                return region_id;
            }
            YIELD_PROCESSOR;
        }
    }

    return -1;
}

static inline void release_shm_region(OpsReq* req, int region_id)
{
    KVDB_CHECK(region_id >= 0 && region_id < MAX_REGION_NUM,
        "invalid shared memory region id %d", region_id);

    unsigned long long mask = BITMAP_MASK(region_id);
    unsigned long long old = __sync_fetch_and_and(
        &req->bitmap[BITMAP_WORD(region_id)], ~mask);
    KVDB_CHECK((old & mask) != 0, "freeing unallocated shared memory region %d", region_id);
}

#ifdef __cplusplus
}
#endif
