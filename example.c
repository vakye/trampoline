
#include "tr.h"

void Setup(void)
{
    tr_str Message = trStr("Hello, world!\n");

    trWriteStdOut(Message.Data, Message.Size);
}

#include "tr.c"

