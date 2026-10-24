
/////////////////////////////////////////////////
// NOTE(vak): Intrinsics
/////////////////////////////////////////////////

#if TR_ARCHITECTURE_X64
    #include <immintrin.h>
#endif

/////////////////////////////////////////////////
// NOTE(vak): Memory
/////////////////////////////////////////////////

tr_func void trZeroMemory(void* DestInit, tr_usize Size)
{
    tr_u8* Dest = (tr_u8*)DestInit;
    while (Size--)
        *Dest++ = 0;
}

tr_func void trFillMemory(void* DestInit, tr_u8 Byte, tr_usize Size)
{
    tr_u8* Dest = (tr_u8*)DestInit;
    while (Size--)
        *Dest++ = Byte;
}

tr_func void trCopyMemory(void* DestInit, void* SourceInit, tr_usize Size)
{
    tr_u8* Dest = (tr_u8*)DestInit;
    tr_u8* Source = (tr_u8*)SourceInit;
    while (Size--)
        *Dest++ = *Source++;
}

/////////////////////////////////////////////////
// NOTE(vak): Endianness
/////////////////////////////////////////////////

tr_func tr_u16 trBigToHostEndian16(tr_u16 Value)
{
    tr_u16 Result =
        ((Value & 0xFF00) >> 8) |
        ((Value & 0x00FF) << 8);

    return (Result);
}

tr_func tr_u32 trBigToHostEndian32(tr_u32 Value)
{
    tr_u32 Result =
        ((Value & 0xFF000000) >> 24) |
        ((Value & 0x00FF0000) >>  8) |
        ((Value & 0x0000FF00) <<  8) |
        ((Value & 0x000000FF) << 24);

    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Strings
/////////////////////////////////////////////////

tr_func tr_str trCString(const char* A)
{
    tr_str Result = {0};

    if (A)
    {
        Result = trStrData((char*)A, 0);

        while (A[Result.Size] != '\0')
            Result.Size++;
    }

    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Arena
/////////////////////////////////////////////////

#define TR_ARENA_GRANULE_SIZE TR_KB(256)

tr_func tr_arena* trGetPermanentArena(void)
{
    static tr_arena Arena = {0};

    if (!Arena.Base)
    {
        tr_b32 Okay = trMakeArena(&Arena, TR_KB(256), TR_GB(64));
        TR_ASSERT(Okay);
    }

    TR_ASSERT(Arena.Base);

    return (&Arena);
}

tr_func tr_arena* trGetTemporaryArena(void)
{
    static tr_arena Arena = {0};

    if (!Arena.Base)
    {
        tr_b32 Okay = trMakeArena(&Arena, TR_KB(256), TR_GB(64));
        TR_ASSERT(Okay);
    }

    TR_ASSERT(Arena.Base);

    return (&Arena);
}

tr_func tr_b32 trMakeArena(tr_arena* Arena, tr_usize MinCommited, tr_usize MinReserved)
{
    trZeroType(Arena);

    Arena->Commited = TR_ALIGN_UP(MinCommited, TR_ARENA_GRANULE_SIZE);
    Arena->Reserved = TR_ALIGN_UP(MinReserved, TR_ARENA_GRANULE_SIZE);

    Arena->Base = trReserveMemory(Arena->Reserved);
    if (!Arena->Base)
        return (TR_FALSE);

    if (Arena->Commited)
    {
        if (!trCommitMemory(Arena->Base, Arena->Commited))
            return (TR_FALSE);
    }

    return (TR_TRUE);
}

tr_func void trResetArena(tr_arena* Arena)
{
    trSetArenaPosition(Arena, 0);
}

tr_func tr_usize trGetArenaPosition(tr_arena* Arena)
{
    tr_usize Result = Arena->Used;
    return (Result);
}

tr_func void trSetArenaPosition(tr_arena* Arena, tr_usize Position)
{
    Arena->Used = Position;
}

tr_func void* trPushArena(tr_arena* Arena, tr_usize Size)
{
    if (Arena->Used + Size > Arena->Commited)
    {
        tr_usize ExpandSize = (Arena->Used + Size) - Arena->Commited;
        tr_usize CommitSize = TR_ALIGN_UP(ExpandSize, TR_ARENA_GRANULE_SIZE);
        void*    CommitAt   = (tr_u8*)Arena->Base + Arena->Commited;

        if (!trCommitMemory(CommitAt, CommitSize))
            return (0);

        Arena->Commited += CommitSize;
    }

    void* Result = (tr_u8*)Arena->Base + Arena->Used;
    Arena->Used += Size;

    return (Result);
}

tr_func void* trPushArenaZero(tr_arena* Arena, tr_usize Size)
{
    void* Result = trPushArena(Arena, Size);
    if (Result)
        trZeroMemory(Result, Size);

    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Random number generator
/////////////////////////////////////////////////

tr_func void trRandomSeed(tr_random* Entropy)
{
    Entropy->State = 11400714819323198485ull * trGetWallClock();
}

tr_func tr_u32 trRandomU32(tr_random* Entropy)
{
    // NOTE(vak): PCG-XSH-RR with 64-bit state and 32-bit output
    // Taken from https://en.wikipedia.org/wiki/Permuted_congruential_generator

    tr_u64 X = Entropy->State;
    tr_u32 Count = (tr_u32)(X >> 59);

    Entropy->State = X * 6364136223846793005ull + 1442695040888963407ull;
    X ^= X >> 18;

    tr_u32 V = (tr_u32)(X >> 27);
    tr_u32 Result = (V << Count) | (V >> (-Count & 31));

    return (Result);
}

tr_func tr_f32 trRandomF32(tr_random* Entropy, tr_f32 From, tr_f32 To)
{
    tr_f32 Normalized = trRandomU32(Entropy) / 4294967295.0f;
    tr_f32 Result = From + (To - From)*Normalized;
    return (Result);
}

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

    tr_usize Result = trPrint(trStrData(Buffer + DigitIndex, DigitCount));

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

    tr_usize Result = trPrint(trStrData(Buffer + DigitIndex, DigitCount));

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

    tr_usize Result = trPrint(trStrData(Buffer + DigitIndex, DigitCount));

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

        Result += trPrint(trStrData(Buffer + DigitIndex, DigitCount));
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

        Result += trPrint(trStrData(Buffer, DecimalDigits));
    }

    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Rounding
/////////////////////////////////////////////////

tr_func tr_f32 trFloorF32(tr_f32 Value)
{
#if TR_ARCHITECTURE_X64
    tr_f32 Result = _mm_cvtss_f32(_mm_floor_ps(_mm_set_ss(Value)));
#else
    #error Fallback implementation not yet written for trFloorF32
#endif
    return (Result);
}

tr_func tr_f64 trFloorF64(tr_f64 Value)
{
#if TR_ARCHITECTURE_X64
    tr_f64 Result = _mm_cvtsd_f64(_mm_floor_pd(_mm_set_sd(Value)));
#else
    #error Fallback implementation not yet written for trFloorF64
#endif
    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Trigonometry
/////////////////////////////////////////////////

tr_func tr_f32 trSinF32(tr_turns32 X)
{
    tr_s32 K = (tr_s32)(X * 2.0f);

    X = X - 0.5f*K;

    // NOTE(vak): Taken from Sin, rel. error minimized, degree 9
    // at https://publik-void.github.io/sin-cos-approximations/

    static tr_f32 Coefficients[5] =
    {
        +0.9999999946860073367523161202596403177040898496501611216768f,
        -0.1666665668400715135906952699991284533668603175829321974375f,
        +0.0083330251389693672984848155313618031435079212166530368507f,
        -0.0001980741872742697087457411410886410708022403778296896364f,
        +2.6019030676514601858250088533777315352551909009268267e-6f,
    };

    tr_radians32 R = TR_TURNS_TO_RADIANS(X);
    tr_radians32 RR = R*R;

    tr_f32 Y = Coefficients[4] * RR;
    Y = (Y + Coefficients[3]) * RR;
    Y = (Y + Coefficients[2]) * RR;
    Y = (Y + Coefficients[1]) * RR;
    Y = (Y + Coefficients[0]) * R;

    tr_f32 Negate = 1.0f - 2.0f*(K & 1);
    tr_f32 Result = Y * Negate;

    return (Result);
}

tr_func tr_f32 trCosF32(tr_turns32 X)
{
    tr_s32 K = (tr_s32)(X * 2.0f);

    X = X - 0.5f*K;

    // NOTE(vak): Taken from Cos, rel. error minimized, degree 10
    // at https://publik-void.github.io/sin-cos-approximations/

    static tr_f32 Coefficients[6] =
    {
        +0.9999999990181006763221859215241467592343833431607956876104f,
        -0.4999999804925358106448883126472417799170538129969117112355f,
        +0.0416665985274352494970529831079268817924347682625283903769f,
        -0.0013887969715117499354050093607473354568100294068286064696f,
        +0.0000247432468979897784677199531432331700043016419859875338f,
        -2.579241831825205598039811545787635083596569894942531e-7f,
    };

    tr_radians32 R = TR_TURNS_TO_RADIANS(X);
    tr_radians32 RR = R*R;

    tr_f32 Y = Coefficients[5] * RR;
    Y = (Y + Coefficients[4]) * RR;
    Y = (Y + Coefficients[3]) * RR;
    Y = (Y + Coefficients[2]) * RR;
    Y = (Y + Coefficients[1]) * RR;
    Y = (Y + Coefficients[0]);

    tr_f32 Negate = 1.0f - 2.0f*(K & 1);
    tr_f32 Result = Y * Negate;

    return (Result);
}

tr_func tr_f32 trTanF32(tr_turns32 X)
{
    tr_f32 Result = trSinF32(X) / trCosF32(X);
    return (Result);
}

/////////////////////////////////////////////////
// NOTE(vak): Power & Exponential
/////////////////////////////////////////////////

tr_func tr_f32 trSquareRootF32(tr_f32 X)
{
#if TR_ARCHITECTURE_X64
    tr_f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(X)));
#else
    #error Fallback not yet implemented for trSquareRootF32()
#endif

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

#include <dlfcn.h> // NOTE(vak): For loading libvulkan.so

#if TR_ARCHITECTURE_X64
    #define TR_LINUX_SYSCALL_NR_READ            (0)
    #define TR_LINUX_SYSCALL_NR_WRITE           (1)
    #define TR_LINUX_SYSCALL_NR_OPEN            (2)
    #define TR_LINUX_SYSCALL_NR_CLOSE           (3)
    #define TR_LINUX_SYSCALL_NR_FSTAT           (5)
    #define TR_LINUX_SYSCALL_NR_MMAP            (9)
    #define TR_LINUX_SYSCALL_NR_MPROTECT        (10)
    #define TR_LINUX_SYSCALL_NR_MUNMAP          (11)
    #define TR_LINUX_SYSCALL_NR_NANOSLEEP       (35)
    #define TR_LINUX_SYSCALL_NR_EXIT            (60)
    #define TR_LINUX_SYSCALL_NR_CLOCK_GETTIME   (228)
    #define TR_LINUX_SYSCALL_NR_EXIT_GROUP      (231)
#else
    #error Linux syscall numbers are not defined for this architecture
#endif

static tr_usize trLinuxSyscall(
    tr_usize NR,
    tr_usize Arg0, tr_usize Arg1, tr_usize Arg2,
    tr_usize Arg3, tr_usize Arg4, tr_usize Arg5
)
{
    tr_usize Result = 0;

#if TR_ARCHITECTURE_X64
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
#else
    #error Linux syscall is not implemented for this architecture
#endif

    return (Result);
}

__attribute__((naked))
void trEntryPoint(void)
{
#if TR_ARCHITECTURE_X64
    __asm__ volatile
    (
        "mov 0 (%rsp),          %edi\n"
        "lea 8 (%rsp),          %rsi\n"
        "lea 16(%rsp, %rdi, 8), %rdx\n"
        "call trLinuxBootstrap\n"
    );
#else
    #error Linux entry point is not implemented for this architecture
#endif
}

void Setup(void); // NOTE(vak): Implemented by user
void Frame(tr_f32 LastFrameTime); // NOTE(vak): Implemented by user

tr_func void trLinuxWaylandSetup(void);
tr_func tr_b32 trLinuxWaylandShouldQuit(void);
tr_func void trLinuxWaylandPollEvents(void);
tr_func void trLinuxWaylandPresent(void);
tr_func tr_u32 trLinuxWaylandGetWindowWidth(void);
tr_func tr_u32 trLinuxWaylandGetWindowHeight(void);
tr_func tr_f32 trLinuxWaylandGetMouseX(void);
tr_func tr_f32 trLinuxWaylandGetMouseY(void);
tr_func tr_b32 trLinuxWaylandIsButtonDown(tr_button Button);
tr_func tr_b32 trLinuxWaylandWasButtonDown(tr_button Button);

tr_func void trVulkanSetup(void);
tr_func void trVulkanBeginFrame(void);
tr_func void trVulkanEndFrame(void);

typedef struct
{
    tr_f64 TargetFPS;
} tr_linux_state;

static tr_linux_state trLinux = {0};

void trLinuxBootstrap(tr_s32 ArgCount, char* Args[], char* Envp[])
{
    trLinuxWaylandSetup();
    trVulkanSetup();

    trLinux.TargetFPS = 60.0;

    Setup();

    tr_clock FrameBegin = trGetWallClock();
    tr_f64 LastFrameTime = 0.0f;

    while (!trLinuxWaylandShouldQuit())
    {
        trLinuxWaylandPollEvents();
        trVulkanBeginFrame();

        Frame(LastFrameTime);

        trVulkanEndFrame();
        trLinuxWaylandPresent();

        tr_f64 DesiredFrameTime = 1.0 / trLinux.TargetFPS;

        LastFrameTime = trGetSecondsElapsed(FrameBegin, trGetWallClock());

        while (LastFrameTime < DesiredFrameTime)
        {
            trWait(DesiredFrameTime - LastFrameTime);
            LastFrameTime = trGetSecondsElapsed(FrameBegin, trGetWallClock());
        }

        FrameBegin = trGetWallClock();
    }

    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_EXIT_GROUP, 0,
        0, 0, 0, 0, 0
    );
}

int main(tr_s32 ArgCount, char* Args[], char* Envp[])
{
    trLinuxBootstrap(ArgCount, Args, Envp);
    return (0);
}

tr_func tr_f64 trGetTargetFPS(void)
{
    return (trLinux.TargetFPS);
}

tr_func void trSetTargetFPS(tr_f64 FPS)
{
    trLinux.TargetFPS = FPS;
}

tr_func tr_u32 trGetWindowWidth(void)
{
    tr_u32 Result = trLinuxWaylandGetWindowWidth();
    return (Result);
}

tr_func tr_u32 trGetWindowHeight(void)
{
    tr_u32 Result = trLinuxWaylandGetWindowHeight();
    return (Result);
}

tr_func tr_f32 trGetMouseX(void)
{
    tr_f32 Result = trLinuxWaylandGetMouseX();
    return (Result);
}

tr_func tr_f32 trGetMouseY(void)
{
    tr_f32 Result = trLinuxWaylandGetMouseY();
    return (Result);
}

tr_func tr_b32 trIsButtonDown(tr_button Button)
{
    tr_b32 Result = trLinuxWaylandIsButtonDown(Button);
    return (Result);
}

tr_func tr_b32 trIsButtonUp(tr_button Button)
{
    tr_b32 Result = !trIsButtonDown(Button);
    return (Result);
}

tr_func tr_b32 trIsButtonPressed(tr_button Button)
{
    tr_b32 Result =
        (TR_TRUE  == trLinuxWaylandIsButtonDown(Button)) &&
        (TR_FALSE == trLinuxWaylandWasButtonDown(Button));

    return (Result);
}

tr_func tr_b32 trIsButtonReleased(tr_button Button)
{
    tr_b32 Result =
        (TR_FALSE == trLinuxWaylandIsButtonDown(Button)) &&
        (TR_TRUE  == trLinuxWaylandWasButtonDown(Button));

    return (Result);
}

typedef struct
{
    // NOTE(vak): Taken from /usr/include/asm/stat.h

    tr_u64          Device;
    tr_u64          FileSerialNumber;
    tr_u64          LinkCount;

    tr_u32          Mode;
    tr_u32          UserID;
    tr_u32          GroupID;
    tr_u32          Pad0;

    tr_u64          DeviceNumber;
    tr_s64          FileSize;
    tr_s64          BlockSize;
    tr_s64          Blocks512;

    tr_s64          AccessSeconds;
    tr_u64          AccessNanoseconds;
    tr_s64          ModifySeconds;
    tr_u64          ModifyNanoseconds;
    tr_s64          StatusSeconds;
    tr_u64          StatusNanoseconds;

    tr_s64          Unused[3];
} tr_linux_stat;

tr_func tr_loaded_file trLoadFile(tr_str FilePath)
{
    tr_loaded_file Result = {0};

    tr_s32 FileDescriptor = -1;

    static char NullTerminated[4096] = {0};

    if (FilePath.Size + 1 <= sizeof(NullTerminated))
    {
        trCopyMemory(NullTerminated, FilePath.Data, FilePath.Size);

        NullTerminated[FilePath.Size] = '\0';

        FileDescriptor = trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_OPEN,
            (tr_usize)NullTerminated,
            0, // NOTE(vak): O_RDONLY
            0, 0, 0, 0
        );
    }

    if (FileDescriptor > 2)
    {
        tr_linux_stat Stat = {0};

        tr_s32 StatResult = trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_FSTAT,
            FileDescriptor,
            (tr_usize)&Stat,
            0, 0, 0, 0
        );

        if (StatResult >= 0)
            Result.Size = Stat.FileSize;
    }

    if (Result.Size)
    {
        tr_ssize Mapping = trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_MMAP,
            0,
            Result.Size,
            0x3,    // NOTE(vak): PROT_READ | PROT_WRITE,
            0x22,   // NOTE(vak): MAP_PRIVATE | MAP_ANON
            -1,
            0
        );

        Result.Data = (void*)TR_MAX(0, Mapping);
    }

    if (Result.Data)
    {
        tr_ssize ReadResult = trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_READ,
            FileDescriptor,
            (tr_usize)Result.Data,
            Result.Size,
            0, 0, 0
        );

        tr_usize BytesRead = TR_MAX(0, ReadResult);

        if (BytesRead != Result.Size)
        {
            trUnloadFile(Result);

            Result.Data = 0;
            Result.Size = 0;
        }
    }

    if (FileDescriptor > 2)
    {
        trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_CLOSE,
            FileDescriptor,
            0, 0, 0, 0, 0
        );
    }

    return (Result);
}

tr_func void trUnloadFile(tr_loaded_file LoadedFile)
{
    if (LoadedFile.Data)
    {
        trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_MUNMAP,
            (tr_usize)LoadedFile.Data,
            LoadedFile.Size,
            0, 0, 0, 0
        );
    }
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

tr_func void trWait(tr_f64 Seconds)
{
    tr_f64 IntegerPart = trFloorF64(Seconds);
    tr_f64 DecimalPart = Seconds - IntegerPart;

    tr_linux_timespec Duration =
    {
        .Seconds        = (tr_usize)(IntegerPart),
        .Nanoseconds    = (tr_usize)(DecimalPart * 1e9),
    };

    while (Duration.Seconds || Duration.Nanoseconds)
    {
        tr_linux_timespec Remainder = {0};

        trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_NANOSLEEP,
            (tr_usize)(&Duration),
            (tr_usize)(&Remainder),
            0, 0, 0, 0
        );

        Duration = Remainder;
    }
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

