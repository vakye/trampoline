
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
    Arena->Used = 0;
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
    Entropy->State = trGetWallClock();
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
    tr_u32 Result = (V >> Count) | (V << (-Count & 31));

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

tr_func tr_f32 trSinF32(tr_f32 X)
{
    tr_s32 K = (tr_s32)(X * 2.0f);

    X = X - 0.5f*K;
    X = X * 6.28318530717958647693f; // NOTE(vak): 2PI

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

    tr_f32 XX = X * X;

    tr_f32 Y = Coefficients[4] * XX;
    Y = (Y + Coefficients[3]) * XX;
    Y = (Y + Coefficients[2]) * XX;
    Y = (Y + Coefficients[1]) * XX;
    Y = (Y + Coefficients[0]) * X;

    tr_f32 Negate = 1.0f - 2.0f*(K & 1);
    tr_f32 Result = Y * Negate;

    return (Result);
}

tr_func tr_f32 trCosF32(tr_f32 X)
{
    tr_s32 K = (tr_s32)(X * 2.0f);

    X = X - 0.5f*K;
    X = X * 6.28318530717958647693f; // NOTE(vak): 2PI

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

    tr_f32 XX = X * X;

    tr_f32 Y = Coefficients[5] * XX;
    Y = (Y + Coefficients[4]) * XX;
    Y = (Y + Coefficients[3]) * XX;
    Y = (Y + Coefficients[2]) * XX;
    Y = (Y + Coefficients[1]) * XX;
    Y = (Y + Coefficients[0]);

    tr_f32 Negate = 1.0f - 2.0f*(K & 1);
    tr_f32 Result = Y * Negate;

    return (Result);
}

tr_func tr_f32 trTanF32(tr_f32 X)
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
    #define TR_LINUX_SYSCALL_NR_WRITE           (1)
    #define TR_LINUX_SYSCALL_NR_MMAP            (9)
    #define TR_LINUX_SYSCALL_NR_MPROTECT        (10)
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

    tr_f32 MouseX, MouseY;

    tr_u32 TopLevelW, TopLevelH;
    tr_u32 WindowW, WindowH;
    tr_b32 Quit;

    tr_b32 IsDown[TR_BUTTON_MAX];
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

tr_func void trLinuxWaylandSeatCapabilities(
    void* Data,
    struct wl_seat* Seat,
    tr_u32 Capabilities
)
{
    if (trWayland.Pointer)
        wl_pointer_release(trWayland.Pointer);

    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
        trWayland.Pointer = wl_seat_get_pointer(Seat);

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
    trAssert(trWayland.Display);

    trWayland.Registry = wl_display_get_registry(trWayland.Display);
    trAssert(trWayland.Registry);

    static struct wl_registry_listener RegistryListener =
    {
        .global = trLinuxWaylandRegistryGlobal,
    };

    wl_registry_add_listener(trWayland.Registry, &RegistryListener, 0);

    wl_display_roundtrip(trWayland.Display);

    trAssert(trWayland.Compositor);
    trAssert(trWayland.XdgWmBase);

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
    trAssert(trWayland.Surface);

    trWayland.XdgSurface = xdg_wm_base_get_xdg_surface(trWayland.XdgWmBase, trWayland.Surface);
    trAssert(trWayland.XdgSurface);

    trWayland.XdgTopLevel = xdg_surface_get_toplevel(trWayland.XdgSurface);
    trAssert(trWayland.XdgTopLevel);

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
    X(vkCreateDescriptorSetLayout) \
    X(vkCreatePipelineLayout) \
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    X(vkCreateGraphicsPipelines) \
    X(vkCreateBuffer) \
    X(vkGetBufferMemoryRequirements) \
    X(vkBindBufferMemory) \
    X(vkAllocateMemory) \
    X(vkMapMemory) \
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
    X(vkCmdPushConstants) \
    X(vkCmdDraw) \
    X(vkCmdPipelineBarrier) \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR)

#define TR_VULKAN_DECLARE(Name) PFN_##Name Name = 0;
    TR_VULKAN_ALL_FUNCTIONS(TR_VULKAN_DECLARE)
#undef TR_VULKAN_DECLARE

