//
// Created by dan on 7/7/25.
//

#include "PlayerAimbot.h"

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

void PlayerAimbot::Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> Ships, DrawingContext *ctx, InputManager *inpMngr) {

    uintptr_t WieldedItem = ReadMemory<uintptr_t>(LPawn + Offsets::WieldedItemComponent); //ProjectileWeapon, ProjectileWeaponParameters,
    if (!WieldedItem) return; //No wielded item, cannot aim
    uintptr_t CurrentlyWieldedItem = ReadMemory<uintptr_t>(WieldedItem + Offsets::ReplicatedCurrentlyWieldedItem);
    float bulletSpeed = ReadMemory<float>(CurrentlyWieldedItem + Offsets::WeaponParameters +
            Offsets::ProjectileWeaponParameters_AmmoParams +
            Offsets::WeaponProjectileParams_Velocity); //ProjectileWeapons->WeaponParameters + AmmoParams + Velocity

    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);

    FVector LPVelocityWithShip = GetPlayerGlobalVelocitySloppy(LPawn, Ships);

    FVector bestTargetLocation = {-1, -1, -1};
    Coords bestTargetScreenCoords = {-1, -1};
    float lowestTargetScore = FLT_MAX; // owest=we want to aim at most
    float worldDistanceTarget = 0.0f;

    for (int i = 0; i < EnemyPlayers.size(); i++) {
        Entity Enemy = EnemyPlayers[i];

        Coords aimCoords = {0, 0};
        FVector targetLocation = {-1, -1, -1};
        float targetWorldDistance = 0.0f;
        if (getPlayerAimToCoords(LPVelocityWithShip, CameraCache, bulletSpeed,
            Enemy, Ships, targetLocation, aimCoords, targetWorldDistance, 1.0f)) {
            if (coordsOnScreen(aimCoords, MonWidth, MonHeight)) {
                // Distance from crosshair (in pixels)
                float dx = aimCoords.x - (MonWidth / 2.0f);
                float dy = aimCoords.y - (MonHeight / 2.0f);
                float dist_from_crosshair = sqrt(dx * dx + dy * dy);

                //Check if valid in accordance with settings
                bool isInThreatRadius = targetWorldDistance < this->THREAT_RADIUS;
                bool isInFOV = dist_from_crosshair < this->AIMBOT_FOV;

                if (isInThreatRadius || isInFOV) {
                    float currentScore; //aim at lowest
                    if (isInThreatRadius) {
                        currentScore = dist_from_crosshair; //if in immediate danger area, make score distance
                    } else {
                        currentScore = this->THREAT_RADIUS + dist_from_crosshair; //if not, make score danger area max + dist from crosshair
                    }

                    if (currentScore < lowestTargetScore) {
                        bestTargetLocation = targetLocation;
                        lowestTargetScore = currentScore;
                        worldDistanceTarget = targetWorldDistance;
                        bestTargetScreenCoords = aimCoords;
                    }
                }
            }
        }
        drawX(ctx, aimCoords, 5, COLOR::RED);
    }

    bool conditionsMetForShooting = false;
    if (bestTargetLocation.x != -1 || bestTargetLocation.y != -1 || bestTargetLocation.z != -1) {
        if ((this->runAimbot && inpMngr->isKeyDown(this->aimbotKeyCode)) || (this->runAimAndShootKey && inpMngr->isKeyDown(this->aimAndShootKeyCode))) {
            float deltaX = bestTargetScreenCoords.x - (MonWidth / 2.0f);
            float deltaY = bestTargetScreenCoords.y - (MonHeight / 2.0f);

            // Aims at full body (also give info for auto shoot)
            Coords head = WorldToScreen({bestTargetLocation.x, bestTargetLocation.y, bestTargetLocation.z + 35}, CameraCache.POV, MonWidth, MonHeight);
            Coords feet = WorldToScreen({bestTargetLocation.x, bestTargetLocation.y, bestTargetLocation.z - 25}, CameraCache.POV, MonWidth, MonHeight);
            Coords left = WorldToScreen({bestTargetLocation.x - 5, bestTargetLocation.y, bestTargetLocation.z}, CameraCache.POV, MonWidth, MonHeight);
            Coords right = WorldToScreen({bestTargetLocation.x + 5, bestTargetLocation.y, bestTargetLocation.z}, CameraCache.POV, MonWidth, MonHeight);

            if (!this->aimAtCenter) {
                float screenCenterY = MonHeight / 2.0f;
                float screenCenterX = MonWidth / 2.0f;

                if (!(head.y > feet.y)) { //wierd edge case
                    //clamp crosshair y, to be within player
                    if (screenCenterY < head.y) { //crosshair above
                        deltaY = head.y - screenCenterY;
                    } else if (screenCenterY > feet.y) {
                        deltaY = feet.y - screenCenterY;
                    } else {
                        deltaY = 0; //crosshair is within player bounds
                    }
                }

                if (!(left.x < right.x)) { //wierd edge case
                    //clamp crosshair x, to be within player
                    if (screenCenterX < left.x) { //crosshair left
                        deltaX = left.x - screenCenterX;
                    } else if (screenCenterX > right.x) {
                        deltaX = right.x - screenCenterX;
                    } else {
                        deltaX = 0; //crosshair is within player bounds
                    }
                }
            }

            if (!coordsOnScreen({(int)((MonWidth / 2.0f) + deltaX), (int)((MonHeight / 2.0f) + deltaY)}, MonWidth, MonHeight)) {
                deltaX = 0; deltaY = 0;
            }

            if (std::abs(deltaX) > AIM_DEADZONE || std::abs(deltaY) > AIM_DEADZONE) {
                float dynamic_smoothness; //faster when player is closer
                if (worldDistanceTarget < this->MIN_SMOOTH_DIST) {
                    dynamic_smoothness = this->MIN_SMOOTHNESS;
                } else if (worldDistanceTarget > this->MAX_SMOOTH_DIST) {
                    dynamic_smoothness = this->MAX_SMOOTHNESS;
                } else {
                    float ratio = (worldDistanceTarget - this->MIN_SMOOTH_DIST) / (this->MAX_SMOOTH_DIST - this->MIN_SMOOTH_DIST);
                    dynamic_smoothness = this->MIN_SMOOTHNESS + ratio * (this->MAX_SMOOTHNESS - this->MIN_SMOOTHNESS);
                }

                float moveX = deltaX / dynamic_smoothness;
                float moveY = deltaY / dynamic_smoothness;

                this->buildUpAimX += moveX;
                this->buildUpAimY += moveY;

                int finalMoveX = static_cast<int>(std::round(this->buildUpAimX));
                int finalMoveY = static_cast<int>(std::round(this->buildUpAimY));

                if (this->buildUpAimX != 0 || this->buildUpAimY != 0) {
                    this->buildUpAimX -= finalMoveX; //Reset the build up
                    this->buildUpAimY -= finalMoveY;

                    inpMngr->moveMouseRelative(finalMoveX, finalMoveY);
                }
                if (this->runAimAndShootKey && inpMngr->isKeyDown(this->aimAndShootKeyCode)) {
                    // Check if crosshair X is within the player's horizontal bounds
                    bool xIsOnTarget = (left.x < MonWidth / 2.0f) && (right.x > MonWidth / 2.0f);
                    // Check if crosshair Y is within the player's vertical bounds
                    bool yIsOnTarget = (head.y < MonHeight / 2.0f) && (feet.y > MonHeight / 2.0f);

                    if ((xIsOnTarget && yIsOnTarget) || ((int)moveX == 0 && (int)moveY == 0)) {
                        std::cout << "xIsOnTarget: " << xIsOnTarget << ", yIsOnTarget: " << yIsOnTarget << std::endl;
                        std::cout << "x: " << (int)moveX << ", y: " << (int)moveY << std::endl;
                        conditionsMetForShooting = true;
                    } //if on player
                } // if autoshoot + key
            } // if not in deadzone
        } // if aimbot or aim and shoot key pressed
    }
    HandleAutoShoot(conditionsMetForShooting, inpMngr);
}