tr_func void trExit(tr_u8 ExitCode)
{
    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_EXIT,
        ExitCode,
        0, 0, 0, 0, 0
    );
}

/////////////////////////////////////////////////
// NOTE(vak): Wayland windowing backend
/////////////////////////////////////////////////

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

// NOTE(vak): Stuff copied from xdg-shell-client-protocol.h
// and xdg_private.h
//
// https://github.com/sagarthereal/wayland/blob/main/xdg-shell-client-protocol.h
// https://github.com/sagarthereal/wayland/blob/main/xdg_private.h

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

struct xdg_wm_base_listener
{
    void (*ping)(void *data,
                 struct xdg_wm_base *xdg_wm_base,
                 uint32_t serial);
};

static inline int
xdg_wm_base_add_listener(struct xdg_wm_base *xdg_wm_base,
			 const struct xdg_wm_base_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_wm_base,
				     (void (**)(void)) listener, data);
}

static inline void
xdg_wm_base_pong(struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	wl_proxy_marshal_flags((struct wl_proxy *) xdg_wm_base,
			 3, NULL, wl_proxy_get_version((struct wl_proxy *) xdg_wm_base), 0, serial);
}

static inline struct xdg_surface *
xdg_wm_base_get_xdg_surface(struct xdg_wm_base *xdg_wm_base, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) xdg_wm_base,
			 2, &xdg_surface_interface, wl_proxy_get_version((struct wl_proxy *) xdg_wm_base), 0, NULL, surface);

	return (struct xdg_surface *) id;
}

struct xdg_surface_listener
{
	void (*configure)(void *data,
			  struct xdg_surface *xdg_surface,
			  uint32_t serial);
};

static inline int
xdg_surface_add_listener(struct xdg_surface *xdg_surface,
			 const struct xdg_surface_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_surface,
				     (void (**)(void)) listener, data);
}

static inline struct xdg_toplevel *
xdg_surface_get_toplevel(struct xdg_surface *xdg_surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) xdg_surface,
			 1, &xdg_toplevel_interface, wl_proxy_get_version((struct wl_proxy *) xdg_surface), 0, NULL);

	return (struct xdg_toplevel *) id;
}

static inline void
xdg_surface_ack_configure(struct xdg_surface *xdg_surface, uint32_t serial)
{
	wl_proxy_marshal_flags((struct wl_proxy *) xdg_surface,
			 4, NULL, wl_proxy_get_version((struct wl_proxy *) xdg_surface), 0, serial);
}

struct xdg_toplevel_listener {
	void (*configure)(void *data,
			  struct xdg_toplevel *xdg_toplevel,
			  int32_t width,
			  int32_t height,
			  struct wl_array *states);

	void (*close)(void *data,
		      struct xdg_toplevel *xdg_toplevel);

	void (*configure_bounds)(void *data,
				 struct xdg_toplevel *xdg_toplevel,
				 int32_t width,
				 int32_t height);

	void (*wm_capabilities)(void *data,
				struct xdg_toplevel *xdg_toplevel,
				struct wl_array *capabilities);
};

static inline int
xdg_toplevel_add_listener(struct xdg_toplevel *xdg_toplevel,
			  const struct xdg_toplevel_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) xdg_toplevel,
				     (void (**)(void)) listener, data);
}

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;

static const struct wl_interface *xdg_shell_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&xdg_surface_interface,
	&wl_surface_interface,
	&xdg_toplevel_interface,
	NULL,
	&xdg_surface_interface,
	NULL,
	&xdg_toplevel_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
	&wl_seat_interface,
	NULL,
	&wl_seat_interface,
	NULL,
	NULL,
	&wl_output_interface,
	&wl_seat_interface,
	NULL,
	NULL,
	NULL,
};

static const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "create_positioner", "n", xdg_shell_types + 4 },
	{ "get_xdg_surface", "no", xdg_shell_types + 5 },
	{ "pong", "u", xdg_shell_types + 0 },
};

static const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", xdg_shell_types + 0 },
};

const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base", 5,
	4, xdg_wm_base_requests,
	1, xdg_wm_base_events,
};

static const struct wl_message xdg_surface_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "get_toplevel", "n", xdg_shell_types + 7 },
	{ "get_popup", "n?oo", xdg_shell_types + 8 },
	{ "set_window_geometry", "iiii", xdg_shell_types + 0 },
	{ "ack_configure", "u", xdg_shell_types + 0 },
};

static const struct wl_message xdg_surface_events[] = {
	{ "configure", "u", xdg_shell_types + 0 },
};

const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 5,
	5, xdg_surface_requests,
	1, xdg_surface_events,
};

static const struct wl_message xdg_toplevel_requests[] = {
	{ "destroy", "", xdg_shell_types + 0 },
	{ "set_parent", "?o", xdg_shell_types + 11 },
	{ "set_title", "s", xdg_shell_types + 0 },
	{ "set_app_id", "s", xdg_shell_types + 0 },
	{ "show_window_menu", "ouii", xdg_shell_types + 12 },
	{ "move", "ou", xdg_shell_types + 16 },
	{ "resize", "ouu", xdg_shell_types + 18 },
	{ "set_max_size", "ii", xdg_shell_types + 0 },
	{ "set_min_size", "ii", xdg_shell_types + 0 },
	{ "set_maximized", "", xdg_shell_types + 0 },
	{ "unset_maximized", "", xdg_shell_types + 0 },
	{ "set_fullscreen", "?o", xdg_shell_types + 21 },
	{ "unset_fullscreen", "", xdg_shell_types + 0 },
	{ "set_minimized", "", xdg_shell_types + 0 },
};

static const struct wl_message xdg_toplevel_events[] = {
	{ "configure", "iia", xdg_shell_types + 0 },
	{ "close", "", xdg_shell_types + 0 },
	{ "configure_bounds", "4ii", xdg_shell_types + 0 },
	{ "wm_capabilities", "5a", xdg_shell_types + 0 },
};

const struct wl_interface xdg_toplevel_interface = {
	"xdg_toplevel", 5,
	14, xdg_toplevel_requests,
	4, xdg_toplevel_events,
};

typedef struct
{
    struct wl_display* Display;
    struct wl_registry* Registry;
    struct wl_compositor* Compositor;
    struct xdg_wm_base* XdgWmBase;
    struct wl_surface* Surface;
    struct xdg_surface* XdgSurface;
    struct xdg_toplevel* XdgTopLevel;

    struct wl_seat* Seat;
    struct wl_pointer* Pointer;
    struct wl_keyboard* Keyboard;

    struct xkb_context* XkbContext;
    struct xkb_keymap* XkbKeymap;
    struct xkb_state* XkbState;

    tr_f32 MouseX, MouseY;

    tr_u32 TopLevelW, TopLevelH;
    tr_u32 WindowW, WindowH;
    tr_b32 Quit;

    tr_b32 IsDown[TR_BUTTON_MAX];
    tr_b32 WasDown[TR_BUTTON_MAX];
} tr_wayland_state;

static tr_wayland_state trWayland = {0};

tr_func tr_b32 trLinuxWaylandMatchString(const char* A, const char* B)
{
    while (*A && *B)
    {
        if (*A != *B)
            break;

        A++;
        B++;
    }

    tr_b32 Result = (*A == *B);
    return (Result);
}

tr_func void trLinuxWaylandRegistryGlobal(
    void* Data,
    struct wl_registry* Registry,
    tr_u32 Name,
    const char* InterfaceData,
    tr_u32 Version
)
{
    if (trLinuxWaylandMatchString(InterfaceData, wl_compositor_interface.name))
    {
        trWayland.Compositor = wl_registry_bind(Registry, Name, &wl_compositor_interface, Version);
    }
    else if (trLinuxWaylandMatchString(InterfaceData, wl_seat_interface.name))
    {
        trWayland.Seat = wl_registry_bind(Registry, Name, &wl_seat_interface, Version);
    }
    else if (trLinuxWaylandMatchString(InterfaceData, xdg_wm_base_interface.name))
    {
        trWayland.XdgWmBase = wl_registry_bind(Registry, Name, &xdg_wm_base_interface, Version);
    }
}

tr_func void trLinuxWaylandXdgWmBasePing(
    void* Data,
    struct xdg_wm_base* XdgWmBase,
    tr_u32 Serial
)
{
    xdg_wm_base_pong(XdgWmBase, Serial);
}

tr_func void trLinuxWaylandXdgSurfaceConfigure(
    void* Data,
    struct xdg_surface* XdgSurface,
    tr_u32 Serial
)
{
    xdg_surface_ack_configure(XdgSurface, Serial);

    trWayland.WindowW = trWayland.TopLevelW;
    trWayland.WindowH = trWayland.TopLevelH;
}

tr_func void trLinuxWaylandXdgTopLevelConfigure(
    void* Data,
    struct xdg_toplevel* XdgTopLevel,
    tr_s32 Width,
    tr_s32 Height,
    struct wl_array* States
)
{
    trWayland.TopLevelW = TR_MAX(0, Width);
    trWayland.TopLevelH = TR_MAX(0, Height);
}

tr_func void trLinuxWaylandXdgTopLevelClose(
    void* Data,
    struct xdg_toplevel* XdgTopLevel
)
{
    trWayland.Quit = TR_TRUE;
}

tr_func void trLinuxWaylandXdgTopLevelConfigureBounds(
    void* Data,
    struct xdg_toplevel* XdgTopLevel,
    tr_s32 Width,
    tr_s32 Height
)
{
}

tr_func void trLinuxWaylandXdgTopLevelWmCapabilities(
    void* Data,
    struct xdg_toplevel* XdgTopLevel,
    struct wl_array* Capabilities
)
{
}

tr_func void trLinuxWaylandPointerEnter(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Serial,
    struct wl_surface* Surface,
    wl_fixed_t SurfaceX,
    wl_fixed_t SurfaceY
)
{
    trWayland.MouseX = (tr_f32)wl_fixed_to_double(SurfaceX);
    trWayland.MouseY = (tr_f32)wl_fixed_to_double(SurfaceY);
}

tr_func void trLinuxWaylandPointerLeave(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Serial,
    struct wl_surface* Surface
)
{
}

tr_func void trLinuxWaylandPointerMotion(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Time,
    wl_fixed_t SurfaceX,
    wl_fixed_t SurfaceY
)
{
    trWayland.MouseX = (tr_f32)wl_fixed_to_double(SurfaceX);
    trWayland.MouseY = (tr_f32)wl_fixed_to_double(SurfaceY);
}

tr_func void trLinuxWaylandPointerButton(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Serial,
    tr_u32 Time,
    tr_u32 ButtonCode,
    tr_u32 State
)
{
    tr_b32 IsDown = (State == WL_POINTER_BUTTON_STATE_PRESSED);
    tr_button Button = TR_BUTTON_NIL;

    switch (ButtonCode)
    {
        default: break;

        // NOTE(vak): From linux/input-event-codes.h
        //      0x110 = BTN_LEFT
        //      0x111 = BTN_RIGHT
        //      0x112 = BTN_MIDDLE

        case 0x110: Button = TR_BUTTON_MOUSE_LEFT; break;
        case 0x111: Button = TR_BUTTON_MOUSE_RIGHT; break;
        case 0x112: Button = TR_BUTTON_MOUSE_MIDDLE; break;
    }

    if (Button != TR_BUTTON_NIL)
        trWayland.IsDown[Button] = IsDown;
}

tr_func void trLinuxWaylandPointerAxis(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Time,
    tr_u32 Axis,
    wl_fixed_t Value
)
{
}

tr_func void trLinuxWaylandPointerFrame(
    void* Data,
    struct wl_pointer* Pointer
)
{
}

tr_func void trLinuxWaylandPointerAxisSource(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 AxisSource
)
{
}

tr_func void trLinuxWaylandPointerAxisStop(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Time,
    tr_u32 Axis
)
{
}

tr_func void trLinuxWaylandPointerAxisDiscrete(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Axis,
    tr_s32 Discrete
)
{
}

tr_func void trLinuxWaylandPointerAxisValue120(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Axis,
    tr_s32 Value120
)
{
}

tr_func void trLinuxWaylandPointerAxisRelativeDirection(
    void* Data,
    struct wl_pointer* Pointer,
    tr_u32 Axis,
    tr_u32 Direction
)
{
}

tr_func void trLinuxWaylandPointerWarp(
    void* Data,
    struct wl_pointer* Pointer,
    wl_fixed_t SurfaceX,
    wl_fixed_t SurfaceY
)
{
    trWayland.MouseX = (tr_f32)wl_fixed_to_double(SurfaceX);
    trWayland.MouseY = (tr_f32)wl_fixed_to_double(SurfaceY);
}

tr_func void trLinuxWaylandKeyboardKeymap(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_u32 Format,
    tr_s32 FileDescriptor,
    tr_u32 Size
)
{
    if (trWayland.XkbState) xkb_state_unref(trWayland.XkbState);
    if (trWayland.XkbKeymap) xkb_keymap_unref(trWayland.XkbKeymap);
    if (trWayland.XkbContext) xkb_context_unref(trWayland.XkbContext);

    if (Format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        return;

    tr_ssize Mapping = (tr_ssize)trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_MMAP,
        0,
        Size,
        0x3,    // NOTE(vak): PROT_READ | PROT_WRITE
        0x02,   // NOTE(vak): MAP_PRIVATE
        FileDescriptor,
        0
    );

    void* KeymapString = (void*)TR_MAX(0, Mapping);

    trWayland.XkbContext = 0;
    trWayland.XkbState = 0;
    trWayland.XkbKeymap = 0;

    if (KeymapString)
        trWayland.XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (trWayland.XkbContext)
    {
        trWayland.XkbKeymap = xkb_keymap_new_from_string(
            trWayland.XkbContext,
            KeymapString,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS
        );
    }

    if (trWayland.XkbKeymap)
        trWayland.XkbState = xkb_state_new(trWayland.XkbKeymap);

    if (KeymapString)
    {
        trLinuxSyscall(
            TR_LINUX_SYSCALL_NR_MUNMAP,
            (tr_usize)KeymapString,
            Size,
            0, 0, 0, 0
        );
    }
}

tr_func void trLinuxWaylandKeyboardEnter(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_u32 Serial,
    struct wl_surface* Surface,
    struct wl_array* Keys
)
{
}

tr_func void trLinuxWaylandKeyboardLeave(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_u32 Serial,
    struct wl_surface* Surface
)
{
}

tr_func void trLinuxWaylandKeyboardKey(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_u32 Serial,
    tr_u32 Time,
    tr_u32 EvdevScancode,
    tr_u32 State
)
{
    if (!trWayland.XkbState)
        return;

    tr_b32 IsDown = (State == WL_KEYBOARD_KEY_STATE_PRESSED);

    tr_u32 XkbScancode = 8 + EvdevScancode;

    xkb_keysym_t KeySym = xkb_state_key_get_one_sym(
        trWayland.XkbState,
        XkbScancode
    );

    tr_button Button = TR_BUTTON_NIL;

    switch (KeySym)
    {
        default: break;

        case XKB_KEY_space: Button = TR_BUTTON_KEY_SPACE; break;
        case XKB_KEY_Left:  Button = TR_BUTTON_KEY_LEFT; break;
        case XKB_KEY_Right: Button = TR_BUTTON_KEY_RIGHT; break;
    }

    if (Button != TR_BUTTON_NIL)
        trWayland.IsDown[Button] = IsDown;
}

tr_func void trLinuxWaylandKeyboardModifiers(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_u32 Serial,
    tr_u32 ModifiersDepressed,
    tr_u32 ModifiersLatched,
    tr_u32 ModifiersLocked,
    tr_u32 Group
)
{
}

tr_func void trLinuxWaylandKeyboardRepeatInfo(
    void* Data,
    struct wl_keyboard* Keyboard,
    tr_s32 Rate,
    tr_s32 Delay
)
{
}

