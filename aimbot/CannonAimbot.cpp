#include "CannonAimbot.h"
#include <cmath>
#include <complex>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdio> // For printf debugging

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper to print FVector
void print_fvector(const char* name, const FVector& vec) {
    printf("  %s: {x: %.2f, y: %.2f, z: %.2f}\n", name, vec.x, vec.y, vec.z);
}

// --- HELPER FUNCTIONS FOR PREDICTION ---

void solve_quartic(const std::vector<std::complex<double>>& coeffs, std::vector<std::complex<double>>& roots) {
    if (coeffs.size() != 5) return;
    double a = coeffs[4].real(), b = coeffs[3].real(), c = coeffs[2].real(), d = coeffs[1].real(), e = coeffs[0].real();
    if (std::abs(a) < 1e-9) return;
    b /= a; c /= a; d /= a; e /= a;
    double b2 = b * b;
    double p = c - 0.375 * b2;
    double q = d - 0.5 * b * c + 0.125 * b2 * b;
    double r = e - 0.25 * b * d + 0.0625 * b2 * c - 3.0 / 256.0 * b2 * b2;
    double c3_a = p / 2.0;
    double c3_b = (p * p - 4.0 * r) / 16.0;
    double c3_c = -q * q / 64.0;
    double c3_p = c3_b - c3_a * c3_a / 3.0;
    double c3_q = c3_c + (2.0 * c3_a * c3_a * c3_a - 9.0 * c3_a * c3_b) / 27.0;
    double c3_D = c3_q * c3_q / 4.0 + c3_p * c3_p * c3_p / 27.0;
    std::complex<double> y_root;
    if (c3_D >= 0) {
        double D_sqrt = sqrt(c3_D);
        y_root = {cbrt(-c3_q / 2.0 + D_sqrt) + cbrt(-c3_q / 2.0 - D_sqrt), 0.0};
    } else {
        std::complex<double> D_sqrt = std::sqrt(std::complex<double>(c3_D, 0.0));
        y_root = std::pow(-c3_q/2.0 + D_sqrt, 1.0/3.0) + std::pow(-c3_q/2.0 - D_sqrt, 1.0/3.0);
    }
    y_root -= c3_a / 3.0;
    double y = y_root.real();
    std::complex<double> R_complex = std::sqrt(std::complex<double>(p/2.0 + y, 0));
    std::complex<double> D_complex, E_complex;
    if (std::abs(R_complex.real()) > 1e-6 || std::abs(R_complex.imag()) > 1e-6) {
        std::complex<double> q_over_R = std::complex<double>(q, 0) / R_complex;
        D_complex = std::sqrt(-(p + y) - q_over_R);
        E_complex = std::sqrt(-(p + y) + q_over_R);
    } else {
        D_complex = std::sqrt(std::complex<double>(-(p + y), 0));
        E_complex = std::sqrt(std::complex<double>(-(p + y), 0));
    }
    double sub = b / 4.0;
    roots[0] = (R_complex + D_complex) / 2.0 - sub;
    roots[1] = (R_complex - D_complex) / 2.0 - sub;
    roots[2] = (-R_complex + E_complex) / 2.0 - sub;
    roots[3] = (-R_complex - E_complex) / 2.0 - sub;
}

