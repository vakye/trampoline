
#include "tr.h"

typedef struct
{
    tr_str FilePath;
    tr_str Artist;
    tr_texture Texture;
    tr_u32 Width;
    tr_u32 Height;
} gallery_entry;

static gallery_entry Entries[] =
{
    {trStaticStr("art1.png"), trStaticStr("n/a (pixiv 108016082)"), 0, 0, 0},
    {trStaticStr("art2.png"), trStaticStr("Homutan (pixiv 58914348)"), 0, 0, 0},
    {trStaticStr("art3.png"), trStaticStr("OOZ (pixiv 9763273)"), 0, 0, 0},
    {trStaticStr("test1.png"), trStaticStr("n/a"), 0, 0, 0},
    {trStaticStr("test2.png"), trStaticStr("n/a"), 0, 0, 0},
};

static tr_u32 CurrentEntryIndex = 0;

void Setup(void)
{
    // NOTE(vak): Setup gallery entries

    for (tr_usize Index = 0; Index < TR_ARRAY_COUNT(Entries); Index++)
    {
        gallery_entry* Entry = Entries + Index;

        Entry->Texture = trLoadTexture(Entry->FilePath);

        if (Entry->Texture)
        {
            Entry->Width = trGetTextureWidth(Entry->Texture);
            Entry->Height = trGetTextureHeight(Entry->Texture);
        }
    }
}

void Frame(tr_f32 LastFrameTime)
{
    trSetClearColor(0.0f, 0.0f, 0.0f, 0.25f);

    gallery_entry* Entry = Entries + CurrentEntryIndex;

    if (trIsButtonPressed(TR_BUTTON_KEY_LEFT))
    {
        if (CurrentEntryIndex == 0) CurrentEntryIndex = TR_ARRAY_COUNT(Entries) - 1;
        else                        CurrentEntryIndex -= 1;
    }

    if (trIsButtonPressed(TR_BUTTON_KEY_RIGHT))
    {
        CurrentEntryIndex += 1;
        CurrentEntryIndex %= TR_ARRAY_COUNT(Entries);
    }

    tr_texture Texture = Entry->Texture;

    // NOTE(vak): Scale width and height up so that small images
    // aren't displayed as a really, really small image. Bigger
    // images will be fitted within the canvas by the code below.

    tr_u32 Width = 10*Entry->Width;
    tr_u32 Height = 10*Entry->Height;

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    tr_f32 CanvasW = WindowW * 0.75f;
    tr_f32 CanvasH = WindowH * 0.75f;

    // NOTE(vak): Fit image within canvas

    tr_f32 DisplayW = Width;
    tr_f32 DisplayH = Height;

    if (DisplayW > CanvasW)
    {
        DisplayW = CanvasW;
        DisplayH = DisplayW * ((tr_f32)Height / (tr_f32)Width);
    }

    if (DisplayH > CanvasH)
    {
        DisplayH = CanvasH;
        DisplayW = DisplayH * ((tr_f32)Width / (tr_f32)Height);
    }

    // NOTE(vak): Center the image

    tr_f32 MinX = 0.5f * (WindowW - DisplayW);
    tr_f32 MinY = 0.5f * (WindowH - DisplayH);

    // NOTE(vak): Put it on the screen

    trTexturedRect2D(
        MinX,
        MinY,
        MinX + DisplayW,
        MinY + DisplayH,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        Texture
    );

    // NOTE(vak): Viewing and Artist text
    {
        char Buffer[512] = {0};
        tr_f32 Y = 10.0f;

        {
            tr_usize Count = 0;
            tr_str Prefix = trStr("Viewing: ");

            TR_ASSERT(Prefix.Size + Entry->FilePath.Size <= sizeof(Buffer));

            trCopyMemory(Buffer + Count, Prefix.Data, Prefix.Size);
            Count += Prefix.Size;

            trCopyMemory(Buffer + Count, Entry->FilePath.Data, Entry->FilePath.Size);
            Count += Entry->FilePath.Size;

            tr_str Text = trStrData(Buffer, Count);
            tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));
            trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);
            Y += trGetTextLineHeight();
        }

        {
            tr_usize Count = 0;
            tr_str Prefix = trStr("Artist: ");

            TR_ASSERT(Prefix.Size + Entry->Artist.Size <= sizeof(Buffer));

            trCopyMemory(Buffer + Count, Prefix.Data, Prefix.Size);
            Count += Prefix.Size;

            trCopyMemory(Buffer + Count, Entry->Artist.Data, Entry->Artist.Size);
            Count += Entry->Artist.Size;

            tr_str Text = trStrData(Buffer, Count);
            tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));
            trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // NOTE(vak): Controls
    {
        {
            tr_str Text = trStr("[LeftArrow]: Previous");

            tr_f32 X = 10.0f;
            tr_f32 Y = WindowH - 10.0f - trGetTextLineHeight();

            trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        {
            tr_str Text = trStr("[RightArrow]: Next");

            tr_f32 X = WindowW - trMeasureTextWidth(Text) - 10.0f;
            tr_f32 Y = WindowH - 10.0f - trGetTextLineHeight();

            trText2D(Text, X, Y, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

#include "tr.c"

