
#include "tr.h"

typedef tr_u32 ui_id;
#define UI_NIL_ID (0)

static void     UI_Setup        (void);
static void     UI_BeginWindow  (ui_id ID, tr_str Title, tr_f32 InitialX, tr_f32 InitialY);
static void     UI_Text         (ui_id ID, tr_str Text);
static tr_b32   UI_Button       (ui_id ID, tr_str Text);
static void     UI_EndWindow    (void);

void Setup(void)
{
    UI_Setup();
}

void Frame(tr_f32 LastFrameTime)
{
    trResetArena(trGetTemporaryArena());

    // NOTE(vak): Gives a nice opaque background on platforms
    // where transparent windows are supported (Linux wayland, ...)
    trSetClearColor(0.0f, 0.0f, 0.0f, 0.5f);

    ui_id ID = 1;
    static tr_u32 Counter = 0;

    UI_BeginWindow(ID++, trStr("My window"), 100, 100);

    UI_Text(ID++, trStr("Click the button below to increment counter!"));

    if (UI_Button(ID++, trFormatTemp(trStr("Click here (%u32)"), Counter)))
        Counter++;

    UI_EndWindow();
}

typedef union
{
    struct { tr_u32 U32; };
    struct { tr_u8 B, G, R, A; }; // NOTE(vak): Organized this way so you can write hex colors for U32
} ui_rgba_u8;

typedef struct
{
    tr_b32 Initialized;
    tr_str Text;
    tr_f32 X;
    tr_f32 Y;
    tr_f32 SizeX;
    tr_f32 SizeY;
} ui_element;

typedef struct
{
    ui_id HotElementID;
    ui_id ActiveElementID;

    tr_f32 PadX;
    tr_f32 PadY;

    ui_id WindowID;
    tr_f32 CurrentX;
    tr_f32 CurrentY;

    ui_rgba_u8 Foreground;
    ui_rgba_u8 Background;
    ui_rgba_u8 ForegroundMild;
    ui_rgba_u8 BackgroundMild;

    ui_rgba_u8 WindowBorder;

    ui_rgba_u8 ForegroundButton;
    ui_rgba_u8 BackgroundButton;
    ui_rgba_u8 ForegroundButtonHot;
    ui_rgba_u8 BackgroundButtonHot;
    ui_rgba_u8 ForegroundButtonActive;
    ui_rgba_u8 BackgroundButtonActive;

    ui_element Elements[4096];
} ui_state;

static ui_state UIState = {0};

static void UI_Setup(void)
{
    UIState.PadX = 20.0f;
    UIState.PadY = 20.0f;

    UIState.Foreground.U32              = 0xFFF0F0F0;
    UIState.Background.U32              = 0xFF202028;
    UIState.ForegroundMild.U32          = 0xFFF0F0F0;
    UIState.BackgroundMild.U32          = 0xFF303038;

    UIState.WindowBorder.U32            = 0xFF587880;

    UIState.ForegroundButton.U32        = 0xFFF0F0F0;
    UIState.BackgroundButton.U32        = 0xFF404048;
    UIState.ForegroundButtonHot.U32     = 0xFFF0F0F0;
    UIState.BackgroundButtonHot.U32     = 0xFF585860;
    UIState.ForegroundButtonActive.U32  = 0xFFF0F0F0;
    UIState.BackgroundButtonActive.U32  = 0xFF686870;
}

static ui_element* UI_GetElement(ui_id ID)
{
    TR_ASSERT(ID > 0);
    TR_ASSERT(ID < TR_ARRAY_COUNT(UIState.Elements));

    ui_element* Element = UIState.Elements + ID;
    return (Element);
}

static void UI_BeginWindow(ui_id ID, tr_str Title, tr_f32 InitialX, tr_f32 InitialY)
{
    TR_ASSERT(UIState.WindowID == 0);

    ui_element* Element = UI_GetElement(ID);

    if (!Element->Initialized)
    {
        Element->Text = Title;
        Element->X = InitialX;
        Element->Y = InitialY;
        Element->SizeX = 700.0f;
        Element->SizeY = 400.0f;

        Element->Initialized = TR_TRUE;
    }

    ui_rgba_u8 Background = UIState.Background;
    ui_rgba_u8 ForegroundMild = UIState.ForegroundMild;
    ui_rgba_u8 BackgroundMild = UIState.BackgroundMild;
    ui_rgba_u8 Border = UIState.WindowBorder;

    tr_f32 Inv255 = 1.0f / 255.0f;

    // NOTE(vak): Border

    trRectOutline2D(
        Element->X,
        Element->Y,
        Element->X + Element->SizeX,
        Element->Y + Element->SizeY,
        8.0f,
        Border.R * Inv255,
        Border.G * Inv255,
        Border.B * Inv255,
        Border.A * Inv255
    );

    // NOTE(vak): Body Background

    trRect2D(
        Element->X,
        Element->Y,
        Element->X + Element->SizeX,
        Element->Y + Element->SizeY,
        Background.R * Inv255,
        Background.G * Inv255,
        Background.B * Inv255,
        Background.A * Inv255
    );

    // NOTE(vak): Title Bar Background

    tr_f32 TitleBarSizeY = trMeasureTextHeight(Element->Text) + 2*UIState.PadY;

    trRect2D(
        Element->X,
        Element->Y,
        Element->X + Element->SizeX,
        Element->Y + TitleBarSizeY,
        BackgroundMild.R * Inv255,
        BackgroundMild.G * Inv255,
        BackgroundMild.B * Inv255,
        BackgroundMild.A * Inv255
    );

    // NOTE(vak): Title Text

    trText2D(
        Element->Text,
        Element->X + UIState.PadX,
        Element->Y + 0.5f * (TitleBarSizeY - trMeasureTextHeight(Element->Text)),
        ForegroundMild.R * Inv255,
        ForegroundMild.G * Inv255,
        ForegroundMild.B * Inv255,
        ForegroundMild.A * Inv255
    );

    UIState.CurrentX = Element->X + UIState.PadX;
    UIState.CurrentY = Element->Y + UIState.PadY + TitleBarSizeY;

    UIState.WindowID = ID;
}

