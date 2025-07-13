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

// Forward Declarations for Helper Functions
FRotator ToFRotator(FVector vec);
void SolveQuartic(const std::complex<float> coefficients[5], std::complex<float> roots[4]);
int AimAtStaticTarget(const FVector& oTargetPos, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, FRotator& oOutLow, FRotator& oOutHigh);
int AimAtMovingTarget(const FVector& oTargetPos, const FVector& oTargetVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, const FVector& oSourceVelocity, FRotator& oOutLow, FRotator& oOutHigh);
float newtonRaphson(float t, float K, float L, float M, float N, float r, float w, float theta, float S2);

void CannonAimbot::Run(uintptr_t GNames, uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> ships, std::vector<Entity> Enemies, std::vector<Entity> OtherEntities, DrawingContext *ctx, InputManager *inpMngr) {
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

    for (const auto& ship : ships) {
        FVector shipLinearVel, shipAngularVel;
        FRotator shipInitialRotation;
        GetShipInfo(ship.pawn, shipLinearVel, shipAngularVel, shipInitialRotation);
        FVector shipCoords = ship.location;

        FRotator AimLow, AimHigh;

        // --- HULL AIMING (with full visuals) ---
        int solutions = AimAtShip(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, projectileSpeed, projectileGravityScale, AimLow, AimHigh, ctx, CameraInfo, MonWidth, MonHeight);
        if (solutions > 0) {
            FVector startPoint = CameraCache.POV.Location;
            FVector aimDirection = RotatorToVector(AimLow);
            FVector worldAimPoint = startPoint + (aimDirection * (shipCoords - startPoint).Size());
            Coords screenPos = WorldToScreen(worldAimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                drawX(ctx, screenPos, 8, COLOR::MAGENTA);
            }
        }

        // --- PLAYER AIMING (calculation only, simple visual) ---
    	int playerDeckOffset = 100;
    	if (ship.name.find("Large") != std::string::npos) playerDeckOffset += 200;
        FVector playerTargetPos = {shipCoords.x, shipCoords.y, shipCoords.z + playerDeckOffset};
        AimSolution playerSolution = SolveAimingProblem(playerTargetPos, shipLinearVel, shipAngularVel, CameraCache.POV.Location, projectileSpeed, 1.3f);
        if (playerSolution.success) {
            solutions = AimAtMovingTarget(playerSolution.impactPoint, {0,0,0}, projectileSpeed, 1.3f, CameraCache.POV.Location, localPlayerVelocity, AimLow, AimHigh);
            if (solutions > 0) {
                FVector startPoint = CameraCache.POV.Location;
                FVector aimDirection = RotatorToVector(AimLow);
                FVector worldAimPoint = startPoint + (aimDirection * (shipCoords - startPoint).Size());
                Coords screenPos = WorldToScreen(worldAimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    drawX(ctx, screenPos, 8, COLOR::RED);
                }
            }
        }

        // --- Prediction for Ship Components ---
        std::vector<FVector> shipActiveHoles, shipInactiveHoles, shipMasts, cannonLocations;
        FVector shipWheel;
        GetShipComponents(ship, OtherEntities, shipActiveHoles, shipInactiveHoles, shipMasts, cannonLocations, shipWheel);

        for (const auto& hole : shipActiveHoles) {
            FVector aimPoint = PredictAimpointForPart(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, projectileSpeed, projectileGravityScale, hole);
            if (aimPoint.x != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    drawX(ctx, screenPos, 5, COLOR::RED);
                }
            }
        }
        for (const auto& hole : shipInactiveHoles) {
            FVector aimPoint = PredictAimpointForPart(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, projectileSpeed, projectileGravityScale, hole);
            if (aimPoint.x != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    drawX(ctx, screenPos, 5, COLOR::BLUE);
                }
            }
        }
        for (const auto& mast : shipMasts) {
             FVector aimPoint = PredictAimpointForPart(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, projectileSpeed, projectileGravityScale, mast);
            if (aimPoint.x != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
					ctx->draw_text(screenPos.x, screenPos.y, "[MAST]", COLOR::MAGENTA);
                }
            }
        }
        // ... (rest of component loops are the same)
    }
}