double get_2d_distance(FVector a, FVector b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

double get_launch_angle_tan(FVector cannon_coords, FVector target_coords, double gravity, double projectile_speed) {
    double distance = get_2d_distance(cannon_coords, target_coords);
    const double height_diff = target_coords.z - cannon_coords.z;
    const double speed2 = projectile_speed * projectile_speed;
    const double speed4 = speed2 * speed2;
    const double gravity_x = gravity * distance;
    const double root_part = speed4 - gravity * (gravity * distance * distance + 2.0 * height_diff * speed2);
    if (root_part < 0.0) return 0.0;
    const double root_sqrt = std::sqrt(root_part);
    return std::min((speed2 - root_sqrt) / gravity_x, (speed2 + root_sqrt) / gravity_x);
}

double get_aim_z(FVector cannon_coords, FVector target_coords, double gravity, double projectile_speed) {
    double distance = get_2d_distance(cannon_coords, target_coords);
    const double new_angle_tan = get_launch_angle_tan(cannon_coords, target_coords, gravity, projectile_speed);
    return cannon_coords.z + (distance * new_angle_tan);
}

// --- CLASS IMPLEMENTATION ---

void CannonAimbot::Run(uintptr_t GNames, uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> ships, std::vector<Entity> Enemies, DrawingContext *ctx, InputManager *inpMngr) {
    // This top part remains the same...
    uintptr_t cannonActor = GetCannonActor(LPawn, GNames);
    if (cannonActor == 0x0) return;

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

    // Loop through each target ship
    for (const auto& ship : ships) {
        FVector shipLinearVel, shipAngularVel;
        GetShipInfo(ship.pawn, shipLinearVel, shipAngularVel);

        FVector shipCoords = ship.location;

        // --- NEW LOGIC TO HANDLE ANCHORED/STATIONARY SHIPS ---
        // Calculate the horizontal speed of the target ship.
        float horizontalSpeed = std::sqrt(shipLinearVel.x * shipLinearVel.x + shipLinearVel.y * shipLinearVel.y);

        // Define a threshold to consider the ship "stationary". 50 cm/s is a good starting point.
        const float stationaryThreshold = 50.0f;

        if (horizontalSpeed < stationaryThreshold) {
            // If the ship is moving slower than the threshold, it's likely just bobbing in the waves.
            // Treat it as a static target by zeroing out its velocity.
            // This prevents wave motion from corrupting the prediction.
            printf("[DEBUG] Ship %s is considered stationary (H-Speed: %.2f). Zeroing velocity for prediction.\n", ship.name.c_str(), horizontalSpeed);
            shipLinearVel = {0, 0, 0};
            shipAngularVel = {0, 0, 0};
        }
        // --- END OF NEW LOGIC ---

        // Now, the prediction functions will receive clean data.
        FVector rotationAimPoint = RotationPrediction(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel);
        if (rotationAimPoint.x != 0.f || rotationAimPoint.y != 0.f || rotationAimPoint.z != 0.f) {
            Coords screenPos = WorldToScreen(rotationAimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                // ctx->draw_box(screenPos.x - 5, screenPos.y - 5, 10, 10, 1, COLOR::BLUE);
                drawX(ctx, screenPos, 5, COLOR::PINK);
            }
        }

        FVector quarticAimPoint = QuarticPrediction(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel);
        if (quarticAimPoint.x != 0.f || quarticAimPoint.y != 0.f || quarticAimPoint.z != 0.f) {
            Coords screenPos = WorldToScreen(quarticAimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                ctx->draw_box(screenPos.x - 5, screenPos.y - 5, 10, 10, 1, COLOR::RED);
            }
        }
    }
}

FVector CannonAimbot::RotationPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel, FVector ShipAngularVel) {
    // Perform distance check with original game units first
    double distance = get_2d_distance(LPCam.POV.Location, ShipCoords);
    if (distance >= 60000.0) return {0,0,0}; // Check against 600m in game units (600 * 100)

    // Now, proceed with the logic using scaled-down meter values for stability
    double gravityM = static_cast<double>(ProjectileGravityScale * 981.0f) / 100.0;
    double projectileSpeedM = static_cast<double>(ProjectileSpeed) / 100.0;
    FVector cannonCoordsM = LPCam.POV.Location / 100.0;
    FVector shipCoordsM = ShipCoords / 100.0;
    FVector lpShipLinearVelM = LPLinearVel / 100.0;

    double angularVelZ = ShipAngularVel.z;
    double angularVelRadians = angularVelZ * M_PI / 180.0;
    if (std::abs(angularVelRadians) < 1e-4) return {0,0,0};

    // Use meter-scaled velocities for this calculation
    double linearVelX_M = ShipLinearVel.x / 100.0;
    double linearVelY_M = ShipLinearVel.y / 100.0;
    double speedMagnitudeM = std::sqrt(linearVelX_M * linearVelX_M + linearVelY_M * linearVelY_M);
    if (speedMagnitudeM < 1.0) return {0,0,0};

    double diskRadius = speedMagnitudeM / angularVelRadians;
    double effectiveRadius = diskRadius * 0.98;
    double actualHeading = std::atan2(linearVelY_M, linearVelX_M);

    FVector center = {static_cast<float>(effectiveRadius*std::cos(actualHeading+M_PI/2.0)+shipCoordsM.x), static_cast<float>(effectiveRadius*std::sin(actualHeading+M_PI/2.0)+shipCoordsM.y), shipCoordsM.z};
    double angleTheta = actualHeading - M_PI / 2.0;
    float tTime = 0.0f;
    const float tIterator = 0.1f;
    FVector targetPosT_M;
    while (tTime < 50.0f) {
        double wTime = angularVelRadians * tTime;
        targetPosT_M.x = center.x + (effectiveRadius * std::cos(wTime + angleTheta)) - (lpShipLinearVelM.x * tTime);
        targetPosT_M.y = center.y + (effectiveRadius * std::sin(wTime + angleTheta)) - (lpShipLinearVelM.y * tTime);
        targetPosT_M.z = shipCoordsM.z;
        double nDist = get_2d_distance(cannonCoordsM, targetPosT_M);
        double angleTan = get_launch_angle_tan(cannonCoordsM, targetPosT_M, gravityM, projectileSpeedM);
        if (projectileSpeedM * std::cos(std::atan(angleTan)) == 0) {
             tTime += tIterator;
             continue;
        }
        double sTime = nDist / (projectileSpeedM * std::cos(std::atan(angleTan)));

        if (sTime < tTime) {
            targetPosT_M.z = get_aim_z(cannonCoordsM, targetPosT_M, gravityM, projectileSpeedM);
            return targetPosT_M * 100.0;
        }
        tTime += tIterator;
    }
    return {0,0,0};
}

