
#include "tr.h"

static tr_str Text = {0};
static tr_f32 X = 0.0f;
static tr_f32 Y = 0.0f;
static tr_f32 SizeX = 0.0f;
static tr_f32 SizeY = 0.0f;

void Setup(void)
{
    Text = trStr("Drag Me");

    X = 100.0f;
    Y = 100.0f;
    SizeX = 1.5f * trMeasureTextWidth(Text);
    SizeY = 1.5f * trMeasureTextHeight(Text);
}

void Frame(tr_f32 LastFrameTime)
{
    static tr_b32 Picking = TR_FALSE;
    static tr_f32 PickedRelativeX = 0.0f;
    static tr_f32 PickedRelativeY = 0.0f;

    tr_f32 MouseX = trGetMouseX();
    tr_f32 MouseY = trGetMouseY();

    tr_b32 Hovering =
        (MouseX > X) &&
        (MouseY > Y) &&
        (MouseX < (X + SizeX)) &&
        (MouseY < (Y + SizeY));

    if (trIsButtonDown(TR_BUTTON_MOUSE_LEFT))
    {
        if (Hovering && !Picking)
        {
            Picking = TR_TRUE;
            PickedRelativeX = MouseX - X;
            PickedRelativeY = MouseY - Y;
        }
    }
    else
    {
        Picking = TR_FALSE;
    }

    if (Picking)
    {
        tr_f32 RelativeX = MouseX - X;
        tr_f32 RelativeY = MouseY - Y;

        X += RelativeX - PickedRelativeX;
        Y += RelativeY - PickedRelativeY;

        Hovering = TR_TRUE;
    }

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    X = TR_MAX(X, 0);
    Y = TR_MAX(Y, 0);
    X = TR_MIN(X, WindowW - SizeX);
    Y = TR_MIN(Y, WindowH - SizeY);

    tr_f32 Glow = 0.8f + 0.1f*Hovering + 0.1f*Picking;

    trRectOutline2D(
        X, Y, SizeX, SizeY,
        10.0f,
        0.82f, 0.83f, 0.87f, 0.5f*Hovering + 0.3f*Picking
    );

    trRect2D(
        X, Y, SizeX, SizeY,
        0.78f*Glow, 0.73f*Glow, 0.83f*Glow, 1.0f
    );

    tr_f32 TextSizeX = trMeasureTextWidth(Text);
    tr_f32 TextSizeY = trMeasureTextHeight(Text);

    tr_f32 PadX = 0.5f*(SizeX - TextSizeX);
    tr_f32 PadY = 0.5f*(SizeY - TextSizeY);

    trText2D(
        Text,
        X + PadX, Y + PadY,
        0.95f, 0.92f, 0.87f, 1.0f
    );
}

#include "tr.c"

