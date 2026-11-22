
#include "tr.h"

#define SPACES_PER_TAB      (4)
#define SCROLL_OFF_LINES    (8)

typedef enum
{
    MODIFIER_FLAG_NONE      = 0,
    MODIFIER_FLAG_SHIFT     = (1 << 0),
    MODIFIER_FLAG_ALT       = (1 << 1),
    MODIFIER_FLAG_CTRL      = (1 << 2),
} modifier_flags;

typedef enum
{
    EDITOR_MODE_NORMAL = 0,
    EDITOR_MODE_INSERT,
} editor_mode;

typedef struct
{
    modifier_flags ModifierFlags;
    editor_mode EditorMode;

    tr_u32 Cursor;
    tr_u32 Mark;
    tr_u32 PreferredColumn;

    tr_u32 BufferSize;
    tr_u8 Buffer[512 * 1024];

    // NOTE(vak): View Panning

    tr_u32 ScrolledLines;

    tr_f32 ViewOffsetX;
    tr_f32 ViewOffsetY;

    tr_f32 ViewTargetOffsetX;
    tr_f32 ViewTargetOffsetY;

    // NOTE(vak): Cursor positions

    tr_f32 CursorOffsetX;
    tr_f32 CursorOffsetY;

    tr_f32 CursorTargetOffsetX;
    tr_f32 CursorTargetOffsetY;

    tr_f32 MarkOffsetX;
    tr_f32 MarkOffsetY;

    tr_f32 MarkTargetOffsetX;
    tr_f32 MarkTargetOffsetY;
} application;

// NOTE(vak): Buffer

static void InsertCharacterAt(tr_u32 Offset, char Character);
static void InsertStringAt(tr_u32 Offset, tr_str String);
static void DeleteCharacterAt(tr_u32 Offset);
static void DeleteRangeAt(tr_u32 Offset, tr_u32 Size);
static tr_u32 SeekLineStartAt(tr_u32 Offset);
static tr_u32 SeekLineEndAt(tr_u32 Offset);

// NOTE(vak): Helpers

static void UpdateCursorPreferredColumn(void);
static void InsertCharacterAtCursor(char Character);
static void ClampCursorAndMark(void);

static tr_b32 IsWhitespace(char Character);
static tr_b32 IsAlphabet(char Character);
static tr_b32 IsDigit(char Character);
static tr_b32 IsPunctuation(char Character);

// NOTE(vak): Actions

static void ToggleEditorMode(void);
static void ToggleEditorModeAndNewLineAbove(void);
static void ToggleEditorModeAndNewLineBelow(void);

static void AnchorMarkAtCursor(void);
static void DeleteMarkedRange(void);

static void MoveCursorLeft(void);
static void MoveCursorRight(void);
static void MoveCursorUp(void);
static void MoveCursorDown(void);

static void MoveCursorLineStart(void);
static void MoveCursorLineEnd(void);

static void MoveCursorPrevParagraph(void);
static void MoveCursorNextParagraph(void);

static void MoveCursorPrevWord(void);
static void MoveCursorNextWord(void);

static void DeleteBackward(void);
static void DeleteForward(void);
static void DeleteCurrentLine(void);

static void StartNewLine(void);
static void InsertTabulation(void);

static application App = {0};

void Setup(void)
{
    trSetTargetFPS(120.0f);

    tr_loaded_file LoadedFile = {0};
    if (trLoadFile(&LoadedFile, trStr("example_editor.c")))
    {
        TR_ASSERT(LoadedFile.Size <= sizeof(App.Buffer));

        App.BufferSize = LoadedFile.Size;

        trCopyMemory(App.Buffer, LoadedFile.Data, LoadedFile.Size);

        trUnloadFile(&LoadedFile);
    }
}

