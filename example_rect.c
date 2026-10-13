
#include "tr.h"

void Setup(void)
{
}

void Frame(tr_f32 LastFrameTime)
{
    trSetClearColor(0.07f, 0.08f, 0.1f, 1.0f);

    trRect2D(
        100, 100,
        100, 100,
        1.0f, 0.8f, 0.5f, 1.0f
    );
}

#include "tr.c"

