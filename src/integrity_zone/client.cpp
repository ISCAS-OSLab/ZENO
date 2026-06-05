// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2024 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <extension.hpp>


extern "C" {
PG_MODULE_MAGIC;
}

#include "rid.h"
#include "util.h"
#include "rpc.h"
#include "request_types.h"
#include "tdx_shm.hpp"
#include <extension.hpp>
#include <interface.hpp>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>

static void *shm_addr;
static int shm_id;

static void *get_shm(size_t size)
{
    int fd = -1;
#ifdef USE_TDX_FIXED_SHM
    fd = tdx_shm_open();
    int saved_errno = errno;
    KVDB_CHECK(fd != -1, "failed to open TDX shared memory %s: errno=%d",
        tdx_shm_dev_path(), saved_errno);

    void* p = tdx_shm_mmap(fd, size);
    saved_errno = errno;
    close(fd);
    KVDB_CHECK(p != MAP_FAILED, "failed to mmap TDX shared memory, size=%zu, addr=0x%llx, errno=%d",
        size, (unsigned long long)tdx_shm_phys_addr(), saved_errno);
    return p;
#else
    if ((fd = open(SHM_NAME, O_RDWR)) == -1)
        fd = open("/dev/uio0", O_RDWR);
    KVDB_CHECK(fd != -1, "failed to open shared memory %s or /dev/uio0", SHM_NAME);

    void* p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    KVDB_CHECK(p != MAP_FAILED, "failed to mmap shared memory, size=%zu", size);
    return p;
#endif
}

static void* getSharedBuffer()
{
    shm_addr = get_shm(SHM_SIZE);
    OpsReq* ops_server = (OpsReq*)shm_addr;
    time_t start = time(0);

    KVDB_CHECK(ops_server->status != SHM_EXIT, "ops server is shutting down");
    while ((shm_id = claim_shm_region(ops_server)) < 0) {
        YIELD_PROCESSOR;
        KVDB_CHECK(ops_server->status != SHM_EXIT, "ops server is shutting down");
        if (difftime(time(0), start) > 10) {
            return 0;
        }
    }

    void* buffer = (char*)shm_addr + REGION_N_OFFSET(shm_id);
    memset(buffer, 0, REQ_REGION_SIZE);
    STORE_BARRIER;
    return buffer;
}

static void freeBuffer(void* buffer)
{
    OpsReq* ops_server = (OpsReq*)shm_addr;
    BaseRequest* req = static_cast<BaseRequest*>(buffer);

    while (req->status == SENT) {
        YIELD_PROCESSOR;
    }
    LOAD_BARRIER;
    req->status = NONE;
    STORE_BARRIER;
    release_shm_region(ops_server, shm_id);
}

TEEInvoker* TEEInvoker::invoker = nullptr;

static void exit_handler()
{
    TEEInvoker* invoker = TEEInvoker::getInstance();
    delete invoker;
}

TEEInvoker::TEEInvoker()
{
    req_buffer = getSharedBuffer();
    KVDB_CHECK(req_buffer != nullptr, "failed to allocate shared request buffer");
    BaseRequest* req_control = static_cast<BaseRequest*>(req_buffer);
    req_control->status = NONE;
    atexit(exit_handler);
}

TEEInvoker::~TEEInvoker()
{
    freeBuffer(req_buffer);
}

int TEEInvoker::sendRequest(Request* req)
{
    int resp;

    /* 1. serialize request data structure to shared memory buffer */
    req->serializeTo(req_buffer);

    /*
    Treat buffer as BaseRequest structure, which locates in first several bytes of requests structure, e.g.:
    xxxRequest {
        BaseRequest : reqType, status, resp
        xxxRequest-specific parameters...
    }
    */
    BaseRequest* req_control = static_cast<BaseRequest*>(req_buffer);

    STORE_BARRIER;
    req_control->status = SENT;
    /* wait for status */
    while (req_control->status != DONE)
        YIELD_PROCESSOR;
    LOAD_BARRIER;

    /* memcpy results in req_buffer to result_buffer
        result buffer is determined when request is constructed.
    */
    req->copyResultFrom(req_buffer);
    resp = req_control->resp;

    /* read-write barrier, no read move after this barrier, no write move before this barrier */
    req_control->status = NONE;
    return resp;
}