FVector CannonAimbot::QuarticPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel) {
    printf("  [QuartPred] --- Starting ---\n");
    // All calculations and checks in this function use original game units.
    double distance = get_2d_distance(LPCam.POV.Location, ShipCoords);
    printf("  [QuartPred] Initial Distance: %.2f\n", distance);
    if (distance <= 5000.0 || distance >= 50000.0) {
        printf("  [QuartPred] SKIPPED: Out of range (5k-50k)\n");
        return {0,0,0};
    }

    const double gravity = static_cast<double>(ProjectileGravityScale * 981.0f);
    const FVector gravityVec = {0.0, 0.0, static_cast<float>(-gravity)};
    const FVector netVelVec = ShipLinearVel - LPLinearVel;
    const FVector netPosVec = ShipCoords - LPCam.POV.Location;

    print_fvector("  [QuartPred] Net Velocity", netVelVec);
    print_fvector("  [QuartPred] Net Position", netPosVec);

    auto dot = [](const FVector& a, const FVector& b) {return a.x*b.x+a.y*b.y+a.z*b.z;};

    // Check if the target is stationary
    bool isStatic = (std::abs(netVelVec.x) < 0.1 && std::abs(netVelVec.y) < 0.1 && std::abs(netVelVec.z) < 0.1);

    double bestRoot = -1.0; // Use -1 to indicate no solution found yet

    if (isStatic) {
        printf("  [QuartPred] DETECTED STATIC TARGET. Using biquadratic solver.\n");

        // Equation is: (0.25*g^2)*t^4 + (dot(p,g) - V^2)*t^2 + dot(p,p) = 0
        // Which is A*y^2 + B*y + C = 0 where y = t^2
        double A = dot(gravityVec, gravityVec) * 0.25;
        double B = dot(netPosVec, gravityVec) - (ProjectileSpeed * ProjectileSpeed);
        double C = dot(netPosVec, netPosVec);

        printf("  [QuartPred] Biquad Coeffs: A=%.2e, B=%.2e, C=%.2e\n", A, B, C);

        // Solve the quadratic equation for y = t^2
        double discriminant = B * B - 4 * A * C;
        printf("  [QuartPred] Discriminant: %.2e\n", discriminant);

        if (discriminant >= 0) {
            double sqrt_discriminant = std::sqrt(discriminant);
            double y1 = (-B + sqrt_discriminant) / (2 * A);
            double y2 = (-B - sqrt_discriminant) / (2 * A);

            printf("  [QuartPred] Solutions for t^2: y1=%.4f, y2=%.4f\n", y1, y2);

            // We need a positive, real t, so we need a positive y
            if (y1 > 0) bestRoot = std::sqrt(y1);
            if (y2 > 0) bestRoot = std::min(bestRoot, std::sqrt(y2));
        } else {
             printf("  [QuartPred] FAILED: Negative discriminant, no real solution for t^2.\n");
        }

    } else {
        printf("  [QuartPred] DETECTED MOVING TARGET. Using general quartic solver.\n");

        const double c4 = dot(gravityVec, gravityVec) * 0.25;
        const double c3 = dot(netVelVec, gravityVec);
        const double c2 = dot(netPosVec, gravityVec) + dot(netVelVec, netVelVec) - (ProjectileSpeed * ProjectileSpeed);
        const double c1 = 2.0 * dot(netPosVec, netVelVec);
        const double c0 = dot(netPosVec, netPosVec);
        printf("  [QuartPred] Coeffs: c4=%.2e, c3=%.2e, c2=%.2e, c1=%.2e, c0=%.2e\n", c4, c3, c2, c1, c0);

        std::vector<std::complex<double>> coeffs = {{c0,0},{c1,0},{c2,0},{c3,0},{c4,0}};
        std::vector<std::complex<double>> roots(4);
        solve_quartic(coeffs, roots);

        bestRoot = std::numeric_limits<double>::max();
        for (int i = 0; i < 4; ++i) {
            printf("  [QuartPred] Root %d: (%.4f, %.4fi)\n", i, roots[i].real(), roots[i].imag());
            if (roots[i].real() > 0.0 && std::abs(roots[i].imag()) < 1e-4) {
                bestRoot = std::min(bestRoot, roots[i].real());
            }
        }
        if (bestRoot == std::numeric_limits<double>::max()) bestRoot = -1.0; // Standardize "no solution"
    }


    if (bestRoot <= 0) {
        printf("  [QuartPred] FAILED: No valid real, positive root found.\n");
        return {0,0,0};
    }

    printf("  [QuartPred] SOLUTION FOUND: Best root (time-to-impact) = %.4f\n", bestRoot);

    FVector aimAt = ShipCoords + (netVelVec * bestRoot);
    aimAt.z = get_aim_z(LPCam.POV.Location, aimAt, gravity, ProjectileSpeed);
    return aimAt;
}