tr_func void trLinuxWaylandSeatCapabilities(
    void* Data,
    struct wl_seat* Seat,
    tr_u32 Capabilities
)
{
    if (trWayland.Pointer) wl_pointer_release(trWayland.Pointer);
    if (trWayland.Keyboard) wl_keyboard_release(trWayland.Keyboard);

    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
        trWayland.Pointer = wl_seat_get_pointer(Seat);

    if (Capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        trWayland.Keyboard = wl_seat_get_keyboard(Seat);

    if (trWayland.Pointer)
    {
        static struct wl_pointer_listener PointerListener =
        {
            .enter = trLinuxWaylandPointerEnter,
            .leave = trLinuxWaylandPointerLeave,
            .motion = trLinuxWaylandPointerMotion,
            .button = trLinuxWaylandPointerButton,
            .axis = trLinuxWaylandPointerAxis,
            .frame = trLinuxWaylandPointerFrame,
            .axis_source = trLinuxWaylandPointerAxisSource,
            .axis_stop = trLinuxWaylandPointerAxisStop,
            .axis_discrete = trLinuxWaylandPointerAxisDiscrete,
            .axis_value120 = trLinuxWaylandPointerAxisValue120,
            .axis_relative_direction = trLinuxWaylandPointerAxisRelativeDirection,
            .warp = trLinuxWaylandPointerWarp,
        };

        wl_pointer_add_listener(trWayland.Pointer, &PointerListener, 0);
    }

    if (trWayland.Keyboard)
    {
        static struct wl_keyboard_listener KeyboardListener =
        {
            .keymap = trLinuxWaylandKeyboardKeymap,
            .enter = trLinuxWaylandKeyboardEnter,
            .leave = trLinuxWaylandKeyboardLeave,
            .key = trLinuxWaylandKeyboardKey,
            .modifiers = trLinuxWaylandKeyboardModifiers,
            .repeat_info = trLinuxWaylandKeyboardRepeatInfo,
        };

        wl_keyboard_add_listener(trWayland.Keyboard, &KeyboardListener, 0);
    }
}

tr_func void trLinuxWaylandSeatName(
    void* Data,
    struct wl_seat* Seat,
    const char* Name
)
{
}

tr_func void trLinuxWaylandSetup(void)
{
    trWayland.Display = wl_display_connect(0);
    TR_ASSERT(trWayland.Display);

    trWayland.Registry = wl_display_get_registry(trWayland.Display);
    TR_ASSERT(trWayland.Registry);

    static struct wl_registry_listener RegistryListener =
    {
        .global = trLinuxWaylandRegistryGlobal,
    };

    wl_registry_add_listener(trWayland.Registry, &RegistryListener, 0);

    wl_display_roundtrip(trWayland.Display);

    TR_ASSERT(trWayland.Compositor);
    TR_ASSERT(trWayland.XdgWmBase);

    if (trWayland.Seat)
    {
        static struct wl_seat_listener SeatListener =
        {
            .capabilities = trLinuxWaylandSeatCapabilities,
            .name = trLinuxWaylandSeatName,
        };

        wl_seat_add_listener(trWayland.Seat, &SeatListener, 0);
    }

    static struct xdg_wm_base_listener XdgWmBaseListener =
    {
        .ping = trLinuxWaylandXdgWmBasePing,
    };

    xdg_wm_base_add_listener(trWayland.XdgWmBase, &XdgWmBaseListener, 0);

    trWayland.Surface = wl_compositor_create_surface(trWayland.Compositor);
    TR_ASSERT(trWayland.Surface);

    trWayland.XdgSurface = xdg_wm_base_get_xdg_surface(trWayland.XdgWmBase, trWayland.Surface);
    TR_ASSERT(trWayland.XdgSurface);

    trWayland.XdgTopLevel = xdg_surface_get_toplevel(trWayland.XdgSurface);
    TR_ASSERT(trWayland.XdgTopLevel);

    static struct xdg_surface_listener XdgSurfaceListener =
    {
        .configure = trLinuxWaylandXdgSurfaceConfigure,
    };

    xdg_surface_add_listener(trWayland.XdgSurface, &XdgSurfaceListener, 0);

    static struct xdg_toplevel_listener XdgTopLevelListener =
    {
        .configure = trLinuxWaylandXdgTopLevelConfigure,
        .close = trLinuxWaylandXdgTopLevelClose,
        .configure_bounds = trLinuxWaylandXdgTopLevelConfigureBounds,
        .wm_capabilities = trLinuxWaylandXdgTopLevelWmCapabilities,
    };

    xdg_toplevel_add_listener(trWayland.XdgTopLevel, &XdgTopLevelListener, 0);

    wl_surface_commit(trWayland.Surface);
    wl_display_roundtrip(trWayland.Display);
    wl_surface_commit(trWayland.Surface);
}

tr_func tr_b32 trLinuxWaylandShouldQuit(void)
{
    tr_b32 Result = trWayland.Quit;
    return (Result);
}

tr_func void trLinuxWaylandPollEvents(void)
{
    for (tr_button Button = 1; Button < TR_BUTTON_MAX; Button++)
    {
        trWayland.WasDown[Button] = trWayland.IsDown[Button];
    }

    wl_display_roundtrip(trWayland.Display);
}

tr_func void trLinuxWaylandPresent(void)
{
    wl_surface_commit(trWayland.Surface);
}

tr_func tr_u32 trLinuxWaylandGetWindowWidth(void)
{
    tr_u32 Result = trWayland.WindowW;
    return (Result);
}

tr_func tr_u32 trLinuxWaylandGetWindowHeight(void)
{
    tr_u32 Result = trWayland.WindowH;
    return (Result);
}

tr_func tr_f32 trLinuxWaylandGetMouseX(void)
{
    tr_f32 Result = trWayland.MouseX;
    return (Result);
}

tr_func tr_f32 trLinuxWaylandGetMouseY(void)
{
    tr_f32 Result = trWayland.MouseY;
    return (Result);
}

tr_func tr_b32 trLinuxWaylandIsButtonDown(tr_button Button)
{
    if ((Button == TR_BUTTON_NIL) || (Button >= TR_BUTTON_MAX))
        return (TR_FALSE);

    return trWayland.IsDown[Button];
}

tr_func tr_b32 trLinuxWaylandWasButtonDown(tr_button Button)
{
    if ((Button == TR_BUTTON_NIL) || (Button >= TR_BUTTON_MAX))
        return (TR_FALSE);

    return trWayland.WasDown[Button];
}

#else
    #error Unimplemented platform
#endif

/////////////////////////////////////////////////
// NOTE(vak): Vulkan rendering backend
/////////////////////////////////////////////////

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

#if TR_PLATFORM_LINUX
    #include <vulkan/vulkan_wayland.h>
#else
    #error Missing vulkan include for platform
#endif

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = 0;
static PFN_vkCreateInstance vkCreateInstance = 0;
static PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = 0;

#define TR_VULKAN_ALL_FUNCTIONS(X) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceMemoryProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkCreateDevice) \
    X(vkDeviceWaitIdle) \
    X(vkGetDeviceQueue) \
    X(vkCreateCommandPool) \
    X(vkAllocateCommandBuffers) \
    X(vkCreateSemaphore) \
    X(vkCreateDescriptorPool) \
    X(vkCreateDescriptorSetLayout) \
    X(vkAllocateDescriptorSets) \
    X(vkUpdateDescriptorSets) \
    X(vkCreatePipelineLayout) \
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    X(vkCreateGraphicsPipelines) \
    X(vkCreateBuffer) \
    X(vkGetBufferMemoryRequirements) \
    X(vkBindBufferMemory) \
    X(vkAllocateMemory) \
    X(vkMapMemory) \
    X(vkCreateSampler) \
    X(vkCreateImage) \
    X(vkGetImageMemoryRequirements) \
    X(vkBindImageMemory) \
    X(vkCreateImageView) \
    X(vkDestroyImageView) \
    X(vkCreateSwapchainKHR) \
    X(vkDestroySwapchainKHR) \
    X(vkGetSwapchainImagesKHR) \
    X(vkAcquireNextImageKHR) \
    X(vkResetCommandBuffer) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    X(vkCmdBeginRendering) \
    X(vkCmdEndRendering) \
    X(vkCmdBindPipeline) \
    X(vkCmdSetViewport) \
    X(vkCmdSetScissor) \
    X(vkCmdPushDescriptorSet) \
    X(vkCmdBindDescriptorSets) \
    X(vkCmdPushConstants) \
    X(vkCmdDraw) \
    X(vkCmdPipelineBarrier) \
    X(vkCmdCopyBufferToImage) \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR)

#define TR_VULKAN_DECLARE(Name) PFN_##Name Name = 0;
    TR_VULKAN_ALL_FUNCTIONS(TR_VULKAN_DECLARE)
#undef TR_VULKAN_DECLARE

typedef struct
{
    tr_f32 X, Y;    
    tr_f32 U, V;    
    tr_u32 PackedRGBA;
    tr_u32 TextureIndex;
} tr_vulkan_vertex;

typedef struct
{
    tr_f32 Projection[16];
} tr_vulkan_push_constants;

typedef struct
{
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    tr_usize Size;
    void* Mapping;
} tr_vulkan_buffer;

typedef struct
{
    VkImage Image;
    VkDeviceMemory Memory;
    VkImageView View;
    tr_u32 Width, Height;

    // NOTE(vak): Format that the user created the image with.
    // The actual backing memory is RGBA8, so conversion is
    // needed when uploading.
    VkFormat Format; 
} tr_vulkan_texture;

typedef struct
{
    tr_u32 VersionOfAPI;
    VkInstance Instance;
    VkSurfaceKHR Surface;
    VkPhysicalDevice PhysicalDevice;
    tr_u32 QueueFamilyIndex;
    VkDevice Device;
    VkQueue Queue;
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;
    VkSemaphore AcquireSemaphore;
    VkSemaphore SubmitSemaphore;
    VkSurfaceFormatKHR SwapchainFormat;
    VkPresentModeKHR PresentMode;

    VkSampler NearestSampler;

    tr_vulkan_buffer VertexBuffer;
    tr_vulkan_buffer TransferBuffer;

    tr_u32 MaxTextureCount;
    tr_u32 TextureCount;
    tr_vulkan_texture* Textures;

    VkDescriptorPool TexturePool;
    VkDescriptorSet TextureSet;

    VkDescriptorSetLayout BufferSetLayout;
    VkDescriptorSetLayout TextureSetLayout;

    VkPipelineLayout PipelineLayout;
    VkPipeline Pipeline;

    tr_texture DefaultTexture;
    tr_texture FontTexture;

    VkExtent2D SwapchainExtent;
    VkSwapchainKHR Swapchain;
    tr_u32 MaxImageCount;
    tr_u32 ImageCount;
    VkImage* SwapchainImages;
    VkImageView* SwapchainImageViews;

    tr_u32 ImageIndex;
    tr_u32 VertexCount;
    tr_f32 ClearColor[4];
} tr_vulkan_state;

static tr_vulkan_state trVulkan = {0};

#define TR_VK_CHECK(VulkanCall) \
    { \
        VkResult __VulkanResult__ = (VulkanCall); \
        TR_ASSERT(__VulkanResult__ == VK_SUCCESS); \
    }

tr_func tr_u32 trVulkanSelectMemoryType(
    VkMemoryPropertyFlags DesiredFlags,
    tr_u32 MemoryTypeBits
)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(trVulkan.PhysicalDevice, &MemoryProperties);

    tr_u32 MemoryTypeIndex = ~0u;

    for (tr_u32 Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
    {
        VkMemoryType* MemoryType = MemoryProperties.memoryTypes + Index;

        if ((MemoryTypeBits & (1 << Index)) == 0)
            continue;

        if ((MemoryType->propertyFlags & DesiredFlags) == DesiredFlags)
        {
            MemoryTypeIndex = Index;
            break;
        }
    }

    TR_ASSERT(MemoryTypeIndex != ~0u);

    return (MemoryTypeIndex);
}

tr_func void trVulkanMakeBuffer(
    tr_vulkan_buffer* Buffer,
    tr_usize Size,
    VkBufferUsageFlags UsageFlags,
    VkMemoryPropertyFlags DesiredMemoryFlags,
    tr_b32 Mapped
)
{
    trZeroType(Buffer);

    Buffer->Size = Size;

    VkBufferCreateInfo BufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = UsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    TR_VK_CHECK(vkCreateBuffer(trVulkan.Device, &BufferInfo, 0, &Buffer->Buffer));

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetBufferMemoryRequirements(trVulkan.Device, Buffer->Buffer, &MemoryRequirements);

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = trVulkanSelectMemoryType(
            DesiredMemoryFlags,
            MemoryRequirements.memoryTypeBits
        ),
    };

    TR_VK_CHECK(vkAllocateMemory(trVulkan.Device, &AllocateInfo, 0, &Buffer->Memory));
    TR_VK_CHECK(vkBindBufferMemory(trVulkan.Device, Buffer->Buffer, Buffer->Memory, 0));

    TR_VK_CHECK(vkMapMemory(
        trVulkan.Device,
        Buffer->Memory,
        0,
        Buffer->Size,
        0,
        &Buffer->Mapping
    ));
}

tr_func void trVulkanMakeTexture(
    tr_vulkan_texture* Texture,
    tr_u32 Width,
    tr_u32 Height,
    VkFormat Format,
    VkImageUsageFlags UsageFlags
)
{
    trZeroType(Texture);

    tr_b32 IsSRGB =
        (Format == VK_FORMAT_R8G8B8_SRGB) ||
        (Format == VK_FORMAT_R8G8B8A8_SRGB);

    Texture->Width = Width;
    Texture->Height = Height;
    Texture->Format = Format;

    // NOTE(vak): At the time of this comment, B8G8R8A8 seems to
    // be the most widely supported image format, so it is always
    // used as the internal storage format of the image. However,
    // the actual pixel data sent to the GPU will always be RGBA.
    // The swizzling (see image view creation below) is setup so
    // that the RGBA pixel data is read correctly.

    // NOTE(vak): Optimal tiled image format feature flag coverge
    // for all platforms can be found at:
    //      https://vulkan.gpuinfo.org/listoptimaltilingformats.php

    VkImageCreateInfo ImageInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (IsSRGB) ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM,
        .extent = {.width = Width, .height = Height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = UsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    TR_VK_CHECK(vkCreateImage(trVulkan.Device, &ImageInfo, 0, &Texture->Image));

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetImageMemoryRequirements(trVulkan.Device, Texture->Image, &MemoryRequirements);

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = trVulkanSelectMemoryType(
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            MemoryRequirements.memoryTypeBits
        ),
    };

    TR_VK_CHECK(vkAllocateMemory(trVulkan.Device, &AllocateInfo, 0, &Texture->Memory));
    TR_VK_CHECK(vkBindImageMemory(trVulkan.Device, Texture->Image, Texture->Memory, 0));

    VkImageViewCreateInfo ViewInfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = Texture->Image,
        .format = ImageInfo.format,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components =
        {
            // NOTE(vak): We upload RGBA pixel data, but our
            // texture format is BGRA, so we have to setup
            // swizzling to get the correct components when
            // fetching texels.

            .r = VK_COMPONENT_SWIZZLE_B,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_R,
            .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
            .levelCount = 1,
        },
    };

    TR_VK_CHECK(vkCreateImageView(trVulkan.Device, &ViewInfo, 0, &Texture->View));
}

