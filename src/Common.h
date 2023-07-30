#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define PI 3.141592653f

#define Assert(Expression) if (!(Expression)) { *(int *) 0 = 0; }

#define MAX_INTERNAL_NAME_LENGTH 32
#define MAX_FILENAME_LENGTH 64
#define MAX_PATH_LENGTH 256

#endif