static void EditorNormalModeOnKeyDown(tr_key Key)
{
    if (App.ModifierFlags == MODIFIER_FLAG_NONE)
    {
        // NOTE(vak): No modifiers down

        switch (Key)
        {
            default: break;

            case TR_KEY_O:              MoveCursorUp(); break;
            case TR_KEY_K:              MoveCursorLeft(); break;
            case TR_KEY_L:              MoveCursorDown(); break;
            case TR_KEY_SEMICOLON:      MoveCursorRight(); break;

            case TR_KEY_1:              MoveCursorLineStart(); break;
            case TR_KEY_2:              MoveCursorLineEnd(); break;
        }
    }
    else if (App.ModifierFlags == MODIFIER_FLAG_SHIFT)
    {
        // NOTE(vak): *Only* shift down

        switch (Key)
        {
            default: break;

            case TR_KEY_O:              MoveCursorPrevParagraph(); break;
            case TR_KEY_L:              MoveCursorNextParagraph(); break;
            case TR_KEY_K:              MoveCursorPrevWord(); break;
            case TR_KEY_SEMICOLON:      MoveCursorNextWord(); break;

            case TR_KEY_Q:              ToggleEditorMode(); break;
            case TR_KEY_W:              ToggleEditorModeAndNewLineBelow(); break;
            case TR_KEY_E:              ToggleEditorModeAndNewLineAbove(); break;

            case TR_KEY_D:              DeleteCurrentLine(); break;

            case TR_KEY_SPACE:          AnchorMarkAtCursor(); break;
        }
    }
    else if (App.ModifierFlags == MODIFIER_FLAG_CTRL)
    {
        // NOTE(vak): *Only* ctrl down

        switch (Key)
        {
            default: break;

            case TR_KEY_D:              DeleteMarkedRange(); break;
        }
    }
}

static void EditorInsertModeOnKeyDown(tr_key Key)
{
    if (App.ModifierFlags == MODIFIER_FLAG_NONE)
    {
        // NOTE(vak): No modifiers down

        switch (Key)
        {
            default: break;

            case TR_KEY_ENTER:          StartNewLine(); break;
            case TR_KEY_TAB:            InsertTabulation(); break;

            case TR_KEY_HOME:           MoveCursorLineStart(); break;
            case TR_KEY_END:            MoveCursorLineEnd(); break;
            case TR_KEY_BACKSPACE:      DeleteBackward(); break;
            case TR_KEY_DELETE:         DeleteForward(); break;
        }
    }
    else if (App.ModifierFlags == MODIFIER_FLAG_SHIFT)
    {
        // NOTE(vak): *Only* shift down

        switch (Key)
        {
            default: break;

            case TR_KEY_Q:              ToggleEditorMode(); break;
            case TR_KEY_ENTER:          StartNewLine(); break;
            case TR_KEY_BACKSPACE:      DeleteBackward(); break;
            case TR_KEY_TAB:            InsertTabulation(); break;
        }
    }

}

