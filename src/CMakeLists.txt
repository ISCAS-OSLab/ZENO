cmake_minimum_required(VERSION 3.10)
project(extension)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-error=register -Wno-error=unused-parameter -Wno-unused-parameter")

option(USE_CCACHE "Use ccache for faster incremental rebuilds when available" ON)
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

##### Options

# Measures for file-map storage
option (USE_LOCAL_STORE_CLEAR "Clear non-overwriting local KV store at PostgreSQL query boundaries" ON) # GC1
option (USE_LAZY_DELETE "Turn on lazy delete to reuse deleted IDs" OFF) # GC2
option (USE_PG_WAL_FLUSH_HOOK "Use PostgreSQL core WAL hook for ZENO WAL batch append and flush" ON)
option (USE_VARLEN_STRING "Use variable-length text storage" ON)
option (USE_KVMAP_PARTITION "Partition kvmap files by PostgreSQL relation id embedded in RID high bits" ON)
set(KVMAP_PARTITION_BITS 16 CACHE STRING "Number of high RID bits reserved for kvmap partition id")
option (USE_OPS_CONTENTION_OPT "Enable RPC wait/yield optimizations for high-contention workloads" ON)
option (USE_TDX_FIXED_SHM "Map L1/L2 shared memory through /dev/mem at a fixed TD-partitioning physical address" OFF)
set(TDX_SHM_PHYS_ADDR "0x2b8000000" CACHE STRING "Physical address used by USE_TDX_FIXED_SHM")

if(USE_LAZY_DELETE)
    add_definitions(-DUSE_LAZY_DELETE)
else()
    remove_definitions(-DUSE_LAZY_DELETE)
endif()

if(USE_PG_WAL_FLUSH_HOOK)
    add_definitions(-DUSE_PG_WAL_FLUSH_HOOK)
else()
    remove_definitions(-DUSE_PG_WAL_FLUSH_HOOK)
endif()

if(USE_VARLEN_STRING)
    add_definitions(-DUSE_VARLEN_STRING)
else()
    remove_definitions(-DUSE_VARLEN_STRING)
endif()

if(USE_KVMAP_PARTITION)
    add_definitions(-DUSE_KVMAP_PARTITION -DKVMAP_PARTITION_BITS=${KVMAP_PARTITION_BITS})
else()
    remove_definitions(-DUSE_KVMAP_PARTITION)
endif()

if(USE_LOCAL_STORE_CLEAR)
    add_definitions(-DUSE_LOCAL_STORE_CLEAR)
else()
    remove_definitions(-DUSE_LOCAL_STORE_CLEAR)
endif()

if(USE_OPS_CONTENTION_OPT)
    add_definitions(-DUSE_OPS_CONTENTION_OPT)
else()
    remove_definitions(-DUSE_OPS_CONTENTION_OPT)
endif()

if(USE_TDX_FIXED_SHM)
    add_definitions(-DUSE_TDX_FIXED_SHM -DTDX_SHM_PHYS_ADDR=${TDX_SHM_PHYS_ADDR})
else()
    remove_definitions(-DUSE_TDX_FIXED_SHM)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

find_package(PostgreSQL)
if(PostgreSQL_FOUND)
set(_link_flags "${PostgreSQL_SHARED_LINK_OPTIONS}")
foreach(_dir ${PostgreSQL_SERVER_LIBRARY_DIRS})
    set(_link_flags "${_link_flags} -L${_dir}")
endforeach()
endif()

file(GLOB SRCS
    privacy_zone/*.cpp
    utils/*.cpp
)
add_executable(tee_server ${SRCS})

target_include_directories(tee_server
    PRIVATE privacy_zone/include
    PRIVATE include
    )
target_link_libraries(tee_server mbedcrypto rt)

if(PostgreSQL_FOUND)

file(GLOB USRCS
    utils/*.cpp
    )
add_library(integrity_zone ${USRCS})

target_include_directories(integrity_zone
    PRIVATE integrity_zone/include
    PRIVATE include
    )

target_include_directories(integrity_zone
    PRIVATE ${PostgreSQL_INCLUDE_DIRS}
    PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
)

endif()

## for extension
if(PostgreSQL_FOUND)
add_subdirectory(integrity_zone)
endif()