// THIS IS THE NEW CORE CALCULATION FUNCTION. IT DRAWS NOTHING.
AimSolution CannonAimbot::SolveAimingProblem(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, float fProjectileSpeed, float fProjectileGravityScalar)
{
	const float w_deg = oTargetAngularVelocity.z;

	// Use linear prediction if the target isn't turning significantly or is moving too slow for rotational to be stable.
	if (std::abs(w_deg) < 1.0f || FVector(oTargetVelocity.x, oTargetVelocity.y, 0.0f).Size() < 50.f)
	{
        // For linear, we can just use the quartic solver to find time, then calculate impact point.
        const FVector v(oTargetVelocity - FVector{0,0,0});
        const FVector g(0.f, 0.f, -981.f * fProjectileGravityScalar);
        const FVector p(oTargetPos - oSourcePos);
        const float c4 = g | g * 0.25f; const float c3 = v | g; const float c2 = (p | g) + (v | v) - (fProjectileSpeed * fProjectileSpeed);
        const float c1 = 2.f * (p | v); const float c0 = p | p;
        std::complex<float> pOutRoots[4]; std::complex<float> pInCoeffs[5] = { c0, c1, c2, c3, c4 };
        SolveQuartic(pInCoeffs, pOutRoots);
        float bestRoot = -1.f;
        for (int i = 0; i < 4; i++) {
            if (pOutRoots[i].real() > 0.f && std::abs(pOutRoots[i].imag()) < 0.0001f) {
                if (bestRoot < 0.f || pOutRoots[i].real() < bestRoot) bestRoot = pOutRoots[i].real();
            }
        }

        if (bestRoot > 0.f && bestRoot < 12.f) {
            return {true, oTargetPos + (oTargetVelocity * bestRoot), bestRoot};
        }
	} else { // Rotational Prediction
        const FVector pPos = oSourcePos;
        const FVector diff_to_target = oTargetPos - pPos;
        const float w_rad = w_deg * (M_PI / 180.f);
        const float r_solver = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.0f).Size() / std::abs(w_rad);

        FVector vel_dir_2d = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.f).unit();
        FVector to_center_dir = (w_rad > 0) ? FVector(-vel_dir_2d.y, vel_dir_2d.x, 0.f) : FVector(vel_dir_2d.y, -vel_dir_2d.x, 0.f);
        FVector turn_center = oTargetPos + (to_center_dir * r_solver);
        FVector from_center_to_ship = oTargetPos - turn_center;
        float theta_rad = atan2f(from_center_to_ship.y, from_center_to_ship.x);

        const FVector diff_to_center = turn_center - pPos;
        const float K = diff_to_center.x; const float L = diff_to_center.y;
        const float M = 0.f; // Use M=0 to solve in 2D for stability.
        const float N = (981.f * fProjectileGravityScalar) / 2.f;
        const float S2 = fProjectileSpeed * fProjectileSpeed;
        const float fGroundDist = FVector(diff_to_target.x, diff_to_target.y, 0.0f).Size();
        float t_init = fGroundDist / fProjectileSpeed;
        float t_best = newtonRaphson(t_init, K, L, M, N, r_solver, w_rad, theta_rad, S2);

        if (t_best > 0.f && t_best < 12.0f) {
            float final_angle = theta_rad + (w_rad * t_best);
            const FVector impactPoint = turn_center + FVector(r_solver * cosf(final_angle), r_solver * sinf(final_angle), oTargetPos.z);
            return {true, impactPoint, t_best};
        }
    }
    return {false, {0,0,0}, 0.f}; // Return failure
}

