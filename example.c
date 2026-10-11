
#include "tr.h"

void Setup(void)
{
    tr_clock Begin = trGetWallClock();

    trPrintln(TR_STR("Hello, world!"));

    void* MyMemory = trReserveMemory(TR_GB(64));

    trPrint(TR_STR("Reserved memory at "));
    trPrintHex((tr_usize)(MyMemory));
    trPrintNewLine();

    trCommitMemory(MyMemory, TR_KB(4));

    trPrint(TR_STR("Commited 4kb at "));
    trPrintHex((tr_usize)(MyMemory));
    trPrintNewLine();

    tr_u32* Stuff = (tr_u32*)MyMemory;

    for (tr_u32 Index = 0; Index < TR_KB(4) / sizeof(tr_u32); Index++)
        Stuff[Index] = Index;

    tr_clock End = trGetWallClock();

    trPrint(TR_STR("Execution finished in "));
    trPrintF64(1000.0 * trGetSecondsElapsed(Begin, End));
    trPrint(TR_STR(" ms"));
    trPrintNewLine();
}

#include "tr.c"

