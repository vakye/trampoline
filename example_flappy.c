
#include "tr.h"

#define GAME_MAX_PIPE_COUNT (8) // NOTE(vak): Circular buffer of pipes

typedef struct
{
    tr_random Entropy;

    tr_f32 PlayerX;
    tr_f32 PlayerY;

    tr_f32 PlayerDX;
    tr_f32 PlayerDY;

    tr_f32 PlayerDDX;
    tr_f32 PlayerDDY;

    tr_f32 PlayerW;
    tr_f32 PlayerH;

    tr_f32 PipeSpeed;       // NOTE(vak): Speed that pipe moves across the screen
    tr_f32 PipeSpacing;     // NOTE(vak): Size of pipe "hole" for player to pass through
    tr_f32 PipeDistance;    // NOTE(vak): Distance between pipes
    tr_f32 PipeWidth;       // NOTE(vak): Width of pipe

    tr_u32 PipeIndex;
    tr_f32 PipesX[GAME_MAX_PIPE_COUNT]; // NOTE(vak): Position of pipe
    tr_f32 PipesY[GAME_MAX_PIPE_COUNT]; // NOTE(vak): Position of pipe "hole" center for player to pass through

    tr_u32 Score;
    tr_b32 Started;
    tr_b32 Over;
} game;

game Game = {0};

static void SpawnPipe(void)
{
    tr_u32 LastPipeIndex = GAME_MAX_PIPE_COUNT - 1;

    if (Game.PipeIndex > 0)
        LastPipeIndex = Game.PipeIndex - 1;

    tr_f32 LastX = Game.PipesX[LastPipeIndex];
    tr_f32 WindowH = trGetWindowHeight();

    Game.PipesX[Game.PipeIndex] = LastX + Game.PipeDistance;
    Game.PipesY[Game.PipeIndex] = trRandomF32(
        &Game.Entropy,
        1.25f * (Game.PipeSpacing * 0.5f),
        WindowH - 1.25f * (Game.PipeSpacing * 0.5f)
    );

    Game.PipeIndex = (Game.PipeIndex + 1) % GAME_MAX_PIPE_COUNT;
}

static void GameRestart(void)
{
    Game.PlayerX = 0.5f * trGetWindowWidth();
    Game.PlayerY = 0.5f * trGetWindowHeight();

    Game.PlayerDX = 0.0f;
    Game.PlayerDY = 0.0f;

    Game.PlayerDDX = 0.0f;
    Game.PlayerDDY = 0.0f;

    Game.PlayerW = 50.0f;
    Game.PlayerH = 50.0f;

    Game.Score = 0;
    Game.Started = TR_FALSE;
    Game.Over = TR_FALSE;

    Game.PipeSpeed      = 250.0f;
    Game.PipeWidth      = 75.0f;
    Game.PipeSpacing    = 4.5f * Game.PlayerH;
    Game.PipeDistance   = trGetWindowWidth() * 0.45f;

    for (tr_usize Index = 0; Index < GAME_MAX_PIPE_COUNT; Index++)
    {
        Game.PipesX[Index] = -99999.0f;
    }

    Game.PipesX[0] = trGetWindowWidth() + 2.0f*Game.PipeWidth;
    Game.PipesY[0] = trRandomF32(
        &Game.Entropy,
        1.25f * (Game.PipeSpacing * 0.5f),
        trGetWindowHeight() - 1.25f * (Game.PipeSpacing * 0.5f)
    );

    Game.PipeIndex = 1;
}

void Setup(void)
{
    trRandomSeed(&Game.Entropy);

    GameRestart();
}

