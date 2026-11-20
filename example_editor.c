
#include "tr.h"

#define SPACES_PER_TAB (4)

typedef struct
{
    tr_u32 Cursor;
    tr_u32 PreferredColumn;
    tr_u32 BufferSize;
    tr_u8 Buffer[2048];
} application;

static application App = {0};

void Setup(void)
{
}

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

static void DeleteCharacterAt(tr_u32 Offset)
{
    TR_ASSERT(App.BufferSize > 0);
    TR_ASSERT(Offset < App.BufferSize);

    trMoveMemory(
        App.Buffer + Offset,
        App.Buffer + Offset + 1,
        App.BufferSize - Offset - 1
    );

    App.BufferSize -= 1;
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

static void UpdateCursorPreferredColumn(void)
{
    App.PreferredColumn = App.Cursor - SeekLineStartAt(App.Cursor);
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
}

static void MoveCursorLineEnd(void)
{
    App.Cursor = SeekLineEndAt(App.Cursor);
}

static void InsertCharacterAtCursor(char Character)
{
    TR_ASSERT(App.Cursor <= App.BufferSize);

    InsertCharacterAt(App.Cursor, Character);
    MoveCursorRight();
}

void Frame(tr_f32 LastFrameTime)
{
    // NOTE(vak): Just in case a nasty oopsie happened.
    TR_ASSERT(App.Cursor <= App.BufferSize);

    trSetClearColor(0.0f, 0.0f, 0.0f, 0.5f);

    tr_u32 EventCount = trGetEventCount();

    for (tr_u32 Index = 0; Index < EventCount; Index++)
    {
        tr_event Event = trGetEvent(Index);

        switch (Event.Kind)
        {
            default: break;

            case TR_EVENT_KIND_UNICODE:
            {
                tr_u32 Codepoint = Event.Unicode.Codepoint;
                char Character = 0;

                if ((Codepoint >= 32) && (Codepoint <= 126))
                {
                    Character = Codepoint;
                }
                else if ((Codepoint == '\r') || (Codepoint == '\n'))
                {
                    Character = '\n';
                }
                else if (Codepoint == '\t')
                {
                    Character = '\t';
                }

                if (Character == 0) break;

                if (App.BufferSize < sizeof(App.Buffer))
                {
                    TR_ASSERT(App.Cursor <= App.BufferSize);

                    if (Character == '\t')
                    {
                        for (tr_u32 Index = 0; Index < SPACES_PER_TAB; Index++)
                            InsertCharacterAtCursor(' ');
                    }
                    else
                    {
                        InsertCharacterAtCursor(Character);
                    }
                }
            } break;

            case TR_EVENT_KIND_KEY:
            {
                tr_key Key = Event.Key.Key;
                tr_b32 IsDown = Event.Key.IsDown;

                if (!IsDown) break;

                if (Key == TR_KEY_LEFT)
                {
                    MoveCursorLeft();
                }
                else if (Key == TR_KEY_RIGHT)
                {
                    MoveCursorRight();
                }
                else if (Key == TR_KEY_UP)
                {
                    MoveCursorUp();
                }
                else if (Key == TR_KEY_DOWN)
                {
                    MoveCursorDown();
                }
                else if (Key == TR_KEY_HOME)
                {
                    MoveCursorLineStart();
                }
                else if (Key == TR_KEY_END)
                {
                    MoveCursorLineEnd();
                }
                else if (Key == TR_KEY_BACKSPACE)
                {
                    if (App.Cursor > 0)
                    {
                        MoveCursorLeft();
                        DeleteCharacterAt(App.Cursor);
                    }
                }
                else if (Key == TR_KEY_DELETE)
                {
                    if (App.Cursor < App.BufferSize)
                    {
                        DeleteCharacterAt(App.Cursor);
                    }
                }
            } break;
        }
    }

    tr_str BufferText = trStrData((char*)App.Buffer, App.BufferSize);

    tr_f32 PadX = 4;
    tr_f32 PadY = 4;

    trText2D(
        BufferText,
        PadX, PadY,
        0.9f, 0.9f, 0.9f, 1.0f
    );

    tr_f32 CursorSizeX = 15.0f;
    tr_f32 CursorSizeY = 33.0f;

    tr_f32 CursorMinX = PadX + trGetGlyphOffsetX(BufferText, App.Cursor);
    tr_f32 CursorMinY = PadY + trGetGlyphOffsetY(BufferText, App.Cursor);
    tr_f32 CursorMaxX = CursorMinX + CursorSizeX;
    tr_f32 CursorMaxY = CursorMinY + CursorSizeY;

    trRect2D(
        CursorMinX, CursorMinY,
        CursorMaxX, CursorMaxY,
        1.0f, 0.8f, 0.5f, 0.75f
    );
}

#include "tr.c"