void Frame(tr_f32 LastFrameTime)
{
    // NOTE(vak): Only allow character typing if the editor
    // mode is set to EDITOR_MODE_INSERT at the start of the
    // frame. This is to prevent insertion when the editor mode
    // is switched to insert in a frame.
    tr_b32 InsertionAllowed = (App.EditorMode == EDITOR_MODE_INSERT);

    trSetClearColor(0.0f, 0.0f, 0.0f, 0.5f);

    tr_u32 EventCount = trGetEventCount();

    for (tr_u32 Index = 0; Index < EventCount; Index++)
    {
        ClampCursorAndMark();

        tr_event Event = trGetEvent(Index);

        switch (Event.Kind)
        {
            default: break;

            case TR_EVENT_KIND_UNICODE:
            {
                // NOTE(vak): Prevent inserting when editor mode is
                // switched to normal in this frame.
                if (App.EditorMode == EDITOR_MODE_NORMAL)
                    break;

                if (!InsertionAllowed)
                    break;

                tr_u32 Codepoint = Event.Unicode.Codepoint;
                char Character = 0;

                if ((Codepoint >= 32) && (Codepoint <= 126))
                {
                    Character = Codepoint;
                }

                if (Character == 0) break;

                if (App.BufferSize < sizeof(App.Buffer))
                    InsertCharacterAtCursor(Character);
            } break;

            case TR_EVENT_KIND_KEY:
            {
                tr_key Key = Event.Key.Key;
                tr_b32 IsDown = Event.Key.IsDown;

                // NOTE(vak): Update modifier flags.

                {
                    modifier_flags Modifier = MODIFIER_FLAG_NONE;

                    switch (Key)
                    {
                        default: break;

                        case TR_KEY_SHIFT_LEFT: case TR_KEY_SHIFT_RIGHT:
                            Modifier = MODIFIER_FLAG_SHIFT;
                            break;

                        case TR_KEY_ALT_LEFT: case TR_KEY_ALT_RIGHT:
                            Modifier = MODIFIER_FLAG_ALT;
                            break;

                        case TR_KEY_CTRL_LEFT: case TR_KEY_CTRL_RIGHT:
                            Modifier = MODIFIER_FLAG_CTRL;
                            break;
                    }

                    App.ModifierFlags &= ~Modifier;
                    App.ModifierFlags |= IsDown * Modifier;
                }

                // NOTE(vak): Perform actions corresponding to key bindings.
                // and editor mode

                if (IsDown)
                {
                    switch (App.EditorMode)
                    {
                        case EDITOR_MODE_NORMAL:    EditorNormalModeOnKeyDown(Key); break;
                        case EDITOR_MODE_INSERT:    EditorInsertModeOnKeyDown(Key); break;
                    }
                }
            } break;
        }
    }

    ClampCursorAndMark();

    tr_str BufferText = trStrData((char*)App.Buffer, App.BufferSize);

    // NOTE(vak): Cursor animation

    {
        App.CursorTargetOffsetX = trGetGlyphOffsetX(BufferText, App.Cursor);
        App.CursorTargetOffsetY = trGetGlyphOffsetY(BufferText, App.Cursor);

        App.CursorOffsetX += 0.5f * (App.CursorTargetOffsetX - App.CursorOffsetX);
        App.CursorOffsetY += 0.5f * (App.CursorTargetOffsetY - App.CursorOffsetY);
    }

    {
        App.MarkTargetOffsetX = trGetGlyphOffsetX(BufferText, App.Mark);
        App.MarkTargetOffsetY = trGetGlyphOffsetY(BufferText, App.Mark);

        App.MarkOffsetX += 0.5f * (App.MarkTargetOffsetX - App.MarkOffsetX);
        App.MarkOffsetY += 0.5f * (App.MarkTargetOffsetY - App.MarkOffsetY);
    }

    // NOTE(vak): Perform horizontal scrolling
    {
        tr_f32 CursorRelativeX = App.CursorTargetOffsetX;

        tr_f32 WindowSizeX = trGetWindowWidth();
        tr_f32 Threshold = TR_MIN(WindowSizeX, 30.0f); // NOTE(vak): About same size as cursor

        tr_f32 Left     = App.ViewTargetOffsetX;
        tr_f32 Right    = Left + trGetWindowWidth();

        tr_f32 DistanceToLeft   = CursorRelativeX - Left;
        tr_f32 DistanceToRight  = Right - CursorRelativeX - Threshold;

        if (DistanceToLeft < Threshold)
        {
            App.ViewTargetOffsetX -= Threshold - DistanceToLeft;
        }
        else if (DistanceToRight < Threshold)
        {
            App.ViewTargetOffsetX += Threshold - DistanceToRight;
        }

        App.ViewTargetOffsetX = TR_MAX(0.0f, App.ViewTargetOffsetX);
    }

    // NOTE(vak): Perform vertical scrolling
    {
        tr_f32 CursorRelativeY = App.CursorTargetOffsetY;

        tr_f32 LineHeight = trGetTextLineHeight();
        tr_u32 LinesInView = (tr_u32)(trGetWindowHeight() / LineHeight);

        tr_u32 ScrolloffLines = SCROLL_OFF_LINES;

        if (ScrolloffLines >= LinesInView)
            ScrolloffLines = 1;

        if (LinesInView == 0)
            ScrolloffLines = 0;

        tr_u32 ScrolloffThreshold = ScrolloffLines * LineHeight;

        tr_f32 Top      = App.ScrolledLines * LineHeight;
        tr_f32 Bottom   = Top + trGetWindowHeight();

        tr_f32 DistanceToTop    = CursorRelativeY - Top;
        tr_f32 DistanceToBottom = Bottom - CursorRelativeY;

        if (DistanceToTop < ScrolloffThreshold)
        {
            tr_u32 OffsetToScrollUp = (ScrolloffThreshold - DistanceToTop);
            tr_u32 LinesToScrollUp = (tr_u32)(OffsetToScrollUp / LineHeight + 0.5f);

            if (App.ScrolledLines < LinesToScrollUp)
                App.ScrolledLines = 0;
            else
                App.ScrolledLines -= LinesToScrollUp;
        }
        else if (DistanceToBottom < ScrolloffThreshold)
        {
            tr_u32 OffsetToScrollDown = (ScrolloffThreshold - DistanceToBottom);
            tr_u32 LinesToScrollDown = (tr_u32)(OffsetToScrollDown / LineHeight + 0.5f);

            App.ScrolledLines += LinesToScrollDown;
        }

        App.ViewTargetOffsetY = App.ScrolledLines * LineHeight;
    }

    // NOTE(vak): Do view panning animation
    {
        App.ViewOffsetX += 0.2f * (App.ViewTargetOffsetX - App.ViewOffsetX);
        App.ViewOffsetY += 0.2f * (App.ViewTargetOffsetY - App.ViewOffsetY);
    }

    tr_f32 EditorOffsetX = 4 - App.ViewOffsetX;
    tr_f32 EditorOffsetY = 4 - App.ViewOffsetY;

    // NOTE(vak): Draw editor text

    trText2D(
        BufferText,
        EditorOffsetX, EditorOffsetY,
        0.9f, 0.9f, 0.9f, 1.0f
    );

    // NOTE(vak): Draw editor cursor

    if (App.EditorMode == EDITOR_MODE_NORMAL)
    {
        tr_f32 MarkR = 0.8f;
        tr_f32 MarkG = 0.9f;
        tr_f32 MarkB = 0.7f;
        tr_f32 MarkA = 0.9f;

        tr_f32 CursorR = 0.9f;
        tr_f32 CursorG = 0.4f;
        tr_f32 CursorB = 0.5f;
        tr_f32 CursorA = 0.9f;

        tr_u32 LowerOffset = TR_MIN(App.Mark, App.Cursor);
        tr_u32 UpperOffset = TR_MAX(App.Mark, App.Cursor);

        tr_f32 LowerMinX = EditorOffsetX;
        tr_f32 LowerMinY = EditorOffsetY;

        tr_f32 UpperMinX = EditorOffsetX;
        tr_f32 UpperMinY = EditorOffsetY;

        if (LowerOffset == App.Mark)
        {
            LowerMinX += App.MarkOffsetX;
            LowerMinY += App.MarkOffsetY;

            UpperMinX += App.CursorOffsetX;
            UpperMinY += App.CursorOffsetY;
        }
        else
        {
            UpperMinX += App.MarkOffsetX;
            UpperMinY += App.MarkOffsetY;

            LowerMinX += App.CursorOffsetX;
            LowerMinY += App.CursorOffsetY;
        }

        tr_f32 LowerR = 0.0f;
        tr_f32 LowerG = 0.0f;
        tr_f32 LowerB = 0.0f;
        tr_f32 LowerA = 0.0f;

        tr_f32 UpperR = 0.0f;
        tr_f32 UpperG = 0.0f;
        tr_f32 UpperB = 0.0f;
        tr_f32 UpperA = 0.0f;

        if (LowerOffset == App.Mark)
        {
            LowerR = MarkR;
            LowerG = MarkG;
            LowerB = MarkB;
            LowerA = MarkA;

            UpperR = CursorR;
            UpperG = CursorG;
            UpperB = CursorB;
            UpperA = CursorA;
        }
        else
        {
            UpperR = MarkR;
            UpperG = MarkG;
            UpperB = MarkB;
            UpperA = MarkA;

            LowerR = CursorR;
            LowerG = CursorG;
            LowerB = CursorB;
            LowerA = CursorA;
        }

        tr_f32 BarSizeX = 6.0f;
        tr_f32 BarSizeY = 33.0f;

        tr_f32 UnderSizeX = 10.0f;
        tr_f32 UnderSizeY = 6.0f;

        trRect2D(
            LowerMinX,
            LowerMinY,
            LowerMinX + BarSizeX,
            LowerMinY + BarSizeY,
            LowerR, LowerG, LowerB, LowerA
        );

        trRect2D(
            LowerMinX + BarSizeX,
            LowerMinY,
            LowerMinX + BarSizeX + UnderSizeX,
            LowerMinY + UnderSizeY,
            LowerR, LowerG, LowerB, LowerA
        );

        trRect2D(
            UpperMinX,
            UpperMinY,
            UpperMinX + BarSizeX,
            UpperMinY + BarSizeY,
            UpperR, UpperG, UpperB, UpperA
        );

        trRect2D(
            UpperMinX,
            UpperMinY + BarSizeY - UnderSizeY,
            UpperMinX - UnderSizeX,
            UpperMinY + BarSizeY,
            UpperR, UpperG, UpperB, UpperA
        );
    }
    else if (App.EditorMode == EDITOR_MODE_INSERT)
    {
        tr_f32 CursorSizeX = 5.0f;
        tr_f32 CursorSizeY = 33.0f;

        tr_f32 CursorR = 0.3f;
        tr_f32 CursorG = 0.6f;
        tr_f32 CursorB = 0.9f;
        tr_f32 CursorA = 0.9f;

        tr_f32 CursorMinX = EditorOffsetX + App.CursorOffsetX;
        tr_f32 CursorMinY = EditorOffsetY + App.CursorOffsetY;
        tr_f32 CursorMaxX = CursorMinX + CursorSizeX;
        tr_f32 CursorMaxY = CursorMinY + CursorSizeY;

        trRect2D(
            CursorMinX, CursorMinY,
            CursorMaxX, CursorMaxY,
            CursorR, CursorG, CursorB, CursorA
        );
    }
}