uintptr_t CannonAimbot::GetCannonActor(uintptr_t LPawn, uintptr_t GNames) {
    TArray<uintptr_t> LPChildren = ReadMemory<TArray<uintptr_t>>(LPawn + Offsets::Children);
    ptr cannonActor = 0x0;
    for (int i = 0; i < LPChildren.Length(); i++) {
        ptr childActor = ReadMemory<ptr>(LPChildren.GetAddress() + (i * sizeof(ptr)));
        int childID = ReadMemory<int>(childActor + Offsets::ActorID);
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

void CannonAimbot::GetShipInfo(uintptr_t ShipPawn, FVector &outShipLinearVel, FVector &outShipAngularVel) {
    ptr movementProxyComponent = ReadMemory<ptr>(ShipPawn + Offsets::ShipMovementProxyComponent);
    ptr movementProxyActor = ReadMemory<ptr>(movementProxyComponent + Offsets::ChildActor);
    ptr RepMovement_Address = movementProxyActor + Offsets::ShipMovement + Offsets::ReplicatedShipMovement_Movement;
    outShipLinearVel = ReadMemory<FVector>(RepMovement_Address + Offsets::LinearVelocity);
    outShipAngularVel = ReadMemory<FVector>(RepMovement_Address + Offsets::AngularVelocity);
}