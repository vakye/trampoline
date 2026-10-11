
#include <immintrin.h>

/////////////////////////////////////////////////
// NOTE(vak): Printing
/////////////////////////////////////////////////

tr_func tr_usize trPrintCharacter(char Character)
{
    tr_usize Result = trWriteStdOut(&Character, 1);
    return (Result);
}

tr_func tr_usize trPrintNewLine(void)
{
    tr_usize Result = trPrintCharacter('\n');
    return (Result);
}

tr_func tr_usize trPrint(tr_str Message)
{
    tr_usize Result = trWriteStdOut(Message.Data, Message.Size);
    return (Result);
}

tr_func tr_usize trPrintln(tr_str Message)
{
    tr_usize Result = 0;

    Result += trPrint(Message);
    Result += trPrintNewLine();

    return (Result);
}

tr_func tr_usize trPrintHex(tr_usize Value)
{
    char Buffer[64] = {0};

    tr_usize DigitIndex = sizeof(Buffer);
    tr_usize DigitCount = 0;

    do
    {
        tr_usize Digit = (Value % 16);
        Value /= 16;

        char DigitCharacter =
            (Digit <= 9)  * ('0' + Digit) +
            (Digit >= 10) * ('a' + Digit - 10);

        DigitIndex--;
        DigitCount++;

        Buffer[DigitIndex] = DigitCharacter;
    } while (Value);

    tr_usize Result = trPrint(TR_STR_DATA(Buffer + DigitIndex, DigitCount));

    return (Result);
}

tr_func tr_usize trPrintBinary(tr_usize Value)
{
    char Buffer[64] = {0};

    tr_usize DigitIndex = sizeof(Buffer);
    tr_usize DigitCount = 0;

    do
    {
        char DigitCharacter = '0' + (Value % 2);
        Value /= 2;

        DigitIndex--;
        DigitCount++;

        Buffer[DigitIndex] = DigitCharacter;
    } while (Value);

    tr_usize Result = trPrint(TR_STR_DATA(Buffer + DigitIndex, DigitCount));

    return (Result);
}

tr_func tr_usize trPrintUSize(tr_usize Value)
{
    char Buffer[64] = {0};

    tr_usize DigitIndex = sizeof(Buffer);
    tr_usize DigitCount = 0;

    do
    {
        char DigitCharacter = '0' + (Value % 10);
        Value /= 10;

        DigitIndex--;
        DigitCount++;

        Buffer[DigitIndex] = DigitCharacter;
    } while (Value);

    tr_usize Result = trPrint(TR_STR_DATA(Buffer + DigitIndex, DigitCount));

    return (Result);
}

tr_func tr_usize trPrintSSize(tr_ssize Value)
{
    tr_usize Result = 0;

    if (Value < 0)
    {
        Result += trPrintCharacter('-');
        Value = -Value;
    }

    Result += trPrintUSize(Value);

    return (Result);
}