// NOTE(vak): Buffer implementation

static void InsertCharacterAt(tr_u32 Offset, char Character)
{
    TR_ASSERT(App.BufferSize < TR_ARRAY_COUNT(App.Buffer));
    TR_ASSERT(Offset <= App.BufferSize);

    trMoveMemory(
        App.Buffer + Offset + 1,
        App.Buffer + Offset,
        App.BufferSize - Offset
    );

    App.Buffer[Offset] = Character;

    App.BufferSize++;
}

static void InsertStringAt(tr_u32 Offset, tr_str String)
{
    TR_ASSERT(App.BufferSize + String.Size <= TR_ARRAY_COUNT(App.Buffer));
    TR_ASSERT(Offset <= App.BufferSize);

    trMoveMemory(
        App.Buffer + Offset + String.Size,
        App.Buffer + Offset,
        App.BufferSize - Offset
    );

    trCopyMemory(
        App.Buffer + Offset,
        String.Data,
        String.Size
    );

    App.BufferSize += String.Size;
}

static void DeleteCharacterAt(tr_u32 Offset)
{
    TR_ASSERT(Offset < App.BufferSize);

    trMoveMemory(
        App.Buffer + Offset,
        App.Buffer + Offset + 1,
        App.BufferSize - Offset - 1
    );

    App.BufferSize -= 1;
}

