#include "Aimbot.h"
#include <complex>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

//TODO: Implement checks that the player is not outside of max distance for weapon

//Limitations, does not account for the ship turning I belive this will only affect when the enemy player is turning but since bullets dont travel for more than like 2 seconds, turnigng spees should not inpact this in a significant way
//Add velocity of the ship to the Velocity for both player and target
bool GetPlayerAimPosition_NoGravity(const FVector& localPlayerPos, const FVector& localPlayerVel,
    const FVector& targetPos, const FVector& targetVel,
    float bulletSpeed,
    FVector& outAimPosition)
{
    const FVector relativeVelocity = targetVel - localPlayerVel;
    const FVector relativeLocation = targetPos - localPlayerPos;

    const float a = relativeVelocity.SizeSquared() - (bulletSpeed * bulletSpeed);
    const float b = 2.0f * relativeVelocity.Dot(relativeLocation);
    const float c = relativeLocation.SizeSquared();

    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
    {
        return false;
    }

    const float rootD = sqrtf(discriminant);
    const float t1 = (-b + rootD) / (2.0f * a);
    const float t2 = (-b - rootD) / (2.0f * a);

    float timeToImpact = -1.0f;
    if (t1 > 0 && t2 > 0) timeToImpact = fminf(t1, t2);
    else if (t1 > 0) timeToImpact = t1;
    else if (t2 > 0) timeToImpact = t2;
    else return false;

    // Calculate the predicted position and output it directly
    outAimPosition = targetPos + (relativeVelocity * timeToImpact);

    return true;

}

bool GetPlayerAimPosition_WithGravity(const FVector& localPlayerPos, const FVector& localPlayerVel,
    const FVector& targetPos, const FVector& targetVel,
    float bulletSpeed, float gravityScale,
    FVector& outAimPosition)
{
    const float gravity = 981.0f * gravityScale;

    // Start with the current position as a first guess.
    FVector predictedPosition = targetPos;

    // Iteratively refine the prediction to account for gravity.
    for (int i = 0; i < 3; ++i)
    {
        const float distance = (predictedPosition - localPlayerPos).Size();
        const float timeToImpact = distance / bulletSpeed;

        // Predict the target's future linear position based on the refined time.
        predictedPosition = targetPos + ((targetVel - localPlayerVel) * timeToImpact);

        // Account for bullet drop over that time. We must aim higher to compensate.
        const float bulletDrop = 0.5f * gravity * (timeToImpact * timeToImpact);
        predictedPosition.z += bulletDrop;
    }

    // Output the final, gravity-compensated position
    outAimPosition = predictedPosition;

    return true;
}