tr_func tr_usize trPrintF64(tr_f64 Value)
{
    tr_usize Result = 0;

    if (Value < 0)
    {
        Result += trPrintCharacter('-');
        Value = -Value;
    }

    char Buffer[512] = {0};

    tr_f64 IntegerPart = trFloorF64(Value);
    tr_f64 DecimalPart = Value - IntegerPart;

    {
        tr_usize DigitIndex = sizeof(Buffer);
        tr_usize DigitCount = 0;

        do
        {
            tr_f64 A = trFloorF64(IntegerPart / 10.0);
            tr_f64 B = IntegerPart - A*10.0;

            char DigitCharacter = '0' + (tr_usize)(B);
            IntegerPart = A;

            DigitIndex--;
            DigitCount++;

            Buffer[DigitIndex] = DigitCharacter;
        } while (IntegerPart);

        Result += trPrint(TR_STR_DATA(Buffer + DigitIndex, DigitCount));
    }

    Result += trPrintCharacter('.');

    tr_usize DecimalDigits = 4;

    {
        for (tr_usize Index = 0; Index < DecimalDigits; Index++)
        {
            DecimalPart *= 10.0;

            tr_usize Digit = (tr_usize)DecimalPart;
            char DigitCharacter = '0' + (tr_usize)(DecimalPart);

            DecimalPart -= Digit;

            Buffer[Index] = DigitCharacter;
        }

        Result += trPrint(TR_STR_DATA(Buffer, DecimalDigits));
    }

    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Rounding
/////////////////////////////////////////////////

tr_func tr_f32 trFloorF32(tr_f32 Value)
{
    tr_f32 Result = _mm_cvtss_f32(_mm_floor_ps(_mm_set_ss(Value)));
    return (Result);
}

tr_func tr_f64 trFloorF64(tr_f64 Value)
{
    tr_f64 Result = _mm_cvtsd_f64(_mm_floor_pd(_mm_set_sd(Value)));
    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Windows Implementation
/////////////////////////////////////////////////

#if TR_PLATFORM_WINDOWS
    #error Windows is not implemented yet

/////////////////////////////////////////////////
// NOTE(vak): Linux Implementation
/////////////////////////////////////////////////

#elif TR_PLATFORM_LINUX

#define TR_LINUX_SYSCALL_NR_WRITE           (1)
#define TR_LINUX_SYSCALL_NR_MMAP            (9)
#define TR_LINUX_SYSCALL_NR_MPROTECT        (10)
#define TR_LINUX_SYSCALL_NR_CLOCK_GETTIME   (228)
#define TR_LINUX_SYSCALL_NR_EXIT_GROUP      (231)

static tr_usize trLinuxSyscall(
    tr_usize NR,
    tr_usize Arg0, tr_usize Arg1, tr_usize Arg2,
    tr_usize Arg3, tr_usize Arg4, tr_usize Arg5
)
{
    tr_usize Result = 0;

    register tr_usize R10 __asm__("r10") = Arg3;
    register tr_usize R8  __asm__("r8")  = Arg4;
    register tr_usize R9  __asm__("r9")  = Arg5;

    __asm__ volatile
    (
        "syscall" :
        "=a"(Result) :
        "a"(NR),
        "D"(Arg0),
        "S"(Arg1),
        "d"(Arg2),
        "r"(R10),
        "r"(R8),
        "r"(R9) :
        "memory", "rcx", "r11"
    );

    return (Result);
}

__attribute__((naked))
void trEntryPoint(void)
{
    __asm__ volatile
    (
        "mov 0 (%rsp),          %edi\n"
        "lea 8 (%rsp),          %rsi\n"
        "lea 16(%rsp, %rdi, 8), %rdx\n"
        "call trLinuxBootstrap\n"
    );
}

void Setup(void); // NOTE(vak): Implemented by user

void trLinuxBootstrap(tr_s32 ArgCount, char* Args[], char* Envp[])
{
    Setup();

    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_EXIT_GROUP, 0,
        0, 0, 0, 0, 0
    );
}

tr_func tr_usize trWriteStdOut(void* Data, tr_usize Size)
{
    tr_ssize BytesWritten = (tr_ssize)trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_WRITE,
        1, // NOTE(vak): STDOUT_FILENO
        (tr_usize)Data,
        Size,
        0, 0, 0
    );

    tr_usize Result = TR_MAX(0, BytesWritten);
    return (Result);
}

tr_func tr_usize trWriteStdErr(void* Data, tr_usize Size)
{
    tr_ssize BytesWritten = (tr_ssize)trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_WRITE,
        2, // NOTE(vak): STDERR_FILENO
        (tr_usize)Data,
        Size,
        0, 0, 0
    );

    tr_usize Result = TR_MAX(0, BytesWritten);
    return (Result);
}

typedef struct
{
    tr_usize Seconds;
    tr_ssize Nanoseconds;
} tr_linux_timespec;

tr_func tr_clock trGetWallClock(void)
{
    tr_linux_timespec Now = {0};

    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_CLOCK_GETTIME,
        1, // NOTE(vak): CLOCK_MONOTONIC
        (tr_usize)(&Now),
        0, 0, 0, 0
    );

    tr_clock Result = Now.Seconds*1000000000 + Now.Nanoseconds;
    return (Result);
}

tr_func tr_f64 trGetSecondsElapsed(tr_clock From, tr_clock To)
{
    tr_f64 Result = (tr_f64)(To - From) * 1e-9;
    return (Result);
}

tr_func void* trReserveMemory(tr_usize Size)
{
    tr_ssize Address = (tr_ssize)trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_MMAP,
        0,
        Size,
        0x0,    // NOTE(vak): PROT_NONE,
        0x22,   // NOTE(vak): MAP_PRIVATE | MAP_ANON
        -1,
        0
    );

    void* Result = (void*) TR_MAX(Address, 0);
    return (Result);
}

tr_func tr_b32 trCommitMemory(void* At, tr_usize Size)
{
    tr_ssize Result = (tr_ssize)trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_MPROTECT,
        (tr_usize)At,
        Size,
        0x3, // NOTE(vak): PROT_READ | PROT_WRITE,
        0, 0, 0
    );

    tr_b32 Good = (Result >= 0);
    return (Good);
}

tr_func void trDecommitMemory(void* At, tr_usize Size)
{
    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_MPROTECT,
        (tr_usize)At,
        Size,
        0x0, // NOTE(vak): PROT_NONE
        0, 0, 0
    );
}

#else
    #error Unimplemented platform
#endif

void* memset(void* DestInit, tr_s32 Byte, tr_usize Size)
{
    tr_u8* Dest = (tr_u8*)DestInit;

    while (Size--)
        *Dest++ = (tr_u8)Byte;

    return (DestInit);
}

void* memcpy(void* DestInit, void* SourceInit, tr_usize Size)
{
    tr_u8* Dest = (tr_u8*)DestInit;
    tr_u8* Source = (tr_u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (DestInit);
}