static void DeleteRangeAt(tr_u32 Offset, tr_u32 Size)
{
    TR_ASSERT(Offset + Size <= App.BufferSize);

    trMoveMemory(
        App.Buffer + Offset,
        App.Buffer + Offset + Size,
        App.BufferSize - Offset - Size
    );

    App.BufferSize -= Size;
}

static tr_u32 SeekLineStartAt(tr_u32 Offset)
{
    TR_ASSERT(Offset <= App.BufferSize);

    tr_u32 Result = Offset;

    while (Result > 0)
    {
        if (App.Buffer[Result - 1] == '\n')
            break;

        Result--;
    }

    return (Result);
}

static tr_u32 SeekLineEndAt(tr_u32 Offset)
{
    TR_ASSERT(Offset <= App.BufferSize);

    tr_u32 Result = Offset;

    while (Result < App.BufferSize)
    {
        if (App.Buffer[Result] == '\n')
            break;

        Result++;
    }

    return (Result);
}

// NOTE(vak): Helpers

static void InsertCharacterAtCursor(char Character)
{
    TR_ASSERT(App.Cursor <= App.BufferSize);

    if (App.Mark > App.Cursor)
        App.Mark++;

    InsertCharacterAt(App.Cursor, Character);
    MoveCursorRight();
}

