
#include "tr.h"

static tr_random Entropy = {0};
static tr_str FileText = {0};
static tr_str Text = {0};
static tr_f32 X = 0.0f;
static tr_f32 Y = 0.0f;
static tr_f32 DX = 0.0f;
static tr_f32 DY = 0.0f;
static tr_f32 Alpha = 0.0f;

void Setup(void)
{
    tr_loaded_file LoadedFile = trLoadFile(trStr("tr.c"));

    if (LoadedFile.Size)
    {
        FileText = trStrData(LoadedFile.Data, LoadedFile.Size);
    }

    trRandomSeed(&Entropy);

    Text = trStr("The quick brown fox jumps over the lazy dog");

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    X = 0.5f * (WindowW - trMeasureTextWidth(Text));
    Y = 0.5f * (WindowH - trMeasureTextHeight(Text));

    tr_f32 RandomAngle = trRandomF32(&Entropy, 0.0f, 1.0f);

    tr_f32 DirectionX = trCosF32(RandomAngle);
    tr_f32 DirectionY = trSinF32(RandomAngle);

    DX = DirectionX * 400.0f;
    DY = DirectionY * 400.0f;

    Alpha = 0.5f;
}

void Frame(tr_f32 LastFrameTime)
{
    tr_f32 SizeX = trMeasureTextWidth(Text);
    tr_f32 SizeY = trMeasureTextHeight(Text);

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_f32 HitNormalX = 0.0f;
    tr_f32 HitNormalY = 0.0f;

    if (X < 0.0f) { HitNormalX += 1.0f; X = 0.0f; }
    if (Y < 0.0f) { HitNormalY += 1.0f; Y = 0.0f; }
    if (X + SizeX > WindowW) { HitNormalX -= 1.0f; X = WindowW - SizeX; }
    if (Y + SizeY > WindowH) { HitNormalY -= 1.0f; Y = WindowH - SizeY; }

    if (HitNormalX || HitNormalY)
    {
        tr_f32 ChaosX = trRandomF32(&Entropy, -0.15f, 0.15f);
        tr_f32 ChaosY = trRandomF32(&Entropy, -0.15f, 0.15f);

        HitNormalX += HitNormalX*ChaosX;
        HitNormalY += HitNormalY*ChaosY;

        tr_f32 NormalizeLength = trSquareRootF32(
            HitNormalX*HitNormalX +
            HitNormalY*HitNormalY
        );

        HitNormalX /= NormalizeLength;
        HitNormalY /= NormalizeLength;

        tr_f32 DotProduct = DX*HitNormalX + DY*HitNormalY;

        DX = DX - 2.0f*DotProduct*HitNormalX;
        DY = DY - 2.0f*DotProduct*HitNormalY;

        Alpha = 1.0f;
    }

    Alpha += 2.0f*(0.5f - Alpha)*LastFrameTime;

    X += DX*LastFrameTime;
    Y += DY*LastFrameTime;

    static tr_f32 ScrollT = 0.0f;

    tr_f32 FileTextHeight = trMeasureTextHeight(FileText);
    tr_f32 ScrollSpeed = 50.0f;

    ScrollT += (ScrollSpeed / FileTextHeight) * LastFrameTime;
    ScrollT -= trFloorF32(ScrollT);

    tr_f32 ScrollY = (FileTextHeight - trGetTextLineHeight()) * trSinF32(ScrollT);

    trText2D(
        FileText,
        0.0f, -ScrollY,
        1.0f, 1.0f, 1.0f, 0.8f
    );

    trText2D(
        Text,
        X, Y,
        1.0f, 1.0f, 1.0f, Alpha
    );

}

#include "tr.c"

