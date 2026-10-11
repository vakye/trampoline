
#ifndef __TR_H__
#define __TR_H__ 1

/////////////////////////////////////////////////
// NOTE(vak): Platform
/////////////////////////////////////////////////

#if defined(_WIN32) || defined(_WIN64)
    #define TR_PLATFORM_WINDOWS (1)
#elif defined(__linux__)
    #define TR_PLATFORM_LINUX (1)
#else
    #error Unknown platform
#endif

#if !defined(TR_PLATFORM_WINDOWS)
    #define TR_PLATFORM_WINDOWS (0)
#endif

#if !defined(TR_PLATFORM_LINUX)
    #define TR_PLATFORM_LINUX (0)
#endif

/////////////////////////////////////////////////
// NOTE(vak): Types
/////////////////////////////////////////////////

typedef signed char         tr_s8;
typedef signed short        tr_s16;
typedef signed int          tr_s32;
typedef signed long long    tr_s64;

typedef unsigned char       tr_u8;
typedef unsigned short      tr_u16;
typedef unsigned int        tr_u32;
typedef unsigned long long  tr_u64;

typedef tr_s64 tr_ssize;
typedef tr_u64 tr_usize;

typedef float   tr_f32;
typedef double  tr_f64;

typedef tr_u8   tr_b8;
typedef tr_u16  tr_b16;
typedef tr_u32  tr_b32;
typedef tr_u64  tr_b64;

/////////////////////////////////////////////////
// NOTE(vak): Constants
/////////////////////////////////////////////////

#define TR_TRUE     (1)
#define TR_FALSE    (0)

/////////////////////////////////////////////////
// NOTE(vak): Keywords
/////////////////////////////////////////////////

#if !defined(tr_func)
    #define tr_func static
#endif

/////////////////////////////////////////////////
// NOTE(vak): Macros
/////////////////////////////////////////////////

#define TR_MIN(A, B) ((A) < (B) ? (A) : (B))
#define TR_MAX(A, B) ((A) > (B) ? (A) : (B))

/////////////////////////////////////////////////
// NOTE(vak): Strings
/////////////////////////////////////////////////

typedef struct
{
    char* Data;
    tr_usize Size;
} tr_str;

#define TR_STR(Literal) (tr_str){Literal, sizeof(Literal) - 1}
#define TR_STR_DATA(Data, Size) (tr_str){Data, Size}

/////////////////////////////////////////////////
// NOTE(vak): Cheatsheet
/////////////////////////////////////////////////

typedef tr_usize tr_clock;

tr_func tr_usize trPrintCharacter(char Character);
tr_func tr_usize trPrintNewLine(void);
tr_func tr_usize trPrint(tr_str Message);
tr_func tr_usize trPrintln(tr_str Message);
tr_func tr_usize trPrintUSize(tr_usize Value);
tr_func tr_usize trPrintSSize(tr_ssize Value);
tr_func tr_usize trPrintF64(tr_f64 Value);

tr_func tr_f32 trFloorF32(tr_f32 Value);
tr_func tr_f64 trFloorF64(tr_f64 Value);

tr_func tr_usize trWriteStdOut(void* Data, tr_usize Size);
tr_func tr_usize trWriteStdErr(void* Data, tr_usize Size);

tr_func tr_clock trGetWallClock(void);
tr_func tr_f64 trGetSecondsElapsed(tr_clock From, tr_clock To);

/////////////////////////////////////////////////
// NOTE(vak): Implementation
/////////////////////////////////////////////////

#if defined(TR_IMPLEMENTATION)
    #include "tr.c"
#endif

#endif