static void UpdateCursorPreferredColumn(void)
{
    App.PreferredColumn = App.Cursor - SeekLineStartAt(App.Cursor);
}

static void ClampCursorAndMark(void)
{
    App.Cursor = TR_MIN(App.Cursor, App.BufferSize);
    App.Mark = TR_MIN(App.Mark, App.BufferSize);
}

static tr_b32 IsWhitespace(char Character)
{
    tr_b32 Result =
        (Character == ' ') ||
        (Character == '\t') ||
        (Character == '\r') ||
        (Character == '\n');

    return (Result);
}

static tr_b32 IsAlphabet(char Character)
{
    tr_b32 Result =
        ((Character >= 'a') && (Character <= 'z')) ||
        ((Character >= 'A') && (Character <= 'Z'));

    return (Result);
}

static tr_b32 IsDigit(char Character)
{
    tr_b32 Result = ((Character >= '0') && (Character <= '9'));
    return (Result);
}

static tr_b32 IsPunctuation(char Character)
{
    tr_b32 NotControlCode   = ((Character >= 32) && (Character <= 126));
    tr_b32 NotDigit         = !IsDigit(Character);
    tr_b32 NotAlphabet      = !IsAlphabet(Character);
    tr_b32 NotWhitespace    = !IsWhitespace(Character);

    tr_b32 Result =
        (NotControlCode) &&
        (NotDigit) &&
        (NotAlphabet) &&
        (NotWhitespace);

    return (Result);
}

// NOTE(vak): Actions implementation

static void ToggleEditorMode(void)
{
    if (App.EditorMode == EDITOR_MODE_NORMAL)
        App.EditorMode = EDITOR_MODE_INSERT;
    else
        App.EditorMode = EDITOR_MODE_NORMAL;
}

static void ToggleEditorModeAndNewLineAbove(void)
{
    ToggleEditorMode();
    MoveCursorLineStart();
    StartNewLine();
    MoveCursorUp();
}

static void ToggleEditorModeAndNewLineBelow(void)
{
    ToggleEditorMode();
    MoveCursorLineEnd();
    StartNewLine();
}

static void AnchorMarkAtCursor(void)
{
    App.Mark = App.Cursor;
}

static void DeleteMarkedRange(void)
{
    tr_u32 Lower = TR_MIN(App.Mark, App.Cursor);
    tr_u32 Upper = TR_MAX(App.Mark, App.Cursor);

    DeleteRangeAt(Lower, Upper - Lower);

    App.Mark = Lower;
    App.Cursor = Lower;

    UpdateCursorPreferredColumn();
}

static void MoveCursorLeft(void)
{
    if (App.Cursor > 0)
    {
        App.Cursor -= 1;
        UpdateCursorPreferredColumn();
    }
}

static void MoveCursorRight(void)
{
    if (App.Cursor < App.BufferSize)
    {
        App.Cursor += 1;
        UpdateCursorPreferredColumn();
    }
}

static void MoveCursorUp(void)
{
    tr_u32 ThisLineStart = SeekLineStartAt(App.Cursor);

    if (ThisLineStart > 0)
    {
        tr_u32 LastLineEnd = ThisLineStart - 1;
        tr_u32 LastLineStart = SeekLineStartAt(LastLineEnd);
        tr_u32 LastLineSize = LastLineEnd - LastLineStart;

        tr_u32 Column = TR_MIN(App.PreferredColumn, LastLineSize);

        App.Cursor = LastLineStart + Column;
    }
    else
    {
        App.Cursor = 0;
    }
}