// THIS FUNCTION NOW HANDLES THE VISUALS
int CannonAimbot::AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, FRotator& oOutLow, FRotator& oOutHigh, DrawingContext* ctx, const FMinimalViewInfo& CameraInfo, int MonWidth, int MonHeight)
{
    // Step 1: Get the calculated solution without drawing anything
    AimSolution solution = SolveAimingProblem(oTargetPos, oTargetVelocity, oTargetAngularVelocity, oSourcePos, fProjectileSpeed, fProjectileGravityScalar);

    if (!solution.success) {
        // Fallback to simple linear prediction if rotational fails or isn't applicable
		return AimAtMovingTarget(oTargetPos, oTargetVelocity, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
    }

    // Step 2: Draw all the debug visuals based on the successful solution
    const float w_rad = oTargetAngularVelocity.z * (M_PI / 180.f);
    const float r_solver = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.0f).Size() / std::abs(w_rad);
    FVector vel_dir_2d = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.f).unit();
    FVector to_center_dir = (w_rad > 0) ? FVector(-vel_dir_2d.y, vel_dir_2d.x, 0.f) : FVector(vel_dir_2d.y, -vel_dir_2d.x, 0.f);
	FVector turn_center = oTargetPos + (to_center_dir * r_solver);

	FVector last_path_point = oTargetPos;
	float time_step = solution.timeOfFlight / 10.f;
	for (float t = time_step; t <= solution.timeOfFlight; t += time_step) {
		float current_t = std::min(t, solution.timeOfFlight);
        float initial_angle = atan2f(oTargetPos.y - turn_center.y, oTargetPos.x - turn_center.x);
		float current_angle = initial_angle + (w_rad * current_t);
		FVector current_path_point = turn_center + FVector(r_solver * cosf(current_angle), r_solver * sinf(current_angle), oTargetPos.z);
		Coords p1 = WorldToScreen(last_path_point, CameraInfo, MonWidth, MonHeight);
		Coords p2 = WorldToScreen(current_path_point, CameraInfo, MonWidth, MonHeight);
		if ((p1.x > 1 && p1.y > 1) || (p2.x > 1 && p2.y > 1)) {
			ctx->draw_line(p1.x, p1.y, p2.x, p2.y, 2.0f, COLOR::GREEN);
		}
		last_path_point = current_path_point;
	}
	Coords aimAtScreen = WorldToScreen(solution.impactPoint, CameraInfo, MonWidth, MonHeight);
	drawX(ctx, aimAtScreen, 10, COLOR::GREEN);

    Coords targetScreenPos = WorldToScreen(oTargetPos, CameraInfo, MonWidth, MonHeight);
    if(targetScreenPos.x > 1 && targetScreenPos.y > 1) {
	    char buffer[128];
	    snprintf(buffer, sizeof(buffer), "Mode: Rotational\nTime: %.2fs\nRadius: %.0f", solution.timeOfFlight, r_solver);
	    ctx->draw_text(targetScreenPos.x + 20, targetScreenPos.y + 20, buffer, COLOR::YELLOW);
    }

    // Step 3: Final aiming calculation
	return AimAtMovingTarget(solution.impactPoint, {0, 0, 0}, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
}

FVector CannonAimbot::PredictAimpointForPart(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& partLocation)
{
    // Step 1: Get the time-of-flight to the ship's center
    AimSolution solution = SolveAimingProblem(oTargetPos, oTargetVelocity, oTargetAngularVelocity, oSourcePos, fProjectileSpeed, fProjectileGravityScalar);

    if (!solution.success) return {0,0,0};

    // Step 2: Use that time to predict where the part will be
    float time_of_flight = solution.timeOfFlight;
    const float w_deg = oTargetAngularVelocity.z;

    // Calculate future position of the ship's center
    FVector predicted_ship_center;
    if (std::abs(w_deg) >= 1.0f) {
        float r_turn = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.0f).Size() / std::abs(w_deg * (M_PI/180.f));
        FVector vel_dir_2d = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.f).unit();
	    FVector to_center_dir = (w_deg > 0) ? FVector(-vel_dir_2d.y, vel_dir_2d.x, 0.f) : FVector(vel_dir_2d.y, -vel_dir_2d.x, 0.f);
	    FVector turn_center = oTargetPos + (to_center_dir * r_turn);
        float initial_angle = atan2f(oTargetPos.y - turn_center.y, oTargetPos.x - turn_center.x);
        float new_angle = initial_angle + (w_deg * (M_PI/180.f) * time_of_flight);
        predicted_ship_center.x = turn_center.x + r_turn * cos(new_angle);
        predicted_ship_center.y = turn_center.y + r_turn * sin(new_angle);
        predicted_ship_center.z = oTargetPos.z;
    } else {
        predicted_ship_center = oTargetPos + (oTargetVelocity * time_of_flight);
    }

    FVector initial_offset = partLocation - oTargetPos;
    float delta_yaw_rad = w_deg * (M_PI / 180.f) * time_of_flight;
    FVector rotated_offset;
    rotated_offset.x = initial_offset.x * cosf(delta_yaw_rad) - initial_offset.y * sinf(delta_yaw_rad);
    rotated_offset.y = initial_offset.x * sinf(delta_yaw_rad) + initial_offset.y * cosf(delta_yaw_rad);
    rotated_offset.z = initial_offset.z;
    FVector part_destination = predicted_ship_center + rotated_offset;

    // Step 3: Calculate the cannon angle needed to hit that destination and return a point along the aim vector
    FRotator AimLow, AimHigh;
    int solutions = AimAtMovingTarget(part_destination, {0,0,0}, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, AimLow, AimHigh);
    if (solutions > 0) {
        FVector aimDirection = RotatorToVector(AimLow);
        return oSourcePos + (aimDirection * (part_destination - oSourcePos).Size());
    }

    return {0,0,0};
}