bool PlayerAimbot::getPlayerAimToCoords(FVector LocalPlayerVelocityWithShip, FCameraCacheEntry CameraCache, float bulletSpeed, Entity Enemy, std::vector<Entity> ships, FVector &outAimingCoords, Coords &outCoords, float &targetWorldDistance, float gravityScale) {
    FVector EnemyVelocityWithShip = GetPlayerGlobalVelocitySloppy(Enemy.pawn, ships);
    FVector toAimCoords = {0, 0, 0};
    bool worked = calculateWithGravity ?
        GetPlayerAimPosition_WithGravity(CameraCache.POV.Location, LocalPlayerVelocityWithShip, Enemy.location, EnemyVelocityWithShip, bulletSpeed, gravityScale, toAimCoords) :
        GetPlayerAimPosition_NoGravity(CameraCache.POV.Location, LocalPlayerVelocityWithShip, Enemy.location, EnemyVelocityWithShip, bulletSpeed, toAimCoords);
    if (!worked) {
        return false;
    }
    Coords screenCoords = WorldToScreen(toAimCoords, CameraCache.POV, MonWidth, MonHeight);

    outAimingCoords = toAimCoords;
    outCoords = screenCoords;
    targetWorldDistance = toAimCoords.Distance(CameraCache.POV.Location);
    return true;
}

void PlayerAimbot::HandleAutoShoot(bool shouldBeShooting, InputManager *inpMngr) {
    double currentTime = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();

    // std::cout << "Mouse currently pressed: " << this->mouseCurrentlyPressed << std::endl;
    if (this->mouseCurrentlyPressed) {
        // std::cout << "Last mouse down time: " << this->lastMouseDownTime - (double)1752103992 * (double)1000 << std::endl;
        // std::cout << "Mouse time down random: " << this->mouseTimeDownRandom << std::endl;
        if ((currentTime - this->lastMouseDownTime >= this->mouseTimeDownRandom) || !shouldBeShooting) {
            inpMngr->leftButtonUp();
            this->mouseCurrentlyPressed = false;
            this->lastMouseUpTime = currentTime;
        }
    }
    else { // mouseCurrentlyPressed is false
        // std::cout << "ShouldBeShooting: " << shouldBeShooting << std::endl;
        // std::cout << "LastMouseUpDiff: " << currentTime - this->lastMouseUpTime << std::endl;
        // std::cout << "MIN_TIME_BETWEEN_SHOTS_MS: " << MIN_TIME_BETWEEN_SHOTS_MS << std::endl;
        if (shouldBeShooting && (currentTime - this->lastMouseUpTime > MIN_TIME_BETWEEN_SHOTS_MS)) {
            inpMngr->leftButtonDown();
            this->mouseCurrentlyPressed = true;
            this->lastMouseDownTime = currentTime;
            // Set a new random hold time for this specific shot
            this->mouseTimeDownRandom = rand() % 100 + 50; // 50-150ms hold
        }
    }
}