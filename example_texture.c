
#include "tr.h"

static tr_texture CheckerTexture = 0;

void Setup(void)
{
    tr_u32 CheckerImage[4 * 4] =
    {
        0xFFFFFFFF, 0x80000000, 0xFFFFFFFF, 0x80000000, 
        0x80000000, 0xFFFFFFFF, 0x80000000, 0xFFFFFFFF,
        0xFFFFFFFF, 0x80000000, 0xFFFFFFFF, 0x80000000, 
        0x80000000, 0xFFFFFFFF, 0x80000000, 0xFFFFFFFF,
    };

    CheckerTexture = trMakeTexture(4, 4);

    trUploadTexture(CheckerTexture, CheckerImage);
}

void Frame(tr_f32 LastFrameTime)
{
    trSetClearColor(0.1f, 0.12f, 0.14f, 1.0f);

    trTexturedRect2D(
        100, 100, 200, 200,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        CheckerTexture
    );
}

#include "tr.c"

