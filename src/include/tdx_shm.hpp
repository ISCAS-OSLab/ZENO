#pragma once

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef TDX_SHM_PHYS_ADDR
#define TDX_SHM_PHYS_ADDR 0x2b8000000ULL
#endif

#ifndef TDX_SHM_DEV_PATH
#define TDX_SHM_DEV_PATH "/dev/mem"
#endif

static inline uint64_t tdx_shm_phys_addr()
{
    return TDX_SHM_PHYS_ADDR;
}

static inline const char* tdx_shm_dev_path()
{
    return TDX_SHM_DEV_PATH;
}

static inline int tdx_shm_open()
{
    return open(tdx_shm_dev_path(), O_RDWR | O_SYNC);
}

static inline void* tdx_shm_mmap(int fd, size_t size)
{
    return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
        static_cast<off_t>(tdx_shm_phys_addr()));
}
