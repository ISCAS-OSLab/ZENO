// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include <fcntl.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <sched.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <iostream>

#include "tee.hpp"
#include "rpc.h"
#include "rid.h"
#include "request_types.h"
#include "tdx_shm.hpp"
#include "util.h"

using namespace std;

uint64_t counter = 0;

#define OPS_SERVER_IDLE_SPIN_LIMIT 10000

#ifndef OPS_WORKER_NUM
#define OPS_WORKER_NUM 15
#endif

#if OPS_WORKER_NUM < 1
#error "OPS_WORKER_NUM must be at least 1"
#endif

static inline void idle_wait(int* idle_spins, int spin_limit = OPS_SERVER_IDLE_SPIN_LIMIT)
{
#ifdef USE_OPS_CONTENTION_OPT
    if (++(*idle_spins) < spin_limit) {
        YIELD_PROCESSOR;
    } else {
        *idle_spins = 0;
        sched_yield();
    }
#else
    (void)idle_spins;
    (void)spin_limit;
    YIELD_PROCESSOR;
#endif
}

static int shm_fd;
static volatile sig_atomic_t shutdown_requested = 0;

static void shm_exit_handler()
{
    KVDB_CHECK(shm_fd != -1, "invalid shared memory fd during exit");
    close(shm_fd);
}

static void* get_shm(size_t size)
{
#ifdef USE_TDX_FIXED_SHM
    shm_fd = tdx_shm_open();
    int saved_errno = errno;
    KVDB_CHECK(shm_fd != -1, "failed to open TDX shared memory %s: errno=%d",
        tdx_shm_dev_path(), saved_errno);

    void* shm_addr = tdx_shm_mmap(shm_fd, size);
    saved_errno = errno;
    KVDB_CHECK(shm_addr != MAP_FAILED, "failed to mmap TDX shared memory, size=%zu, addr=0x%llx, errno=%d",
        size, (unsigned long long)tdx_shm_phys_addr(), saved_errno);
#else
    if ((shm_fd = open(SHM_NAME, O_RDWR)) == -1) {
        shm_fd = open("/dev/uio0", O_RDWR);
    } else {
        fprintf(stderr, "[INFO] cannot find /dev/uio0, using local /dev/shm instead\n");
    }
    KVDB_CHECK(shm_fd != -1, "failed to open shared memory %s or /dev/uio0", SHM_NAME);

    void* shm_addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    KVDB_CHECK(shm_addr != MAP_FAILED, "failed to mmap shared memory, size=%zu", size);
#endif

    atexit(shm_exit_handler);
    return shm_addr;
}

void signal_handler(int signal)
{
    (void)signal;
    shutdown_requested = 1;
}

static int ops_worker_count()
{
    return OPS_WORKER_NUM < MAX_REGION_NUM ? OPS_WORKER_NUM : MAX_REGION_NUM;
}

static BaseRequest* region_request(OpsReq* req, int region_id)
{
    return (BaseRequest*)((char*)req + REGION_N_OFFSET(region_id));
}

static void bind_to_cpu(int cpu_id)
{
    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count <= 0) {
        return;
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu_id % cpu_count, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        fprintf(stderr, "failed to bind pid %d to cpu %d: %s\n",
            getpid(), cpu_id, strerror(errno));
    }
}

static void handle_request(BaseRequest* req)
{
    LOAD_BARRIER;
    counter++;
    if (req->reqType > 0) {
        handle_ops(req);
        KVDB_CHECK(req->resp == 0, "request type %d failed with response %d", req->reqType, req->resp);
    }
    STORE_BARRIER;
    req->status = DONE;
}

pid_t fork_ops_worker(OpsReq* ops_req, int worker_id, int worker_count)
{
    pid_t pid = fork();
    if (pid != 0) {
        return pid;
    }

    // after fork, child inherit all attached shared memory (man shmat)
    pid = getpid();
    bind_to_cpu(worker_id + 1);
    fprintf(stderr, "[%d] worker %d/%d started\n", pid, worker_id, worker_count);

    // per-process entropy and crypto context
    gcm_init();

    int idle_spins = 0;

#ifdef USE_OPS_CONTENTION_OPT
    YIELD_SCHEDULER;
#endif

    while (1) {
        bool found_work = false;

        for (int region_id = worker_id; region_id < MAX_REGION_NUM; region_id += worker_count) {
            if (!GET(ops_req->bitmap, region_id)) {
                continue;
            }

            BaseRequest* req = region_request(ops_req, region_id);
            if (req->status == SENT) {
                found_work = true;
                idle_spins = 0;
                handle_request(req);
                break;
            }
        }

        if (ops_req->status == SHM_EXIT) {
            fprintf(stderr, "[%d] total ops: %llu\n", pid, (unsigned long long)counter);
            exit(0);
        }

        if (!found_work) {
            idle_wait(&idle_spins);
        }
    }

    exit(0);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int data_size = SHM_SIZE;
    int worker_count = ops_worker_count();

    OpsReq* req = (OpsReq*)get_shm(data_size);
    memset(req, 0, data_size);
    fprintf(stderr, "kvdb tee_server attached shared memory @ %p\n", req);
    bind_to_cpu(0);
    for (int i = 0; i < worker_count; i++) {
        pid_t pid = fork_ops_worker(req, i, worker_count);
        KVDB_CHECK(pid > 0, "failed to fork worker %d: errno=%d", i, errno);
    }

    while (!shutdown_requested) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            fprintf(stderr, "worker %d exited, shutting down tee_server\n", pid);
            shutdown_requested = 1;
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == ECHILD) {
            break;
        }
        KVDB_CHECK(false, "waitpid failed: errno=%d", errno);
    }

    req->status = SHM_EXIT;
    STORE_BARRIER;

    while (1) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == ECHILD) {
            break;
        }
        KVDB_CHECK(false, "waitpid failed during shutdown: errno=%d", errno);
    }

    return 0;
}
