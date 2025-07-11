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

class CannonAimbot {
public:
    void Run(uintptr_t GNames,
        uintptr_t LPawn,
        uintptr_t playerController,
        std::vector<Entity> ships,
        std::vector<Entity> Enemies,
        DrawingContext *ctx, InputManager *inpMngr);

    uintptr_t GetCannonActor(uintptr_t LPawn, uintptr_t GNames);

    void GetProjectileInfo(uintptr_t CannonActor, uintptr_t GNames,
        float &outProjectileSpeed, float &outProjectileGravityScale);

    void GetShipInfo(uintptr_t ShipActor,
        FVector &outShipLinearVel, FVector &outShipAngularVel);

    FVector RotationPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel,
        float ProjectileSpeed, float ProjectileGravityScale,
        FVector ShipCoords, FVector ShipLinearVel, FVector ShipAngularVel);

    FVector QuarticPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel,
        float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel)

private:
    bool drawShipMiddleToAim = true;        //Draw were to aim to hit ship middle
    bool drawShipHolesAim = true;           //Draw were to aim to hit ship holes
    bool drawShipPotentialHolesAim = true;  //Draw were to aim to hit holes not yet created
    bool drawShipMastAim = true;            //Draw were to aim to hit ship mast
    bool drawPlayerAim = true;              //Draw were to aim to hit player
    bool aimbotForHoles = true;             //Aims for you to nearest

    bool drawCannonArk = true;              //Draw the arc of the cannonball

private:
    float lastLoadedProjectileSpeed = 0.0f, lastLoadedProjectileGravityScale = 0.0f;
};



#endif //CANNONAIMBOT_H

/** Non newton raphson cannon prediction
 * Info Needed:
 * Self: If player is on cannon, projectile speed and gravity scale, camera position, linear velocity
 * Enemy: Position, linear velocity, angular velocity
*/

