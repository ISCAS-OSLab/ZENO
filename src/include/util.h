#pragma once

#include <sched.h>

#ifdef __x86_64

#define YIELD_PROCESSOR __asm__ volatile("pause")
#define YIELD_SCHEDULER sched_yield()
#define LOAD_BARRIER __sync_synchronize()
#define STORE_BARRIER __sync_synchronize()

#elif __aarch64__

#define YIELD_PROCESSOR __asm__ volatile("yield")
#define YIELD_SCHEDULER sched_yield()
/* this load barrier is only for arm */
#define LOAD_BARRIER asm volatile("dsb ld" ::: "memory")
#define STORE_BARRIER asm volatile("dsb st" ::: "memory")

#endif

#ifdef __cplusplus
extern "C" {
#endif

void kvdb_fatal_at(const char* file, int line, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((noreturn, format(printf, 3, 4)))
#endif
    ;

void spin_lock(int volatile* p);
void spin_unlock(int volatile* p);
void spin_wait(int volatile* p, int val);

void print_hex(const char* what, const void* v, const unsigned long l);

#ifdef __cplusplus
}
#endif

#ifdef KVDB_USE_POSTGRES_ELOG
#include <postgres.h>
#include <utils/elog.h>
#define KVDB_FATAL(...) elog(ERROR, __VA_ARGS__)
#else
#define KVDB_FATAL(...) kvdb_fatal_at(__FILE__, __LINE__, __VA_ARGS__)
#endif

#define KVDB_CHECK(cond, ...)           \
    do {                                \
        if (!(cond)) {                  \
            KVDB_FATAL(__VA_ARGS__);    \
        }                               \
    } while (0)
