
#include "tr.h"

typedef struct
{
    tr_u32 Cursor;
    tr_u32 BufferSize;
    tr_u8 Buffer[2048];
} application;

static application App = {0};

void Setup(void)
{
    tr_str Text = trStr("Hello, world!\n");

    App.BufferSize = Text.Size;
    App.Cursor = Text.Size;

    trCopyMemory(App.Buffer, Text.Data, Text.Size);
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

            case TR_EVENT_KIND_QUIT:
            {
                trExit(0);
            } break;

            case TR_EVENT_KIND_RESIZE:
            {
                trPrintf(
                    trStr("Resized: %u32 x %u32\n"),
                    Event.Resize.SizeX,
                    Event.Resize.SizeY
                );
            } break;

            case TR_EVENT_KIND_KEY:
            {
                tr_key Key = Event.Key.Key;
                tr_b32 IsDown = Event.Key.IsDown;

                trPrintf(
                    trStr("Key: %u32, %str\n"),
                    Key,
                    IsDown ? trStr("Down") : trStr("Up")
                );

                if (!IsDown) break;

                if (Key == TR_KEY_LEFT)
                {
                    if (App.Cursor > 0)
                        App.Cursor -= 1;
                }
                else if (Key == TR_KEY_RIGHT)
                {
                    if (App.Cursor < App.BufferSize)
                        App.Cursor += 1;
                }
                else if (Key == TR_KEY_BACKSPACE)
                {
                    if (App.Cursor > 0)
                    {
                        App.Cursor -= 1;

                        trMoveMemory(
                            App.Buffer + App.Cursor,
                            App.Buffer + App.Cursor + 1,
                            App.BufferSize - App.Cursor - 1
                        );

                        App.BufferSize -= 1;
                    }
                }
                else if (Key == TR_KEY_DELETE)
                {
                    if (App.Cursor < App.BufferSize)
                    {
                        trMoveMemory(
                            App.Buffer + App.Cursor,
                            App.Buffer + App.Cursor + 1,
                            App.BufferSize - App.Cursor - 1
                        );

                        App.BufferSize -= 1;
                    }
                }
            } break;
        }
    }

    tr_usize TypedCount = trGetTypedCount();

    for (tr_usize Index = 0; Index < TypedCount; Index++)
    {
        char Character = trGetTypedCharacter(Index);

        if (App.BufferSize < sizeof(App.Buffer))
        {
            TR_ASSERT(App.Cursor <= App.BufferSize);

            trMoveMemory(
                App.Buffer + App.Cursor + 1,
                App.Buffer + App.Cursor,
                App.BufferSize - App.Cursor
            );

            App.Buffer[App.Cursor] = Character;

            App.BufferSize += 1;
            App.Cursor += 1;
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

    TR_ASSERT(App.Cursor <= App.BufferSize);

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

