#ifndef Z_COMMON_H
#define Z_COMMON_H

#define KIBIBYTES(n) (n*1024)
#define MEBIBYTES(n) (n*1024*KIBIBYTES(1))
#define GIBIBYTES(n) (n*1024*MEBIBYTES(1))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float	 r32;
typedef double	 r64;

typedef u8 bool;
enum {false, true};

typedef struct {
	i16 i;
	i16 r;
} i16C;

typedef struct {
	i32 i;
	i32 r;
} i32C;

typedef struct {
	r32 i;
	r32 r;
} r32C;

#define ASSERT(x) if(!x) *(r32*)(0) = 1
#define INVALID_CODE_PATH ASSERT(0)

#define ABS(x)    (x<0)?-x:x
#define MAX(x, y) (x>y)?x:y
#define MIN(x, y) (x<y)?x:y

#endif
