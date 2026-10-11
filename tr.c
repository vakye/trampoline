
#include <immintrin.h>

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

#include <dlfcn.h> // NOTE(vak): For loading libvulkan.so

#define TR_LINUX_SYSCALL_NR_WRITE           (1)
#define TR_LINUX_SYSCALL_NR_MMAP            (9)
#define TR_LINUX_SYSCALL_NR_MPROTECT        (10)
#define TR_LINUX_SYSCALL_NR_EXIT            (60)
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

tr_func void trLinuxWaylandSetup(void);
tr_func tr_b32 trLinuxWaylandShouldQuit(void);
tr_func void trLinuxWaylandPollEvents(void);
tr_func void trLinuxWaylandPresent(void);
tr_func tr_u32 trLinuxWaylandGetWindowWidth(void);
tr_func tr_u32 trLinuxWaylandGetWindowHeight(void);

tr_func void trVulkanSetup(void);
tr_func void trVulkanBeginFrame(void);
tr_func void trVulkanEndFrame(void);

void trLinuxBootstrap(tr_s32 ArgCount, char* Args[], char* Envp[])
{
    trLinuxWaylandSetup();
    trVulkanSetup();

    Setup();

    while (!trLinuxWaylandShouldQuit())
    {
        trLinuxWaylandPollEvents();
        trVulkanBeginFrame();

        Frame();

        trVulkanEndFrame();
        trLinuxWaylandPresent();
    }

    trLinuxSyscall(
        TR_LINUX_SYSCALL_NR_EXIT_GROUP, 0,
        0, 0, 0, 0, 0
    );
}

int main(tr_s32 ArgCount, char* Args[], char* Envp[])
{
    Setup();
    return (0);
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
#include "wayland_xdg.h"

typedef struct
{
    struct wl_display* Display;
    struct wl_registry* Registry;
    struct wl_compositor* Compositor;
    struct xdg_wm_base* XdgWmBase;
    struct wl_surface* Surface;
    struct xdg_surface* XdgSurface;
    struct xdg_toplevel* XdgTopLevel;

    tr_u32 TopLevelW, TopLevelH;
    tr_u32 WindowW, WindowH;
    tr_b32 Quit;
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
    X(vkCreateImageView) \
    X(vkDestroyImageView) \
    X(vkCreateSwapchainKHR) \
    X(vkDestroySwapchainKHR) \
    X(vkGetSwapchainImagesKHR) \
    X(vkAcquireNextImageKHR) \
    X(vkResetCommandBuffer) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    X(vkCmdPipelineBarrier) \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR)

#define TR_VULKAN_DECLARE(Name) PFN_##Name Name = 0;
    TR_VULKAN_ALL_FUNCTIONS(TR_VULKAN_DECLARE)
#undef TR_VULKAN_DECLARE

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

    VkExtent2D SwapchainExtent;
    VkSwapchainKHR Swapchain;

    tr_u32 MaxImageCount;
    tr_u32 ImageCount;
    VkImage* SwapchainImages;
    VkImageView* SwapchainImageViews;

    tr_u32 ImageIndex;
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

        VkDeviceCreateInfo DeviceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
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

    VkImageMemoryBarrier PresentBarrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = trVulkan.SwapchainImages[trVulkan.ImageIndex],
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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