static void MoveCursorDown(void)
{
    tr_u32 ThisLineEnd = SeekLineEndAt(App.Cursor);

    if (ThisLineEnd < App.BufferSize)
    {
        tr_u32 NextLineStart = ThisLineEnd + 1;
        tr_u32 NextLineEnd = SeekLineEndAt(NextLineStart);
        tr_u32 NextLineSize = NextLineEnd - NextLineStart;

        tr_u32 Column = TR_MIN(App.PreferredColumn, NextLineSize);

        App.Cursor = NextLineStart + Column;
    }
    else
    {
        App.Cursor = App.BufferSize;
    }
}

static void MoveCursorLineStart(void)
{
    App.Cursor = SeekLineStartAt(App.Cursor);

    UpdateCursorPreferredColumn();
}

static void MoveCursorLineEnd(void)
{
    App.Cursor = SeekLineEndAt(App.Cursor);

    UpdateCursorPreferredColumn();
}

static void MoveCursorPrevParagraph(void)
{
    // NOTE(vak): Skip blank lines

    while (App.Cursor > 0)
    {
        tr_b32 IsBlankLine = TR_TRUE;

        tr_u32 LineStart = SeekLineStartAt(App.Cursor);
        tr_u32 LineEnd = SeekLineEndAt(App.Cursor);

        for (tr_u32 Index = LineStart; Index < LineEnd; Index++)
        {
            char Character = App.Buffer[Index];

            if ((Character != ' ') && (Character != '\t'))
            {
                IsBlankLine = TR_FALSE;
                break;
            }
        }

        if (!IsBlankLine)
            break;

        MoveCursorUp();
    }

    // NOTE(vak): Skip non-blank lines

    while (App.Cursor > 0)
    {
        tr_b32 IsBlankLine = TR_TRUE;

        tr_u32 LineStart = SeekLineStartAt(App.Cursor);
        tr_u32 LineEnd = SeekLineEndAt(App.Cursor);

        for (tr_u32 Index = LineStart; Index < LineEnd; Index++)
        {
            char Character = App.Buffer[Index];

            if ((Character != ' ') && (Character != '\t'))
            {
                IsBlankLine = TR_FALSE;
                break;
            }
        }

        if (IsBlankLine)
            break;

        MoveCursorUp();
    }

    MoveCursorLineEnd();
}

static void MoveCursorNextParagraph(void)
{
    // NOTE(vak): Skip blank lines

    while (App.Cursor < App.BufferSize)
    {
        tr_b32 IsBlankLine = TR_TRUE;

        tr_u32 LineStart = SeekLineStartAt(App.Cursor);
        tr_u32 LineEnd = SeekLineEndAt(App.Cursor);

        for (tr_u32 Index = LineStart; Index < LineEnd; Index++)
        {
            char Character = App.Buffer[Index];

            if ((Character != ' ') && (Character != '\t'))
            {
                IsBlankLine = TR_FALSE;
                break;
            }
        }

        if (!IsBlankLine)
            break;

        MoveCursorDown();
    }

    // NOTE(vak): Skip non-blank lines

    while (App.Cursor < App.BufferSize)
    {
        tr_b32 IsBlankLine = TR_TRUE;

        tr_u32 LineStart = SeekLineStartAt(App.Cursor);
        tr_u32 LineEnd = SeekLineEndAt(App.Cursor);

        for (tr_u32 Index = LineStart; Index < LineEnd; Index++)
        {
            char Character = App.Buffer[Index];

            if ((Character != ' ') && (Character != '\t'))
            {
                IsBlankLine = TR_FALSE;
                break;
            }
        }

        if (IsBlankLine)
            break;

        MoveCursorDown();
    }

    MoveCursorLineEnd();
}