tr_func void trVulkanSetup(void)
{
    // NOTE(vak): Load non-instance functions
    {
    #if TR_PLATFORM_LINUX
        void* Library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);

        if (!Library)
            Library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);

        TR_ASSERT(Library);

        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
            dlsym(Library, "vkGetInstanceProcAddr");

        vkCreateInstance = (PFN_vkCreateInstance)
            dlsym(Library, "vkCreateInstance");

        vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)
            dlsym(Library, "vkEnumerateInstanceVersion");
    #else
        #error Unimplemented vkGetInstanceProcAddr load for platform
    #endif

        TR_ASSERT(vkGetInstanceProcAddr);
        TR_ASSERT(vkCreateInstance);
        TR_ASSERT(vkEnumerateInstanceVersion);
    }

    // NOTE(vak): API Version
    {
        trVulkan.VersionOfAPI = VK_API_VERSION_1_4;

        tr_u32 Version = 0;
        TR_VK_CHECK(vkEnumerateInstanceVersion(&Version));

        TR_ASSERT(Version >= trVulkan.VersionOfAPI);
    }

    // NOTE(vak): Instance
    {
        const char* Layers[] =
        {
            "VK_LAYER_KHRONOS_validation",
        };

        const char* Extensions[] =
        {
            "VK_KHR_surface",

        #if TR_PLATFORM_LINUX
            "VK_KHR_wayland_surface",
        #else
            #error Missing vulkan surface extension for platform
        #endif
        };

        VkInstanceCreateInfo InstanceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo)
            {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "Trampoline App",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                .pEngineName = "Trampoline",
                .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                .apiVersion = trVulkan.VersionOfAPI,
            },
            .enabledLayerCount = TR_ARRAY_COUNT(Layers),
            .ppEnabledLayerNames = Layers,
            .enabledExtensionCount = TR_ARRAY_COUNT(Extensions),
            .ppEnabledExtensionNames = Extensions,
        };

        TR_VK_CHECK(vkCreateInstance(&InstanceInfo, 0, &trVulkan.Instance));
    }

    // NOTE(vak): Load instance functions
    {
        #define TR_VULKAN_LOAD(Name) \
            Name = (PFN_##Name)vkGetInstanceProcAddr(trVulkan.Instance, #Name); \
            TR_ASSERT(Name);

        TR_VULKAN_ALL_FUNCTIONS(TR_VULKAN_LOAD);

        #undef TR_VULKAN_LOAD
    }

    // NOTE(vak): Surface
    {
    #if TR_PLATFORM_LINUX
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)
            vkGetInstanceProcAddr(trVulkan.Instance, "vkCreateWaylandSurfaceKHR");

        TR_ASSERT(vkCreateWaylandSurfaceKHR);

        VkWaylandSurfaceCreateInfoKHR SurfaceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = trWayland.Display,
            .surface = trWayland.Surface,
        };

        TR_VK_CHECK(vkCreateWaylandSurfaceKHR(trVulkan.Instance, &SurfaceInfo, 0, &trVulkan.Surface));
    #else
        #error Missing Vulkan surface creation code for platform
    #endif
    }

    // NOTE(vak): Physical device
    {
        tr_arena* Arena = trGetTemporaryArena();
        tr_usize ResetArenaTo = trGetArenaPosition(Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkEnumeratePhysicalDevices(trVulkan.Instance, &Count, 0));

        VkPhysicalDevice* Array = trPushArenaArrayZero(Arena, VkPhysicalDevice, Count);
        TR_VK_CHECK(vkEnumeratePhysicalDevices(trVulkan.Instance, &Count, Array));

        VkPhysicalDevice Preferred = {0};
        VkPhysicalDevice Fallback = {0};

        for (tr_u32 Index = 0; Index < Count; Index++)
        {
            VkPhysicalDevice GPU = Array[Index];

            VkPhysicalDeviceProperties Properties = {0};
            vkGetPhysicalDeviceProperties(GPU, &Properties);

            if (Properties.apiVersion < trVulkan.VersionOfAPI)
                continue;

            if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                if (!Preferred) Preferred = GPU;
            }
            else
            {
                if (!Fallback) Fallback = GPU;
            }
        }

        trVulkan.PhysicalDevice = (Preferred) ? (Preferred) : (Fallback);
        TR_ASSERT(trVulkan.PhysicalDevice);

        trSetArenaPosition(Arena, ResetArenaTo);
    }

    // NOTE(vak): Queue family
    {
        tr_arena* Arena = trGetTemporaryArena();
        tr_usize ResetArenaTo = trGetArenaPosition(Arena);

        tr_u32 Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(trVulkan.PhysicalDevice, &Count, 0);

        VkQueueFamilyProperties* Array = trPushArenaArrayZero(Arena, VkQueueFamilyProperties, Count);
        vkGetPhysicalDeviceQueueFamilyProperties(trVulkan.PhysicalDevice, &Count, Array);

        trVulkan.QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        for (tr_u32 Index = 0; Index < Count; Index++)
        {
            VkQueueFamilyProperties* Properties = Array + Index;

            VkQueueFlags DesiredFlags =
                VK_QUEUE_GRAPHICS_BIT |
                VK_QUEUE_COMPUTE_BIT |
                VK_QUEUE_TRANSFER_BIT;

            if ((Properties->queueFlags & DesiredFlags) == DesiredFlags)
            {
                trVulkan.QueueFamilyIndex = Index;
                break;
            }
        }

        TR_ASSERT(trVulkan.QueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED);

        trSetArenaPosition(Arena, ResetArenaTo);
    }

    // NOTE(vak): Device
    {
        const char* Extensions[] =
        {
            "VK_KHR_swapchain",
        };

        VkPhysicalDeviceVulkan14Features Vulkan14Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pushDescriptor = VK_TRUE,
        };

        VkPhysicalDeviceVulkan13Features Vulkan13Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &Vulkan14Features,
            .dynamicRendering = VK_TRUE,
        };

        VkDeviceCreateInfo DeviceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &Vulkan13Features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &(VkDeviceQueueCreateInfo)
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = trVulkan.QueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = (float[]){1.0f},
            },
            .enabledExtensionCount = TR_ARRAY_COUNT(Extensions),
            .ppEnabledExtensionNames = Extensions,
        };

        TR_VK_CHECK(vkCreateDevice(trVulkan.PhysicalDevice, &DeviceInfo, 0, &trVulkan.Device));
    }

    // NOTE(vak): Queue
    {
        vkGetDeviceQueue(trVulkan.Device, trVulkan.QueueFamilyIndex, 0, &trVulkan.Queue);
    }

    // NOTE(vak): Command pool
    {
        VkCommandPoolCreateInfo PoolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = trVulkan.QueueFamilyIndex,
        };

        TR_VK_CHECK(vkCreateCommandPool(trVulkan.Device, &PoolInfo, 0, &trVulkan.CommandPool));
    }

    // NOTE(vak): Command buffer
    {
        VkCommandBufferAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = trVulkan.CommandPool,
            .commandBufferCount = 1,
        };

        TR_VK_CHECK(vkAllocateCommandBuffers(trVulkan.Device, &AllocateInfo, &trVulkan.CommandBuffer));
    }

    // NOTE(vak): Semaphores
    {
        VkSemaphoreCreateInfo SemaphoreInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        TR_VK_CHECK(vkCreateSemaphore(trVulkan.Device, &SemaphoreInfo, 0, &trVulkan.AcquireSemaphore));
        TR_VK_CHECK(vkCreateSemaphore(trVulkan.Device, &SemaphoreInfo, 0, &trVulkan.SubmitSemaphore));
    }

    // NOTE(vak): Swapchain format
    {
        tr_arena* Arena = trGetTemporaryArena();
        tr_usize ResetArenaTo = trGetArenaPosition(Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, 0));

        VkSurfaceFormatKHR* Array = trPushArenaArrayZero(Arena, VkSurfaceFormatKHR, Count);
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, Array));

        trVulkan.SwapchainFormat.format = VK_FORMAT_UNDEFINED;

        for (tr_u32 Index = 0; Index < Count; Index++)
        {
            VkSurfaceFormatKHR Format = Array[Index];

            if (Format.format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                trVulkan.SwapchainFormat = Format;
                break;
            }
            else if (Format.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                trVulkan.SwapchainFormat = Format;
                break;
            }
        }

        TR_ASSERT(trVulkan.SwapchainFormat.format != VK_FORMAT_UNDEFINED);

        trSetArenaPosition(Arena, ResetArenaTo);
    }

    // NOTE(vak): Present mode
    {
        tr_arena* Arena = trGetTemporaryArena();
        tr_usize ResetArenaTo = trGetArenaPosition(Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, 0));

        VkPresentModeKHR* Array = trPushArenaArrayZero(Arena, VkPresentModeKHR, Count);
        TR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, Array));

        tr_b32 HasMailbox = TR_FALSE;
        tr_b32 HasImmediate = TR_FALSE;

        trVulkan.PresentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (tr_u32 Index = 0; Index < Count; Index++)
        {
            VkPresentModeKHR PresentMode = Array[Index];

            HasMailbox |= (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR);
            HasImmediate |= (PresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR);
        }

        if (HasMailbox)
            trVulkan.PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (HasImmediate)
            trVulkan.PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

        trSetArenaPosition(Arena, ResetArenaTo);
    }

    // NOTE(vak): Allocate swapchain image arrays
    {
        VkSurfaceCapabilitiesKHR SurfaceCaps = {0};
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &SurfaceCaps));

        trVulkan.MaxImageCount = SurfaceCaps.maxImageCount;

        tr_arena* Arena = trGetPermanentArena();

        trVulkan.SwapchainImages = trPushArenaArrayZero(Arena, VkImage, trVulkan.MaxImageCount);
        trVulkan.SwapchainImageViews = trPushArenaArrayZero(Arena, VkImageView, trVulkan.MaxImageCount);
    }

    // NOTE(vak): Nearest sampler
    {
        VkSamplerCreateInfo SamplerInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        };

        TR_VK_CHECK(vkCreateSampler(trVulkan.Device, &SamplerInfo, 0, &trVulkan.NearestSampler));
    }

    // NOTE(vak): Buffer set layout
    {
        VkDescriptorSetLayoutBinding Bindings[] =
        {
            // NOTE(vak): Vertex buffer
            {
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
        };

        VkDescriptorSetLayoutCreateInfo SetLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
            .bindingCount = TR_ARRAY_COUNT(Bindings),
            .pBindings = Bindings,
        };

        TR_VK_CHECK(vkCreateDescriptorSetLayout(trVulkan.Device, &SetLayoutInfo, 0, &trVulkan.BufferSetLayout));
    }

    // NOTE(vak): Buffers
    {
        trVulkanMakeBuffer(
            &trVulkan.VertexBuffer,
            TR_MB(16),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            TR_TRUE // NOTE(vak): Mapped
        );

        trVulkanMakeBuffer(
            &trVulkan.TransferBuffer,
            TR_MB(16),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            TR_TRUE // NOTE(vak): Mapped
        );
    }

    // NOTE(vak): Textures
    {
        trVulkan.MaxTextureCount = 512;

        tr_arena* Arena = trGetPermanentArena();

        trVulkan.Textures = trPushArenaArrayZero(
            Arena,
            tr_vulkan_texture,
            trVulkan.MaxTextureCount
        );
    }

    // NOTE(vak): Texture descriptor pool
    {
        VkDescriptorPoolCreateInfo PoolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &(VkDescriptorPoolSize)
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = trVulkan.MaxTextureCount,
            },
        };

        TR_VK_CHECK(vkCreateDescriptorPool(trVulkan.Device, &PoolInfo, 0, &trVulkan.TexturePool));
    }

    // NOTE(vak): Texture set layout
    {
        VkDescriptorSetLayoutBinding Bindings[] =
        {
            // NOTE(vak): Textures
            {
                .binding = 0,
                .descriptorCount = trVulkan.MaxTextureCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
        };

        VkDescriptorSetLayoutCreateInfo SetLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = TR_ARRAY_COUNT(Bindings),
            .pBindings = Bindings,
        };

        TR_VK_CHECK(vkCreateDescriptorSetLayout(trVulkan.Device, &SetLayoutInfo, 0, &trVulkan.TextureSetLayout));
    }

    // NOTE(vak): Texture descriptor set
    {
        VkDescriptorSetAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = trVulkan.TexturePool,
            .descriptorSetCount = 1,
            .pSetLayouts = &trVulkan.TextureSetLayout,
        };

        TR_VK_CHECK(vkAllocateDescriptorSets(trVulkan.Device, &AllocateInfo, &trVulkan.TextureSet));
    }

    // NOTE(vak): Pipeline layout
    {
        VkDescriptorSetLayout SetLayouts[2] =
        {
            [0] = trVulkan.BufferSetLayout,
            [1] = trVulkan.TextureSetLayout,
        };

        VkPipelineLayoutCreateInfo LayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = TR_ARRAY_COUNT(SetLayouts),
            .pSetLayouts = SetLayouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &(VkPushConstantRange)
            {
                // NOTE(vak): Projection matrix
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(tr_vulkan_push_constants),
            },
        };

        TR_VK_CHECK(vkCreatePipelineLayout(trVulkan.Device, &LayoutInfo, 0, &trVulkan.PipelineLayout));
    }

    // NOTE(vak): Pipeline
    {
        static tr_u32 VertexCode[] =
        {
            // NOTE(vak): SPIRV compiled from vulkan_default.vert
            0x07230203,0x00010600,0x0008000b,0x00000057,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x000b000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000f,0x00000013,0x0000003b,
            0x0000003f,0x0000004e,0x00000055,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,
            0x6e69616d,0x00000000,0x00040005,0x00000008,0x74726576,0x00007865,0x00040006,0x00000008,
            0x00000000,0x00000058,0x00040006,0x00000008,0x00000001,0x00000059,0x00040006,0x00000008,
            0x00000002,0x00000055,0x00040006,0x00000008,0x00000003,0x00000056,0x00060006,0x00000008,
            0x00000004,0x6b636150,0x47526465,0x00004142,0x00070006,0x00000008,0x00000005,0x74786554,
            0x49657275,0x7865646e,0x00000000,0x00030005,0x0000000a,0x00000056,0x00040005,0x0000000b,
            0x74726576,0x00007865,0x00040006,0x0000000b,0x00000000,0x00000058,0x00040006,0x0000000b,
            0x00000001,0x00000059,0x00040006,0x0000000b,0x00000002,0x00000055,0x00040006,0x0000000b,
            0x00000003,0x00000056,0x00060006,0x0000000b,0x00000004,0x6b636150,0x47526465,0x00004142,
            0x00070006,0x0000000b,0x00000005,0x74786554,0x49657275,0x7865646e,0x00000000,0x00060005,
            0x0000000d,0x74726556,0x75427865,0x72656666,0x00000000,0x00060006,0x0000000d,0x00000000,
            0x74726556,0x73656369,0x00000000,0x00030005,0x0000000f,0x00000000,0x00060005,0x00000013,
            0x565f6c67,0x65747265,0x646e4978,0x00007865,0x00050005,0x0000001b,0x69736f50,0x6e6f6974,
            0x00000000,0x00050005,0x00000023,0x43786554,0x64726f6f,0x00000000,0x00040005,0x0000002d,
            0x6f6c6f43,0x00000072,0x00060005,0x00000033,0x74786554,0x49657275,0x7865646e,0x00000000,
            0x00060005,0x00000039,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000039,
            0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x00000039,0x00000001,0x505f6c67,
            0x746e696f,0x657a6953,0x00000000,0x00070006,0x00000039,0x00000002,0x435f6c67,0x4470696c,
            0x61747369,0x0065636e,0x00070006,0x00000039,0x00000003,0x435f6c67,0x446c6c75,0x61747369,
            0x0065636e,0x00030005,0x0000003b,0x00000000,0x00060005,0x0000003d,0x68737550,0x736e6f43,
            0x746e6174,0x00000073,0x00060006,0x0000003d,0x00000000,0x6a6f7250,0x69746365,0x00006e6f,
            0x00030005,0x0000003f,0x00000000,0x00070005,0x0000004c,0x74726556,0x68537865,0x72656461,
            0x79726156,0x00676e69,0x00060006,0x0000004c,0x00000000,0x43786554,0x64726f6f,0x00000000,
            0x00050006,0x0000004c,0x00000001,0x6f6c6f43,0x00000072,0x00030005,0x0000004e,0x0074754f,
            0x00060005,0x00000055,0x5474754f,0x75747865,0x6e496572,0x00786564,0x00050048,0x0000000b,
            0x00000000,0x00000023,0x00000000,0x00050048,0x0000000b,0x00000001,0x00000023,0x00000004,
            0x00050048,0x0000000b,0x00000002,0x00000023,0x00000008,0x00050048,0x0000000b,0x00000003,
            0x00000023,0x0000000c,0x00050048,0x0000000b,0x00000004,0x00000023,0x00000010,0x00050048,
            0x0000000b,0x00000005,0x00000023,0x00000014,0x00040047,0x0000000c,0x00000006,0x00000018,
            0x00030047,0x0000000d,0x00000002,0x00040048,0x0000000d,0x00000000,0x00000018,0x00050048,
            0x0000000d,0x00000000,0x00000023,0x00000000,0x00030047,0x0000000f,0x00000018,0x00040047,
            0x0000000f,0x00000021,0x00000000,0x00040047,0x0000000f,0x00000022,0x00000000,0x00040047,
            0x00000013,0x0000000b,0x0000002a,0x00030047,0x00000039,0x00000002,0x00050048,0x00000039,
            0x00000000,0x0000000b,0x00000000,0x00050048,0x00000039,0x00000001,0x0000000b,0x00000001,
            0x00050048,0x00000039,0x00000002,0x0000000b,0x00000003,0x00050048,0x00000039,0x00000003,
            0x0000000b,0x00000004,0x00030047,0x0000003d,0x00000002,0x00040048,0x0000003d,0x00000000,
            0x00000005,0x00050048,0x0000003d,0x00000000,0x00000007,0x00000010,0x00050048,0x0000003d,
            0x00000000,0x00000023,0x00000000,0x00030047,0x0000004c,0x00000002,0x00040047,0x0000004e,
            0x0000001e,0x00000001,0x00030047,0x00000055,0x0000000e,0x00040047,0x00000055,0x0000001e,
            0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
            0x00000020,0x00040015,0x00000007,0x00000020,0x00000000,0x0008001e,0x00000008,0x00000006,
            0x00000006,0x00000006,0x00000006,0x00000007,0x00000007,0x00040020,0x00000009,0x00000007,
            0x00000008,0x0008001e,0x0000000b,0x00000006,0x00000006,0x00000006,0x00000006,0x00000007,
            0x00000007,0x0003001d,0x0000000c,0x0000000b,0x0003001e,0x0000000d,0x0000000c,0x00040020,
            0x0000000e,0x0000000c,0x0000000d,0x0004003b,0x0000000e,0x0000000f,0x0000000c,0x00040015,
            0x00000010,0x00000020,0x00000001,0x0004002b,0x00000010,0x00000011,0x00000000,0x00040020,
            0x00000012,0x00000001,0x00000010,0x0004003b,0x00000012,0x00000013,0x00000001,0x00040020,
            0x00000015,0x0000000c,0x0000000b,0x00040017,0x00000019,0x00000006,0x00000002,0x00040020,
            0x0000001a,0x00000007,0x00000019,0x00040020,0x0000001c,0x00000007,0x00000006,0x0004002b,
            0x00000010,0x0000001f,0x00000001,0x0004002b,0x00000010,0x00000024,0x00000002,0x0004002b,
            0x00000010,0x00000027,0x00000003,0x00040017,0x0000002b,0x00000006,0x00000004,0x00040020,
            0x0000002c,0x00000007,0x0000002b,0x0004002b,0x00000010,0x0000002e,0x00000004,0x00040020,
            0x0000002f,0x00000007,0x00000007,0x0004002b,0x00000010,0x00000034,0x00000005,0x0004002b,
            0x00000007,0x00000037,0x00000001,0x0004001c,0x00000038,0x00000006,0x00000037,0x0006001e,
            0x00000039,0x0000002b,0x00000006,0x00000038,0x00000038,0x00040020,0x0000003a,0x00000003,
            0x00000039,0x0004003b,0x0000003a,0x0000003b,0x00000003,0x00040018,0x0000003c,0x0000002b,
            0x00000004,0x0003001e,0x0000003d,0x0000003c,0x00040020,0x0000003e,0x00000009,0x0000003d,
            0x0004003b,0x0000003e,0x0000003f,0x00000009,0x00040020,0x00000040,0x00000009,0x0000003c,
            0x0004002b,0x00000006,0x00000044,0x00000000,0x0004002b,0x00000006,0x00000045,0x3f800000,
            0x00040020,0x0000004a,0x00000003,0x0000002b,0x0004001e,0x0000004c,0x00000019,0x0000002b,
            0x00040020,0x0000004d,0x00000003,0x0000004c,0x0004003b,0x0000004d,0x0000004e,0x00000003,
            0x00040020,0x00000050,0x00000003,0x00000019,0x00040020,0x00000054,0x00000003,0x00000007,
            0x0004003b,0x00000054,0x00000055,0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,
            0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000009,0x0000000a,0x00000007,0x0004003b,
            0x0000001a,0x0000001b,0x00000007,0x0004003b,0x0000001a,0x00000023,0x00000007,0x0004003b,
            0x0000002c,0x0000002d,0x00000007,0x0004003b,0x0000002f,0x00000033,0x00000007,0x0004003d,
            0x00000010,0x00000014,0x00000013,0x00060041,0x00000015,0x00000016,0x0000000f,0x00000011,
            0x00000014,0x0004003d,0x0000000b,0x00000017,0x00000016,0x00040190,0x00000008,0x00000018,
            0x00000017,0x0003003e,0x0000000a,0x00000018,0x00050041,0x0000001c,0x0000001d,0x0000000a,
            0x00000011,0x0004003d,0x00000006,0x0000001e,0x0000001d,0x00050041,0x0000001c,0x00000020,
            0x0000000a,0x0000001f,0x0004003d,0x00000006,0x00000021,0x00000020,0x00050050,0x00000019,
            0x00000022,0x0000001e,0x00000021,0x0003003e,0x0000001b,0x00000022,0x00050041,0x0000001c,
            0x00000025,0x0000000a,0x00000024,0x0004003d,0x00000006,0x00000026,0x00000025,0x00050041,
            0x0000001c,0x00000028,0x0000000a,0x00000027,0x0004003d,0x00000006,0x00000029,0x00000028,
            0x00050050,0x00000019,0x0000002a,0x00000026,0x00000029,0x0003003e,0x00000023,0x0000002a,
            0x00050041,0x0000002f,0x00000030,0x0000000a,0x0000002e,0x0004003d,0x00000007,0x00000031,
            0x00000030,0x0006000c,0x0000002b,0x00000032,0x00000001,0x00000040,0x00000031,0x0003003e,
            0x0000002d,0x00000032,0x00050041,0x0000002f,0x00000035,0x0000000a,0x00000034,0x0004003d,
            0x00000007,0x00000036,0x00000035,0x0003003e,0x00000033,0x00000036,0x00050041,0x00000040,
            0x00000041,0x0000003f,0x00000011,0x0004003d,0x0000003c,0x00000042,0x00000041,0x0004003d,
            0x00000019,0x00000043,0x0000001b,0x00050051,0x00000006,0x00000046,0x00000043,0x00000000,
            0x00050051,0x00000006,0x00000047,0x00000043,0x00000001,0x00070050,0x0000002b,0x00000048,
            0x00000046,0x00000047,0x00000044,0x00000045,0x00050091,0x0000002b,0x00000049,0x00000042,
            0x00000048,0x00050041,0x0000004a,0x0000004b,0x0000003b,0x00000011,0x0003003e,0x0000004b,
            0x00000049,0x0004003d,0x00000019,0x0000004f,0x00000023,0x00050041,0x00000050,0x00000051,
            0x0000004e,0x00000011,0x0003003e,0x00000051,0x0000004f,0x0004003d,0x0000002b,0x00000052,
            0x0000002d,0x00050041,0x0000004a,0x00000053,0x0000004e,0x0000001f,0x0003003e,0x00000053,
            0x00000052,0x0004003d,0x00000007,0x00000056,0x00000033,0x0003003e,0x00000055,0x00000056,
            0x000100fd,0x00010038
        };

        static tr_u32 FragmentCode[] =
        {
            // NOTE(vak): SPIRV compiled from vulkan_default.frag
            0x07230203,0x00010600,0x0008000b,0x00000029,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0009000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000010,0x00000012,0x0000001a,
            0x00000022,0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,
            0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000009,0x65786554,0x0000006c,0x00050005,
            0x00000010,0x74786554,0x73657275,0x00000000,0x00060005,0x00000012,0x65546e49,0x72757478,
            0x646e4965,0x00007865,0x00070005,0x00000018,0x74726556,0x68537865,0x72656461,0x79726156,
            0x00676e69,0x00060006,0x00000018,0x00000000,0x43786554,0x64726f6f,0x00000000,0x00050006,
            0x00000018,0x00000001,0x6f6c6f43,0x00000072,0x00030005,0x0000001a,0x00006e49,0x00060005,
            0x00000022,0x67617246,0x746e656d,0x6f6c6f43,0x00000072,0x00040047,0x00000010,0x00000021,
            0x00000000,0x00040047,0x00000010,0x00000022,0x00000001,0x00030047,0x00000012,0x0000000e,
            0x00040047,0x00000012,0x0000001e,0x00000000,0x00030047,0x00000018,0x00000002,0x00040047,
            0x0000001a,0x0000001e,0x00000001,0x00040047,0x00000022,0x0000001e,0x00000000,0x00020013,
            0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
            0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000007,0x00000007,0x00090019,
            0x0000000a,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,
            0x0003001b,0x0000000b,0x0000000a,0x00040015,0x0000000c,0x00000020,0x00000000,0x0004002b,
            0x0000000c,0x0000000d,0x00000200,0x0004001c,0x0000000e,0x0000000b,0x0000000d,0x00040020,
            0x0000000f,0x00000000,0x0000000e,0x0004003b,0x0000000f,0x00000010,0x00000000,0x00040020,
            0x00000011,0x00000001,0x0000000c,0x0004003b,0x00000011,0x00000012,0x00000001,0x00040020,
            0x00000014,0x00000000,0x0000000b,0x00040017,0x00000017,0x00000006,0x00000002,0x0004001e,
            0x00000018,0x00000017,0x00000007,0x00040020,0x00000019,0x00000001,0x00000018,0x0004003b,
            0x00000019,0x0000001a,0x00000001,0x00040015,0x0000001b,0x00000020,0x00000001,0x0004002b,
            0x0000001b,0x0000001c,0x00000000,0x00040020,0x0000001d,0x00000001,0x00000017,0x00040020,
            0x00000021,0x00000003,0x00000007,0x0004003b,0x00000021,0x00000022,0x00000003,0x0004002b,
            0x0000001b,0x00000024,0x00000001,0x00040020,0x00000025,0x00000001,0x00000007,0x00050036,
            0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,
            0x00000009,0x00000007,0x0004003d,0x0000000c,0x00000013,0x00000012,0x00050041,0x00000014,
            0x00000015,0x00000010,0x00000013,0x0004003d,0x0000000b,0x00000016,0x00000015,0x00050041,
            0x0000001d,0x0000001e,0x0000001a,0x0000001c,0x0004003d,0x00000017,0x0000001f,0x0000001e,
            0x00050057,0x00000007,0x00000020,0x00000016,0x0000001f,0x0003003e,0x00000009,0x00000020,
            0x0004003d,0x00000007,0x00000023,0x00000009,0x00050041,0x00000025,0x00000026,0x0000001a,
            0x00000024,0x0004003d,0x00000007,0x00000027,0x00000026,0x00050085,0x00000007,0x00000028,
            0x00000023,0x00000027,0x0003003e,0x00000022,0x00000028,0x000100fd,0x00010038
        };

        VkShaderModuleCreateInfo VertexModuleInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = VertexCode,
            .codeSize = sizeof(VertexCode),
        };

        VkShaderModuleCreateInfo FragmentModuleInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = FragmentCode,
            .codeSize = sizeof(FragmentCode),
        };

        VkShaderModule VertexModule = {0};
        VkShaderModule FragmentModule = {0};

        TR_VK_CHECK(vkCreateShaderModule(trVulkan.Device, &VertexModuleInfo, 0, &VertexModule));
        TR_VK_CHECK(vkCreateShaderModule(trVulkan.Device, &FragmentModuleInfo, 0, &FragmentModule));

        VkPipelineShaderStageCreateInfo StageInfos[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .module = VertexModule,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .module = FragmentModule,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pName = "main",
            },
        };

        VkPipelineRenderingCreateInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &trVulkan.SwapchainFormat.format,
        };

        VkPipelineVertexInputStateCreateInfo VertexInputState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        };

        VkPipelineTessellationStateCreateInfo TessellationState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        };

        VkPipelineViewportStateCreateInfo ViewportState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &(VkViewport){0},
            .scissorCount = 1,
            .pScissors = &(VkRect2D){0},
        };

        VkPipelineRasterizationStateCreateInfo RasterizationState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo MultisampleState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        VkPipelineDepthStencilStateCreateInfo DepthStencilState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        };

        VkPipelineColorBlendStateCreateInfo ColorBlendState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &(VkPipelineColorBlendAttachmentState)
            {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT,
            },
        };

        VkDynamicState DynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo DynamicState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = TR_ARRAY_COUNT(DynamicStates),
            .pDynamicStates = DynamicStates,
        };

        VkGraphicsPipelineCreateInfo PipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &RenderingInfo,
            .stageCount = TR_ARRAY_COUNT(StageInfos),
            .pStages = StageInfos,
            .pVertexInputState = &VertexInputState,
            .pInputAssemblyState = &InputAssemblyState,
            .pTessellationState = &TessellationState,
            .pViewportState = &ViewportState,
            .pRasterizationState = &RasterizationState,
            .pMultisampleState = &MultisampleState,
            .pDepthStencilState = &DepthStencilState,
            .pColorBlendState = &ColorBlendState,
            .pDynamicState = &DynamicState,
            .layout = trVulkan.PipelineLayout,
        };

        TR_VK_CHECK(vkCreateGraphicsPipelines(
            trVulkan.Device,
            0,
            1,
            &PipelineInfo,
            0,
            &trVulkan.Pipeline
        ));

        vkDestroyShaderModule(trVulkan.Device, FragmentModule, 0);
        vkDestroyShaderModule(trVulkan.Device, VertexModule, 0);
    }

    // NOTE(vak): Default texture
    {
        tr_u32 WhiteImage[2 * 2] =
        {
            0xFFFFFFFF, 0xFFFFFFFF, 
            0xFFFFFFFF, 0xFFFFFFFF, 
        };

        trVulkan.DefaultTexture = trMakeTexture(2, 2, TR_TEXTURE_FORMAT_RGBA);

        // NOTE(vak): Texture index 0 should always be the default texture
        TR_ASSERT(trVulkan.DefaultTexture == TR_DEFAULT_TEXTURE);

        trUploadTexture(trVulkan.DefaultTexture, WhiteImage);

        // NOTE(vak): Initialize every texture descriptor to
        // refer to the default texture

        tr_vulkan_texture* DefaultTexture = &trVulkan.Textures[0];

        for (tr_u32 Index = 1; Index < trVulkan.MaxTextureCount; Index++)
        {
            VkWriteDescriptorSet DescriptorWrite =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = trVulkan.TextureSet,
                .dstBinding = 0,
                .dstArrayElement = Index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &(VkDescriptorImageInfo)
                {
                    .sampler = trVulkan.NearestSampler,
                    .imageView = DefaultTexture->View,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            };

            vkUpdateDescriptorSets(
                trVulkan.Device,
                1, &DescriptorWrite,
                0, 0
            );
        }
    }

    // NOTE(vak): Proggy Clean 32px font texture (ProggyClean.ttf)
    {
        // NOTE(vak):
        // 241 x 199 Font Atlas
        // Each cell is 15x33
        // Stray one pixel padding at the left and top edge of the atlas

        tr_u32 Width = 241;
        tr_u32 Height = 199;

        // Source: https://github.com/bluescan/proggyfonts
        // Font atlas generator used: https://bernhardfritz.github.io/monospace-font-atlas-generator/
        // The original .png was converted to raw pixels using ffmpeg, and
        // then packked into bits by some python magic

        static tr_u64 PackedBitmap[750] =
        {
            // NOTE(vak): 241 x 199 bitmap font packed into bits
            // Each bit represents a pixel's brightness
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000066000, 0x00c018000c000000, 
            0x0000000000000000, 0xc000000000c00000, 0x000000000000000c, 0x0000018030001800, 
            0x0000000000000000, 0x0019800c00000180, 0x3000780c0c018033, 0x000000000c001800, 
            0x0300000000000000, 0x0066003300180000, 0x30006000f0181803, 0x0000000000001800, 
            0x0000060000000000, 0xcc7f80cc00660030, 0x30006000c006180c, 0x0000000000000000, 
            0x0060000003000000, 0x301998ff019800cc, 0x00006000c001800c, 0x0000000000000000, 
            0x000000c000000600, 0x0018603330198fff, 0x3000600300006000, 0x0c00000000000000, 
            0x1ffe000001800000, 0xc0000030c0666033, 0x00006000c0060000, 0x0000180000000000, 
            0x0066033000000300, 0x00018000061e0033, 0x00000000c019980c, 0x060000000c000000, 
            0x006600cc06600000, 0x3018000300000c3c, 0x0000000000018033, 0x00000c0000001800, 
            0x19860cc007e00cc0, 0xf01f803000060000, 0x3000000ffc00003f, 0x1980000018000000, 
            0x0000330c19800fc0, 0x007fe03f0060000c, 0x00006000001ff800, 0x6600fff000003000, 
            0x001800001818ccc0, 0x0000000c019980c0, 0x6000000030000000, 0x9980cc01ffe00000, 
            0x0180003000003031, 0x0000000000180333, 0x0000000000006000, 0x6063330198003300, 
            0x0060030000600000, 0xc001800000060030, 0x6600000000000000, 0x0000c0c666033000, 
            0x006000c0060000c0, 0x000180030000000c, 0xfe00cc0000030000, 0x060000067e030301, 
            0x0018000000000300, 0x00000000c0060000, 0x0603fc0198000006, 0x06000c00000cfc06, 
            0x0000003000000000, 0x000000000001800c, 0x0000000180000000, 0x00000c0018000000, 
            0x0000000000600000, 0x0000000000000003, 0x0000000000030000, 0x0000000018003000, 
            0x00060000000000c0, 0x0000000000000000, 0x8000000000000000, 0x0060000000000c01, 
            0x0000000000000000, 0x0000000000000000, 0x1803000000000000, 0x000000c000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x001800fc00000000, 0xc01ff830001f803f, 0x00007e00fc07fe03, 
            0x0000000000000000, 0x007e003001f801f8, 0xfc07803ff060003f, 0x00000000fc01f80f, 
            0x03f0000000000000, 0x0181830300780c0c, 0x0c180000c00060f0, 0x000000000006060c, 
            0x1818181800000000, 0xc1e00303060600f0, 0x0c18183000018000, 0x000000000000000c, 
            0x0198303030300000, 0xc001833006000c00, 0xc018183030180000, 0x03c00003c0001800, 
            0x1800033060606000, 0x0001800306600c00, 0x3001803030606030, 0xc000078000078000, 
            0x07800c000600ccc0, 0x006000ff01fe0c30, 0xf00060030060603f, 0x99806000f01ffe00, 
            0x18600f0018000c01, 0xc07e00c001fe03fc, 0xfc01e000c00600c0, 0x18033300c001e03f, 
            0x1800303060000c00, 0x01fe030300600c0c, 0x000000003c000000, 0x180030066600603c, 
            0x181830006060c000, 0x000003fc060600c0, 0xc078000000007800, 0x80000c00600c0c00, 
            0x018030306003ffc1, 0x0000000006000c0c, 0x1801800f01ffe00f, 0xff8300001800c018, 
            0x181803006060c007, 0xc01e000000000c00, 0x80303003001e03ff, 0x8183000606000c01, 
            0x060030300180c0c1, 0xc00003c0001800c0, 0x1803006060000003, 0x81830306000c0c00, 
            0x01800c0060600301, 0x0007800007800030, 0xe03ff07fe03f0000, 0x0600fc01f80c0007, 
            0x0060030007803f00, 0x0018000000000000, 0x000fc07fe0ffc07e, 0x7e000c01f803f018, 
            0x000000c006000f00, 0x0000003000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000001800000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000300, 0x0000000000000000, 0x0000000000000000, 0x0180000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000030000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x01f8000000000000, 0xf01fe0fc007f80f0, 0xfc18060fc01ff83f, 
            0xf03c006300c07e00, 0x01e003f003c06078, 0xf07fe03fc1f800ff, 0xfc01f8300c1f803f, 
            0xc0f1e07800c60180, 0x060603c018180780, 0xc0006000c1818c0c, 0x03018000c06018c0, 
            0x30c181e3c0f00183, 0x18180c0c07803030, 0x31818000c0018303, 0x03060603000180c0, 
            0x9e18618303c781e0, 0x1806000c181830c1, 0x01806000c0018003, 0xccc006030c060003, 
            0x61833c3300c6198c, 0x0006300c00183030, 0x00060300c0018003, 0x331999800c06180c, 
            0x7fe0c3066666018c, 0x01fe03fc60180030, 0x3018000c07ff8003, 0x0318663333001803, 
            0x0060ffc1860ccccc, 0x000603fc07f8c030, 0x3006603000180fff, 0x99980630cc666600, 
            0x806000c60183fc19, 0x18063f0c00180031, 0x0c00600fc0600030, 0xf83333300c6618c3, 
            0x006300c0018c0307, 0x0060300c7e180030, 0x31861800c01f80c0, 0x06300c1f866018cc, 
            0x006000c601800318, 0x018000c06018c030, 0x3198630c300180c3, 0x06300c60183f0cc0, 
            0x806000c0018c0300, 0x018603000180c031, 0x61806330c6186003, 0x0630306018c03000, 
            0x8063030001800306, 0xc0060c0c06000301, 0x6000c0c307818c00, 0x060c0c6060c03180, 
            0x060300c606000300, 0x1801800c18180c00, 0xe300c1fe01860f03, 0x060ffc07f83f007f, 
            0x07e03f060183f000, 0x1e0630031ff8c030, 0x00ffc60183fc00f0, 0xe0000c1ff80ff07e, 
            0x80600fc07e0c0307, 0x01e03c0c60063ff1, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x00c007e000301f80, 0x0000000000000000, 0x0000000000000000, 
            0x3f00000000000000, 0x000001800fc00060, 0xf1fe00ff01e003fc, 0x0c6006c00d8063ff, 
            0x00c0060fff1801b0, 0x07f8000003001800, 0xc7ffe3fc01fe03c0, 0x036018c00d801b00, 
            0x300001800c1ffe30, 0x1860303000000600, 0x3601803018060c0c, 0x006006c031861b00, 
            0x330060000c001830, 0x181830c060600000, 0x36006c030060300c, 0x306000c00d80630c, 
            0x00006600c0001800, 0x001830318060c0c0, 0x06186303180600c0, 0x30006030006060c3, 
            0x81800000cc018000, 0x01800030606300c1, 0xc1860c30c606300c, 0x00006000c06000c0, 
            0x0183030000019803, 0x601803000780c0c6, 0x006600f019998c0c, 0x0c06000300018030, 
            0x818c03060600000c, 0x1818c03006000f01, 0x006000cc01e03333, 0x0018180c00060003, 
            0xe000ff180603fc00, 0x66660cc180600c01, 0x00060030006003c0, 0xf80000303018000c, 
            0x1803c001fe300c07, 0x0780cccc198300c0, 0x0018000c006000c0, 0x1800300000606030, 
            0x0180301800030c60, 0x018030c079e03306, 0x006000c000180030, 0x18c0300060000000, 
            0x660c030060300006, 0x006003006180f3c0, 0x000000c001800030, 0x183030618000c000, 
            0x818030180600c060, 0x00600030060300c1, 0x8000000001800300, 0x80c0306060c30001, 
            0x0183030060300c01, 0x060000c000600c06, 0x0003000000000300, 0xe003007f8300c678, 
            0x180c03060600c01f, 0x060030000183ffc0, 0x8cf0000600000000, 0x803fc00600ff0601, 
            0xff803018060c0c01, 0x00000c0060000307, 0x0000180000000000, 0x0000000000000000, 
            0x0600000000000000, 0x7ffe00001800c000, 0x0000000030000000, 0x0000000000000000, 
            0x80000c0000000000, 0x0000fffc00003001, 0x0000000000000000, 0x0000000000000000, 
            0x7e000001f8000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000fc000003f000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0018000000000000, 0x0060000000018000, 
            0x30000600001f8000, 0x0000078000c06000, 0x0000003000000000, 0x000000c000000003, 
            0xc00060000c00003f, 0x000000000f000180, 0x0006000001800000, 0x0001800001800000, 
            0x0300000000001800, 0x0000000000001800, 0x0000000c00000300, 0x3000000300000300, 
            0x3000060000000000, 0x0000000000000000, 0x0600000000180000, 0x0000600000060000, 
            0x000060000c000000, 0x0000000000000000, 0x00000c0000000030, 0x00000000c000000c, 
            0x00000000c0001800, 0x1fe03f0000000000, 0x01fe03f01fe00fc0, 0x301e000f007f83fc, 
            0xfc01fe0f3f018030, 0x1f803fc07e000000, 0x07f803fc07e03fc0, 0x0060603c001e00ff, 
            0x0001f803fc1e7e03, 0x6060c0c181830000, 0x06060c0c00603030, 0x0c060030c0600030, 
            0x0000000c0c1818c3, 0x6060c0c181830306, 0x00600c0c181800c0, 0x3186180c006180c0, 
            0x060ff00000181830, 0x0180ffc181800306, 0x018000c018183030, 0x3060630c30180033, 
            0x060c0c1fe0000030, 0x60600301ff830300, 0x0066030001803030, 0x006060c0c6186030, 
            0x06000c1818303000, 0x6060c0c006000306, 0xc06000fc06000300, 0x600000c0c1818c30, 
            0x060c0c0018303060, 0x0600c0c181800c00, 0x186180c001f80c00, 0x60c0c00001818303, 
            0x180c0c1818303060, 0x18000c0181830300, 0x060630c301800c30, 0x60c0c18180000303, 
            0x0600301818303060, 0x1860300018030306, 0x06060c0c61860300, 0x803f007f83fc0000, 
            0x060ff000600fc07f, 0x0600c0c060003006, 0x000003f01818c30c, 0x80ff007e00ff07f8, 
            0x600c0c1fe000c01f, 0x86180c018180c000, 0x0000000007e03031, 0x0000000000000000, 
            0x8000000000300000, 0x0000000000000001, 0x0000000000000000, 0x0000000000000000, 
            0x0003000000000060, 0x0000000000000000, 0x0000000000000000, 0x00c0000000000000, 
            0x00000001f8000000, 0x0000000000000000, 0x0000000000000000, 0x0000018000000000, 
            0x000000000003f000, 0x0000000000000000, 0x0000000000000000, 0x0000000000fc0000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x00000000000001f8, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 
            0x0000000000000000, 0x0c01e00000000000, 0x00000000000001e0, 0x0000000000000000, 
            0x0000000000000000, 0x03c01803c0000000, 0x0000000000000000, 0x0000000c00000000, 
            0x0000000000000000, 0x0000180030006000, 0x0000000000000000, 0x0000000000180000, 
            0xc000000000000000, 0x0000000030006000, 0x0000000000000000, 0x0000000000000030, 
            0xc001800000000000, 0x0000000000006000, 0x0060000000000000, 0x0000000000000000, 
            0xc001800300000000, 0xfe00ff0000000000, 0x18183fc07f803cc1, 0xc181830318018c0c, 
            0x00018003000600ff, 0x7983fc01fe000000, 0x181830307f80ff00, 0x01ff830306063003, 
            0x000000030006000c, 0x01830f06060c0c00, 0x6186303060600300, 0x0018030006060330, 
            0x180000c3f006000c, 0x060003061e0c0c18, 0x0660c30c6060c0c0, 0x0018003006000c0c, 
            0x183030000187e00c, 0x81800c0078000c18, 0x1818030186183301, 0x31e00030001e0300, 
            0x18303060600000fc, 0x660303001800f000, 0x0600303006030c30, 0x01f863c00060003c, 
            0x0000306060c0c000, 0x6660cc0606003006, 0x0180030060600c06, 0x80000000006000c0, 
            0x600c000060c0c181, 0x180cccc1980c0c00, 0x018003000600c0c0, 0x83030000000000c0, 
            0x1800c0600000c181, 0x8180cc079e00c018, 0x0180030006000301, 0x8303060600000000, 
            0x8030300180c00001, 0x06030301980f3c01, 0x0000030006000c00, 0x800307f803fc0000, 
            0x1803007f80fc007f, 0x1803ff07f80c0c18, 0x0000000006000c00, 0x00ff00060ff007f8, 
            0x1830300600ff01f8, 0x18003007fe0ff018, 0x0030000000000c00, 0x0000000000001800, 
            0x0000000000000000, 0x1800300060000018, 0x3000006000000000, 0x0000000000000000, 
            0x0030000000000000, 0x000030006000c000, 0x0000600000c00000, 0x0000000000000000, 
            0x0000006000000000, 0x000000001e00c01e, 0x00000000c0000180, 0x0000000000000000, 
            0x803c000000c00000, 0x0300000000003c01, 0x0000000000018000, 0x0000000000000000, 
            0x000000000000007e, 0x0000060000000000, 0x0000000000000003, 0x00fc000000000000, 
            0x0000000000000000, 0x0000000000000000, 
       };

        tr_arena* Arena = trGetTemporaryArena();
        tr_usize ResetArenaTo = trGetArenaPosition(Arena);

        tr_u32* UnpackedRGBA = trPushArenaArrayZero(
            Arena,
            tr_u32,
            Width * Height * 4
        );

        for (tr_u32 PixelIndex = 0; PixelIndex < Width*Height; PixelIndex++)
        {
            tr_u32 BitIndex = PixelIndex % 64;
            tr_u32 MaskIndex = PixelIndex / 64;

            tr_u32 Bit = (PackedBitmap[MaskIndex] >> BitIndex) & 1;

            tr_u32 R = Bit * 255;
            tr_u32 G = Bit * 255;
            tr_u32 B = Bit * 255;
            tr_u32 A = Bit * 255;

            UnpackedRGBA[PixelIndex] =
                (R <<  0) |
                (B <<  8) |
                (G << 16) |
                (A << 24);
        }

        trVulkan.FontTexture = trMakeTexture(Width, Height, TR_TEXTURE_FORMAT_RGBA);
        trUploadTexture(trVulkan.FontTexture, UnpackedRGBA);

        trSetArenaPosition(Arena, ResetArenaTo);
    }
}

