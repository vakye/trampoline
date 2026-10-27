
#include "tr.h"

typedef tr_u32 ui_id;

static ui_id ActiveElementID = 0;
static ui_id HoveringElementID = 0;

static tr_b32 ButtonUI(ui_id ID, tr_f32 X, tr_f32 Y, tr_f32 W, tr_f32 H, tr_f32 R, tr_f32 G, tr_f32 B, tr_f32 A)
{
    tr_f32 MouseX = trGetMouseX();
    tr_f32 MouseY = trGetMouseY();

    tr_b32 Hovering =
        (MouseX > X) &&
        (MouseY > Y) &&
        (MouseX < X + W) &&
        (MouseY < Y + H);

    if ((ActiveElementID != 0) && (ActiveElementID != ID))
        Hovering = TR_FALSE;

    tr_b32 Triggered = TR_FALSE;

    if (trIsButtonDown(TR_BUTTON_MOUSE_LEFT))
    {
        if (!ActiveElementID && Hovering)
            ActiveElementID = ID;
    }
    else
    {
        if (ActiveElementID == ID)
        {
            ActiveElementID = 0;
            Triggered = TR_TRUE;
        }
    }

    tr_b32 Pressed = (ActiveElementID == ID);

    tr_f32 Glow = 1.0f;

    if (Hovering) Glow = 1.25f;
    if (Pressed) Glow = 1.75f;

    trRect2D(X, Y, X+W, Y+H, R*Glow, G*Glow, B*Glow, A);

    if (Hovering)
        HoveringElementID = ID;

    return (Triggered);
}

void Setup(void)
{
}

void Frame(tr_f32 LastFrameTime)
{
    trResetArena(trGetTemporaryArena());

    {
        HoveringElementID = 0;

        tr_f32 X = 100.0f;
        tr_f32 Y = 100.0f;
        tr_f32 W = 100.0f;
        tr_f32 H = 40.0f;

        if (ButtonUI(1, X, Y, W, H, 0.9f, 0.4f, 0.5f, 1.0f))
            trPrintln(trStr("Pressed Red"));

        Y += 1.5f * H;

        if (ButtonUI(2, X, Y, W, H, 0.4f, 0.9f, 0.5f, 1.0f))
            trPrintln(trStr("Pressed Green"));

        Y += 1.5f * H;

        if (ButtonUI(3, X, Y, W, H, 0.4f, 0.5f, 0.9f, 1.0f))
            trPrintln(trStr("Pressed Blue"));
    }

    {
        tr_str Text = trFormatTemp(trStr(
            "MouseX: %f32\n"
            "MouseY: %f32\n"
            "HoveringElementID: %u32\n"
            "ActiveElementID: %u32"
        ), trGetMouseX(), trGetMouseY(), HoveringElementID, ActiveElementID);

        tr_f32 X = 10.0f;
        tr_f32 Y = trGetWindowHeight() - trMeasureTextHeight(Text) - 10.0f;

        trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

#include "tr.c"

