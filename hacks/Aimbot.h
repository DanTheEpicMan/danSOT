#pragma once

#include "../utils/GameStructs.h"
#include "../overlay/drawing.h"
#include "../memory/Memory.h"
#include "../utils/GameStructs.h"
#include "../overlay/drawing.h"

/**
 * @brief Calculates the 3D world position to aim at using linear prediction.
 * @param outAimPosition [out] The calculated 3D world position to aim at.
 * @return True if a valid prediction was found, otherwise false.
 */
bool GetPlayerAimPosition_NoGravity(
    const FVector& localPlayerPos, const FVector& localPlayerVel,
    const FVector& targetPos, const FVector& targetVel,
    float bulletSpeed,
    FVector& outAimPosition
);

/**
 * @brief Calculates the 3D world position to aim at, accounting for gravity.
 * @param outAimPosition [out] The calculated 3D world position to aim at.
 * @return True if a valid prediction was found, otherwise false.
 */
bool GetPlayerAimPosition_WithGravity(
    const FVector& localPlayerPos, const FVector& localPlayerVel,
    const FVector& targetPos, const FVector& targetVel,
    float bulletSpeed, float gravityScale,
    FVector& outAimPosition
);

int AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity,
    const FVector& oSourcePos, const FVector& oSourceVelocity,
    float fProjectileSpeed, float fProjectileGravityScalar,
    FRotator& oOutLow, FRotator& oOutHigh);

FVector RotatorToVector(const FRotator& rotation);