typedef struct
{
    tr_f32 X, Y;    
    tr_f32 U, V;    
    tr_f32 R, G, B, A;
} tr_vulkan_vertex;

typedef struct
{
    tr_f32 Projection[16];
} tr_vulkan_push_constants;

typedef struct
{
    tr_arena Arena;

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

    VkDescriptorSetLayout SetLayout;
    VkPipelineLayout PipelineLayout;
    VkPipeline Pipeline;

    tr_usize VertexBufferSize;
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexMemory;
    tr_vulkan_vertex* MappedVertices;

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
        trAssert(__VulkanResult__ == VK_SUCCESS); \
    }

tr_func void trVulkanSetup(void)
{
    // NOTE(vak): Arena
    {
        trMakeArena(&trVulkan.Arena, TR_KB(64), TR_GB(4));
    }

    // NOTE(vak): Load non-instance functions
    {
    #if TR_PLATFORM_LINUX
        void* Library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);

        if (!Library)
            Library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);

        trAssert(Library);

        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)
            dlsym(Library, "vkGetInstanceProcAddr");

        vkCreateInstance = (PFN_vkCreateInstance)
            dlsym(Library, "vkCreateInstance");

        vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)
            dlsym(Library, "vkEnumerateInstanceVersion");
    #else
        #error Unimplemented vkGetInstanceProcAddr load for platform
    #endif

        trAssert(vkGetInstanceProcAddr);
        trAssert(vkCreateInstance);
        trAssert(vkEnumerateInstanceVersion);
    }

    // NOTE(vak): API Version
    {
        trVulkan.VersionOfAPI = VK_API_VERSION_1_4;

        tr_u32 Version = 0;
        TR_VK_CHECK(vkEnumerateInstanceVersion(&Version));

        trAssert(Version >= trVulkan.VersionOfAPI);
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
            trAssert(Name);

        TR_VULKAN_ALL_FUNCTIONS(TR_VULKAN_LOAD);

        #undef TR_VULKAN_LOAD
    }

    // NOTE(vak): Surface
    {
    #if TR_PLATFORM_LINUX
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)
            vkGetInstanceProcAddr(trVulkan.Instance, "vkCreateWaylandSurfaceKHR");

        trAssert(vkCreateWaylandSurfaceKHR);

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
        tr_usize ResetArenaTo = trGetArenaPosition(&trVulkan.Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkEnumeratePhysicalDevices(trVulkan.Instance, &Count, 0));

        VkPhysicalDevice* Array = trPushArenaArrayZero(&trVulkan.Arena, VkPhysicalDevice, Count);
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
        trAssert(trVulkan.PhysicalDevice);

        trSetArenaPosition(&trVulkan.Arena, ResetArenaTo);
    }

    // NOTE(vak): Queue family
    {
        tr_usize ResetArenaTo = trGetArenaPosition(&trVulkan.Arena);

        tr_u32 Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(trVulkan.PhysicalDevice, &Count, 0);

        VkQueueFamilyProperties* Array = trPushArenaArrayZero(&trVulkan.Arena, VkQueueFamilyProperties, Count);
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

        trAssert(trVulkan.QueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED);

        trSetArenaPosition(&trVulkan.Arena, ResetArenaTo);
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
        tr_usize ResetArenaTo = trGetArenaPosition(&trVulkan.Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, 0));

        VkSurfaceFormatKHR* Array = trPushArenaArrayZero(&trVulkan.Arena, VkSurfaceFormatKHR, Count);
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

        trAssert(trVulkan.SwapchainFormat.format != VK_FORMAT_UNDEFINED);

        trSetArenaPosition(&trVulkan.Arena, ResetArenaTo);
    }

    // NOTE(vak): Present mode
    {
        tr_usize ResetArenaTo = trGetArenaPosition(&trVulkan.Arena);

        tr_u32 Count = 0;
        TR_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &Count, 0));

        VkPresentModeKHR* Array = trPushArenaArrayZero(&trVulkan.Arena, VkPresentModeKHR, Count);
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

        trSetArenaPosition(&trVulkan.Arena, ResetArenaTo);
    }

    // NOTE(vak): Allocate swapchain image arrays
    {
        VkSurfaceCapabilitiesKHR SurfaceCaps = {0};
        TR_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(trVulkan.PhysicalDevice, trVulkan.Surface, &SurfaceCaps));

        trVulkan.MaxImageCount = SurfaceCaps.maxImageCount;

        trVulkan.SwapchainImages = trPushArenaArrayZero(&trVulkan.Arena, VkImage, trVulkan.MaxImageCount);
        trVulkan.SwapchainImageViews = trPushArenaArrayZero(&trVulkan.Arena, VkImageView, trVulkan.MaxImageCount);
    }

    // NOTE(vak): Set layout
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

        TR_VK_CHECK(vkCreateDescriptorSetLayout(trVulkan.Device, &SetLayoutInfo, 0, &trVulkan.SetLayout));
    }

    // NOTE(vak): Pipeline layout
    {
        VkPipelineLayoutCreateInfo LayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &trVulkan.SetLayout,
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
            0x07230203,0x00010600,0x0008000b,0x00000058,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000e,0x00000012,0x0000003f,
            0x00000043,0x00000052,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00040005,0x00000007,0x74726576,0x00007865,0x00040006,0x00000007,0x00000000,
            0x00000058,0x00040006,0x00000007,0x00000001,0x00000059,0x00040006,0x00000007,0x00000002,
            0x00000055,0x00040006,0x00000007,0x00000003,0x00000056,0x00040006,0x00000007,0x00000004,
            0x00000052,0x00040006,0x00000007,0x00000005,0x00000047,0x00040006,0x00000007,0x00000006,
            0x00000042,0x00040006,0x00000007,0x00000007,0x00000041,0x00030005,0x00000009,0x00000056,
            0x00040005,0x0000000a,0x74726576,0x00007865,0x00040006,0x0000000a,0x00000000,0x00000058,
            0x00040006,0x0000000a,0x00000001,0x00000059,0x00040006,0x0000000a,0x00000002,0x00000055,
            0x00040006,0x0000000a,0x00000003,0x00000056,0x00040006,0x0000000a,0x00000004,0x00000052,
            0x00040006,0x0000000a,0x00000005,0x00000047,0x00040006,0x0000000a,0x00000006,0x00000042,
            0x00040006,0x0000000a,0x00000007,0x00000041,0x00060005,0x0000000c,0x74726556,0x75427865,
            0x72656666,0x00000000,0x00060006,0x0000000c,0x00000000,0x74726556,0x73656369,0x00000000,
            0x00030005,0x0000000e,0x00000000,0x00060005,0x00000012,0x565f6c67,0x65747265,0x646e4978,
            0x00007865,0x00050005,0x0000001a,0x69736f50,0x6e6f6974,0x00000000,0x00050005,0x00000022,
            0x43786554,0x64726f6f,0x00000000,0x00040005,0x0000002c,0x6f6c6f43,0x00000072,0x00060005,
            0x0000003d,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000003d,0x00000000,
            0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000003d,0x00000001,0x505f6c67,0x746e696f,
            0x657a6953,0x00000000,0x00070006,0x0000003d,0x00000002,0x435f6c67,0x4470696c,0x61747369,
            0x0065636e,0x00070006,0x0000003d,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,
            0x00030005,0x0000003f,0x00000000,0x00060005,0x00000041,0x68737550,0x736e6f43,0x746e6174,
            0x00000073,0x00060006,0x00000041,0x00000000,0x6a6f7250,0x69746365,0x00006e6f,0x00030005,
            0x00000043,0x00000000,0x00070005,0x00000050,0x74726556,0x68537865,0x72656461,0x7074754f,
            0x00007475,0x00060006,0x00000050,0x00000000,0x43786554,0x64726f6f,0x00000000,0x00050006,
            0x00000050,0x00000001,0x6f6c6f43,0x00000072,0x00030005,0x00000052,0x0074754f,0x00050048,
            0x0000000a,0x00000000,0x00000023,0x00000000,0x00050048,0x0000000a,0x00000001,0x00000023,
            0x00000004,0x00050048,0x0000000a,0x00000002,0x00000023,0x00000008,0x00050048,0x0000000a,
            0x00000003,0x00000023,0x0000000c,0x00050048,0x0000000a,0x00000004,0x00000023,0x00000010,
            0x00050048,0x0000000a,0x00000005,0x00000023,0x00000014,0x00050048,0x0000000a,0x00000006,
            0x00000023,0x00000018,0x00050048,0x0000000a,0x00000007,0x00000023,0x0000001c,0x00040047,
            0x0000000b,0x00000006,0x00000020,0x00030047,0x0000000c,0x00000002,0x00040048,0x0000000c,
            0x00000000,0x00000018,0x00050048,0x0000000c,0x00000000,0x00000023,0x00000000,0x00030047,
            0x0000000e,0x00000018,0x00040047,0x0000000e,0x00000021,0x00000000,0x00040047,0x0000000e,
            0x00000022,0x00000000,0x00040047,0x00000012,0x0000000b,0x0000002a,0x00030047,0x0000003d,
            0x00000002,0x00050048,0x0000003d,0x00000000,0x0000000b,0x00000000,0x00050048,0x0000003d,
            0x00000001,0x0000000b,0x00000001,0x00050048,0x0000003d,0x00000002,0x0000000b,0x00000003,
            0x00050048,0x0000003d,0x00000003,0x0000000b,0x00000004,0x00030047,0x00000041,0x00000002,
            0x00040048,0x00000041,0x00000000,0x00000005,0x00050048,0x00000041,0x00000000,0x00000007,
            0x00000010,0x00050048,0x00000041,0x00000000,0x00000023,0x00000000,0x00030047,0x00000050,
            0x00000002,0x00040047,0x00000052,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,
            0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x000a001e,0x00000007,0x00000006,
            0x00000006,0x00000006,0x00000006,0x00000006,0x00000006,0x00000006,0x00000006,0x00040020,
            0x00000008,0x00000007,0x00000007,0x000a001e,0x0000000a,0x00000006,0x00000006,0x00000006,
            0x00000006,0x00000006,0x00000006,0x00000006,0x00000006,0x0003001d,0x0000000b,0x0000000a,
            0x0003001e,0x0000000c,0x0000000b,0x00040020,0x0000000d,0x0000000c,0x0000000c,0x0004003b,
            0x0000000d,0x0000000e,0x0000000c,0x00040015,0x0000000f,0x00000020,0x00000001,0x0004002b,
            0x0000000f,0x00000010,0x00000000,0x00040020,0x00000011,0x00000001,0x0000000f,0x0004003b,
            0x00000011,0x00000012,0x00000001,0x00040020,0x00000014,0x0000000c,0x0000000a,0x00040017,
            0x00000018,0x00000006,0x00000002,0x00040020,0x00000019,0x00000007,0x00000018,0x00040020,
            0x0000001b,0x00000007,0x00000006,0x0004002b,0x0000000f,0x0000001e,0x00000001,0x0004002b,
            0x0000000f,0x00000023,0x00000002,0x0004002b,0x0000000f,0x00000026,0x00000003,0x00040017,
            0x0000002a,0x00000006,0x00000004,0x00040020,0x0000002b,0x00000007,0x0000002a,0x0004002b,
            0x0000000f,0x0000002d,0x00000004,0x0004002b,0x0000000f,0x00000030,0x00000005,0x0004002b,
            0x0000000f,0x00000033,0x00000006,0x0004002b,0x0000000f,0x00000036,0x00000007,0x00040015,
            0x0000003a,0x00000020,0x00000000,0x0004002b,0x0000003a,0x0000003b,0x00000001,0x0004001c,
            0x0000003c,0x00000006,0x0000003b,0x0006001e,0x0000003d,0x0000002a,0x00000006,0x0000003c,
            0x0000003c,0x00040020,0x0000003e,0x00000003,0x0000003d,0x0004003b,0x0000003e,0x0000003f,
            0x00000003,0x00040018,0x00000040,0x0000002a,0x00000004,0x0003001e,0x00000041,0x00000040,
            0x00040020,0x00000042,0x00000009,0x00000041,0x0004003b,0x00000042,0x00000043,0x00000009,
            0x00040020,0x00000044,0x00000009,0x00000040,0x0004002b,0x00000006,0x00000048,0x00000000,
            0x0004002b,0x00000006,0x00000049,0x3f800000,0x00040020,0x0000004e,0x00000003,0x0000002a,
            0x0004001e,0x00000050,0x00000018,0x0000002a,0x00040020,0x00000051,0x00000003,0x00000050,
            0x0004003b,0x00000051,0x00000052,0x00000003,0x00040020,0x00000054,0x00000003,0x00000018,
            0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,
            0x00000008,0x00000009,0x00000007,0x0004003b,0x00000019,0x0000001a,0x00000007,0x0004003b,
            0x00000019,0x00000022,0x00000007,0x0004003b,0x0000002b,0x0000002c,0x00000007,0x0004003d,
            0x0000000f,0x00000013,0x00000012,0x00060041,0x00000014,0x00000015,0x0000000e,0x00000010,
            0x00000013,0x0004003d,0x0000000a,0x00000016,0x00000015,0x00040190,0x00000007,0x00000017,
            0x00000016,0x0003003e,0x00000009,0x00000017,0x00050041,0x0000001b,0x0000001c,0x00000009,
            0x00000010,0x0004003d,0x00000006,0x0000001d,0x0000001c,0x00050041,0x0000001b,0x0000001f,
            0x00000009,0x0000001e,0x0004003d,0x00000006,0x00000020,0x0000001f,0x00050050,0x00000018,
            0x00000021,0x0000001d,0x00000020,0x0003003e,0x0000001a,0x00000021,0x00050041,0x0000001b,
            0x00000024,0x00000009,0x00000023,0x0004003d,0x00000006,0x00000025,0x00000024,0x00050041,
            0x0000001b,0x00000027,0x00000009,0x00000026,0x0004003d,0x00000006,0x00000028,0x00000027,
            0x00050050,0x00000018,0x00000029,0x00000025,0x00000028,0x0003003e,0x00000022,0x00000029,
            0x00050041,0x0000001b,0x0000002e,0x00000009,0x0000002d,0x0004003d,0x00000006,0x0000002f,
            0x0000002e,0x00050041,0x0000001b,0x00000031,0x00000009,0x00000030,0x0004003d,0x00000006,
            0x00000032,0x00000031,0x00050041,0x0000001b,0x00000034,0x00000009,0x00000033,0x0004003d,
            0x00000006,0x00000035,0x00000034,0x00050041,0x0000001b,0x00000037,0x00000009,0x00000036,
            0x0004003d,0x00000006,0x00000038,0x00000037,0x00070050,0x0000002a,0x00000039,0x0000002f,
            0x00000032,0x00000035,0x00000038,0x0003003e,0x0000002c,0x00000039,0x00050041,0x00000044,
            0x00000045,0x00000043,0x00000010,0x0004003d,0x00000040,0x00000046,0x00000045,0x0004003d,
            0x00000018,0x00000047,0x0000001a,0x00050051,0x00000006,0x0000004a,0x00000047,0x00000000,
            0x00050051,0x00000006,0x0000004b,0x00000047,0x00000001,0x00070050,0x0000002a,0x0000004c,
            0x0000004a,0x0000004b,0x00000048,0x00000049,0x00050091,0x0000002a,0x0000004d,0x00000046,
            0x0000004c,0x00050041,0x0000004e,0x0000004f,0x0000003f,0x00000010,0x0003003e,0x0000004f,
            0x0000004d,0x0004003d,0x00000018,0x00000053,0x00000022,0x00050041,0x00000054,0x00000055,
            0x00000052,0x00000010,0x0003003e,0x00000055,0x00000053,0x0004003d,0x0000002a,0x00000056,
            0x0000002c,0x00050041,0x0000004e,0x00000057,0x00000052,0x0000001e,0x0003003e,0x00000057,
            0x00000056,0x000100fd,0x00010038
        };

        static tr_u32 FragmentCode[] =
        {
            // NOTE(vak): SPIRV compiled from vulkan_default.frag
            0x07230203,0x00010600,0x0008000b,0x00000013,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
            0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00060005,0x00000009,0x67617246,0x746e656d,0x6f6c6f43,0x00000072,0x00070005,
            0x0000000b,0x74726556,0x68537865,0x72656461,0x7074754f,0x00007475,0x00060006,0x0000000b,
            0x00000000,0x43786554,0x64726f6f,0x00000000,0x00050006,0x0000000b,0x00000001,0x6f6c6f43,
            0x00000072,0x00030005,0x0000000d,0x00006e49,0x00040047,0x00000009,0x0000001e,0x00000000,
            0x00030047,0x0000000b,0x00000002,0x00040047,0x0000000d,0x0000001e,0x00000000,0x00020013,
            0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
            0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,
            0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,0x00000002,0x0004001e,
            0x0000000b,0x0000000a,0x00000007,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,
            0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,0x00000001,0x0004002b,
            0x0000000e,0x0000000f,0x00000001,0x00040020,0x00000010,0x00000001,0x00000007,0x00050036,
            0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,
            0x00000011,0x0000000d,0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0003003e,
            0x00000009,0x00000012,0x000100fd,0x00010038
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
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
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

    // NOTE(vak): Vertex buffer
    {
        trVulkan.VertexBufferSize = TR_MB(4);

        VkBufferCreateInfo BufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = trVulkan.VertexBufferSize,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        TR_VK_CHECK(vkCreateBuffer(trVulkan.Device, &BufferInfo, 0, &trVulkan.VertexBuffer));

        VkMemoryRequirements MemoryRequirements = {0};
        vkGetBufferMemoryRequirements(trVulkan.Device, trVulkan.VertexBuffer, &MemoryRequirements);

        VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
        vkGetPhysicalDeviceMemoryProperties(trVulkan.PhysicalDevice, &MemoryProperties);

        tr_u32 MemoryTypeIndex = ~0u;

        for (tr_u32 Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
        {
            VkMemoryType* MemoryType = MemoryProperties.memoryTypes + Index;

            if ((MemoryRequirements.memoryTypeBits & (1 << Index)) == 0)
                continue;

            VkMemoryPropertyFlags DesiredFlags =
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

            if ((MemoryType->propertyFlags & DesiredFlags) == DesiredFlags)
            {
                MemoryTypeIndex = Index;
                break;
            }
        }

        trAssert(MemoryTypeIndex != ~0u);

        VkMemoryAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = MemoryRequirements.size,
            .memoryTypeIndex = MemoryTypeIndex,
        };

        TR_VK_CHECK(vkAllocateMemory(trVulkan.Device, &AllocateInfo, 0, &trVulkan.VertexMemory));
        TR_VK_CHECK(vkBindBufferMemory(trVulkan.Device, trVulkan.VertexBuffer, trVulkan.VertexMemory, 0));

        TR_VK_CHECK(vkMapMemory(
            trVulkan.Device,
            trVulkan.VertexMemory,
            0,
            trVulkan.VertexBufferSize,
            0,
            (void**) &trVulkan.MappedVertices
        ));
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
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
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

tr_func void trVertex2D(tr_f32 X, tr_f32 Y, tr_f32 U, tr_f32 V, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    tr_usize MaxVertexCount = trVulkan.VertexBufferSize / sizeof(tr_vulkan_vertex);

    trAssert(trVulkan.VertexCount < MaxVertexCount);

    tr_vulkan_vertex* Vertex = trVulkan.MappedVertices + trVulkan.VertexCount;

    Vertex->X = X;
    Vertex->Y = Y;

    Vertex->U = U;
    Vertex->V = V;

    Vertex->R = R;
    Vertex->G = G;
    Vertex->B = B;
    Vertex->A = A;

    trVulkan.VertexCount++;
}

tr_func void trRect2D(tr_f32 MinX, tr_f32 MinY, tr_f32 SizeX, tr_f32 SizeY, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    tr_f32 MaxX = MinX + SizeX;
    tr_f32 MaxY = MinY + SizeY;

    trVertex2D(MinX, MinY, 0.0f, 0.0f, R, G, B, A);
    trVertex2D(MinX, MaxY, 0.0f, 1.0f, R, G, B, A);
    trVertex2D(MaxX, MaxY, 1.0f, 1.0f, R, G, B, A);

    trVertex2D(MaxX, MaxY, 1.0f, 1.0f, R, G, B, A);
    trVertex2D(MaxX, MinY, 1.0f, 0.0f, R, G, B, A);
    trVertex2D(MinX, MinY, 0.0f, 0.0f, R, G, B, A);
}

tr_func void trCircle2D(tr_f32 CenterX, tr_f32 CenterY, tr_f32 Radius, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    tr_u32 PointCount = 32;

    tr_f32 FirstX = CenterX - Radius;
    tr_f32 FirstY = CenterY;

    trVertex2D(CenterX, CenterY, 0.0f, 0.0f, R, G, B, A);
    trVertex2D(FirstX,  FirstY,  0.0f, 0.5f, R, G, B, A);

    for (tr_u32 Index = 1; Index < PointCount; Index++)
    {
        tr_f32 T = (tr_f32)Index / (tr_f32)PointCount;

        tr_f32 C = trCosF32(T);
        tr_f32 S = trSinF32(T);

        tr_f32 U = 0.5f - 0.5f*C;
        tr_f32 V = 0.5f + 0.5f*S;

        tr_f32 X = CenterX - Radius*C;
        tr_f32 Y = CenterY + Radius*S;

        trVertex2D(X, Y, 0.0f, 0.0f, R, G, B, A);
        trVertex2D(CenterX, CenterY, 0.0f, 0.0f, R, G, B, A);
        trVertex2D(X, Y, 0.0f, 0.0f, R, G, B, A);
    }

    trVertex2D(FirstX, FirstY, 0.0f, 0.0f, R, G, B, A);
}

tr_func void trRectOutline2D(tr_f32 MinX, tr_f32 MinY, tr_f32 SizeX, tr_f32 SizeY, tr_f32 Thickness, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    tr_f32 MaxX = MinX + SizeX;
    tr_f32 MaxY = MinY + SizeY;

    tr_f32 HalfThickness = 0.5f*Thickness;

    // NOTE(vak): Left

    trRect2D(
        MinX - HalfThickness,
        MinY - HalfThickness,
        Thickness,
        SizeY + 2.0f*HalfThickness,
        R, G, B, A
    );

    // NOTE(vak): Right

    trRect2D(
        MaxX - HalfThickness,
        MinY - HalfThickness,
        Thickness,
        SizeY + 2.0f*HalfThickness,
        R, G, B, A
    );

    // NOTE(vak): Top

    trRect2D(
        MinX + HalfThickness,
        MinY - HalfThickness,
        SizeX - 2.0f*HalfThickness,
        Thickness,
        R, G, B, A
    );

    // NOTE(vak): Bottom

    trRect2D(
        MinX + HalfThickness,
        MaxY - HalfThickness,
        SizeX - 2.0f*HalfThickness,
        Thickness,
        R, G, B, A
    );
}

tr_func void trCircleOutline2D(tr_f32 CenterX, tr_f32 CenterY, tr_f32 Radius, tr_f32 Thickness, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
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

        trVertex2D(OutersX[Index0], OutersY[Index0], 0.0f, 0.0f, R, G, B, A);
        trVertex2D(InnersX[Index0], InnersY[Index0], 0.0f, 0.0f, R, G, B, A);
        trVertex2D(OutersX[Index1], OutersY[Index1], 0.0f, 0.0f, R, G, B, A);

        trVertex2D(OutersX[Index1], OutersY[Index1], 0.0f, 0.0f, R, G, B, A);
        trVertex2D(InnersX[Index0], InnersY[Index0], 0.0f, 0.0f, R, G, B, A);
        trVertex2D(InnersX[Index1], InnersY[Index1], 0.0f, 0.0f, R, G, B, A);
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
                        .buffer = trVulkan.VertexBuffer,
                        .offset = 0,
                        .range = trVulkan.VertexBufferSize,
                    },
                },
            };

            vkCmdPushDescriptorSet(
                trVulkan.CommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                trVulkan.PipelineLayout,
                0,
                TR_ARRAY_COUNT(DescriptorWrites),
                DescriptorWrites
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

