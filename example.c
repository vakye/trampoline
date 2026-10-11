
#include "tr.h"

void Setup(void)
{
    tr_clock Begin = trGetWallClock();

    tr_arena Arena = {0};

    trMakeArena(&Arena, 0, TR_GB(64));

    tr_u32 Count = 65536;
    tr_u32* Stuff = trPushArenaArrayZero(&Arena, tr_u32, Count);

    for (tr_u32 Index = 0; Index < Count; Index++)
        Stuff[Index] = Index;

    tr_clock End = trGetWallClock();

    trPrint(TR_STR("Execution finished in "));
    trPrintF64(1000.0 * trGetSecondsElapsed(Begin, End));
    trPrint(TR_STR(" ms"));
    trPrintNewLine();
}

#include "tr.c"