// // Your provided math functions, slightly adapted for the namespace.
// namespace Aimbot {
//
//     void SolveQuartic(const std::complex<float> coefficients[5], std::complex<float> roots[4]) {
//         const std::complex<float> a = coefficients[4];
//         const std::complex<float> b = coefficients[3] / a;
//         const std::complex<float> c = coefficients[2] / a;
//         const std::complex<float> d = coefficients[1] / a;
//         const std::complex<float> e = coefficients[0] / a;
//         const std::complex<float> Q1 = c * c - 3.f * b * d + 12.f * e;
//         const std::complex<float> Q2 = 2.f * c * c * c - 9.f * b * c * d + 27.f * d * d + 27.f * b * b * e - 72.f * c * e;
//         const std::complex<float> Q3 = 8.f * b * c - 16.f * d - 2.f * b * b * b;
//         const std::complex<float> Q4 = 3.f * b * b - 8.f * c;
//         const std::complex<float> Q5 = std::pow(Q2 / 2.f + std::sqrt(Q2 * Q2 / 4.f - Q1 * Q1 * Q1), 1.f / 3.f);
//         const std::complex<float> Q6 = (Q1 / Q5 + Q5) / 3.f;
//         const std::complex<float> Q7 = 2.f * std::sqrt(Q4 / 12.f + Q6);
//         roots[0] = (-b - Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
//         roots[1] = (-b - Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
//         roots[2] = (-b + Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
//         roots[3] = (-b + Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
//     }
//
//     FRotator ToFRotator(FVector vec) {
//         FRotator rot;
//         float RADPI = (float)(180 / M_PI);
//         rot.Yaw = (float)(atan2f(vec.y, vec.x) * RADPI);
//         rot.Pitch = (float)atan2f(vec.z, sqrt((vec.x * vec.x) + (vec.y * vec.y))) * RADPI;
//         rot.Roll = 0;
//         return rot;
//     }
//
//     int AimAtStaticTarget(const FVector& oTargetPos, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, FRotator& oOutLow, FRotator& oOutHigh) {
//         const float gravity = 981.f * fProjectileGravityScalar;
//         const FVector diff(oTargetPos - oSourcePos);
//         const FVector oDiffXY(diff.x, diff.y, 0.0f);
//         const float fGroundDist = oDiffXY.Size();
//         const float s2 = fProjectileSpeed * fProjectileSpeed;
//         const float s4 = s2 * s2;
//         const float y = diff.z;
//         const float x = fGroundDist;
//         const float gx = gravity * x;
//         float root = s4 - (gravity * ((gx * x) + (2 * y * s2)));
//         if (root < 0)
//             return 0;
//         root = std::sqrtf(root);
//         const float fLowAngle = std::atan2f((s2 - root), gx);
//         const float fHighAngle = std::atan2f((s2 + root), gx);
//         int nSolutions = fLowAngle != fHighAngle ? 2 : 1;
//         const FVector oGroundDir(oDiffXY.unit());
//         oOutLow = ToFRotator(oGroundDir * std::cosf(fLowAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fLowAngle) * fProjectileSpeed);
//         if (nSolutions == 2)
//             oOutHigh = ToFRotator(oGroundDir * std::cosf(fHighAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fHighAngle) * fProjectileSpeed);
//         return nSolutions;
//     }
//
//     int AimAtMovingTarget(const FVector& oTargetPos, const FVector& oTargetVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, const FVector& oSourceVelocity, FRotator& oOutLow, FRotator& oOutHigh) {
//         const FVector v(oTargetVelocity - oSourceVelocity);
//         const FVector g(0.f, 0.f, -981.f * fProjectileGravityScalar);
//         const FVector p(oTargetPos - oSourcePos);
//         const float c4 = (g | g) * 0.25f;
//         const float c3 = v | g;
//         const float c2 = (p | g) + (v | v) - (fProjectileSpeed * fProjectileSpeed);
//         const float c1 = 2.f * (p | v);
//         const float c0 = p | p;
//         std::complex<float> pOutRoots[4];
//         const std::complex<float> pInCoeffs[5] = { c0, c1, c2, c3, c4 };
//         SolveQuartic(pInCoeffs, pOutRoots);
//         float fBestRoot = FLT_MAX;
//         for (int i = 0; i < 4; i++) {
//             if (pOutRoots[i].real() > 0.f && std::abs(pOutRoots[i].imag()) < 0.0001f && pOutRoots[i].real() < fBestRoot) {
//                 fBestRoot = pOutRoots[i].real();
//             }
//         }
//         if (fBestRoot == FLT_MAX)
//             return 0;
//         const FVector oAimAt = oTargetPos + (v * fBestRoot);
//         return AimAtStaticTarget(oAimAt, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oOutLow, oOutHigh);
//     }
//
//     // Note: The math for a turning ship is significantly more complex.
//     // This implementation will use linear prediction which is a very good approximation for short projectile flight times.
//     int AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, FRotator& oOutLow, FRotator& oOutHigh)
//     {
//         // For simplicity and robustness in an external context, we treat the ship's movement
//         // as linear for the short flight time of the projectile.
//         // The provided 'AimAtShip' logic with Newton-Raphson is highly complex to implement externally
//         // and prone to issues without direct access to game state. Linear prediction is sufficient.
//         return AimAtMovingTarget(oTargetPos, oTargetVelocity, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
//     }
//
//
//     bool GetProjectilePath(std::vector<FVector>& v, FVector& Vel, FVector& Pos, float Gravity, int count)
//     {
//         float interval = 0.033f;
//         for (unsigned int i = 0; i < count; ++i)
//         {
//             v.push_back(Pos);
//             FVector move;
//             move.X = (Vel.X) * interval;
//             move.Y = (Vel.Y) * interval;
//             float newZ = Vel.Z - (Gravity * interval);
//             move.Z = ((Vel.Z + newZ) * 0.5f) * interval;
//             Vel.Z = newZ;
//             FVector nextPos = Pos + move;
//
//             // External cannot raytrace against the world reliably without significant effort.
//             // We'll just trace the path until it goes below a certain Z level.
//             if (nextPos.Z < -100.0f) { // Assume Z= -100 is below any possible water level
//                 v.push_back(nextPos);
//                 return true; // "Hit" the water
//             }
//             Pos = nextPos;
//         }
//         return false;
//     }
//
//
//     void Run(DrawingContext* ctx, int monWidth, int monHeight) {
//         // Core pointers
//         uintptr_t UWorld = ReadMemory<uintptr_t>(BaseAddress + 0x6273410); // UWorld offset
//         if (!UWorld) return;
//         uintptr_t GameInstance = ReadMemory<uintptr_t>(UWorld + 0x188); // UWorld.OwningGameInstance
//         if (!GameInstance) return;
//         uintptr_t LocalPlayers = ReadMemory<uintptr_t>(GameInstance + 0x38); // GameInstance.LocalPlayers
//         if (!LocalPlayers) return;
//         uintptr_t LocalPlayer = ReadMemory<uintptr_t>(LocalPlayers);
//         if (!LocalPlayer) return;
//         uintptr_t PlayerController = ReadMemory<uintptr_t>(LocalPlayer + 0x30); // LocalPlayer.PlayerController
//         if (!PlayerController) return;
//         uintptr_t LocalPawn = ReadMemory<uintptr_t>(PlayerController + 0x408); // PlayerController.AcknowledgedPawn
//         if (!LocalPawn) return;
//
//         uintptr_t PlayerCameraManager = ReadMemory<uintptr_t>(PlayerController + 0x430);
//         if (!PlayerCameraManager) return;
//         FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(PlayerCameraManager + 0x410); // CameraCachePrivate
//
//         // Target finding logic
//         TargetInfo bestTarget;
//         ProjectileInfo projInfo = { 0.f, 0.f };
//         bool aimReady = false;
//
//         uintptr_t WieldedItem = ReadMemory<uintptr_t>(LocalPawn + 0x870); // WieldableItem->WieldedItemComponent->CurrentlyWieldedItem
//         uintptr_t AttachedTo = ReadMemory<uintptr_t>(LocalPawn + 0x3a8); // Pawn->Actor->Owner
//
//         // Check if on a cannon
//         if (AttachedTo && ReadMemory<char>(AttachedTo + 0x9f1) == 2) { // ACannon->IsCannonInUse
//             projInfo.Speed = ReadMemory<float>(AttachedTo + 0x59c); // ACannon->ProjectileSpeed
//             projInfo.GravityScale = ReadMemory<float>(AttachedTo + 0x5a0); // ACannon->ProjectileGravityScale
//             aimReady = true;
//         }
//         // Check if holding a projectile weapon
//         else if (WieldedItem && IsSubclassOf(WieldedItem, AProjectileWeapon::StaticClass())) {
//             uintptr_t weaponParamsPtr = ReadMemory<uintptr_t>(WieldedItem + 0x848);
//             uintptr_t ammoParamsPtr = ReadMemory<uintptr_t>(weaponParamsPtr + 0x80);
//             projInfo.Speed = ReadMemory<float>(ammoParamsPtr + 0x10); // WeaponProjectileParams.Velocity
//             projInfo.GravityScale = 1.0f; // Handheld weapons have a gravity scale of 1.0
//             aimReady = true;
//         }
//
//         if (aimReady) {
//             // GetShipTarget(...)
//             // GetPlayerTarget(...)
//
//             // For demonstration, let's just pick the first valid player
//             if (!playerEntities.empty()) {
//                 Entity& player = playerEntities[0]; // Assuming playerEntities is populated from your main loop
//
//                 FVector sourceVelocity = {0.f, 0.f, 0.f}; // TODO: Read this from local player's ship
//                 FRotator low, high;
//
//                 if (AimAtMovingTarget(player.location, player.player.velocity, projInfo.Speed, projInfo.GravityScale, CameraCache.POV.Location, sourceVelocity, low, high)) {
//                     bestTarget.TargetActor = (ACharacter*)player.pawn;
//                     bestTarget.location = player.location; // This is a bit simplified, should be predicted location
//                     bestTarget.delta = UKismetMathLibrary::NormalizedDeltaRotator(low, CameraCache.POV.Rotation);
//                 }
//             }
//         }
//
//         // After finding the best target
//         if (bestTarget.TargetActor) {
//             FRotator rotation = UKismetMathLibrary::FindLookAtRotation(CameraCache.POV.Location, bestTarget.location);
//             Coords screenCoords = WorldToScreen(bestTarget.location, CameraCache.POV, monWidth, monHeight);
//             if(screenCoords.x > 0 && screenCoords.y > 0){
//                 ctx->draw_box(screenCoords.x - 5, screenCoords.y - 5, 10, 10, 2.0f, COLOR::RED);
//             }
//         }
//     }
// }