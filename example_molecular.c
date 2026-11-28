
#include "tr.h"

#define MAX_PARTICLE_COUNT  (4096)
#define TIME_STEP           (1.0f / 1000.0f)
#define VIEW_HEIGHT         (40.0f)

// NOTE(vak): Boundary that the particles are contained in.
#define BOUNDARY_MIN_X      (-10.0f)
#define BOUNDARY_MIN_Y      (-10.0f)
#define BOUNDARY_MAX_X      (+10.0f)
#define BOUNDARY_MAX_Y      (+10.0f)

// NOTE(vak): The equilibrium (bonding distance) between particles.
#define LENNARD_JONES_SIGMA     (2.0f)

// NOTE(vak): Scales the Lennard Jones potential. Thereby, controlling
// the intensity of the attraction and repulsion between particles.
#define LENNARD_JONES_EPSILON   (20000.0f)

// NOTE(vak): Target temperature in some imaginary unit...
// Higher means that the particles velocities will be scaled
// so that the target temperature is achieved.
#define TARGET_TEMPERATURE      (273.0f)

typedef struct
{
    tr_f32 Radius;
    tr_f32 InvMass;

    tr_f32 X;
    tr_f32 Y;
    tr_f32 LastX;
    tr_f32 LastY;
    tr_f32 DDX;
    tr_f32 DDY;
} particle;

static particle Particles[MAX_PARTICLE_COUNT] = {0};
static tr_u32 ParticleCount = 0;

static void AddParticle(tr_f32 X, tr_f32 Y, tr_f32 Radius, tr_f32 Mass)
{
    if (ParticleCount < MAX_PARTICLE_COUNT)
    {
        particle* Particle = Particles + ParticleCount++;

        trZeroType(Particle);

        Particle->X = X;
        Particle->Y = Y;
        Particle->LastX = X;
        Particle->LastY = Y;
        Particle->Radius = Radius;

        if (Mass > 1e-10f)
            Particle->InvMass = 1.0f / Mass;
    }
}

void Setup(void)
{
    tr_f32 Radius = 0.5f;
    tr_f32 Mass = 1.0f;

    tr_f32 BoundarySizeX = BOUNDARY_MAX_X - BOUNDARY_MIN_X;
    tr_f32 BoundarySizeY = BOUNDARY_MAX_Y - BOUNDARY_MIN_Y;

    tr_f32 SpawnMinX = BOUNDARY_MIN_X + Radius;
    tr_f32 SpawnMinY = BOUNDARY_MIN_Y + Radius;
    tr_f32 SpawnSizeX = BoundarySizeX - Radius*2.0f;
    tr_f32 SpawnSizeY = BoundarySizeY - Radius*2.0f;

    tr_random Entropy = {.State = 0x1234};

    for (tr_u32 Index = 0; Index < 30; Index++)
    {
        tr_f32 X = SpawnMinX + trRandomF32(&Entropy, 0.0f, SpawnSizeX);
        tr_f32 Y = SpawnMinY + trRandomF32(&Entropy, 0.0f, SpawnSizeY);

        AddParticle(X, Y, Radius, Mass);
    }
}

static tr_f32 ToScreenX(tr_f32 WorldX)
{
    tr_f32 WindowSizeX = TR_MAX(1, trGetWindowWidth());
    tr_f32 WindowSizeY = TR_MAX(1, trGetWindowHeight());

    tr_f32 AspectRatio = WindowSizeX / WindowSizeY;

    tr_f32 ViewSizeX = VIEW_HEIGHT * AspectRatio;
    tr_f32 ViewCenterX = 0.5f * ViewSizeX;

    tr_f32 NormalizedX = (WorldX + ViewCenterX) / ViewSizeX;
    tr_f32 ScreenX = NormalizedX * WindowSizeX;

    return (ScreenX);
}

