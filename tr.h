
#ifndef __TR_H__
#define __TR_H__ 1

/////////////////////////////////////////////////
// NOTE(vak): Architecture
/////////////////////////////////////////////////

#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__amd64) || defined(__amd64__) || \
    defined(_M_AMD64) || defined(_M_X64)
    #define TR_ARCHITECTURE_X64 (1)
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define TR_ARCHITECTURE_ARM64 (1)
#else
    #error Unknown architecture
#endif

#if !defined(TR_ARCHITECTURE_X64)
    #define TR_ARCHITECTURE_X64 (0)
#endif

#if !defined(TR_ARCHITECTURE_ARM64)
    #define TR_ARCHITECTURE_ARM64 (0)
#endif

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

#if !defined(trInvalidCodePath)
    #define trInvalidCodePath __builtin_trap()
#endif

/////////////////////////////////////////////////
// NOTE(vak): Macros
/////////////////////////////////////////////////

#define TR_ARRAY_COUNT(Array) \
    (sizeof(Array) / sizeof((Array)[0]))

#define TR_MIN(A, B) ((A) < (B) ? (A) : (B))
#define TR_MAX(A, B) ((A) > (B) ? (A) : (B))

#define TR_KB(Amount) ((tr_ssize)(Amount) << 10)
#define TR_MB(Amount) ((tr_ssize)(Amount) << 20)
#define TR_GB(Amount) ((tr_ssize)(Amount) << 30)
#define TR_TB(Amount) ((tr_ssize)(Amount) << 40)

#define TR_ALIGN_DOWN(Value, PowerOf2) \
        ((Value) & ~((PowerOf2) - 1))

#define TR_ALIGN_UP(Value, PowerOf2) \
        (((Value) + (PowerOf2) - 1) & ~((PowerOf2) - 1))

#define trAssert(Expression) \
    if (!(Expression)) trInvalidCodePath

/////////////////////////////////////////////////
// NOTE(vak): Memory
/////////////////////////////////////////////////

tr_func void trZeroMemory(void* DestInit, tr_usize Size);
tr_func void trFillMemory(void* DestInit, tr_u8 Byte, tr_usize Size);
tr_func void trCopyMemory(void* DestInit, void* SourceInit, tr_usize Size);

#define trZeroArray(Pointer, Count) trZeroMemory(Pointer, sizeof(*(Pointer)) * (Count))
#define trZeroType(Pointer) trZeroArray(Pointer, 1)

/////////////////////////////////////////////////
// NOTE(vak): Strings
/////////////////////////////////////////////////

typedef struct
{
    char* Data;
    tr_usize Size;
} tr_str;

#define trStr(Literal) (tr_str){Literal, sizeof(Literal) - 1}
#define trStrData(Data, Size) (tr_str){Data, Size}

tr_func tr_str trCString(const char* A);

/////////////////////////////////////////////////
// NOTE(vak): Arena
/////////////////////////////////////////////////

typedef struct
{
    void* Base;
    tr_usize Used;
    tr_usize Commited;
    tr_usize Reserved;
} tr_arena;

tr_func tr_b32 trMakeArena(tr_arena* Arena, tr_usize MinCommited, tr_usize MinReserved);
tr_func void trResetArena(tr_arena* Arena);

tr_func tr_usize trGetArenaPosition(tr_arena* Arena);
tr_func void trSetArenaPosition(tr_arena* Arena, tr_usize Position);

tr_func void* trPushArena(tr_arena* Arena, tr_usize Size);
tr_func void* trPushArenaZero(tr_arena* Arena, tr_usize Size);

#define trPushArenaArray(Arena, Type, Count) \
    (Type*)trPushArena(Arena, sizeof(Type) * (Count))

#define trPushArenaArrayZero(Arena, Type, Count) \
    (Type*)trPushArenaZero(Arena, sizeof(Type) * (Count))

#define trPushArenaType(Arena, Type) trPushArenaArray(Arena, Type, 1)
#define trPushArenaTypeZero(Arena, Type) trPushArenaArrayZero(Arena, Type, 1)

/////////////////////////////////////////////////
// NOTE(vak): Printing
/////////////////////////////////////////////////

tr_func tr_usize trPrintCharacter(char Character);
tr_func tr_usize trPrintNewLine(void);
tr_func tr_usize trPrint(tr_str Message);
tr_func tr_usize trPrintln(tr_str Message);
tr_func tr_usize trPrintHex(tr_usize Value);
tr_func tr_usize trPrintBinary(tr_usize Value);
tr_func tr_usize trPrintUSize(tr_usize Value);
tr_func tr_usize trPrintSSize(tr_ssize Value);
tr_func tr_usize trPrintF64(tr_f64 Value);

/////////////////////////////////////////////////
// NOTE(vak): Rounding
/////////////////////////////////////////////////

tr_func tr_f32 trFloorF32(tr_f32 Value);
tr_func tr_f64 trFloorF64(tr_f64 Value);

/////////////////////////////////////////////////
// NOTE(vak): Trigonometry
// Please note that these functions take in
// normalized arguments:
//      X=0   -> 0   radians
//      X=0.5 -> pi  radians
//      X=1   -> 2pi radians
/////////////////////////////////////////////////

tr_func tr_f32 trSinF32(tr_f32 X);
tr_func tr_f32 trCosF32(tr_f32 X);
tr_func tr_f32 trTanF32(tr_f32 X);

/////////////////////////////////////////////////
// NOTE(vak): Platform
/////////////////////////////////////////////////

typedef tr_usize tr_clock;

tr_func tr_f64 trGetTargetFPS(void);
tr_func void trSetTargetFPS(tr_f64 FPS);

tr_func tr_u32 trGetWindowWidth(void);
tr_func tr_u32 trGetWindowHeight(void);

tr_func tr_usize trWriteStdOut(void* Data, tr_usize Size);
tr_func tr_usize trWriteStdErr(void* Data, tr_usize Size);

tr_func tr_clock trGetWallClock(void);
tr_func tr_f64 trGetSecondsElapsed(tr_clock From, tr_clock To);
tr_func void trWait(tr_f64 Seconds);

tr_func void* trReserveMemory(tr_usize Size);
tr_func tr_b32 trCommitMemory(void* At, tr_usize Size);
tr_func void trDecommitMemory(void* At, tr_usize Size);

tr_func void trExit(tr_u8 ExitCode);

/////////////////////////////////////////////////
// NOTE(vak): Renderer
// All coordinates are assumed to be screen
// coordinates where:
//      Origin (0, 0) is at top-left
//      Positive X goes right
//      Positive Y goes down
/////////////////////////////////////////////////

tr_func void trSetClearColor(tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A);
tr_func void trVertex2D(tr_f32 X, tr_f32 Y, tr_f32 U, tr_f32 V, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A);
tr_func void trRect2D(tr_f32 MinX, tr_f32 MinY, tr_f32 SizeX, tr_f32 SizeY, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A);
tr_func void trCircle2D(tr_f32 CenterX, tr_f32 CenterY, tr_f32 Radius, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A);

/////////////////////////////////////////////////
// NOTE(vak): Implementation
/////////////////////////////////////////////////

#if defined(TR_IMPLEMENTATION)
    #include "tr.c"
#endif

#endif