static void MoveCursorPrevWord(void)
{
    if (App.Cursor > 0)
    {
        char Character = App.Buffer[App.Cursor - 1];

        tr_b32 Punctuation  = IsPunctuation(Character);
        tr_b32 Whitespace   = IsWhitespace(Character);
        tr_b32 Alphabet     = IsAlphabet(Character);
        tr_b32 Digit        = IsDigit(Character);

        App.Cursor--;

        while (App.Cursor > 0)
        {
            Character = App.Buffer[App.Cursor - 1];

            tr_b32 KeepGoing = TR_FALSE;

            KeepGoing |= (Punctuation  && IsPunctuation(Character));
            KeepGoing |= (Whitespace   && IsWhitespace(Character));
            KeepGoing |= (Alphabet     && IsAlphabet(Character));
            KeepGoing |= (Digit        && IsDigit(Character));

            if (!KeepGoing)
                break;

            App.Cursor--;
        }

        UpdateCursorPreferredColumn();
    }
}

static void MoveCursorNextWord(void)
{
    if (App.Cursor < App.BufferSize)
    {
        char Character = App.Buffer[App.Cursor];

        tr_b32 Punctuation  = IsPunctuation(Character);
        tr_b32 Whitespace   = IsWhitespace(Character);
        tr_b32 Alphabet     = IsAlphabet(Character);
        tr_b32 Digit        = IsDigit(Character);

        while (App.Cursor < App.BufferSize)
        {
            Character = App.Buffer[App.Cursor];

            tr_b32 KeepGoing = TR_FALSE;

            KeepGoing |= (Punctuation  && IsPunctuation(Character));
            KeepGoing |= (Whitespace   && IsWhitespace(Character));
            KeepGoing |= (Alphabet     && IsAlphabet(Character));
            KeepGoing |= (Digit        && IsDigit(Character));

            if (!KeepGoing)
                break;

            App.Cursor++;
        }

        UpdateCursorPreferredColumn();
    }
}

static void DeleteBackward(void)
{
    if (App.Cursor > 0)
    {
        if (App.Mark >= App.Cursor)
            App.Mark--;

        MoveCursorLeft();
        DeleteCharacterAt(App.Cursor);
    }
}

static void DeleteForward(void)
{
    if (App.Cursor < App.BufferSize)
    {
        if (App.Mark >= App.Cursor)
            App.Mark--;

        DeleteCharacterAt(App.Cursor);
    }
}

static void DeleteCurrentLine(void)
{
    tr_u32 LineStart = SeekLineStartAt(App.Cursor);
    tr_u32 LineEnd = SeekLineEndAt(App.Cursor);

    // NOTE(vak): Include '\n' if not at end of buffer
    LineEnd = TR_MIN(LineEnd + 1, App.BufferSize);

    TR_ASSERT(LineStart <= LineEnd);

    DeleteRangeAt(LineStart, LineEnd - LineStart);

    // NOTE(vak): Configure mark position
    if ((LineStart <= App.Mark) && (App.Mark <= LineEnd))
        App.Mark = LineStart;
    else if ((LineEnd < App.Mark))
        App.Mark -= LineEnd - LineStart;

    // NOTE(vak): After deletion, 'LineStart' will be the
    // start of the next line before deletion. We will use
    // this to figure out the line that the cursor should
    // be on, and reposition the cursor correctly.

    LineEnd = SeekLineEndAt(LineStart);

    tr_u32 LineSize = LineEnd - LineStart;

    tr_u32 Column = TR_MIN(App.PreferredColumn, LineSize);

    App.Cursor = LineStart + Column;
}

static void StartNewLine(void)
{
    InsertCharacterAtCursor('\n');
}

static void InsertTabulation(void)
{
    tr_u32 Column = App.Cursor - SeekLineStartAt(App.Cursor);

    tr_u32 TabIndex = Column / SPACES_PER_TAB;
    tr_u32 SpacesToInsert = (TabIndex + 1) * SPACES_PER_TAB - Column;

    for (tr_u32 Index = 0; Index < SpacesToInsert; Index++)
        InsertCharacterAtCursor(' ');
}

#include "tr.c"