FVector CannonAimbot::StaticPrediction(FVector PlayerPos, FVector TargetPos, float ProjectileSpeed, float ProjectileGravityScale) {
    float distance = (TargetPos - PlayerPos).Size();
    if (distance <= 100.0 || distance >= 60000.0) {
        return {0, 0, 0};
    }

    const double g = static_cast<double>(ProjectileGravityScale * 981.0f);
    const FVector diff = TargetPos - PlayerPos;
    const double dx = sqrt(diff.x * diff.x + diff.y * diff.y);
    const double dy = diff.z;
    const double v = static_cast<double>(ProjectileSpeed);
    const double v2 = v * v;
    const double v4 = v2 * v2;
    const double root_term = v4 - g * (g * dx * dx + 2 * dy * v2);

    if (root_term < 0) return {0, 0, 0};

    const double launch_angle_tan = (v2 - sqrt(root_term)) / (g * dx);
    const double time_of_flight = dx / (cos(atan(launch_angle_tan)) * v);

    if (time_of_flight <= 0) return {0,0,0};

    FVector launch_vel = diff / time_of_flight;
    launch_vel.z += 0.5f * g * time_of_flight;

    return PlayerPos + launch_vel;
}

// Math and Info-gathering functions (unchanged)
FRotator ToFRotator(FVector vec)
{
	FRotator rot;
	float RADPI = (float)(180 / M_PI);
	rot.Yaw = (float)(atan2f(vec.y, vec.x) * RADPI);
	rot.Pitch = (float)atan2f(vec.z, sqrt((vec.x * vec.x) + (vec.y * vec.y))) * RADPI;
	rot.Roll = 0;
	return rot;
}

FVector CannonAimbot::RotatorToVector(const FRotator& Rot) {
	const float pitchRad = Rot.Pitch * (M_PI / 180.0f);
	const float yawRad = Rot.Yaw * (M_PI / 180.0f);
	const float sp = sinf(pitchRad);
	const float cp = cosf(pitchRad);
	const float sy = sinf(yawRad);
	const float cy = cosf(yawRad);
	return FVector(cp * cy, cp * sy, sp);
}

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
	float root = s4 - (gravity * ((gx * x) + (2 * y * s2)));
	if (root < 0)
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

int AimAtMovingTarget(const FVector& oTargetPos, const FVector& oTargetVelocity, float fProjectileSpeed, float fProjectileGravityScalar, const FVector& oSourcePos, const FVector& oSourceVelocity, FRotator& oOutLow, FRotator& oOutHigh) {
	const FVector v(oTargetVelocity - oSourceVelocity);
	const FVector g(0.f, 0.f, -981.f * fProjectileGravityScalar);
	const FVector p(oTargetPos - oSourcePos);
	const float c4 = g | g * 0.25f;
	const float c3 = v | g;
	const float c2 = (p | g) + (v | v) - (fProjectileSpeed * fProjectileSpeed);
	const float c1 = 2.f * (p | v);
	const float c0 = p | p;
	std::complex<float> pOutRoots[4];
	std::complex<float> pInCoeffs[5] = { c0, c1, c2, c3, c4 };
	SolveQuartic(pInCoeffs, pOutRoots);
	float fBestRoot = FLT_MAX;
	for (int i = 0; i < 4; i++) {
		if (pOutRoots[i].real() > 0.f && std::abs(pOutRoots[i].imag()) < 0.0001f && pOutRoots[i].real() < fBestRoot) {
			fBestRoot = pOutRoots[i].real();
		}
	}
	if (fBestRoot == FLT_MAX)
		return 0;
	const FVector oAimAt = oTargetPos + (v * fBestRoot);
	return AimAtStaticTarget(oAimAt, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oOutLow, oOutHigh);
}

float time_func(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
{
	const float K2 = K * K; const float L2 = L * L; const float M2 = M * M; const float N2 = N * N; const float r2 = r * r;
	return N2 * t * t * t * t + (2 * M * N - S2) * t * t + 2 * r * (K * cos(theta + (w * t)) + L * sin(theta + (w * t))) + K2 + L2 + M2 + r2;
}

float time_derivFunc(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
{
	const float N2 = N * N;
	return 4 * N2 * t * t * t + 2 * (2 * M * N - S2) * t + 2 * r * w * (L * cos(theta + (w * t)) - K * sin(theta + w * t));
}

float newtonRaphson(float t, float K, float L, float M, float N, float r, float w, float theta, float S2)
{
	if (t < 0.f) return -1.f;
	for (int i = 0; i < 20; ++i)
	{
		float f_t = time_func(t, K, L, M, N, r, w, theta, S2);
		float fd_t = time_derivFunc(t, K, L, M, N, r, w, theta, S2);
		if (std::abs(fd_t) < 1e-6) return -1.f;
		float h = f_t / fd_t;
		t = t - h;
		if (std::abs(h) < 0.001) return t;
	}
	return -1.f;
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