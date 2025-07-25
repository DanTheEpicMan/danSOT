#include "CannonAimbot.h"
#include <cmath>
#include <complex>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdio> // For snprintf used in debug text

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cfloat>

void SolveQuartic(const std::complex<float> coefficients[5], std::complex<float> roots[4]) {
    const std::complex<float> a = coefficients[4];
    const std::complex<float> b = coefficients[3] / a;
    const std::complex<float> c = coefficients[2] / a;
    const std::complex<float> d = coefficients[1] / a;
    const std::complex<float> e = coefficients[0] / a;
    const std::complex<float> Q1 = c * c - 3.f * b * d + 12.f * e;
    const std::complex<float> Q2 = 2.f * c * c * c - 9.f * b * c * d + 27.f * d * d + 27.f * b * b * e - 72.f * c * e;
    const std::complex<float> Q3 = 8.f * b * c - 16.f * d - 2.f * b * b * b;
    const std::complex<float> Q4 = 3.f * b * b - 8.f * c;
    const std::complex<float> Q5 = std::pow(Q2 / 2.f + std::sqrt(Q2 * Q2 / 4.f - Q1 * Q1 * Q1), 1.f / 3.f);
    const std::complex<float> Q6 = (Q1 / Q5 + Q5) / 3.f;
    const std::complex<float> Q7 = 2.f * std::sqrt(Q4 / 12.f + Q6);
    roots[0] = (-b - Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
    roots[1] = (-b - Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 - Q3 / Q7)) / 4.f;
    roots[2] = (-b + Q7 - std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
    roots[3] = (-b + Q7 + std::sqrt(4.f * Q4 / 6.f - 4.f * Q6 + Q3 / Q7)) / 4.f;
}

/**
 * @brief Converts a direction vector into Pitch/Yaw rotation angles.
 */
FRotator ToFRotator(FVector vec)
{
    FRotator rot;
    const double RADPI = 180.0 / M_PI;
    rot.Yaw = atan2(vec.y, vec.x) * RADPI;
    rot.Pitch = atan2(vec.z, sqrt((vec.x * vec.x) + (vec.y * vec.y))) * RADPI;
    rot.Roll = 0.0;
    return rot;
}

/**
 * @brief Calculates the launch angle(s) to hit a stationary target.
 * @return The number of solutions found (0, 1, or 2).
 */
int AimAtStaticTarget(const FVector& oTargetPos, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, FRotator& oOutLow, FRotator& oOutHigh) {
    const float gravity = 981.f * fProjectileGravityScalar;
    const FVector diff(oTargetPos - oSourcePos);
    const FVector oDiffXY(diff.x, diff.y, 0.0f);
    const float fGroundDist = oDiffXY.Size();
    const float s2 = fProjectileSpeed * fProjectileSpeed;
    const float s4 = s2 * s2;
    const float y = diff.z;
    const float x = fGroundDist;
    const float gx = gravity * x;
    float root = s4 - (gravity * ((gx * x) + (2.f * y * s2)));
    if (root < 0.f)
        return 0;
    root = std::sqrtf(root);
    const float fLowAngle = std::atan2f((s2 - root), gx);
    const float fHighAngle = std::atan2f((s2 + root), gx);
    int nSolutions = fLowAngle != fHighAngle ? 2 : 1;
    const FVector oGroundDir(oDiffXY.unit());
    oOutLow = ToFRotator(oGroundDir * std::cosf(fLowAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fLowAngle) * fProjectileSpeed);
    if (nSolutions == 2)
        oOutHigh = ToFRotator(oGroundDir * std::cosf(fHighAngle) * fProjectileSpeed + FVector(0.f, 0.f, 1.f) * std::sinf(fHighAngle) * fProjectileSpeed);
    return nSolutions;
}

/**
 * @brief Calculates the launch angle(s) to hit a target moving with linear velocity.
 * @return The number of solutions found (0, 1, or 2).
 */
int AimAtMovingTarget(const FVector& oTargetPos, const FVector& oTargetVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, const FVector& oSourceVelocity, FRotator& oOutLow, FRotator& oOutHigh) {
    const FVector v(oTargetVelocity - oSourceVelocity);
    const FVector g(0.f, 0.f, -981.f * fProjectileGravityScalar);
    const FVector p(oTargetPos - oSourcePos);
    const float c4 = (g | g) * 0.25f;
    const float c3 = v | g;
    const float c2 = (p | g) + (v | v) - (fProjectileSpeed * fProjectileSpeed);
    const float c1 = 2.f * (p | v);
    const float c0 = p | p;
    std::complex<float> pOutRoots[4];
    const std::complex<float> pInCoeffs[5] = { c0, c1, c2, c3, c4 };
    SolveQuartic(pInCoeffs, pOutRoots);
    float fBestRoot = std::numeric_limits<float>::max();
    for (int i = 0; i < 4; i++) {
        if (pOutRoots[i].real() > 0.f && std::abs(pOutRoots[i].imag()) < 0.0001f && pOutRoots[i].real() < fBestRoot) {
            fBestRoot = pOutRoots[i].real();
        }
    }
    if (fBestRoot == std::numeric_limits<float>::max())
        return 0;
    const FVector oAimAt = oTargetPos + (v * fBestRoot);
    return AimAtStaticTarget(oAimAt, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oOutLow, oOutHigh);
}

// --- Helper functions for Newton-Raphson method (used in AimAtShip) ---
namespace NewtonRaphson
{
    float time_func(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
    {
        const float K2 = K * K;
        const float L2 = L * L;
        const float M2 = M * M;
        const float N2 = N * N;
        const float r2 = r * r;
        return N2 * t * t * t * t + ((2.f * M * N) - S2) * t * t + 2.f * r * (K * cosf(theta + (w * t)) + L * sinf(theta + (w * t))) + K2 + L2 + M2 + r2;
    }

    // FIX #1: Corrected the derivative from t^4 to t^3
    float time_derivFunc(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
    {
        const float N2 = N * N;
        return 4.f * N2 * t * t * t + 2.f * ((2.f * M * N) - S2) * t + 2.f * r * w * (L * cosf(theta + (w * t)) - K * sinf(theta + (w * t)));
    }

    float solve(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
    {
        int counter = 0;
        while (counter < 200) // Safety break
        {
            float func_val = time_func(t, K, L, M, N, r, w, theta, S2);
            float deriv_val = time_derivFunc(t, K, L, M, N, r, w, theta, S2);

            if (std::abs(deriv_val) < 1e-5f) break; // Avoid division by zero

            float h = func_val / deriv_val;
            if (std::abs(h) < 0.01f) {
                return t; // Converged
            }
            t = t - h;
            counter++;
        }
        return t; // Return best guess if not converged
    }
}

/**
 * @brief Calculates launch angle(s) to hit a turning target (ship).
 *        Models the target's motion as a circle and uses a numerical solver.
 * @return The number of solutions found (0, 1, or 2).
 */
int CannonAimbot::AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, FRotator& oOutLow, FRotator& oOutHigh)
{
    const float w = oTargetAngularVelocity.z;

    // If turn rate is negligible, use the simpler linear model.
    if (std::abs(w) < 0.5f) {
        return AimAtMovingTarget(oTargetPos, oTargetVelocity, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
    }

    const FVector rel_vel = oTargetVelocity - oSourceVelocity;

    const float w_rad = w * (M_PI / 180.f);

    const float v_target_boat = FVector(rel_vel.x, rel_vel.y, 0.0f).Size();
    if (std::abs(w_rad) < 1e-5f) return 0; // Avoid division by zero, invalid turn

    // Turn radius r = v / ω
    const float r = v_target_boat / std::abs(w_rad);

    // FIX #2: Robust calculation of turn center using perpendicular vectors
    FVector to_center_dir(0.f, 0.f, 0.f);
    if (w_rad > 0.f) { // Turning left (CCW)
        to_center_dir = FVector(-rel_vel.y, rel_vel.x, 0.f).unit();
    } else { // Turning right (CW)
        to_center_dir = FVector(rel_vel.y, -rel_vel.x, 0.f).unit();
    }

    const FVector center_of_turn = oTargetPos + (to_center_dir * r);

    // This is the angle of the vector from the turn center to the ship's current position.
    // It's the ship's initial phase angle in its circular path.
    const float theta_rad = atan2f(oTargetPos.y - center_of_turn.y, oTargetPos.x - center_of_turn.x);

    // ---- The rest of the logic uses these corrected values ----

    const FVector diff(oTargetPos - oSourcePos);

    // K, L, M are the components of the vector from the cannon to the center of the turn.
    const FVector source_to_center = center_of_turn - oSourcePos;
    const float K = source_to_center.x;
    const float L = source_to_center.y;
    const float M = source_to_center.z;
    const float N = (981.f * fProjectileGravityScalar) / 2.f;
    const float S2 = fProjectileSpeed * fProjectileSpeed;

    const FVector oDiffXY(diff.x, diff.y, 0.0f);
    const float fGroundDist = oDiffXY.Size();

    float t_init = 0.f;
    if (fProjectileSpeed > 1.f) { // Avoid division by zero
        // Initial guess is based on the simple formula: time = distance / speed
        t_init = fGroundDist / fProjectileSpeed;
    } else {
        // Fallback for an invalid projectile speed
        t_init = 5.f;
    }
    // Clamp the initial guess to a reasonable range to improve stability
    t_init = std::max(0.1f, std::min(20.f, t_init));

    float t_best = NewtonRaphson::solve(t_init, K, L, M, N, r, w_rad, theta_rad, S2);

    if (t_best < 0.f) {
        return 0; // No valid future intercept found
    }

    // --- DEBUG DRAWING ---
    // Your drawing code for the center was almost correct, but use the new `center_of_turn` vector.
    Coords centerCoords = WorldToScreen(center_of_turn, this->CamInfo, MonWidth, MonHeight);
    drawX(this->draw, centerCoords, 5, COLOR::YELLOW); // Draw Center

    // Calculate the future intercept point
    const float future_angle = theta_rad + (w_rad * t_best);
    const float At = center_of_turn.x + r * cosf(future_angle);
    const float Bt = center_of_turn.y + r * sinf(future_angle);
    const FVector oAimAt = FVector(At, Bt, oTargetPos.z);

    Coords coords = WorldToScreen(oAimAt, this->CamInfo, MonWidth, MonHeight);
    drawX(this->draw, coords, 5, COLOR::ORANGE); // Draw Intercept Point

    // Now solve as a static target problem
    return AimAtStaticTarget(oAimAt, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oOutLow, oOutHigh);
}

FVector RotatorToVector(const FRotator& rot)
{
    // Convert degrees to radians
    const double pitchRad = rot.Pitch * (M_PI / 180.0);
    const double yawRad = rot.Yaw * (M_PI / 180.0);

    // Calculate components using trigonometry
    const float x = cos(yawRad) * cos(pitchRad);
    const float y = sin(yawRad) * cos(pitchRad);
    const float z = sin(pitchRad);

    return FVector(x, y, z);
}

void CannonAimbot::Run(uintptr_t GNames, uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> ships, std::vector<Entity> Enemies, std::vector<Entity> OtherEntities, DrawingContext *ctx, InputManager *inpMngr) {
    uintptr_t cannonActor = GetCannonActor(LPawn, GNames);
	//if (cannonActor == 0x0) return;

    this->draw = ctx;

    float projectileSpeed, projectileGravityScale;
    GetProjectileInfo(cannonActor, GNames, projectileSpeed, projectileGravityScale);
    if (projectileSpeed == 0) {
        projectileSpeed = this->lastLoadedProjectileSpeed;
        projectileGravityScale = this->lastLoadedProjectileGravityScale;
    }
    if (projectileSpeed == 0) return;
    this->lastLoadedProjectileSpeed = projectileSpeed;
    this->lastLoadedProjectileGravityScale = projectileGravityScale;

    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);
    FMinimalViewInfo CameraInfo = CameraCache.POV;
    FVector localPlayerVelocity = GetShipVelocityByDistance(LPawn, ships);
    localPlayerVelocity.z = 0; //not needed
    this->CamInfo = CameraInfo;

    std::cout << "LPZ: " << CamInfo.Location.z << std::endl;

    for (const auto& ship : ships) {
        FVector shipLinearVel, shipAngularVel;
        FRotator shipInitialRotation;
        GetShipInfo(ship.pawn, shipLinearVel, shipAngularVel, shipInitialRotation);
        FVector shipCoords = ship.location;

        if (CameraCache.POV.Location.Distance(shipCoords) > 600 * 100) continue; //greater than 600m away

    	//ShipCoords = targetPos
    	//targetVelocity = shipLinearVel
    	//targetAngulerVelocity = shipAngularVel
    	//sourcePos = CameraCache.POV.Location
    	//sourceVelocity = localPlayerVelocity
    	//ProjectileSpeed = this->lastLoadedProjectileSpeed
    	//ProjectileGravityScalar = this->lastLoadedProjectileGravityScale
        FRotator oOutLow, oOutHigh;


        int solutions = AimAtShip(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, this->lastLoadedProjectileSpeed, this->lastLoadedProjectileGravityScale, oOutLow, oOutHigh);

        if (solutions > 0)
        {
            // We'll use the "low arc" solution (oOutLow) as it's the most common one to use.
            FRotator targetRotation = oOutLow;

            // 1. Convert the target rotation (Pitch/Yaw) into a 3D direction vector.
            FVector aimDirection = RotatorToVector(targetRotation);

            // 2. Create a point in the world 100 meters (10000 cm) in front of the camera along that direction.
            // This gives us a 3D coordinate to feed into WorldToScreen.
            const float drawDistance = 10000.f;
            FVector aimWorldPosition = CameraInfo.Location + (aimDirection * drawDistance);

            // 3. Project this 3D point to your 2D screen.
            Coords screenPosition = WorldToScreen(aimWorldPosition, this->CamInfo, MonWidth, MonHeight);

            // 4. Draw a marker (e.g., a green 'X') at the calculated screen position.
            // This 'X' shows you exactly where on the screen you need to move your mouse to.
            if (screenPosition.x > 0 && screenPosition.y > 0) // Basic check to see if it's on screen
            {
                drawX(this->draw, screenPosition, 15, COLOR::ORANGE); // A larger, green X
            }
        }
    }
}

uintptr_t CannonAimbot::GetCannonActor(uintptr_t LPawn, uintptr_t GNames) {
    TArray<uintptr_t> LPChildren = ReadMemory<TArray<uintptr_t>>(LPawn + Offsets::Children);
    ptr cannonActor = 0x0;
    for (int i = 0; i < LPChildren.Length(); i++) {
        ptr childActor = ReadMemory<ptr>(LPChildren.GetAddress() + (i * sizeof(ptr)));
        std::string childName = getNameFromPawn(childActor, GNames);
        if (childName.find("IslandCannon") != std::string::npos || childName.find("Cannon_Ship") != std::string::npos) {
            cannonActor = childActor;
        }
    }
    return cannonActor;
}

void CannonAimbot::GetProjectileInfo(uintptr_t CannonActor, uintptr_t GNames, float &outProjectileSpeed, float &outProjectileGravityScale) {
    float ProjectileSpeed = ReadMemory<float>(CannonActor + Offsets::ProjectileSpeed);
    float projectileGravityScale = ReadMemory<float>(CannonActor + Offsets::ProjectileGravityScale);
    ptr LoadableComponent = ReadMemory<ptr>(CannonActor + Offsets::LoadableComponent);
    ptr LoadedItem = ReadMemory<ptr>(LoadableComponent + Offsets::LoadableComponentState + Offsets::LoadedItem);
    if (LoadedItem != 0) {
        std::string loadedItemName = getNameFromPawn(LoadedItem, GNames);
        if (loadedItemName.find("Player") != std::string::npos) {
            projectileGravityScale = 1.3f;
        } else if (loadedItemName.find("chain_shot") != std::string::npos) {
            projectileGravityScale = 1.0f;
        }
    }
    outProjectileSpeed = ProjectileSpeed;
    outProjectileGravityScale = projectileGravityScale;
}

void CannonAimbot::GetShipInfo(uintptr_t ShipPawn, FVector &outShipLinearVel, FVector &outShipAngularVel, FRotator &outShipInitialRotation) {
    ptr movementProxyComponent = ReadMemory<ptr>(ShipPawn + Offsets::ShipMovementProxyComponent);
    ptr movementProxyActor = ReadMemory<ptr>(movementProxyComponent + Offsets::ChildActor);
    ptr RepMovement_Address = movementProxyActor + Offsets::ShipMovement + Offsets::ReplicatedShipMovement_Movement;
    outShipLinearVel = ReadMemory<FVector>(RepMovement_Address + Offsets::LinearVelocity);
    outShipAngularVel = ReadMemory<FVector>(RepMovement_Address + Offsets::AngularVelocity);
    outShipInitialRotation = ReadMemory<FRotator>(RepMovement_Address + Offsets::Rotation);
}

void CannonAimbot::GetShipComponents(Entity ShipActor, std::vector<Entity> &OtherEntities, std::vector<FVector> &outShipActiveHoles, std::vector<FVector> &outShipInactiveHoles, std::vector<FVector> &outShipMasts, std::vector<FVector> &outCannonLocation, FVector &outShipWheel) {
    for (int i = 0; i < OtherEntities.size(); i++) {
        Entity &ent = OtherEntities[i];
    	ptr EntityRootComponent  = ReadMemory<ptr>(ent.pawn + Offsets::RootComponent);
    	FTransform EntityComponentToWorld = ReadMemory<FTransform>(EntityRootComponent + Offsets::ComponentToWorld);
    	ent.location = EntityComponentToWorld.Translation;
        if ((ent.location - ShipActor.location).Size() > 4000.0f) continue;
        if (ent.name == "BP_SmallShip_Mast_C" || ent.name == "BP_large_mast_mizzen_C" || ent.name == "BP_large_mast_main_C" || ent.name == "BP_medium_mast_fore_C" || ent.name == "BP_medium_mast_main_C") {
            outShipMasts.push_back(ent.location);
        } else if (ent.name.find("BP_Cannon_ShipPartMMC_") != std::string::npos) {
            outCannonLocation.push_back(ent.location);
        } else if (ent.name.find("hipWheel_C") != std::string::npos) {
            outShipWheel = ent.location;
        }
    }
    ptr hullDamage = ReadMemory<ptr>(ShipActor.pawn + Offsets::HullDamage);
    TArray<ptr> DamageZones = ReadMemory<TArray<ptr>>(hullDamage + Offsets::DamageZones);
    for (int j = 0; j < DamageZones.Length(); j++) {
        uintptr_t DamageZone = ReadMemory<ptr>(DamageZones.GetAddress() + (j * sizeof(uintptr_t)));
        int DamageLevel = ReadMemory<int>(DamageZone + Offsets::DamageLevel);
        ptr ShipSceneComponent = ReadMemory<ptr>(DamageZone + Offsets::SceneRootComponent);
        FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
        if (DamageLevel > 0) {
            outShipActiveHoles.push_back(location);
        } else {
            outShipInactiveHoles.push_back(location);
        }
    }
}