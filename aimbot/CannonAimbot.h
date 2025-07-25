#ifndef CANNONAIMBOT_H
#define CANNONAIMBOT_H

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

// A struct to hold the results of a prediction calculation
struct AimSolution {
    bool success;
    FVector impactPoint;
    float timeOfFlight;
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

private:
    int AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, FRotator& oOutLow, FRotator& oOutHigh);

    // Helpers
    uintptr_t GetCannonActor(uintptr_t LPawn, uintptr_t GNames);
    void GetProjectileInfo(uintptr_t CannonActor, uintptr_t GNames, float &outProjectileSpeed, float &outProjectileGravityScale);
    void GetShipInfo(uintptr_t ShipActor, FVector &outShipLinearVel, FVector &outShipAngularVel, FRotator &outShipInitialRotation);
    void GetShipComponents(Entity ShipActor, std::vector<Entity> &OtherEntities, std::vector<FVector> &outShipActiveHoles, std::vector<FVector> &outShipInactiveHoles, std::vector<FVector> &outShipMasts, std::vector<FVector> &outCannonLocation, FVector &outShipWheel);

    float lastLoadedProjectileSpeed = 5700.0f, lastLoadedProjectileGravityScale = 0.791f;
    DrawingContext *draw = nullptr;
    FMinimalViewInfo CamInfo = {{0,0,0},{0,0,0}, {0}, 0, 0, 0, 0, 0, 0,};
};

#endif //CANNONAIMBOT_H