tr_func tr_texture trMakeTexture(tr_u32 Width, tr_u32 Height, tr_texture_format TextureFormat)
{
    TR_ASSERT(Width > 0);
    TR_ASSERT(Height > 0);
    TR_ASSERT(TextureFormat > 0);
    TR_ASSERT(TextureFormat < TR_TEXTURE_FORMAT_COUNT);

    TR_ASSERT(trVulkan.TextureCount < trVulkan.MaxTextureCount);

    VkFormat ImageFormat = VK_FORMAT_UNDEFINED;

    switch (TextureFormat)
    {
        default: TR_ASSERT(!"Missing texture format to vulkan image format conversion");

        case TR_TEXTURE_FORMAT_RGB:         ImageFormat = VK_FORMAT_R8G8B8_UNORM; break;
        case TR_TEXTURE_FORMAT_RGBA:        ImageFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
        case TR_TEXTURE_FORMAT_RGB_SRGB:    ImageFormat = VK_FORMAT_R8G8B8_SRGB; break;
        case TR_TEXTURE_FORMAT_RGBA_SRGB:   ImageFormat = VK_FORMAT_R8G8B8A8_SRGB; break;
    }

    tr_texture TextureIndex = trVulkan.TextureCount++;
    tr_vulkan_texture* Texture = trVulkan.Textures + TextureIndex;

    trVulkanMakeTexture(
        Texture,
        Width,
        Height,
        ImageFormat,
        VK_IMAGE_USAGE_SAMPLED_BIT|
        VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    VkWriteDescriptorSet DescriptorWrite =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = trVulkan.TextureSet,
        .dstBinding = 0,
        .dstArrayElement = TextureIndex,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &(VkDescriptorImageInfo)
        {
            .sampler = trVulkan.NearestSampler,
            .imageView = Texture->View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
    };

    vkUpdateDescriptorSets(
        trVulkan.Device,
        1, &DescriptorWrite,
        0, 0
    );

    return (TextureIndex);
}

tr_func void trUploadTexture(tr_texture Texture, void* Pixels)
{
    TR_ASSERT(Texture < trVulkan.TextureCount);
    TR_ASSERT(Pixels);

    tr_vulkan_texture* Internal = trVulkan.Textures + Texture;

    // NOTE(vak): The internal image storage is RGBA8, so the transfer buffer
    // pixel data have to be in RGBA8 format. These calculations reflect that.

    tr_usize BytesPerRow = 4 * Internal->Width;
    tr_usize RowsPerTransfer = trVulkan.TransferBuffer.Size / BytesPerRow;
    tr_usize RowsRemaining = Internal->Height;

    TR_ASSERT(RowsPerTransfer > 0);

    while (RowsRemaining)
    {
        tr_usize SourceRow = (Internal->Height - RowsRemaining);
        tr_usize RowsToTransfer = TR_MIN(RowsRemaining, RowsPerTransfer);

        // NOTE(vak): Copy RGBA8 pixel data into transfer buffer before uploading
        // it to the GPU. If the pixel data isn't in RGBA8 format then we have to
        // convert it.

        switch (Internal->Format)
        {
            default: TR_ASSERT(!"Missing RGBA conversion in trUploadTexture (Vulkan backend)"); break;

            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SRGB:
            {
                tr_u8* Dest   = (tr_u8*)trVulkan.TransferBuffer.Mapping;
                tr_u8* Source = (tr_u8*)Pixels + 3*Internal->Width*SourceRow;

                for (tr_u32 Row = 0; Row < RowsToTransfer; Row++)
                {
                    for (tr_u32 X = 0; X < Internal->Width; X++)
                    {
                        Dest[X*4 + 0] = Source[X*3 + 0];
                        Dest[X*4 + 1] = Source[X*3 + 1];
                        Dest[X*4 + 2] = Source[X*3 + 2];
                        Dest[X*4 + 3] = 255;
                    }

                    Dest   += 4*Internal->Width;
                    Source += 3*Internal->Width;
                }
            } break;

            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            {
                trCopyMemory(
                    trVulkan.TransferBuffer.Mapping,
                    (tr_u8*)Pixels + 4*Internal->Width*SourceRow,
                    RowsToTransfer * 4*Internal->Width
                );
            } break;
        }

        TR_VK_CHECK(vkDeviceWaitIdle(trVulkan.Device));

        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        TR_VK_CHECK(vkResetCommandBuffer(trVulkan.CommandBuffer, 0));
        TR_VK_CHECK(vkBeginCommandBuffer(trVulkan.CommandBuffer, &BeginInfo));

        tr_b32 FirstTransfer = (Internal->Height == RowsRemaining);

        if (FirstTransfer)
        {
            VkImageMemoryBarrier TransferBeginBarrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .image = Internal->Image,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                    .levelCount = 1,
                },
            };

            vkCmdPipelineBarrier(
                trVulkan.CommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, 0, 0, 0,
                1, &TransferBeginBarrier
            );
        }

        VkBufferImageCopy CopyRegion =
        {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset =
            {
                .x = 0,
                .y = SourceRow,
                .z = 0
            },
            .imageExtent =
            {
                .width = Internal->Width,
                .height = RowsToTransfer,
                .depth = 1,
            },
        };

        vkCmdCopyBufferToImage(
            trVulkan.CommandBuffer,
            trVulkan.TransferBuffer.Buffer,
            Internal->Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &CopyRegion
        );

        tr_b32 LastTransfer = (RowsRemaining == RowsToTransfer);

        if (LastTransfer)
        {
            VkImageMemoryBarrier TransferEndBarrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .image = Internal->Image,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                    .levelCount = 1,
                },
            };

            vkCmdPipelineBarrier(
                trVulkan.CommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, 0, 0, 0,
                1, &TransferEndBarrier
            );
        }

        TR_VK_CHECK(vkEndCommandBuffer(trVulkan.CommandBuffer));

        VkSubmitInfo SubmitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &trVulkan.CommandBuffer,
        };

        TR_VK_CHECK(vkQueueSubmit(trVulkan.Queue, 1, &SubmitInfo, 0));

        TR_VK_CHECK(vkDeviceWaitIdle(trVulkan.Device));

        RowsRemaining -= RowsToTransfer;
    }
}