static tr_f32 ToScreenY(tr_f32 WorldY)
{
    tr_f32 WindowSizeY = TR_MAX(1, trGetWindowHeight());

    tr_f32 ViewSizeY = VIEW_HEIGHT;
    tr_f32 ViewCenterY = 0.5f * ViewSizeY;

    tr_f32 NormalizedY = (-WorldY + ViewCenterY) / ViewSizeY;
    tr_f32 ScreenY = NormalizedY * WindowSizeY;

    return (ScreenY);
}

static tr_f32 ToScreenScale(tr_f32 WorldScale)
{
    tr_f32 ScreenScale = (WorldScale / VIEW_HEIGHT) * trGetWindowHeight();
    return (ScreenScale);
}

static void Simulate(tr_f32 TimeStep)
{
    // NOTE(vak): Reset forces

    for (tr_u32 Index = 0; Index < ParticleCount; Index++)
    {
        particle* Particle = Particles + Index;

        Particle->DDX = 0.0f;
        Particle->DDY = 0.0f;
    }

    // NOTE(vak): Compute forces

    for (tr_u32 IndexA = 0; IndexA < ParticleCount; IndexA++)
    {
        particle* ParticleA = Particles + IndexA;

        for (tr_u32 IndexB = IndexA+1; IndexB < ParticleCount; IndexB++)
        {
            particle* ParticleB = Particles + IndexB;

            tr_f32 DeltaX = ParticleB->X - ParticleA->X;
            tr_f32 DeltaY = ParticleB->Y - ParticleA->Y;

            // NOTE(vak):
            // Lennard Jones Potential function P(r),
            // for a distance r between particles:
            //
            //      P(r) = epsilon * ((sigma/r)^12 - (sigma/r)^6)
            //
            // The derivative of the Lennard Jones
            // Potential function yields the force
            // field
            //
            //      F(r) = epsilon * (-12 * (sigma^12)/(r^13) + 6 * (sigma^6)/(r^7))

            tr_f32 DistanceSq = DeltaX*DeltaX + DeltaY*DeltaY;

            if (DistanceSq > 1e-10f)
            {
                tr_f32 InvDistanceSq = 1.0f / DistanceSq;
                tr_f32 InvDistance = trSquareRootF32(InvDistanceSq);

                tr_f32 InvDistance3 = InvDistanceSq * InvDistance;
                tr_f32 InvDistance6 = InvDistance3 * InvDistance3;

                tr_f32 InvDistance7  = InvDistance6 * InvDistance;
                tr_f32 InvDistance13 = InvDistance6 * InvDistance7;

                tr_f32 TangentX = DeltaX * InvDistance;
                tr_f32 TangentY = DeltaY * InvDistance;

                tr_f32 Sigma2  = LENNARD_JONES_SIGMA * LENNARD_JONES_SIGMA;
                tr_f32 Sigma4  = Sigma2 * Sigma2;
                tr_f32 Sigma6  = Sigma4 * Sigma2;
                tr_f32 Sigma12 = Sigma6 * Sigma6;

                tr_f32 Force = LENNARD_JONES_EPSILON * (
                    -12.0f * Sigma12 * InvDistance13 +
                    +6.0f  * Sigma6  * InvDistance7
                );

                ParticleA->DDX += TangentX * Force;
                ParticleA->DDY += TangentY * Force;

                ParticleB->DDX -= TangentX * Force;
                ParticleB->DDY -= TangentY * Force;
            }
        }
    }

    // NOTE(vak): Compute system kinetic energy so that
    // we can scale the particle velocities to achieve
    // the desired temperature.

    // KineticEnergy = Sum(Mass * Velocity^2) for all particles
    // KineticEnergy = (3 * ParticleCount) / 2 * BoltzmannConstant * Temperature

    // We want our sum of mass and squared velocities to be equal to the
    // kinetic energy computed from temperature. We compute the system and target
    // kinetic energies, and then divide the target from the system kinetic energy
    // to obtain the squared velocity scaling factor.

    // NOTE(vak): Ignored Boltzmann constant for simplicity...

    tr_f32 TargetKineticEnergy = TARGET_TEMPERATURE * (3.0f * ParticleCount) / 2.0f;
    tr_f32 SystemKineticEnergy = 0.0f;

    for (tr_u32 Index = 0; Index < ParticleCount; Index++)
    {
        particle* Particle = Particles + Index;

        tr_f32 VelocityX = (Particle->X - Particle->LastX) / TimeStep;
        tr_f32 VelocityY = (Particle->Y - Particle->LastY) / TimeStep;

        tr_f32 VelocitySq = VelocityX*VelocityX + VelocityY*VelocityY;

        SystemKineticEnergy += (1.0f/Particle->InvMass) * VelocitySq;
    }

    // NOTE(vak): Integrate forces

    tr_f32 VelocityScale = 1.0f;

    if (SystemKineticEnergy > 0.0f)
        VelocityScale = trSquareRootF32(TargetKineticEnergy / SystemKineticEnergy);

    for (tr_u32 Index = 0; Index < ParticleCount; Index++)
    {
        particle* Particle = Particles + Index;

        tr_f32 TranslationX = VelocityScale * (Particle->X - Particle->LastX) + Particle->DDX * TimeStep * TimeStep;
        tr_f32 TranslationY = VelocityScale * (Particle->Y - Particle->LastY) + Particle->DDY * TimeStep * TimeStep;

        tr_f32 NewX = Particle->X + TranslationX;
        tr_f32 NewY = Particle->Y + TranslationY;

        Particle->LastX = Particle->X;
        Particle->LastY = Particle->Y;

        Particle->X = NewX;
        Particle->Y = NewY;
    }

    // NOTE(vak): Solve boundary constraints

    for (tr_u32 Index = 0; Index < ParticleCount; Index++)
    {
        particle* Particle = Particles + Index;

        tr_f32 CorrectionX = 0.0f;
        tr_f32 CorrectionY = 0.0f;

        if (Particle->X - Particle->Radius < BOUNDARY_MIN_X)
            CorrectionX += (BOUNDARY_MIN_X + Particle->Radius) - Particle->X;

        if (Particle->Y - Particle->Radius < BOUNDARY_MIN_Y)
            CorrectionY += (BOUNDARY_MIN_Y + Particle->Radius) - Particle->Y;

        if (Particle->X + Particle->Radius > BOUNDARY_MAX_X)
            CorrectionX += (BOUNDARY_MAX_X - Particle->Radius) - Particle->X;

        if (Particle->Y + Particle->Radius > BOUNDARY_MAX_Y)
            CorrectionY += (BOUNDARY_MAX_Y - Particle->Radius) - Particle->Y;

        Particle->X += CorrectionX;
        Particle->Y += CorrectionY;
    }
}

static void Render(void)
{
    trRectOutline2D(
        ToScreenX(BOUNDARY_MIN_X),
        ToScreenY(BOUNDARY_MIN_Y),
        ToScreenX(BOUNDARY_MAX_X),
        ToScreenY(BOUNDARY_MAX_Y),
        5.0f,
        1.0f, 0.3f, 0.3f, 1.0f
    );

    for (tr_u32 Index = 0; Index < ParticleCount; Index++)
    {
        particle* Particle = Particles + Index;

        trCircle2D(
            ToScreenX(Particle->X),
            ToScreenY(Particle->Y),
            ToScreenScale(Particle->Radius),
            0.9f, 0.9f, 0.9f, 0.8f
        );
    }
}

void Frame(tr_f32 LastFrameTime)
{
    static tr_f32 AccumulatedTime = 0.0f;

    trSetClearColor(0.0f, 0.0f, 0.0f, 0.75f);

    AccumulatedTime += LastFrameTime;

    while (AccumulatedTime >= TIME_STEP)
    {
        Simulate(TIME_STEP);

        AccumulatedTime -= TIME_STEP;
    }

    Render();
}

#include "tr.c"

