
#define TR_CUSTOM_MAIN
#include "tr.h"

int main(int ArgCount, char* Args[])
{
    trSetup();

    tr_f64 FrameTime = 0.0;

    trSetTargetFPS(120.0f);

    while (!trShouldQuit())
    {
        tr_clock FrameBegin = trGetWallClock();

        trBeginFrame();
        trResetArena(trGetTemporaryArena());

        trSetClearColor(0.1f, 0.11f, 0.12f, 1.0f);
        trRect2D(100, 100, 200, 200, 1.0f, 0.8f, 0.5f, 1.0f);

        tr_f64 FPS = (FrameTime > 0.0) ? (1.0f / FrameTime) : (0.0f);
        tr_str Text = trFormatTemp(trStr("FPS: %f64"), FPS);

        tr_f32 X = trGetWindowWidth() - trMeasureTextWidth(Text) - 10.0;
        tr_f32 Y = 10.0f;

        trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);

        trEndFrame();

        tr_clock FrameEnd = trGetWallClock();

        FrameTime = trGetSecondsElapsed(FrameBegin, FrameEnd);
    }

    return (0);
}

#include "tr.c"