tr_func void trVulkanBeginFrame(void)
{
    // NOTE(vak): Resize swapchain if necessary

    tr_u32 WindowW = trGetWindowWidth();
    tr_u32 WindowH = trGetWindowHeight();

    if ((WindowW != trVulkan.SwapchainExtent.width) ||
        (WindowH != trVulkan.SwapchainExtent.height))
    {
        TR_VK_CHECK(vkDeviceWaitIdle(trVulkan.Device));

        if (trVulkan.Swapchain)
        {
            vkDestroySwapchainKHR(trVulkan.Device, trVulkan.Swapchain, 0);

            for (tr_u32 Index = 0; Index < trVulkan.ImageCount; Index++)
                vkDestroyImageView(trVulkan.Device, trVulkan.SwapchainImageViews[Index], 0);
        }

        trVulkan.SwapchainExtent.width = WindowW;
        trVulkan.SwapchainExtent.height = WindowH;

        VkSurfaceCapabilitiesKHR SurfaceCaps = {0};
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &SurfaceCaps));

        tr_u32 MinImageCount = TR_MIN(
            TR_MAX(3, SurfaceCaps.minImageCount),
            SurfaceCaps.maxImageCount
        );

        tr_b32 PremulSupported = (
            SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
        ) != 0;

        tr_b32 PostmulSupported = (
            SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
        ) != 0;

        VkCompositeAlphaFlagsKHR CompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (PostmulSupported) CompositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (PremulSupported) CompositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        VkSwapchainCreateInfoKHR SwapchainInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = trVulkan.Surface,
            .minImageCount = MinImageCount,
            .imageFormat = trVulkan.SwapchainFormat.format,
            .imageColorSpace = trVulkan.SwapchainFormat.colorSpace,
            .imageExtent = trVulkan.SwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = SurfaceCaps.currentTransform,
            .compositeAlpha = CompositeAlpha,
            .presentMode = trVulkan.PresentMode,
            .clipped = VK_TRUE,
        };

        TR_VK_CHECK(vkCreateSwapchainKHR(trVulkan.Device, &SwapchainInfo, 0, &trVulkan.Swapchain));

        trVulkan.ImageCount = trVulkan.MaxImageCount;

        TR_VK_CHECK(vkGetSwapchainImagesKHR(trVulkan.Device, trVulkan.Swapchain, &trVulkan.ImageCount, trVulkan.SwapchainImages));

        for (tr_u32 Index = 0; Index < trVulkan.ImageCount; Index++)
        {
            VkImageViewCreateInfo ViewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = trVulkan.SwapchainImages[Index],
                .format = trVulkan.SwapchainFormat.format,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_R,
                    .g = VK_COMPONENT_SWIZZLE_G,
                    .b = VK_COMPONENT_SWIZZLE_B,
                    .a = VK_COMPONENT_SWIZZLE_A,
                },
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                    .levelCount = 1,
                },
            };

            TR_VK_CHECK(vkCreateImageView(trVulkan.Device, &ViewInfo, 0, &trVulkan.SwapchainImageViews[Index]));
        }

        TR_VK_CHECK(vkDeviceWaitIdle(trVulkan.Device));
    }

    TR_VK_CHECK(vkAcquireNextImageKHR(
        trVulkan.Device,
        trVulkan.Swapchain,
        ~0ull,
        trVulkan.AcquireSemaphore,
        0,
        &trVulkan.ImageIndex
    ));

    trVulkan.VertexCount = 0;
    trVulkan.ClearColor[0] = 0.0f;
    trVulkan.ClearColor[1] = 0.0f;
    trVulkan.ClearColor[2] = 0.0f;
    trVulkan.ClearColor[3] = 1.0f;
}

tr_func void trSetClearColor(tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    trVulkan.ClearColor[0] = R;
    trVulkan.ClearColor[1] = G;
    trVulkan.ClearColor[2] = B;
    trVulkan.ClearColor[3] = A;
}

tr_func void trVertex2D(
    tr_f32 X, tr_f32 Y,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    trTexturedVertex2D(
        X, Y,
        R, G, B, A,
        0.0f, 0.0f,
        TR_DEFAULT_TEXTURE
    );
}

tr_func void trTexturedVertex2D(
    tr_f32 X, tr_f32 Y,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A,
    tr_f32 U, tr_f32 V,
    tr_texture Texture
)
{
    TR_ASSERT(Texture < trVulkan.TextureCount);

    tr_vulkan_buffer* VertexBuffer = &trVulkan.VertexBuffer;

    tr_usize MaxVertexCount = VertexBuffer->Size / sizeof(tr_vulkan_vertex);

    TR_ASSERT(trVulkan.VertexCount < MaxVertexCount);

    tr_vulkan_vertex* Vertex =
        (tr_vulkan_vertex*)VertexBuffer->Mapping + trVulkan.VertexCount;

    trZeroType(Vertex);

    Vertex->X = X;
    Vertex->Y = Y;

    Vertex->U = U;
    Vertex->V = V;

    R = TR_MAX(0, TR_MIN(R, 1));
    G = TR_MAX(0, TR_MIN(G, 1));
    B = TR_MAX(0, TR_MIN(B, 1));
    A = TR_MAX(0, TR_MIN(A, 1));

    Vertex->PackedRGBA =
        ((tr_u32)(R * 255.0f + 0.5f) <<  0) |
        ((tr_u32)(G * 255.0f + 0.5f) <<  8) |
        ((tr_u32)(B * 255.0f + 0.5f) << 16) |
        ((tr_u32)(A * 255.0f + 0.5f) << 24);

    Vertex->TextureIndex = Texture;

    trVulkan.VertexCount++;
}

tr_func void trRect2D(
    tr_f32 MinX, tr_f32 MinY,
    tr_f32 MaxX, tr_f32 MaxY,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_b32 OutsideScreen =
        (MinX > WindowW) ||
        (MinY > WindowH) ||
        (MaxX < 0) ||
        (MaxY < 0);

    if (!OutsideScreen)
    {
        trVertex2D(MinX, MinY, R, G, B, A);
        trVertex2D(MinX, MaxY, R, G, B, A);
        trVertex2D(MaxX, MaxY, R, G, B, A);

        trVertex2D(MaxX, MaxY, R, G, B, A);
        trVertex2D(MaxX, MinY, R, G, B, A);
        trVertex2D(MinX, MinY, R, G, B, A);
    }
}

tr_func void trCircle2D(
    tr_f32 CenterX, tr_f32 CenterY,
    tr_f32 Radius,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    tr_f32 MinX = CenterX - 0.5f*Radius;
    tr_f32 MinY = CenterY - 0.5f*Radius;
    tr_f32 MaxX = CenterX + 0.5f*Radius;
    tr_f32 MaxY = CenterY + 0.5f*Radius;

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_b32 OutsideScreen =
        (MinX > WindowW) ||
        (MinY > WindowH) ||
        (MaxX < 0) ||
        (MaxY < 0);

    if (OutsideScreen)
        return;

    tr_u32 PointCount = 32;

    tr_f32 FirstX = CenterX - Radius;
    tr_f32 FirstY = CenterY;

    trVertex2D(CenterX, CenterY, R, G, B, A);
    trVertex2D(FirstX, FirstY, R, G, B, A);

    for (tr_u32 Index = 1; Index < PointCount; Index++)
    {
        tr_f32 T = (tr_f32)Index / (tr_f32)PointCount;

        tr_f32 C = trCosF32(T);
        tr_f32 S = trSinF32(T);

        tr_f32 U = 0.5f - 0.5f*C;
        tr_f32 V = 0.5f + 0.5f*S;

        tr_f32 X = CenterX - Radius*C;
        tr_f32 Y = CenterY + Radius*S;

        trVertex2D(X, Y, R, G, B, A);
        trVertex2D(CenterX, CenterY, R, G, B, A);
        trVertex2D(X, Y, R, G, B, A);
    }

    trVertex2D(FirstX, FirstY, R, G, B, A);
}

tr_func void trRectOutline2D(
    tr_f32 MinX, tr_f32 MinY,
    tr_f32 MaxX, tr_f32 MaxY,
    tr_f32 Thickness,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    tr_f32 OutlineMinX = MinX - 0.5f*Thickness;
    tr_f32 OutlineMinY = MinY - 0.5f*Thickness;
    tr_f32 OutlineMaxX = MaxX + 0.5f*Thickness;
    tr_f32 OutlineMaxY = MaxY + 0.5f*Thickness;

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_b32 OutsideScreen =
        (OutlineMinX > WindowW) ||
        (OutlineMinY > WindowH) ||
        (OutlineMaxX < 0) ||
        (OutlineMaxY < 0);

    if (OutsideScreen)
        return;

    tr_f32 HalfThickness = 0.5f*Thickness;

    // NOTE(vak): Left

    trRect2D(
        MinX - HalfThickness,
        MinY - HalfThickness,
        MinX + HalfThickness,
        MaxY + HalfThickness,
        R, G, B, A
    );

    // NOTE(vak): Right

    trRect2D(
        MaxX - HalfThickness,
        MinY - HalfThickness,
        MaxX + HalfThickness,
        MaxY + HalfThickness,
        R, G, B, A
    );

    // NOTE(vak): Top

    trRect2D(
        MinX + HalfThickness,
        MinY - HalfThickness,
        MaxX - HalfThickness,
        MinY + HalfThickness,
        R, G, B, A
    );

    // NOTE(vak): Bottom

    trRect2D(
        MinX + HalfThickness,
        MaxY - HalfThickness,
        MaxX - HalfThickness,
        MaxY + HalfThickness,
        R, G, B, A
    );
}

tr_func void trCircleOutline2D(
    tr_f32 CenterX, tr_f32 CenterY,
    tr_f32 Radius,
    tr_f32 Thickness,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    tr_f32 MinX = CenterX - 0.5f*Radius - 0.5f*Thickness;
    tr_f32 MinY = CenterY - 0.5f*Radius - 0.5f*Thickness;
    tr_f32 MaxX = CenterX + 0.5f*Radius + 0.5f*Thickness;
    tr_f32 MaxY = CenterY + 0.5f*Radius + 0.5f*Thickness;

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_b32 OutsideScreen =
        (MinX > WindowW) ||
        (MinY > WindowH) ||
        (MaxX < 0) ||
        (MaxY < 0);

    if (OutsideScreen)
        return;

    tr_f32 InnerRadius = Radius - 0.5f*Thickness;
    tr_f32 OuterRadius = Radius + 0.5f*Thickness;

    tr_u32 PointCount = 32;
    tr_f32 OutersX[32] = {0};
    tr_f32 OutersY[32] = {0};
    tr_f32 InnersX[32] = {0};
    tr_f32 InnersY[32] = {0};

    for (tr_u32 Index = 0; Index < 32; Index++)
    {
        tr_f32 OuterT = (tr_f32)Index / (tr_f32)PointCount;
        tr_f32 InnerT = ((tr_f32)Index + 0.5f) / (tr_f32)PointCount;

        tr_f32 OuterC = trCosF32(OuterT);
        tr_f32 OuterS = trSinF32(OuterT);

        tr_f32 InnerC = trCosF32(InnerT);
        tr_f32 InnerS = trSinF32(InnerT);

        OutersX[Index] = CenterX - OuterRadius*OuterC;
        OutersY[Index] = CenterY + OuterRadius*OuterS;

        InnersX[Index] = CenterX - InnerRadius*InnerC;
        InnersY[Index] = CenterY + InnerRadius*InnerS;
    }

    for (tr_u32 Index = 0; Index < PointCount; Index++)
    {
        tr_u32 Index0 = Index;
        tr_u32 Index1 = (Index + 1) % PointCount;

        trVertex2D(OutersX[Index0], OutersY[Index0], R, G, B, A);
        trVertex2D(InnersX[Index0], InnersY[Index0], R, G, B, A);
        trVertex2D(OutersX[Index1], OutersY[Index1], R, G, B, A);

        trVertex2D(OutersX[Index1], OutersY[Index1], R, G, B, A);
        trVertex2D(InnersX[Index0], InnersY[Index0], R, G, B, A);
        trVertex2D(InnersX[Index1], InnersY[Index1], R, G, B, A);
    }
}

tr_func void trTexturedRect2D(
    tr_f32 MinX, tr_f32 MinY,
    tr_f32 MaxX, tr_f32 MaxY,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A,
    tr_f32 MinU, tr_f32 MinV,
    tr_f32 MaxU, tr_f32 MaxV,
    tr_texture Texture
)
{
    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_b32 OutsideScreen =
        (MinX > WindowW) ||
        (MinY > WindowH) ||
        (MaxX < 0) ||
        (MaxY < 0);

    if (!OutsideScreen)
    {
        trTexturedVertex2D(MinX, MinY, R, G, B, A, MinU, MinV, Texture);
        trTexturedVertex2D(MinX, MaxY, R, G, B, A, MinU, MaxV, Texture);
        trTexturedVertex2D(MaxX, MaxY, R, G, B, A, MaxU, MaxV, Texture);

        trTexturedVertex2D(MaxX, MaxY, R, G, B, A, MaxU, MaxV, Texture);
        trTexturedVertex2D(MaxX, MinY, R, G, B, A, MaxU, MinV, Texture);
        trTexturedVertex2D(MinX, MinY, R, G, B, A, MinU, MinV, Texture);
    }
}

