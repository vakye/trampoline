
#include "tr.h"

#define BODY_COUNT 50

typedef struct
{
    // NOTE(vak): General

    tr_f32 X, Y;        // NOTE(vak): Position
    tr_f32 DX, DY;      // NOTE(vak): Velocity
    tr_f32 DDX, DDY;    // NOTE(vak): Acceleration
    tr_f32 InvMass;     // NOTE(vak): 1.0 / Mass
    tr_f32 Radius;      // NOTE(vak): Circle radius

    // NOTE(vak): Constraint solver

    tr_f32 DeltaX;      // NOTE(vak): X correction to be applied to position
    tr_f32 DeltaY;      // NOTE(vak): Y correction to be applied to position
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

        Body->InvMass = 1.0f / Body->Radius;
    }
}

void Frame(tr_f32 DeltaTime)
{
    static tr_f32 AccumulatedTime = 0.0f;

    // NOTE(vak): Maintain fixed simulation time step
    // for accuracy and determinism

    tr_f32 TimeStep = 1.0f / 60.0f;
    tr_u32 SolverIterations = 4;

    AccumulatedTime += DeltaTime;

    while (AccumulatedTime >= TimeStep)
    {
        // NOTE(vak): Integrate forces

        for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
        {
            body* Body = Bodies + Index;

            Body->DDY = 500.0; // NOTE(vak): Gravity

            Body->DX += Body->DDX * TimeStep;
            Body->DY += Body->DDY * TimeStep;

            Body->X += Body->DX * TimeStep;
            Body->Y += Body->DY * TimeStep;
        }

        // NOTE(vak): Solve constraints (collisions, ...) using the Jacobi
        // method:
        //      + Accumulate position corrections for each constraint
        //      + Apply correction to position and adjust velocity
        //
        // Note that this is different from Gauss-Seidel as position corrections
        // are accumulated and applied only at the end. Gauss-Seidel applies the
        // position corrections immediately and uses the new positions to further
        // apply corrections for other constraints.

        for (tr_u32 Iteration = 0; Iteration < SolverIterations; Iteration++)
        {
            // NOTE(vak): Reset corrections

            for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
            {
                body* Body = Bodies + Index;

                Body->DeltaX = 0.0f;
                Body->DeltaY = 0.0f;
            }

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

                if (MinX < 0.0)     Body->DeltaX += (0.0 - MinX);
                if (MinY < 0.0)     Body->DeltaY += (0.0 - MinY);
                if (MaxX > WindowW) Body->DeltaX += (WindowW - MaxX);
                if (MaxY > WindowH) Body->DeltaY += (WindowH - MaxY);
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

                    BodyA->DeltaX -= DirectionX * Penetration * WA;
                    BodyA->DeltaY -= DirectionY * Penetration * WA;

                    BodyB->DeltaX += DirectionX * Penetration * WB;
                    BodyB->DeltaY += DirectionY * Penetration * WB;
                }
            }

            // NOTE(vak): Apply corrections

            for (tr_usize Index = 0; Index < BODY_COUNT; Index++)
            {
                body* Body = Bodies + Index;

                Body->X += Body->DeltaX;
                Body->Y += Body->DeltaY;

                Body->DX += Body->DeltaX / TimeStep;
                Body->DY += Body->DeltaY / TimeStep;
            }
        }

        AccumulatedTime -= TimeStep;
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
}

#include "tr.c"

