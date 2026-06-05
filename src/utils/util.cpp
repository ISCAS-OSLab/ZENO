// SPDX-License-Identifier: Mulan PSL v2

#include <util.h>

void spin_lock(int volatile* p)
{
    while (!__sync_bool_compare_and_swap(p, 0, 1)) {
        while (*p)
#ifdef __x86_64
            __asm__("pause");
#else /* ARM */
            __asm__ __volatile__("yield" ::: "memory");
#endif
        ;
    }
}

void spin_unlock(int volatile* p)
{
    asm volatile(""); // acts as a memory barrier.
    *p = 0;
}

void spin_wait(int volatile* p, int val)
{
    while (*p != val)
#ifdef __x86_64
        __asm__ volatile("pause" ::: "memory");
#else /* ARM */
        __asm__ __volatile__("yield" ::: "memory");
#endif
}

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void kvdb_fatal_at(const char* file, int line, const char* fmt, ...)
{
    fprintf(stderr, "[FATAL] %s:%d: ", file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    abort();
}

void print_hex(const char* what, const void* v, const unsigned long l)
{
    char tmp[512];
    const unsigned char* p = (const unsigned char*)v;
    unsigned long x, y = 0, z;
    sprintf(tmp, "%s contents: \n", what);
    for (x = 0; x < l;) {
        sprintf(tmp + strlen(tmp), "%02x ", p[x]);
        if (!(++x % 16) || x == l) {
            if ((x % 16) != 0) {
                z = 16 - (x % 16);
                if (z >= 8)
                    sprintf(tmp + strlen(tmp), " ");
                for (; z != 0; --z) {
                    sprintf(tmp + strlen(tmp), "   ");
                }
            }
            sprintf(tmp + strlen(tmp), " | ");
            for (; y < x; y++) {
                if ((y % 8) == 0)
                    sprintf(tmp + strlen(tmp), " ");
                if (isgraph(p[y]))
                    sprintf(tmp + strlen(tmp), "%c", p[y]);
                else
                    sprintf(tmp + strlen(tmp), ".");
            }
            sprintf(tmp + strlen(tmp), "\n");
        } else if ((x % 8) == 0) {
            sprintf(tmp + strlen(tmp), " ");
        }
    }
    // print_info("%s\n", tmp);
}