// NOTE(vak): All text related functions currently uses a
// 241x199 font atlas with one pixel padding at the start
// of the texture, and each glyph cell is 15x33.
//
// The font used is Proggy Clean at 32px.

tr_func tr_f32 trMeasureTextWidth(tr_str Text)
{
    tr_usize MaxRowCharCount = 0;
    tr_usize RowCharCount = 0;

    for (tr_usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if ((Character >= 32) && (Character <= 126))
        {
            RowCharCount++;
        }
        else if (Character == '\t')
        {
            RowCharCount += 4;
        }
        else if (Character == '\n')
        {
            MaxRowCharCount = TR_MAX(MaxRowCharCount, RowCharCount);
            RowCharCount = 0;
        }
    }

    MaxRowCharCount = TR_MAX(MaxRowCharCount, RowCharCount);

    tr_f32 GlyphWidth = 15.0f;
    tr_f32 Result = MaxRowCharCount * GlyphWidth;

    return (Result);
}

tr_func tr_f32 trMeasureTextHeight(tr_str Text)
{
    tr_u32 LineCount = 1;

    for (tr_usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if (Character == '\n')
            LineCount++;
    }

    tr_f32 Result = LineCount * trGetTextLineHeight();

    return (Result);
}

tr_func tr_f32 trGetTextLineHeight(void)
{
    tr_f32 GlyphHeight = 33.0f;
    tr_f32 LineHeightScale = 1.0f;
    tr_f32 Result = GlyphHeight * LineHeightScale;

    return (Result);
}

tr_func void trText2D(
    tr_str Text,
    tr_f32 X, tr_f32 Y,
    tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A
)
{
    tr_u32 AtlasWidth = 241;
    tr_u32 AtlasHeight = 199;

    tr_u32 CellWidth = 15;
    tr_u32 CellHeight = 33;

    tr_u32 CellCountX = AtlasWidth / CellWidth;
    tr_u32 CellCountY = AtlasHeight / CellHeight;

    tr_f32 GlyphWidth = 15.0f;
    tr_f32 GlyphHeight = 33.0f;

    tr_f32 LineHeight = trGetTextLineHeight();

    tr_f32 CurrentX = X;
    tr_f32 CurrentY = Y + 0.5f*(LineHeight - GlyphHeight);

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    for (tr_usize Index = 0; Index < Text.Size; Index++)
    {
        char Character = Text.Data[Index];

        if ((Character > 32) && (Character <= 126))
        {
            tr_u32 CellIndex = (Character - 32);

            tr_u32 CellX = CellIndex % CellCountX;
            tr_u32 CellY = CellIndex / CellCountX;

            tr_u32 TexelMinX = 1 + CellX * CellWidth;
            tr_u32 TexelMinY = 1 + CellY * CellHeight;
            tr_u32 TexelMaxX = TexelMinX + CellWidth;
            tr_u32 TexelMaxY = TexelMinY + CellHeight;

            tr_f32 MinU = (tr_f32)TexelMinX / (tr_f32)AtlasWidth;
            tr_f32 MinV = (tr_f32)TexelMinY / (tr_f32)AtlasHeight;
            tr_f32 MaxU = (tr_f32)TexelMaxX / (tr_f32)AtlasWidth;
            tr_f32 MaxV = (tr_f32)TexelMaxY / (tr_f32)AtlasHeight;

            trTexturedRect2D(
                CurrentX,
                CurrentY,
                CurrentX+GlyphWidth,
                CurrentY+GlyphHeight,
                R, G, B, A,
                MinU, MinV, MaxU, MaxV,
                trVulkan.FontTexture
            );

            CurrentX += GlyphWidth;
        }
        else if (Character == ' ')
        {
            CurrentX += GlyphWidth;
        }
        else if (Character == '\t')
        {
            CurrentX += 4*GlyphWidth;
        }
        else if (Character == '\n')
        {
            CurrentX = X;
            CurrentY += LineHeight;
        }
    }
}

tr_func void trVulkanEndFrame(void)
{
    VkCommandBufferBeginInfo BeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    TR_VK_CHECK(vkResetCommandBuffer(trVulkan.CommandBuffer, 0));
    TR_VK_CHECK(vkBeginCommandBuffer(trVulkan.CommandBuffer, &BeginInfo));

    {
        VkImageMemoryBarrier RenderBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .image = trVulkan.SwapchainImages[trVulkan.ImageIndex],
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            trVulkan.CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &RenderBarrier
        );
    }

    {
        VkRenderingInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea =
            {
                .offset = {.x = 0, .y = 0},
                .extent = trVulkan.SwapchainExtent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkRenderingAttachmentInfo)
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = trVulkan.SwapchainImageViews[trVulkan.ImageIndex],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue =
                {
                    .color = {.float32 =
                    {
                        [0] = trVulkan.ClearColor[0],
                        [1] = trVulkan.ClearColor[1],
                        [2] = trVulkan.ClearColor[2],
                        [3] = trVulkan.ClearColor[3],
                    }},
                },
            },
        };

        vkCmdBeginRendering(trVulkan.CommandBuffer, &RenderingInfo);

        if (trVulkan.VertexCount > 0)
        {
            VkViewport Viewport =
            {
                .x = 0,
                .y = 0,
                .width = (tr_f32)trVulkan.SwapchainExtent.width,
                .height = (tr_f32)trVulkan.SwapchainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            VkRect2D Scissor = RenderingInfo.renderArea;

            vkCmdBindPipeline(trVulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trVulkan.Pipeline);
            vkCmdSetViewport(trVulkan.CommandBuffer, 0, 1, &Viewport);
            vkCmdSetScissor(trVulkan.CommandBuffer, 0, 1, &Scissor);

            VkWriteDescriptorSet DescriptorWrites[] =
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &(VkDescriptorBufferInfo)
                    {
                        .buffer = trVulkan.VertexBuffer.Buffer,
                        .offset = 0,
                        .range = trVulkan.VertexBuffer.Size,
                    },
                },
            };

            tr_u32 BufferSetIndex = 0;
            tr_u32 TextureSetIndex = 1;

            vkCmdPushDescriptorSet(
                trVulkan.CommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                trVulkan.PipelineLayout,
                BufferSetIndex,
                TR_ARRAY_COUNT(DescriptorWrites),
                DescriptorWrites
            );

            vkCmdBindDescriptorSets(
                trVulkan.CommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                trVulkan.PipelineLayout,
                TextureSetIndex,
                1,
                &trVulkan.TextureSet,
                0, 0
            );

            tr_vulkan_push_constants PushConstants =
            {
                .Projection =
                {
                    [0]  = 2.0f / Viewport.width,
                    [5]  = 2.0f / Viewport.height,
                    [10] = 1.0f,
                    [15] = 1.0f,

                    [12] = -1.0f,
                    [13] = -1.0f,
                },
            };

            vkCmdPushConstants(
                trVulkan.CommandBuffer,
                trVulkan.PipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstants),
                &PushConstants
            );

            vkCmdDraw(trVulkan.CommandBuffer, trVulkan.VertexCount, 1, 0, 0);
        }

        vkCmdEndRendering(trVulkan.CommandBuffer);
    }

    {
        VkImageMemoryBarrier PresentBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .image = trVulkan.SwapchainImages[trVulkan.ImageIndex],
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            trVulkan.CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &PresentBarrier
        );
    }

    TR_VK_CHECK(vkEndCommandBuffer(trVulkan.CommandBuffer));

    VkPipelineStageFlags WaitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo SubmitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &trVulkan.AcquireSemaphore,
        .commandBufferCount = 1,
        .pCommandBuffers = &trVulkan.CommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &trVulkan.SubmitSemaphore,
        .pWaitDstStageMask = &WaitStageMask,
    };

    TR_VK_CHECK(vkQueueSubmit(trVulkan.Queue, 1, &SubmitInfo, 0));

    VkPresentInfoKHR PresentInfo =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &trVulkan.SubmitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &trVulkan.Swapchain,
        .pImageIndices = &trVulkan.ImageIndex,
    };

    TR_VK_CHECK(vkQueuePresentKHR(trVulkan.Queue, &PresentInfo));

    TR_VK_CHECK(vkDeviceWaitIdle(trVulkan.Device));
}

/////////////////////////////////////////////////
// NOTE(vak): Bytestream reader
/////////////////////////////////////////////////

tr_func tr_reader trMakeReader(void* Data, tr_usize Size)
{
    tr_reader Reader =
    {
        .At = (tr_u8*)Data,
        .Remaining = Size,
    };

    return (Reader);
}

tr_func void trReaderTryNextChunk(tr_reader* Reader)
{
    if (Reader->Remaining == 0)
    {
        if (Reader->FirstChunk)
        {
            tr_reader_chunk* Chunk = Reader->FirstChunk;

            Reader->At = Chunk->Data;
            Reader->Remaining = Chunk->Size;

            Reader->FirstChunk = Chunk->Next;
        }

        if (!Reader->FirstChunk)
        {
            Reader->FirstChunk = 0;
            Reader->LastChunk = 0;
        }
    }
}

tr_func void* trReaderConsumeSize(tr_reader* Reader, tr_usize Size)
{
    trReaderTryNextChunk(Reader);

    TR_ASSERT(Size <= Reader->Remaining);

    void* Result = Reader->At;

    Reader->At += Size;
    Reader->Remaining -= Size;

    return (Result);
}

tr_func tr_b32 trReaderConsumeToBuffer(tr_reader* Reader, tr_usize Size, void* Buffer)
{
    trReaderTryNextChunk(Reader);

    if (Reader->Remaining < Size)
        return (TR_FALSE);

    void* ConsumedAt = trReaderConsumeSize(Reader, Size);
    trCopyMemory(Buffer, ConsumedAt, Size);

    return (TR_TRUE);
}

tr_func tr_b32 trReaderAddChunk(tr_reader* Reader, tr_arena* Arena, void* Data, tr_usize Size)
{
    tr_b32 ParametersOkay = (Reader && Arena && Data && Size);
    if (!ParametersOkay)
        return (TR_FALSE);

    tr_reader_chunk* Chunk = trPushArenaTypeZero(Arena, tr_reader_chunk);

    if (!Chunk)
        return (TR_FALSE);

    Chunk->Data = Data;
    Chunk->Size = Size;

    if (!Reader->FirstChunk || !Reader->LastChunk)
    {
        Reader->FirstChunk = Chunk;
        Reader->LastChunk = Chunk;
    }
    else
    {
        Reader->LastChunk->Next = Chunk;
        Reader->LastChunk = Chunk;
    }

    return (TR_TRUE);
}

tr_func tr_bit_reader trMakeBitReader(tr_reader* Reader)
{
    TR_ASSERT(Reader);

    tr_bit_reader BitReader =
    {
        .Reader = Reader,
        .BitBuffer = 0,
        .BitCount = 0,
    };

    return (BitReader);
}

tr_func tr_b32 trBitReaderConsume(tr_bit_reader* BitReader, tr_u32* Buffer, tr_usize BitCount)
{
    tr_b32 PeekOkay = trBitReaderPeek(BitReader, Buffer, BitCount);
    tr_b32 AdvanceOkay = TR_FALSE;

    if (PeekOkay)
        AdvanceOkay = trBitReaderAdvance(BitReader, BitCount);

    tr_b32 Okay = PeekOkay && AdvanceOkay;

    return (Okay);
}

tr_func tr_b32 trBitReaderPeek(tr_bit_reader* BitReader, tr_u32* Buffer, tr_usize BitCount)
{
    tr_b32 ParametersOkay = (BitReader && Buffer && (BitCount <= 32));
    if (!ParametersOkay)
        return (TR_FALSE);

    tr_reader* BackingReader = BitReader->Reader;

    while (BitReader->BitCount < BitCount)
    {
        tr_u8 Byte = 0;

        if (!trReaderConsumeInto(BackingReader, &Byte)) return (TR_FALSE);

        BitReader->BitBuffer |= (tr_u64)Byte << BitReader->BitCount;
        BitReader->BitCount += 8;
    }

    tr_u64 Mask = (((tr_usize)1) << BitCount) - 1;

    *Buffer = (tr_u32)(BitReader->BitBuffer & Mask);

    return (TR_TRUE);
}

tr_func tr_b32 trBitReaderAdvance(tr_bit_reader* BitReader, tr_usize BitCount)
{
    tr_b32 ParametersOkay = (BitReader && (BitCount <= 32));
    if (!ParametersOkay)
        return (TR_FALSE);

    tr_b32 Okay = (BitCount <= BitReader->BitCount);
    if (Okay)
    {
        BitReader->BitBuffer >>= BitCount;
        BitReader->BitCount -= BitCount;
    }

    return (Okay);
}

/////////////////////////////////////////////////
// NOTE(vak): PNG
/////////////////////////////////////////////////

typedef struct __attribute__((packed))
{
    tr_u32 Length;
    tr_u32 ChunkType;
} tr_png_chunk_header;

typedef struct __attribute__((packed))
{
    tr_u32 CRC;
} tr_png_chunk_footer;

typedef struct __attribute__((packed))
{
    tr_u32 Width;
    tr_u32 Height;
    tr_u8  BitDepth;
    tr_u8  ColorType;
    tr_u8  CompressionMethod;
    tr_u8  FilterMethod;
    tr_u8  InterlaceMethod;
} tr_png_ihdr;

typedef struct
{
    tr_u16 Value;
    tr_u16 BitCount;
} tr_png_huffman_entry;

typedef struct
{
    // NOTE(vak): EntryCount = (1 << BitCount)

    tr_usize BitCount;
    tr_png_huffman_entry* Entries;
} tr_png_huffman;

tr_func tr_b32 trAllocateHuffmanPNG(tr_png_huffman* Huffman, tr_arena* Arena, tr_usize BitCount)
{
    tr_b32 ParametersOkay = (Huffman && Arena && ((BitCount > 0) && (BitCount <= 15)));
    if (!ParametersOkay)
        return (TR_FALSE);

    Huffman->BitCount = BitCount;

    tr_usize EntryCount = ((tr_usize)1) << BitCount;

    Huffman->Entries = trPushArenaArrayZero(
        Arena,
        tr_png_huffman_entry,
        EntryCount
    );

    if (!Huffman->Entries)
        return (TR_FALSE);

    return (TR_TRUE);
}

tr_func tr_b32 trBuildHuffmanPNG(
    tr_png_huffman* Huffman,
    tr_u32* SymbolCodeLengths,
    tr_u32 SymbolCount
)
{
    tr_b32 ParametersOkay = (Huffman && SymbolCodeLengths && ((SymbolCount > 0) && (SymbolCount <= 288)));
    if (!ParametersOkay)
        return (TR_FALSE);

    // NOTE(vak): Zero everything
    {
        tr_usize EntryCount = ((tr_usize)1) << Huffman->BitCount;
        trZeroArray(Huffman->Entries, EntryCount);
    }

    tr_u32 NextCodes[288] = {0};

    // NOTE(vak): Construct codes from code lengths
    {
        tr_u32 Histogram[288] = {0};

        for (tr_u16 Symbol = 0; Symbol < SymbolCount; Symbol++)
        {
            tr_u32 CodeLength = SymbolCodeLengths[Symbol];
            Histogram[CodeLength]++;
        }

        Histogram[0] = 0;

        for (tr_u32 BitIndex = 1; BitIndex <= Huffman->BitCount; BitIndex++)
        {
            NextCodes[BitIndex] = (NextCodes[BitIndex-1] + Histogram[BitIndex - 1]) << 1;
        }
    }

    // NOTE(vak): Fill in lookup table for mapping codes to symbols

    for (tr_u16 Symbol = 0; Symbol < SymbolCount; Symbol++)
    {
        tr_u32 CodeLength = SymbolCodeLengths[Symbol];
        if (CodeLength == 0)
            continue;

        tr_u32 BitsRemaining = (Huffman->BitCount - CodeLength);

        tr_u32 EntryCount = (1 << BitsRemaining);
        tr_u32 Code = NextCodes[CodeLength]++;

        for (tr_u32 EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++)
        {
            tr_u32 FullIndex = (Code << BitsRemaining) + EntryIndex;
            tr_u32 BitReversedIndex = 0;

            for (tr_u32 BitIndex = 0; BitIndex <= Huffman->BitCount/2; BitIndex++)
            {
                tr_u32 A = BitIndex;
                tr_u32 B = Huffman->BitCount - BitIndex - 1;

                BitReversedIndex |= ((FullIndex >> B) & 1) << A;
                BitReversedIndex |= ((FullIndex >> A) & 1) << B;
            }

            tr_png_huffman_entry* Entry = Huffman->Entries + BitReversedIndex;

            // NOTE(vak): Something is wrong if we're overwriting entries...
            if (Entry->BitCount > 0)
                return (TR_FALSE);

            Entry->Value = Symbol;
            Entry->BitCount = (tr_u16)CodeLength;
        }
    }

    return (TR_TRUE);
}

tr_func tr_b32 trDecodeHuffmanPNG(
    tr_png_huffman* Huffman,
    tr_bit_reader* BitReader,
    tr_u32* Buffer
)
{
    tr_b32 ParametersOkay = (Huffman && BitReader && Buffer);
    if (!ParametersOkay)
        return (TR_FALSE);

    tr_u32 Bits = 0;

    if (!trBitReaderPeek(BitReader, &Bits, Huffman->BitCount)) return (TR_FALSE);

    tr_png_huffman_entry* Entry = Huffman->Entries + Bits;
    if (!Entry->BitCount) return (TR_FALSE);

    *Buffer = (tr_u32)Entry->Value;

    if (!trBitReaderAdvance(BitReader, Entry->BitCount)) return (TR_FALSE);

    return (TR_TRUE);
}

