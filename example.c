
#include "tr.h"

void Setup(void)
{
    trPrintln(TR_STR("Hello, world!"));

    trPrintUSize(1337);
    trPrintNewLine();

    trPrintSSize(-1234);
    trPrintNewLine();

    trPrintF64(-124874.23);
    trPrintNewLine();
}

#include "tr.c"

