#ifndef CANNONAIMBOT_H
#define CANNONAIMBOT_H

#include <chrono> // For timing the prediction test
#include <map>    // To store prediction tests per-ship

#include <cstdint>
#include <vector>
#include "../overlay/drawing.h"
#include "../utils/GameData.h"
#include "../utils/LocalData.h"
#include "../utils/LocalFunctions.h"
#include "../offsets.h"
#include "../config.h"
#include <complex>


class InputManager;

// Holds the data for a single prediction vs. reality test
struct PredictionTestSnapshot {
    std::chrono::steady_clock::time_point snapshotTime; // The time the prediction was made
    double predictionDuration;                          // How many seconds into the future we predicted
    FVector predictedPosition;                          // The calculated future position (X, Y, Z)
};

class CannonAimbot {
public:
    void Run(uintptr_t GNames,
        uintptr_t LPawn,
        uintptr_t playerController,
        std::vector<Entity> ships,
        std::vector<Entity> Enemies,
        std::vector<Entity> OtherEntities,
        DrawingContext *ctx, InputManager *inpMngr);

    uintptr_t GetCannonActor(uintptr_t LPawn, uintptr_t GNames);

    void GetProjectileInfo(uintptr_t CannonActor, uintptr_t GNames,
        float &outProjectileSpeed, float &outProjectileGravityScale);

    void GetShipInfo(uintptr_t ShipActor,
        FVector &outShipLinearVel, FVector &outShipAngularVel, FRotator &outShipInitialRotation);

    void GetShipComponents(Entity ShipActor, std::vector<Entity> &OtherEntities,
        std::vector<FVector> &outShipActiveHoles, std::vector<FVector> &outShipInactiveHoles,
        std::vector<FVector> &outShipMasts, std::vector<FVector> &outCannonLocation, FVector &outShipWheel);

    // Prediction Functions
    FVector RotationPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel,
        float ProjectileSpeed, float ProjectileGravityScale,
        FVector ShipCoords, FVector ShipLinearVel, FVector ShipAngularVel);

    FVector RotationPredictionForPart(
        FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale,
        FVector ShipCenterCoords, FVector ShipLinearVel, FVector ShipAngularVel, FRotator ShipInitialRotation,
        FVector TargetPartGlobalCoords);

    FVector QuarticPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel,
        float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel);

    FVector StaticPrediction(FVector PlayerPos, FVector TargetPos, float ProjectileSpeed, float ProjectileGravityScale);

private:
    float lastLoadedProjectileSpeed = 5700.0f, lastLoadedProjectileGravityScale = 0.791f;
};

#endif //CANNONAIMBOT_H