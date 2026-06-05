#pragma once

#include <stdint.h>
#include <cassert>

#define PAGE_SIZE 4096ULL
#define PAGE_ALIGN_DOWN(v) ((v) & (~(PAGE_SIZE - 1ULL)))

#define IV_SIZE 12
#define TAG_SIZE 16

#define INT32_LENGTH sizeof(int)
#define FLOAT4_LENGTH sizeof(float)

#define TIMESTAMP int64_t
#define TIMESTAMP_LENGTH sizeof(int64_t)

#define STRING_LENGTH 540 // Default value if not defined by CMake
#define BULK_SIZE 256 // Default value if not defined by CMake

/* for ops opcode */
#define CMD_INT_PUT 1
#define CMD_INT_GET 2
#define CMD_FLOAT_PUT 3
#define CMD_FLOAT_GET 4
#define CMD_TEXT_PUT 5
#define CMD_TEXT_GET 6
#define CMD_TIMESTAMP_PUT 7
#define CMD_TIMESTAMP_GET 8
#define CMD_BATCH_PUT 9
#define CMD_BATCH_GET 10

#define CMD_INT_ENC 11
#define CMD_INT_DEC 12
#define CMD_FLOAT_ENC 13
#define CMD_FLOAT_DEC 14
#define CMD_STRING_ENC 15
#define CMD_STRING_DEC 16
#define CMD_TIMESTAMP_ENC 17
#define CMD_TIMESTAMP_DEC 18

#define CMD_INT_PUT_ENC 19
#define CMD_INT_GET_ENC 20
#define CMD_FLOAT_PUT_ENC 21
#define CMD_FLOAT_GET_ENC 22
#define CMD_STRING_PUT_ENC 23
#define CMD_STRING_GET_ENC 24
#define CMD_TIMESTAMP_PUT_ENC 25
#define CMD_TIMESTAMP_GET_ENC 26

#define CMD_INT_PLUS 27
#define CMD_INT_MINUS 28
#define CMD_INT_MULT 29
#define CMD_INT_DIV 30
#define CMD_INT_MOD 31
#define CMD_INT_POW 32
#define CMD_INT_CMP 33
#define CMD_INT_SUM_BULK 34

#define CMD_FLOAT_PLUS 35
#define CMD_FLOAT_MINUS 36
#define CMD_FLOAT_MULT 37
#define CMD_FLOAT_DIV 38
#define CMD_FLOAT_MOD 39
#define CMD_FLOAT_POW 40
#define CMD_FLOAT_CMP 41
#define CMD_FLOAT_SUM_BULK 42

#define CMD_TEXT_CMP 43
#define CMD_TEXT_SUBSTRING 44
#define CMD_TEXT_CONCAT 45
#define CMD_TEXT_LIKE 46

#define CMD_TIMESTAMP_CMP 47
#define CMD_TIMESTAMP_EXTRACT_YEAR 48

#define CMD_PROMOTE_INT 49
#define CMD_PROMOTE_FLOAT 50
#define CMD_PROMOTE_TEXT 51
#define CMD_PROMOTE_TIMESTAMP 52

#define CMD_LOCAL_CLEAR 53
#define CMD_FLUSH 54
#define CMD_TRUNCATE 55
#define CMD_REPLAY 56
#define CMD_WAL_APPEND_BATCH 57
#define CMD_WAL_MATERIALIZE_BATCH 58
#define CMD_WAL_REPLAY_PAYLOAD 59
#define CMD_CKPT 60

#if __GNUC__ >= 3
#define likely(x)	__builtin_expect((x) != 0, 1)
#define unlikely(x) __builtin_expect((x) != 0, 0)
#else
#define likely(x)	((x) != 0)
#define unlikely(x) ((x) != 0)
#endif
