
#include "tr.h"

#define BODY_COUNT 50

typedef struct
{
    // NOTE(vak): General

    tr_f32 X, Y;            // NOTE(vak): Position
    tr_f32 LastX, LastY;    // NOTE(vak): Last position
    tr_f32 DX, DY;          // NOTE(vak): Velocity
    tr_f32 DDX, DDY;        // NOTE(vak): Acceleration
    tr_f32 InvMass;         // NOTE(vak): 1.0 / Mass
    tr_f32 Radius;          // NOTE(vak): Circle radius
} body;

static body Bodies[BODY_COUNT] = {0};

void Setup(void)
{
    // NOTE(vak): Randomly generate bodies

    tr_random Entropy = {0};
    trRandomSeed(&Entropy);

    tr_f32 WindowW = trGetWindowWidth();
    tr_f32 WindowH = trGetWindowHeight();

    for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
    {
        body* Body = Bodies + Index;

        Body->Radius = trRandomF32(&Entropy, 25.0f, 50.0f);
        Body->X = trRandomF32(&Entropy, Body->Radius, WindowW - Body->Radius);
        Body->Y = trRandomF32(&Entropy, Body->Radius, WindowH - Body->Radius);
        Body->LastX = Body->X;
        Body->LastY = Body->Y;

        Body->InvMass = 1.0f / Body->Radius;
    }
}

void Frame(tr_f32 LastFrameTime)
{
    // NOTE(vak): Do mouse picking

    static body* PickingBody = 0;

    tr_f32 MouseX = trGetMouseX();
    tr_f32 MouseY = trGetMouseY();

    body* HoveringBody = 0;

    for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
    {
        body* Body = Bodies + Index;

        tr_f32 DeltaX = Body->X - MouseX;
        tr_f32 DeltaY = Body->Y - MouseY;

        tr_f32 Distance = trSquareRootF32(
            DeltaX*DeltaX +
            DeltaY*DeltaY
        );

        if (Distance < Body->Radius)
            HoveringBody = Body;
    }

    if (trIsButtonDown(TR_BUTTON_MOUSE_LEFT))
    {
        if (HoveringBody && !PickingBody)
            PickingBody = HoveringBody;
    }
    else
    {
        PickingBody = 0;
    }

    static tr_f32 AccumulatedTime = 0.0f;

    // NOTE(vak): Maintain fixed simulation time step
    // for accuracy and determinism

    tr_f32 TimeStep = 1.0f / 60.0f;
    tr_u32 SolverIterations = 4;

    AccumulatedTime += LastFrameTime;

    while (AccumulatedTime >= TimeStep)
    {
        // NOTE(vak): Mouse picking force

        if (PickingBody)
        {
            tr_f32 Friction = 35.0f;
            tr_f32 Force = Friction * 16.0f;

            tr_f32 DeltaX = MouseX - PickingBody->X;
            tr_f32 DeltaY = MouseY - PickingBody->Y;

            PickingBody->DDX += Force*DeltaX - Friction*PickingBody->DX;
            PickingBody->DDY += Force*DeltaY - Friction*PickingBody->DY;
        }

        // NOTE(vak): Integrate forces

        for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
        {
            body* Body = Bodies + Index;

            Body->DDY += 1000.0; // NOTE(vak): Gravity

            Body->LastX = Body->X;
            Body->LastY = Body->Y;

            Body->X += TimeStep * (Body->DX + Body->DDX * 0.5f * TimeStep);
            Body->Y += TimeStep * (Body->DY + Body->DDY * 0.5f * TimeStep);
        }

        // NOTE(vak): Gauss-Seidel constraint solver

        for (tr_u32 Iteration = 0; Iteration < SolverIterations; Iteration++)
        {
            // NOTE(vak): Solve boundary constraints

            tr_f32 WindowW = trGetWindowWidth();
            tr_f32 WindowH = trGetWindowHeight();

            for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
            {
                body* Body = Bodies + Index;

                tr_f32 MinX = Body->X - Body->Radius;
                tr_f32 MinY = Body->Y - Body->Radius;
                tr_f32 MaxX = Body->X + Body->Radius;
                tr_f32 MaxY = Body->Y + Body->Radius;

                if (MinX < 0.0)     Body->X += (0.0 - MinX);
                if (MinY < 0.0)     Body->Y += (0.0 - MinY);
                if (MaxX > WindowW) Body->X += (WindowW - MaxX);
                if (MaxY > WindowH) Body->Y += (WindowH - MaxY);
            }

            // NOTE(vak): Solve collision constraints

            for (tr_usize IndexA = 0; IndexA < BODY_COUNT; IndexA++)
            {
                for (tr_usize IndexB = IndexA+1; IndexB < BODY_COUNT; IndexB++)
                {
                    body* BodyA = Bodies + IndexA;
                    body* BodyB = Bodies + IndexB;

                    tr_f32 TangentX = BodyB->X - BodyA->X;
                    tr_f32 TangentY = BodyB->Y - BodyA->Y;

                    tr_f32 Distance = trSquareRootF32(
                        TangentX*TangentX +
                        TangentY*TangentY
                    );

                    if (Distance > BodyA->Radius + BodyB->Radius)
                        continue;

                    tr_f32 DirectionX = TangentX / Distance;
                    tr_f32 DirectionY = TangentY / Distance;

                    tr_f32 Penetration = (BodyA->Radius + BodyB->Radius) - Distance;

                    tr_f32 TotalW = BodyA->InvMass + BodyB->InvMass;
                    tr_f32 WA = BodyA->InvMass / TotalW;
                    tr_f32 WB = BodyB->InvMass / TotalW;

                    BodyA->X -= DirectionX * Penetration * WA;
                    BodyA->Y -= DirectionY * Penetration * WA;

                    BodyB->X += DirectionX * Penetration * WB;
                    BodyB->Y += DirectionY * Penetration * WB;
                }
            }
        }

        // NOTE(vak): Update velocities and reset forces

        for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
        {
            body* Body = Bodies + Index;

            Body->DX = (Body->X - Body->LastX) / TimeStep;
            Body->DY = (Body->Y - Body->LastY) / TimeStep;

            Body->DDX = 0.0f;
            Body->DDY = 0.0f;
        }

        AccumulatedTime -= TimeStep;
    }

    // NOTE(vak): Mouse picking outline

    if (PickingBody || HoveringBody)
    {
        body* Body = (PickingBody) ? (PickingBody) : (HoveringBody);

        trCircleOutline2D(
            Body->X, Body->Y, Body->Radius,
            15.0f,
            0.6f, 0.8f, 1.0f, 0.7f + 0.3f*(PickingBody != 0)
        );
    }

    // NOTE(vak): Draw bodies to the screen

    for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
    {
        body* Body = Bodies + Index;

        trCircle2D(
            Body->X, Body->Y, Body->Radius,
            1.0f, 1.0f, 1.0f, 1.0f
        );
    }

    // NOTE(vak): Display controls
    {
        tr_str Text = trStr("Drag To Move The Bodies");

        tr_f32 X = 0.5f * (trGetWindowWidth() - trMeasureTextWidth(Text));
        tr_f32 Y = 0.0f;

        trText2D(
            Text,
            X, Y,
            0.98f, 0.97f, 0.89f, 1.0f
        );
    }
}

#include "tr.c"

