
#define TR_CUSTOM_MAIN
#include "tr.h"

int main(int ArgCount, char* Args[])
{
    trSetup();

    tr_f64 FrameTime = 0.0f;

    trSetTargetFPS(120.0f);

    while (!trShouldQuit())
    {
        tr_clock FrameBegin = trGetWallClock();

        trBeginFrame();

        trSetClearColor(0.1f, 0.11f, 0.12f, 1.0f);
        trRect2D(100, 100, 200, 200, 1.0f, 0.8f, 0.5f, 1.0f);

        // TODO(vak): Actually implement a string formatting
        // function, so we don't have to do this complete mess...

        // NOTE(vak): Build FPS counter string

        char Buffer[512] = {0};
        tr_usize Count = 0;
        {
            tr_f64 FPS = 1.0 / FrameTime;
            tr_str Prefix = trStr("FPS: ");

            TR_ASSERT(Count + Prefix.Size <= sizeof(Buffer));
            trCopyMemory(Buffer + Count, Prefix.Data, Prefix.Size);
            Count += Prefix.Size;

            tr_u64 IntegerPart = (tr_u64)FPS;
            tr_u32 DigitCount = 0;
            do
            {
                DigitCount++;
                TR_ASSERT(Count + DigitCount <= sizeof(Buffer));

                Buffer[Count + DigitCount - 1] = '0' + (char)(IntegerPart % 10);
                IntegerPart /= 10;
            } while (IntegerPart);

            for (tr_usize Index = 0; Index <= DigitCount/2; Index++)
            {
                tr_usize A = Index;
                tr_usize B = DigitCount - Index - 1;
                char Temp = Buffer[Count + A];
                Buffer[Count + A] = Buffer[Count + B];
                Buffer[Count + B] = Temp;
            }

            Count += DigitCount;

            TR_ASSERT(Count < sizeof(Buffer));
            Buffer[Count++] = '.';

            tr_f64 DecimalPart = FPS - IntegerPart;
            tr_u32 DecimalDigits = 2;

            for (tr_usize Index = 0; Index < DecimalDigits; Index++)
            {
                TR_ASSERT(Count < sizeof(Buffer));

                tr_f64 Next = DecimalPart * 10.0;

                Buffer[Count++] = '0' + (char)((tr_u64)Next);
                DecimalPart = Next - (tr_s64)Next;
            }
        }

        tr_str Text = trStrData(Buffer, Count);

        tr_f32 X = trGetWindowWidth() - trMeasureTextWidth(Text) - 10.0f;
        tr_f32 Y = 10.0f;

        trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);

        trEndFrame();

        tr_clock FrameEnd = trGetWallClock();

        FrameTime = trGetSecondsElapsed(FrameBegin, FrameEnd);
    }

    return (0);
}

#include "tr.c"

