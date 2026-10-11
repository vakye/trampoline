
#include "tr.h"

void Setup(void)
{
    tr_clock Begin = trGetWallClock();

    trPrintln(TR_STR("Hello, world!"));

    trPrintUSize(1337);
    trPrintNewLine();

    trPrintSSize(-1234);
    trPrintNewLine();

    trPrintF64(-124874.23);
    trPrintNewLine();

    tr_clock End = trGetWallClock();

    trPrint(TR_STR("Execution finished in "));
    trPrintF64(1000.0 * trGetSecondsElapsed(Begin, End));
    trPrint(TR_STR(" ms"));
    trPrintNewLine();
}

#include "tr.c"

