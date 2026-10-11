
/////////////////////////////////////////////////
// NOTE(vak): Windows Implementation
/////////////////////////////////////////////////

#if TR_PLATFORM_WINDOWS
    #error Windows is not implemented yet

/////////////////////////////////////////////////
// NOTE(vak): Linux Implementation
/////////////////////////////////////////////////

#elif TR_PLATFORM_LINUX

#define TR_LINUX_SYSCALL_NR_WRITE       (1)
#define TR_LINUX_SYSCALL_NR_EXIT_GROUP  (231)

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

#else
    #error Unimplemented platform
#endif