void Frame(tr_f32 LastFrameTime)
{
    tr_f32 TimeStep = LastFrameTime;

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    if (!Game.Started)
    {
        if (trIsButtonPressed(TR_BUTTON_MOUSE_LEFT))
            Game.Started = TR_TRUE;
    }

    if (Game.Started && !Game.Over)
    {
        Game.PlayerDDY = 1350.0f; // NOTE(vak): Gravity

        Game.PlayerDX += Game.PlayerDDX * TimeStep;
        Game.PlayerDY += Game.PlayerDDY * TimeStep;

        if (trIsButtonPressed(TR_BUTTON_MOUSE_LEFT))
            Game.PlayerDY = -500.0f;

        Game.PlayerX += Game.PlayerDX * TimeStep;
        Game.PlayerY += Game.PlayerDY * TimeStep;

        if (Game.PlayerY - 0.5f*Game.PlayerH < 0)
            Game.Over = TR_TRUE;

        if (Game.PlayerY + 0.5f*Game.PlayerH > WindowH)
            Game.Over = TR_TRUE;

        for (tr_usize Index = 0; Index < GAME_MAX_PIPE_COUNT; Index++)
        {
            tr_f32 LastPipeX = Game.PipesX[Index];

            Game.PipesX[Index] -= Game.PipeSpeed * TimeStep;

            if ((Game.PlayerX < LastPipeX) &&
                (Game.PlayerX > Game.PipesX[Index]))
            {
                Game.Score++;
            }

            tr_f32 MinX = Game.PipesX[Index] - 0.5f*Game.PipeWidth;
            tr_f32 MaxX = Game.PipesX[Index] + 0.5f*Game.PipeWidth;

            tr_f32 UpperHitY = Game.PipesY[Index] - 0.5f*Game.PipeSpacing;
            tr_f32 LowerHitY = Game.PipesY[Index] + 0.5f*Game.PipeSpacing;

            if (Game.PlayerX + 0.5f*Game.PlayerW < MinX) continue;
            if (Game.PlayerX - 0.5f*Game.PlayerW > MaxX) continue;

            if (Game.PlayerY - 0.5f*Game.PlayerH < UpperHitY)
                Game.Over = TR_TRUE;

            if (Game.PlayerY + 0.5f*Game.PlayerH > LowerHitY)
                Game.Over = TR_TRUE;
        }

        tr_u32 LastPipeIndex = GAME_MAX_PIPE_COUNT - 1;

        if (Game.PipeIndex > 0)
            LastPipeIndex = Game.PipeIndex - 1;

        tr_f32 LastPipeX = Game.PipesX[LastPipeIndex];
        tr_f32 DistanceToRight = trGetWindowWidth() - LastPipeX;

        tr_f32 SpawnDistance = Game.PipeDistance * 1.5f;

        if ((DistanceToRight > 0) && (DistanceToRight < SpawnDistance))
            SpawnPipe();
    }

    if (Game.Over)
    {
        if (trIsButtonPressed(TR_BUTTON_MOUSE_LEFT))
            GameRestart();
    }

    trSetClearColor(0.9f, 0.9f, 0.9f, 1.0f);

    trRect2D(
        Game.PlayerX - 0.5f*Game.PlayerW,
        Game.PlayerY - 0.5f*Game.PlayerH,        
        Game.PlayerW,
        Game.PlayerH,
        0.23f, 0.27f, 0.32f, 1.0f
    );

    for (tr_usize Index = 0; Index < GAME_MAX_PIPE_COUNT; Index++)
    {
        // NOTE(vak): Top

        trRect2D(
            Game.PipesX[Index] - 0.5f*Game.PipeWidth,
            0.0f,
            Game.PipeWidth,
            Game.PipesY[Index] - 0.5f*Game.PipeSpacing,
            0.43f, 0.27f, 0.37f, 1.0f
        );

        // NOTE(vak): Bottom

        trRect2D(
            Game.PipesX[Index] - 0.5f*Game.PipeWidth,
            Game.PipesY[Index] + 0.5f*Game.PipeSpacing,
            Game.PipeWidth,
            100000.0f,
            0.43f, 0.27f, 0.37f, 1.0f
        );
    }

    if (Game.Started)
    {
        char Buffer[32]  = {0};

        tr_u32 DigitIndex = sizeof(Buffer);
        tr_u32 DigitCount = 0;

        tr_u32 Value = Game.Score;
        do
        {
            char Digit = '0' + (Value % 10);
            Value /= 10;

            DigitIndex--;
            DigitCount++;

            Buffer[DigitIndex] = Digit;
        } while (Value);

        tr_str Text = trStrData(Buffer + DigitIndex, DigitCount);

        tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));
        tr_f32 Y = 0.05f * (WindowH - trMeasureTextHeight(Text));

        trText2D(Text, X, Y, 0.43f, 0.54f, 0.47f, 1.0f);
    }

    if (!Game.Started)
    {
        tr_str Text = trStr("Click To Start");

        tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));
        tr_f32 Y = 0.25f * (WindowH - trMeasureTextHeight(Text));

        trText2D(Text, X, Y, 0.43f, 0.37f, 0.39f, 1.0f);
    }

    if (Game.Over)
    {
        tr_f32 Y = 0.25f * (WindowH - trGetTextLineHeight());

        {
            tr_str Text = trStr("Game Over");

            tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));

            trText2D(Text, X, Y, 0.43f, 0.37f, 0.39f, 1.0f);

            Y += trGetTextLineHeight();
        }

        {
            tr_str Text = trStr("Click To Play Again");

            tr_f32 X = 0.50f * (WindowW - trMeasureTextWidth(Text));

            trText2D(Text, X, Y, 0.43f, 0.37f, 0.39f, 1.0f);

            Y += trGetTextLineHeight();
        }
    }
}

#include "tr.c"

