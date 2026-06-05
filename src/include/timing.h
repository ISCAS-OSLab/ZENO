#ifndef TIMING_H
#define TIMING_H

#include <cstdint>

inline uint64_t rdtscp()
{
#if defined(__aarch64__)
    uint64_t cycles;
    __asm__ __volatile__("isb; mrs %0, pmccntr_el0" : "=r"(cycles));
    return cycles;
#elif defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi) : : "rcx");
    return ((uint64_t)hi << 32) | lo;
#else
#error "rdtscp() is not implemented for this architecture"
#endif
}

#endif // TIMING_H