tr_func tr_b32 trDecodePNG(
    tr_decoded_png* Decoded,
    tr_arena* Arena,
    void* Data,
    tr_usize DataSize
)
{
    #define TR_PNG_CHECK(Value) if (!(Value)) goto DecodingFailed;

    // NOTE(vak): This is the initial arena position before the
    // decoder allocates anything. Used for deallocating everything
    // if the decoding process fails. See the "DecodingFailed" label
    // at the very end of this function.

    tr_usize InitialArenaPosition = trGetArenaPosition(Arena);

    TR_PNG_CHECK(Decoded && Arena && Data && DataSize);

    trZeroType(Decoded);

    tr_reader DataReader = {0}; // NOTE(vak): Linked list of IDAT chunks
    tr_usize ResetArenaTo = InitialArenaPosition; // NOTE(vak): Position to reset arena to after the decoder is finished

    // NOTE(vak): Signature and chunks
    {
        tr_reader Reader = trMakeReader(Data, DataSize);

        tr_u64 Signature = 0;

        TR_PNG_CHECK(trReaderConsumeInto(&Reader, &Signature));
        TR_PNG_CHECK(Signature == 0x0A1A0A0D474E5089);

        tr_b32 EncounteredIHDR = TR_FALSE;
        tr_b32 EncounteredIEND = TR_FALSE;
        tr_u32 ChunkIndex = 0; // NOTE(vak): To check if IHDR is the first chunk

        while (Reader.Remaining && !EncounteredIEND)
        {
            tr_png_chunk_header Header = {0};
            tr_png_chunk_footer Footer = {0};

            TR_PNG_CHECK(trReaderConsumeInto(&Reader, &Header));

            tr_u32 ChunkLength = trBigToHostEndian32(Header.Length);
            TR_PNG_CHECK(ChunkLength <= Reader.Remaining);

            void* ChunkData = trReaderConsumeSize(&Reader, ChunkLength);

            TR_PNG_CHECK(trReaderConsumeInto(&Reader, &Footer));

            switch (Header.ChunkType)
            {
                default: break;

                case 0x52444849: // NOTE(vak): IHDR
                {
                    TR_PNG_CHECK(ChunkIndex == 0);
                    TR_PNG_CHECK(ChunkLength == sizeof(tr_png_ihdr));

                    tr_png_ihdr* IHDR = (tr_png_ihdr*)ChunkData;

                    Decoded->Width = trBigToHostEndian32(IHDR->Width);
                    Decoded->Height = trBigToHostEndian32(IHDR->Height);

                    switch (IHDR->ColorType)
                    {
                        default: Decoded->Channels = 0;

                        case 0: Decoded->Channels = 1; break; // NOTE(vak): Greyscale
                        case 2: Decoded->Channels = 3; break; // NOTE(vak): RGB
                        case 4: Decoded->Channels = 2; break; // NOTE(vak): Greyscale + Alpha
                        case 6: Decoded->Channels = 4; break; // NOTE(vak): RGBA
                    }

                    tr_b32 IsValid = TR_TRUE;

                    IsValid &= (Decoded->Width != 0) && (Decoded->Height != 0);
                    IsValid &= (IHDR->BitDepth == 8);
                    IsValid &= (Decoded->Channels != 0);
                    IsValid &= (IHDR->CompressionMethod == 0);
                    IsValid &= (IHDR->FilterMethod == 0);
                    IsValid &= (IHDR->InterlaceMethod == 0);

                    TR_PNG_CHECK(IsValid);

                    // NOTE(vak): Allocate memory for image, and set the arena
                    // reset position. Every single arena allocation made after
                    // the main image allocation is treated as temporary working
                    // memory for the decoder, and will be resetted at the end.

                    Decoded->Pixels = trPushArenaZero(
                        Arena,
                        Decoded->Width * Decoded->Height * Decoded->Channels
                    );

                    TR_PNG_CHECK(Decoded->Pixels);

                    ResetArenaTo = trGetArenaPosition(Arena);

                    EncounteredIHDR = TR_TRUE;
                } break;

                case 0x54414449: // NOTE(vak): IDAT
                {
                    // NOTE(vak): Uses the arena to allocate tr_reader_chunk.
                    // The PNG spec enforces IHDR to be placed before IDAT, so
                    // the arena allocation should be made right after the main
                    // image allocation. Thus, it can be resetted at the end later,
                    // so this code is fine.

                    TR_PNG_CHECK(EncounteredIHDR);
                    TR_PNG_CHECK(trReaderAddChunk(&DataReader, Arena, ChunkData, ChunkLength));
                } break;

                case 0x444e4549: // NOTE(vak): IEND
                {
                    TR_PNG_CHECK(EncounteredIHDR);
                    EncounteredIEND = TR_TRUE;
                } break;
            }

            ChunkIndex++;
        }

        TR_PNG_CHECK(EncounteredIEND);
    }

    TR_PNG_CHECK(DataReader.FirstChunk);

    tr_usize DecompressedSize = (1 + Decoded->Width*Decoded->Channels) * Decoded->Height;

    tr_u8* DecompressedBytes = trPushArenaZero(
        Arena,
        DecompressedSize
    );

    TR_PNG_CHECK(DecompressedBytes);

    tr_usize DecompressedOffset = 0;

    // NOTE(vak): Decompress
    {
        // NOTE(vak): ZLIB Compressed Data Format
        // https://www.rfc-editor.org/info/rfc1950/

        tr_u8 CMF = 0;
        tr_u8 FLG = 0;

        TR_PNG_CHECK(trReaderConsumeInto(&DataReader, &CMF));
        TR_PNG_CHECK(trReaderConsumeInto(&DataReader, &FLG));

        tr_u8 CM     = (CMF & 0x0F);
        tr_u8 CINFO  = (CMF >> 4) & 0x0F;
        tr_u8 FCHECK = (FLG & 0x1F);
        tr_u8 FDICT  = (FLG >> 5) & 0x01;
        tr_u8 FLEVEL = (FLG >> 6) & 0x03;

        {
            tr_u16 CheckValue = 256*((tr_u16)CMF) + (tr_u16)FLG;
            tr_b32 IsValid = TR_TRUE;

            IsValid &= ((CheckValue % 31) == 0);
            IsValid &= (CM == 8);
            IsValid &= (CINFO == 7);
            IsValid &= (FDICT == 0);

            TR_PNG_CHECK(IsValid);
        }

        // NOTE(vak): DEFLATE Compressed Data Format
        // https://www.rfc-editor.org/info/rfc1951/

        tr_bit_reader BitReader = trMakeBitReader(&DataReader);

        tr_u32 BFINAL = 0;
        tr_u32 BTYPE  = 0;

        tr_png_huffman HuffmanCLEN      = {0};
        tr_png_huffman HuffmanLitLen    = {0};
        tr_png_huffman HuffmanDist      = {0};

        TR_PNG_CHECK(trAllocateHuffmanPNG(&HuffmanCLEN, Arena, 7));
        TR_PNG_CHECK(trAllocateHuffmanPNG(&HuffmanLitLen, Arena, 15));
        TR_PNG_CHECK(trAllocateHuffmanPNG(&HuffmanDist, Arena, 15));

        do
        {
            TR_PNG_CHECK(trBitReaderConsume(&BitReader, &BFINAL, 1));
            TR_PNG_CHECK(trBitReaderConsume(&BitReader, &BTYPE,  2));

            if (BTYPE == 0x00)
            {
                // TODO(vak): !!!! Actually test if this thing works or not!!!!
                // NOTE(vak): Uncompressed

                // NOTE(vak): We're reading in a bytestream, so reset bit reader
                // since it isn't needed here.
                BitReader.BitBuffer = 0;
                BitReader.BitCount  = 0;

                tr_u16 LEN  = 0;
                tr_u16 NLEN = 0;

                TR_PNG_CHECK(trReaderConsumeInto(&DataReader, &LEN));
                TR_PNG_CHECK(trReaderConsumeInto(&DataReader, &NLEN));

                TR_PNG_CHECK((tr_u16)(~LEN) == (tr_u16)(NLEN));

                tr_usize Remaining = LEN;

                while (Remaining)
                {
                    // NOTE(vak): Go to next chunk if nothing remains
                    if (DataReader.Remaining == 0)
                        trReaderTryNextChunk(&DataReader);

                    // NOTE(vak): Last chunk now...
                    if (!DataReader.FirstChunk)
                        TR_PNG_CHECK(DataReader.Remaining >= Remaining);

                    tr_usize ConsumeSize = TR_MIN(DataReader.Remaining, Remaining);

                    void* UncompressedData = trReaderConsumeSize(&DataReader, ConsumeSize);

                    TR_PNG_CHECK(DecompressedOffset + ConsumeSize <= DecompressedSize);

                    trCopyMemory(
                        DecompressedBytes + DecompressedOffset,
                        UncompressedData,
                        ConsumeSize
                    );

                    DecompressedOffset += ConsumeSize;
                    Remaining -= ConsumeSize;
                }
            }
            else if (BTYPE == 0x01)
            {
                // NOTE(vak): Compressed with static huffman codes
                // TODO(vak): Implement this
            }
            else if (BTYPE == 0x02)
            {
                // NOTE(vak): Compressed with dynamic huffman codes

                tr_u32 HLIT  = 0;
                tr_u32 HDIST = 0;
                tr_u32 HCLEN = 0;

                TR_PNG_CHECK(trBitReaderConsume(&BitReader, &HLIT,  5));
                TR_PNG_CHECK(trBitReaderConsume(&BitReader, &HDIST, 5));
                TR_PNG_CHECK(trBitReaderConsume(&BitReader, &HCLEN, 4));

                HLIT  += 257;
                HDIST += 1;
                HCLEN += 4;

                // NOTE(vak): Code lengths for code lengths... (literally)

                tr_u32 Swizzle[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
                tr_u32 CodeLengthsForCLEN[19] = {0};

                for (tr_u32 Index = 0; Index < HCLEN; Index++)
                {
                    TR_PNG_CHECK(trBitReaderConsume(
                        &BitReader,
                        &CodeLengthsForCLEN[Swizzle[Index]],
                        3
                    ));
                }

                TR_PNG_CHECK(trBuildHuffmanPNG(&HuffmanCLEN, CodeLengthsForCLEN, 19));

                // NOTE(vak): Code lengths for literal/length and
                // distance.

                tr_u32 CodeLengths[318] = {0};
                tr_u32 CodeLengthCount = 0;

                while (CodeLengthCount < HLIT+HDIST)
                {
                    tr_u32 Symbol = 0;

                    TR_PNG_CHECK(trDecodeHuffmanPNG(&HuffmanCLEN, &BitReader, &Symbol));
                    TR_PNG_CHECK(Symbol <= 18);

                    tr_u32 RepeatValue = 0;
                    tr_u32 RepeatCount = 1;

                    if (Symbol <= 15)
                    {
                        RepeatValue = Symbol;
                    }
                    else if (Symbol <= 16)
                    {
                        if (CodeLengthCount == 0) return (TR_FALSE);

                        tr_u32 Repeat = 0;
                        TR_PNG_CHECK(trBitReaderConsume(&BitReader, &Repeat, 2));

                        RepeatValue = CodeLengths[CodeLengthCount - 1];
                        RepeatCount = 3 + Repeat;
                    }
                    else if (Symbol <= 17)
                    {
                        tr_u32 Repeat = 0;
                        TR_PNG_CHECK(trBitReaderConsume(&BitReader, &Repeat, 3));

                        RepeatCount = 3 + Repeat;
                    }
                    else if (Symbol <= 18)
                    {
                        tr_u32 Repeat = 0;
                        TR_PNG_CHECK(trBitReaderConsume(&BitReader, &Repeat, 7));

                        RepeatCount = 11 + Repeat;
                    }

                    TR_PNG_CHECK(CodeLengthCount + RepeatCount <= HLIT+HDIST);

                    for (tr_u32 Index = 0; Index < RepeatCount; Index++)
                        CodeLengths[CodeLengthCount++] = RepeatValue;
                }

                // NOTE(vak): Build Huffman lookup tables for literal/length
                // and distance

                TR_PNG_CHECK(trBuildHuffmanPNG(&HuffmanLitLen, CodeLengths + 0,    HLIT));
                TR_PNG_CHECK(trBuildHuffmanPNG(&HuffmanDist,   CodeLengths + HLIT, HDIST));

                // NOTE(vak): Start decoding

                for (;;)
                {
                    tr_u32 Symbol = 0;

                    TR_PNG_CHECK(trDecodeHuffmanPNG(&HuffmanLitLen, &BitReader, &Symbol));
                    TR_PNG_CHECK(Symbol <= 285);

                    if (Symbol <= 255)
                    {
                        TR_PNG_CHECK(DecompressedOffset < DecompressedSize);
                        DecompressedBytes[DecompressedOffset++] = Symbol;
                    }
                    else if (Symbol <= 256)
                    {
                        break;
                    }
                    else if (Symbol <= 285)
                    {
                        // NOTE(vak): xxxEncodings[xx]:
                        //      [0]: Extra bits
                        //      [1]: Base value

                        static tr_u32 LengthEncodings[29][2] =
                        {
                            {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7},
                            {0, 8}, {0, 9}, {0, 10}, {1, 11}, {1, 13},
                            {1, 15}, {1, 17}, {2, 19}, {2, 23}, {2, 27},
                            {2, 31}, {3, 35}, {3, 43}, {3, 51}, {3, 59},
                            {4, 67}, {4, 83}, {4, 99}, {4, 115}, {5, 131},
                            {5, 163}, {5, 195}, {5, 227}, {0, 258},
                        };

                        static tr_u32 DistanceEncodings[30][2] =
                        {
                            {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 5},
                            {1, 7}, {2, 9}, {2, 13}, {3, 17}, {3, 25},
                            {4, 33}, {4, 49}, {5, 65}, {5, 97}, {6, 129},
                            {6, 193}, {7, 257}, {7, 385}, {8, 513}, {8, 769},
                            {9, 1025}, {9, 1537}, {10, 2049}, {10, 3073}, {11, 4097},
                            {11, 6145}, {12, 8193}, {12, 12289}, {13, 16385}, {13, 24577},
                        };

                        tr_u32 LengthIndex = Symbol - 257;
                        tr_u32 LengthExtraBits = LengthEncodings[LengthIndex][0];
                        tr_u32 Length = LengthEncodings[LengthIndex][1];

                        if (LengthExtraBits)
                        {
                            tr_u32 Additional = 0;
                            TR_PNG_CHECK(trBitReaderConsume(&BitReader, &Additional, LengthExtraBits));

                            Length += Additional;
                        }

                        tr_u32 DistanceIndex = 0;

                        TR_PNG_CHECK(trDecodeHuffmanPNG(&HuffmanDist, &BitReader, &DistanceIndex));
                        TR_PNG_CHECK(DistanceIndex <= 29);

                        tr_u32 DistanceExtraBits = DistanceEncodings[DistanceIndex][0];
                        tr_u32 Distance = DistanceEncodings[DistanceIndex][1];

                        if (DistanceExtraBits)
                        {
                            tr_u32 Additional = 0;
                            TR_PNG_CHECK(trBitReaderConsume(&BitReader, &Additional, DistanceExtraBits));

                            Distance += Additional;
                        }

                        TR_PNG_CHECK((Length > 0) && (Length <= 258));
                        TR_PNG_CHECK((Distance > 0) && (Distance <= 32768));

                        TR_PNG_CHECK(DecompressedOffset >= Distance);
                        TR_PNG_CHECK(DecompressedOffset + Length <= DecompressedSize);

                        tr_u8* Source = DecompressedBytes + DecompressedOffset - Distance;
                        tr_u8* Dest = DecompressedBytes + DecompressedOffset;

                        for (tr_u32 Index = 0; Index < Length; Index++)
                            *Dest++ = *Source++;

                        DecompressedOffset += Length;
                    }
                }
            }
            else
            {
                goto DecodingFailed;
            }
        } while (!BFINAL);
    }

    TR_PNG_CHECK(DecompressedOffset == DecompressedSize);

    // NOTE(vak): Filter reconstruction
    {
        // NOTE(vak): https://www.w3.org/TR/png-3/#9Filters

        // NOTE(vak): Pixel layout is like this
        //      [C] [B]
        //      [A] [X]
        //
        // where [X] is the current pixel that is being reconstructed.
        // If [A], [B], or [C] are outside of the image then they're treated
        // as zeroes.

        TR_PNG_CHECK(Decoded->Channels <= 8);

        tr_u64 Zero = 0;

        tr_u32 LastRowAdvance = 0;
        tr_u8* PrevDestRow = (tr_u8*)&Zero;

        tr_u8* Dest = Decoded->Pixels;
        tr_u8* Source = DecompressedBytes;

        for (tr_usize Y = 0; Y < Decoded->Height; Y++)
        {
            tr_u8* DestRow = Dest + (Y * Decoded->Width*Decoded->Channels);
            tr_u8* SourceRow = Source + (Y * (1 + Decoded->Width*Decoded->Channels));
            tr_u8 Filter = *SourceRow++;

            TR_PNG_CHECK(Filter <= 4);

            switch (Filter)
            {
                case 0: // NOTE(vak): None
                {
                    trCopyMemory(Dest, Source, Decoded->Width*Decoded->Channels);
                } break;

                case 1: // NOTE(vak): Left
                {
                    tr_u8* X = SourceRow;
                    tr_u8* A = (tr_u8*)&Zero;

                    for (tr_usize Index = 0; Index < Decoded->Width; Index++)
                    {
                        tr_u8* Dest = DestRow + Index*Decoded->Channels;
                        tr_u8* X = SourceRow + Index*Decoded->Channels;

                        for (tr_usize Channel = 0; Channel < Decoded->Channels; Channel++)
                        {
                            Dest[Channel] = (tr_u8)(X[Channel] + A[Channel]);
                        }

                        A = Dest;
                    }
                } break;

                case 2: // NOTE(vak): Top
                {
                    tr_u8* X = SourceRow;
                    tr_u8* B = PrevDestRow;

                    for (tr_usize Index = 0; Index < Decoded->Width; Index++)
                    {
                        tr_u8* Dest = DestRow + Index*Decoded->Channels;
                        tr_u8* X = SourceRow + Index*Decoded->Channels;

                        for (tr_usize Channel = 0; Channel < Decoded->Channels; Channel++)
                        {
                            Dest[Channel] = (tr_u8)(X[Channel] + B[Channel]);
                        }

                        B += LastRowAdvance;
                    }
                } break;

                case 3: // NOTE(vak): Average
                {
                    tr_u8* X = SourceRow;
                    tr_u8* A = (tr_u8*)&Zero;
                    tr_u8* B = PrevDestRow;

                    for (tr_usize Index = 0; Index < Decoded->Width; Index++)
                    {
                        tr_u8* Dest = DestRow + Index*Decoded->Channels;
                        tr_u8* X = SourceRow + Index*Decoded->Channels;

                        for (tr_usize Channel = 0; Channel < Decoded->Channels; Channel++)
                        {
                            Dest[Channel] = (tr_u8)(X[Channel] + (A[Channel] + B[Channel]) / 2);
                        }

                        A = Dest;
                        B += LastRowAdvance;
                    }
                } break;

                case 4: // NOTE(vak): Paeth Predictor
                {
                    tr_u8* X = SourceRow;
                    tr_u8* A = (tr_u8*)&Zero;
                    tr_u8* B = PrevDestRow;
                    tr_u8* C = (tr_u8*)&Zero;

                    for (tr_usize Index = 0; Index < Decoded->Width; Index++)
                    {
                        tr_u8* Dest = DestRow + Index*Decoded->Channels;
                        tr_u8* X = SourceRow + Index*Decoded->Channels;

                        for (tr_usize Channel = 0; Channel < Decoded->Channels; Channel++)
                        {
                            tr_s32 VA = (tr_s32)A[Channel];
                            tr_s32 VB = (tr_s32)B[Channel];
                            tr_s32 VC = (tr_s32)C[Channel];

                            tr_s32 P = VA + VB - VC;
                            tr_s32 PA = TR_ABS(P - VA);
                            tr_s32 PB = TR_ABS(P - VB);
                            tr_s32 PC = TR_ABS(P - VC);

                            tr_u8 Predict = (tr_u8)VC;

                            if ((PA <= PB) && (PA <= PC))
                                Predict = (tr_u8)VA;
                            else if (PB <= PC)
                                Predict = (tr_u8)VB;

                            Dest[Channel] = (tr_u8)(X[Channel] + Predict);
                        }

                        A = Dest;
                        C = B;
                        B += LastRowAdvance;
                    }
                } break;
            }

            LastRowAdvance = Decoded->Channels;
            PrevDestRow = DestRow;
        }
    }

    trSetArenaPosition(Arena, ResetArenaTo); // NOTE(vak): Deallocate everything except image memory (Decoded->Pixels)
    return (TR_TRUE);

DecodingFailed:

    trSetArenaPosition(Arena, InitialArenaPosition); // NOTE(vak): Deallocate everything
    return (TR_FALSE);

    #undef TR_PNG_CHECK
}

/////////////////////////////////////////////////
// NOTE(vak): CRT stuff
/////////////////////////////////////////////////

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

