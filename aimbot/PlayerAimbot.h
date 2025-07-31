//
// Created by dan on 7/7/25.
//

#ifndef PLAYERAIMBOT_H
#define PLAYERAIMBOT_H
#include <cstdint>
#include <vector>
#include <cfloat>
#include <algorithm>
#include <chrono>
#include "../overlay/drawing.h"
#include "../IO/InputManager.h"
#include "../utils/LocalFunctions.h"
#include "../utils/LocalData.h"
#include "../config.h"

struct Entity;

class PlayerAimbot {
public:
    void Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> Ships, DrawingContext *ctx, InputManager *inpMngr);

    bool getPlayerAimToCoords(FVector LocalPlayerVelocityWithShip, FCameraCacheEntry CameraCache, float bulletSpeed,
        Entity Enemy, std::vector<Entity> ships,
        FVector &outAimingCoords, Coords &outCoords, float &targetWorldDistance,
        float gravityScale=1.0f);
private:
    bool runAimbot = true;
    int  aimbotKeyCode = KEY_2;

    bool runAimAndShootKey = true;
    int aimAndShootKeyCode = KEY_5;
    bool mouseCurrentlyPressed = false; //Used to prevent double clicks
    double lastMouseDownTime = 0.0; //used for realistic press time
    double lastMouseUpTime = 0.0; //Used to prevent double clicks
    double mouseTimeDownRandom = 100; //Random time to hold mouse down, in milliseconds
    const double MIN_TIME_BETWEEN_SHOTS_MS = 120.0;
    void HandleAutoShoot(bool shouldBeShooting, InputManager *inpMngr);

    bool calculateWithGravity = false; //Will account for bullet drop

    bool aimAtCenter = false; //Will aim always at center instead of allowing for y to be a range from head to feet.

    float buildUpAimX = 0.0f, buildUpAimY = 0.0f; //If one tick determines we should only move 0.5 pixels, it will build up over time to a full pixel movement.

    float AIMBOT_FOV = 250.0f;          // The radius in pixels from your crosshair to search for targets.
    float THREAT_RADIUS = 750.0f;       // World distance (e.g., in units/cm) for emergency 360-degree targeting.

    // Dynamic Smoothing Parameters
    float MIN_SMOOTHNESS = 3.0f;        // Hardest tracking (for closest targets). Lower is faster.
    float MAX_SMOOTHNESS = 15.0f;       // Softest tracking (for farthest targets). Higher is smoother (and slower).
    float MIN_SMOOTH_DIST = 500.0f;     // World distance where MIN_SMOOTHNESS is fully applied.
    float MAX_SMOOTH_DIST = 8000.0f;    // World distance where MAX_SMOOTHNESS is fully applied.

    // Deadzone
    float AIM_DEADZONE = 2.0f;          // Stop aiming when crosshair is this close (in pixels) to the target.

    enum QuickSwapState {
        QS_IDLE,
        QS_WAITING_FOR_DELAY,
        QS_PRESSING_X1,
        QS_WAITING_BETWEEN_PRESSES,
        QS_PRESSING_X2
    };
    QuickSwapState quickSwapState = QS_IDLE;
    double quickSwapStateStartTime = 0.0;
    double quickSwapInitialDelay = 0.0;
    double quickSwapPressTime1 = 0.0;
    double quickSwapPressTime2 = 0.0;
    double quickSwapTimeBetween = 0.0;
    bool quickSwapLmbWasPressed = false;

    void HandleQuickSwap(InputManager *inpMngr);
};



#endif //PLAYERAIMBOT_H
