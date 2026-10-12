
#include "tr.h"

void Setup(void)
{
}

void Frame(tr_f32 DeltaTime)
{
    static tr_f32 W = 0.0f;

    trSetClearColor(0.07f, 0.08f, 0.1f, 1.0f);

    tr_f32 X = 400.0f + 200.0f*trCosF32(W);
    tr_f32 Y = 400.0f - 200.0f*trSinF32(W);

    trRect2D(
        X, Y,
        100, 100,
        1.0f, 0.8f, 0.5f, 1.0f
    );

    W += 0.25f*DeltaTime;
    W -= trFloorF32(W);
}

#include "tr.c"