static void UI_EndWindow(void)
{
    UIState.WindowID = 0;
}

static void UI_Text(ui_id ID, tr_str Text)
{
    ui_element* Element = UI_GetElement(ID);

    if (!Element->Initialized)
        Element->Initialized = TR_TRUE;

    Element->Text = Text;
    Element->X = UIState.CurrentX;
    Element->Y = UIState.CurrentY;
    Element->SizeX = trMeasureTextWidth(Text);
    Element->SizeY = trMeasureTextHeight(Text);

    ui_rgba_u8 Foreground = UIState.Foreground;

    tr_f32 Inv255 = 1.0f / 255.0f;

    trText2D(
        Element->Text,
        Element->X,
        Element->Y,
        Foreground.R * Inv255,
        Foreground.G * Inv255,
        Foreground.B * Inv255,
        Foreground.A * Inv255
    );

    UIState.CurrentY += Element->SizeY + UIState.PadY;
}

static tr_b32 UI_Button(ui_id ID, tr_str Text)
{
    TR_ASSERT(UIState.WindowID);

    ui_element* Element = UI_GetElement(ID);

    if (!Element->Initialized)
        Element->Initialized = TR_TRUE;

    // NOTE(vak): Layout

    Element->Text = Text;
    Element->X = UIState.CurrentX;
    Element->Y = UIState.CurrentY;
    Element->SizeX = trMeasureTextWidth(Text) + 2*UIState.PadX;
    Element->SizeY = trMeasureTextHeight(Text) + 2*UIState.PadY;

    // NOTE(vak): Button pressing logic

    tr_b32 ButtonPressed = TR_FALSE;

    tr_f32 MouseX = trGetMouseX();
    tr_f32 MouseY = trGetMouseY();

    tr_b32 LeftDown     = trIsButtonDown(TR_BUTTON_MOUSE_LEFT);
    tr_b32 LeftPressed  = trIsButtonPressed(TR_BUTTON_MOUSE_LEFT);
    tr_b32 LeftReleased = trIsButtonReleased(TR_BUTTON_MOUSE_LEFT);

    tr_b32 Hovering =
        (MouseX > Element->X) &&
        (MouseY > Element->Y) &&
        (MouseX < Element->X + Element->SizeX) &&
        (MouseY < Element->Y + Element->SizeY);

    if ((UIState.HotElementID == ID) && (!Hovering))
        UIState.HotElementID = 0;

    if ((Hovering) && (!LeftDown))
        UIState.HotElementID = ID;

    if ((Hovering) && (LeftPressed) && (UIState.ActiveElementID == 0))
        UIState.ActiveElementID = ID;

    if ((LeftReleased) && (UIState.ActiveElementID == ID))
    {
        ButtonPressed = TR_TRUE;
        UIState.ActiveElementID = 0;
    }

    // NOTE(vak): Draw

    ui_rgba_u8 Foreground = UIState.ForegroundButton;
    ui_rgba_u8 Background = UIState.BackgroundButton;

    if (UIState.HotElementID == ID)
    {
        Foreground = UIState.ForegroundButtonHot;
        Background = UIState.BackgroundButtonHot;
    }

    if (UIState.ActiveElementID == ID)
    {
        Foreground = UIState.ForegroundButtonActive;
        Background = UIState.BackgroundButtonActive;
    }

    tr_f32 Inv255 = 1.0f / 255.0f;

    trRect2D(
        Element->X,
        Element->Y,
        Element->X + Element->SizeX,
        Element->Y + Element->SizeY,
        Background.R * Inv255,
        Background.G * Inv255,
        Background.B * Inv255,
        Background.A * Inv255
    );

    trText2D(
        Element->Text,
        Element->X + 0.5f * (Element->SizeX - trMeasureTextWidth(Text)),
        Element->Y + 0.5f * (Element->SizeY - trMeasureTextHeight(Text)),
        Foreground.R * Inv255,
        Foreground.G * Inv255,
        Foreground.B * Inv255,
        Foreground.A * Inv255
    );

    return (ButtonPressed);
}



#include "tr.c"

