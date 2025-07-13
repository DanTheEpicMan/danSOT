#include "CannonAimbot.h"
#include <cmath>
#include <complex>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdio> // For snprintf used in debug text
#include <optional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cfloat>
#include <complex>
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

#include <math.h>

FRotator ToFRotator(FVector vec)
{
	FRotator rot;
	float RADPI = (float)(180 / M_PI);
	rot.Yaw = (float)(atan2f(vec.y, vec.x) * RADPI);
	rot.Pitch = (float)atan2f(vec.z, sqrt((vec.x * vec.x) + (vec.y * vec.y))) * RADPI;
	rot.Roll = 0;
	return rot;
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

#include <limits>
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
	const float K2 = K * K;
	const float L2 = L * L;
	const float M2 = M * M;
	const float N2 = N * N;
	const float r2 = r * r;
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

// =======================================================================================================================
// === REWRITTEN AimAtShip FUNCTION WITH MOVING SOURCE COMPENSATION AND MORE DEBUGGING
// =======================================================================================================================
int AimAtShip(const FVector& oTargetPos, const FVector& oTargetVelocity, const FVector& oTargetAngularVelocity, const FVector& oSourcePos, const FVector& oSourceVelocity, float fProjectileSpeed, float fProjectileGravityScalar, FRotator& oOutLow, FRotator& oOutHigh, DrawingContext* ctx, const FMinimalViewInfo& CameraInfo, int MonWidth, int MonHeight)
{
	Coords targetScreenPos = WorldToScreen(oTargetPos, CameraInfo, MonWidth, MonHeight);
	bool targetOnScreen = targetScreenPos.x > 1 && targetScreenPos.y > 1 && targetScreenPos.x < MonWidth && targetScreenPos.y < MonHeight;

	const float w_deg = oTargetAngularVelocity.z;

	// Use linear prediction if the target isn't turning significantly.
	if (std::abs(w_deg) < 1.0f)
	{
		if (targetOnScreen) {
			ctx->draw_text(targetScreenPos.x, targetScreenPos.y - 40, "Mode: Linear (Not Turning)", COLOR::YELLOW);
		}
		// The standard moving-target solver already handles a moving source.
		return AimAtMovingTarget(oTargetPos, oTargetVelocity, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
	}

	const float v_target_boat = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.0f).Size();
	if (v_target_boat < 50.f)
	{
		if (targetOnScreen) {
			ctx->draw_text(targetScreenPos.x, targetScreenPos.y - 25, "Target too slow, using Linear", COLOR::YELLOW);
		}
		return AimAtMovingTarget(oTargetPos, oTargetVelocity, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
	}

	// --- SETUP PARAMETERS ---
	// IMPORTANT: The rotational solver math assumes a STATIC observer.
	// We run the solver as if we are not moving, find the impact point and time,
	// and THEN we compensate for our own movement in the final aiming step.
	const FVector pPos = oSourcePos;
	const FVector diff_to_target = oTargetPos - pPos;
	const float w_rad = w_deg * (M_PI / 180.f);
	const float r_solver = v_target_boat / std::abs(w_rad);

	// --- GEOMETRY & SOLVER SETUP ---
	FVector vel_dir_2d = FVector(oTargetVelocity.x, oTargetVelocity.y, 0.f).unit();
	FVector to_center_dir = (w_rad > 0) ? FVector(-vel_dir_2d.y, vel_dir_2d.x, 0.f) : FVector(vel_dir_2d.y, -vel_dir_2d.x, 0.f);
	FVector turn_center = oTargetPos + (to_center_dir * r_solver);
	FVector from_center_to_ship = oTargetPos - turn_center;
	float theta_rad = atan2f(from_center_to_ship.y, from_center_to_ship.x);

	const FVector diff_to_center = turn_center - pPos;
	const float K = diff_to_center.x;
	const float L = diff_to_center.y;
	const float M = diff_to_target.z;
	const float N = (981.f * fProjectileGravityScalar) / 2.f;
	const float S2 = fProjectileSpeed * fProjectileSpeed;
	const float fGroundDist = FVector(diff_to_target.x, diff_to_target.y, 0.0f).Size();
	float t_init = fGroundDist / fProjectileSpeed;

	float t_best = newtonRaphson(t_init, K, L, M, N, r_solver, w_rad, theta_rad, S2);

	// --- POST-SOLVER CHECKS ---
	if (t_best < 0.f || t_best > 12.0f)
	{
		if (targetOnScreen) {
			ctx->draw_text(targetScreenPos.x, targetScreenPos.y, "ROTATIONAL SOLVER FAILED", COLOR::RED);
		}
		return 0;
	}

	// --- PREDICTION SUCCESSFUL ---
	float final_angle = theta_rad + (w_rad * t_best);
	const FVector oAimAt = turn_center + FVector(r_solver * cosf(final_angle), r_solver * sinf(final_angle), oTargetPos.z);

	// --- EXTENSIVE DEBUG VISUALIZATION ---
	// Calculate player's future position
	FVector futureSourcePos = oSourcePos + (oSourceVelocity * t_best);

	// Project all key points to the screen
	Coords aimAtScreen = WorldToScreen(oAimAt, CameraInfo, MonWidth, MonHeight);
	Coords sourceScreen = WorldToScreen(oSourcePos, CameraInfo, MonWidth, MonHeight);
	Coords futureSourceScreen = WorldToScreen(futureSourcePos, CameraInfo, MonWidth, MonHeight);

	// Draw the true shot path (from where you WILL BE to where the target WILL BE)
	if ((futureSourceScreen.x > 1 && futureSourceScreen.y > 1) || (aimAtScreen.x > 1 && aimAtScreen.y > 1)) {
		ctx->draw_line(futureSourceScreen.x, futureSourceScreen.y, aimAtScreen.x, aimAtScreen.y, 2.0f, COLOR::YELLOW);
	}
	// Draw a line from your current position to the impact point for reference
	if ((sourceScreen.x > 1 && sourceScreen.y > 1) || (aimAtScreen.x > 1 && aimAtScreen.y > 1)) {
		ctx->draw_line(sourceScreen.x, sourceScreen.y, aimAtScreen.x, aimAtScreen.y, 1.0f, COLOR::CYAN);
	}

	// Draw markers for player's current and future positions
	ctx->draw_text(sourceScreen.x, sourceScreen.y, "[Player Now]", COLOR::CYAN);
	drawX(ctx, futureSourceScreen, 8, COLOR::YELLOW);
	ctx->draw_text(futureSourceScreen.x + 10, futureSourceScreen.y, "[Player Future]", COLOR::YELLOW);

	// Draw the target's predicted path (green line) and final impact point (green X)
	FVector last_path_point = oTargetPos;
	float time_step = t_best / 10.f;
	for (float t = time_step; t < t_best + time_step; t += time_step) {
		float current_t = std::min(t, t_best);
		float current_angle = theta_rad + (w_rad * current_t);
		FVector current_path_point = turn_center + FVector(r_solver * cosf(current_angle), r_solver * sinf(current_angle), oTargetPos.z);
		Coords p1 = WorldToScreen(last_path_point, CameraInfo, MonWidth, MonHeight);
		Coords p2 = WorldToScreen(current_path_point, CameraInfo, MonWidth, MonHeight);
		if ((p1.x > 1 && p1.y > 1) || (p2.x > 1 && p2.y > 1)) {
			ctx->draw_line(p1.x, p1.y, p2.x, p2.y, 2.0f, COLOR::GREEN);
		}
		last_path_point = current_path_point;
	}
	drawX(ctx, aimAtScreen, 10, COLOR::GREEN);

	// Draw the comprehensive text block
	if (targetOnScreen) {
		char buffer[512];
		FVector relativeVel = oTargetVelocity - oSourceVelocity;
		snprintf(buffer, sizeof(buffer),
			"Mode: Rotational (Moving Src)\n"
			"Time: %.2fs\n"
			"Radius: %.0f\n"
			"Player Vel: %.0f, %.0f\n"
			"Target Vel: %.0f, %.0f\n"
			"Relative Vel: %.0f, %.0f",
			t_best, r_solver,
			oSourceVelocity.x, oSourceVelocity.y,
			oTargetVelocity.x, oTargetVelocity.y,
			relativeVel.x, relativeVel.y
		);
		ctx->draw_text(targetScreenPos.x + 20, targetScreenPos.y + 20, buffer, COLOR::YELLOW);
	}

	// ==============================================================================
    // === THE FIX: Use AimAtMovingTarget to compensate for our own movement.
    // ==============================================================================
	// We are aiming at a static point in space (oAimAt) from a moving cannon.
	// We give the target point {0,0,0} velocity.
	return AimAtMovingTarget(oAimAt, {0, 0, 0}, fProjectileSpeed, fProjectileGravityScalar, oSourcePos, oSourceVelocity, oOutLow, oOutHigh);
}


FVector RotatorToVector(const FRotator& Rot) {
	const float pitchRad = Rot.Pitch * (M_PI / 180.0f);
	const float yawRad = Rot.Yaw * (M_PI / 180.0f);
	const float sp = sinf(pitchRad);
	const float cp = cosf(pitchRad);
	const float sy = sinf(yawRad);
	const float cy = cosf(yawRad);
	return FVector(cp * cy, cp * sy, sp);
}

void CannonAimbot::Run(uintptr_t GNames, uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> ships, std::vector<Entity> Enemies, std::vector<Entity> OtherEntities, DrawingContext *ctx, InputManager *inpMngr) {

    uintptr_t cannonActor = GetCannonActor(LPawn, GNames);
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

        FVector rotationAimPoint = RotationPrediction(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel);
        if (rotationAimPoint.x != 0.f || rotationAimPoint.y != 0.f || rotationAimPoint.z != 0.f) {
            Coords screenPos = WorldToScreen(rotationAimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                drawX(ctx, screenPos, 5, COLOR::GREEN);
            }
        }

    	FRotator AimLow, AimHigh;
    	int solutions = AimAtShip(shipCoords, shipLinearVel, shipAngularVel, CameraCache.POV.Location, localPlayerVelocity, projectileSpeed, projectileGravityScale, AimLow, AimHigh, ctx, CameraInfo, MonWidth, MonHeight);

    	if (solutions > 0) {
    		FVector startPoint = CameraCache.POV.Location;
    		FVector aimDirection = RotatorToVector(AimLow);
    		float distanceToShip = (shipCoords - startPoint).Size();
    		FVector worldAimPoint = startPoint + (aimDirection * distanceToShip);
    		Coords screenPos = WorldToScreen(worldAimPoint, CameraInfo, MonWidth, MonHeight);
    		if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
    			drawX(ctx, screenPos, 8, COLOR::MAGENTA);
                ctx->draw_text(screenPos.x + 10, screenPos.y, "Final Aim", COLOR::MAGENTA);
    		}
    	}

        std::vector<FVector> shipActiveHoles, shipInactiveHoles, shipMasts, cannonLocations;
        FVector shipWheel;
        GetShipComponents(ship, OtherEntities, shipActiveHoles, shipInactiveHoles, shipMasts, cannonLocations, shipWheel);
        for (const auto& hole : shipActiveHoles) {
            FVector aimPoint = RotationPredictionForPart(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel, shipInitialRotation, hole);
            if (aimPoint.x != 0.f || aimPoint.y != 0.f || aimPoint.z != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    drawX(ctx, screenPos, 5, COLOR::RED);
                }
            }
        }
        for (const auto& hole : shipInactiveHoles) {
            FVector aimPoint = RotationPredictionForPart(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel, shipInitialRotation, hole);
            if (aimPoint.x != 0.f || aimPoint.y != 0.f || aimPoint.z != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    drawX(ctx, screenPos, 5, COLOR::BLUE);
                }
            }
        }
        for (const auto& mast : shipMasts) {
            FVector aimPoint = RotationPredictionForPart(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel, shipInitialRotation, mast);
            if (aimPoint.x != 0.f || aimPoint.y != 0.f || aimPoint.z != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                Coords screenPosTop = WorldToScreen({mast.x, mast.y, mast.z + 1000.f}, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    ctx->draw_line(screenPos.x, screenPos.y, screenPosTop.x, screenPosTop.y, 2.0f, COLOR::MAGENTA);
                }
            }
        }
        for (const auto& cannon : cannonLocations) {
            FVector aimPoint = RotationPredictionForPart(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel, shipInitialRotation, cannon);
            if (aimPoint.x != 0.f || aimPoint.y != 0.f || aimPoint.z != 0.f) {
                Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
                if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                    ctx->draw_text(screenPos.x, screenPos.y, "[CANNON]", COLOR::CYAN);
                }
            }
        }
        FVector aimPoint = RotationPredictionForPart(CameraCache, localPlayerVelocity, projectileSpeed, projectileGravityScale, shipCoords, shipLinearVel, shipAngularVel, shipInitialRotation, shipWheel);
        if (aimPoint.x != 0.f || aimPoint.y != 0.f || aimPoint.z != 0.f) {
            Coords screenPos = WorldToScreen(aimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                ctx->draw_text(screenPos.x, screenPos.y, "[WHEEL]", COLOR::CYAN);
            }
        }

        FVector staticVel = {0, 0, 0};
        FVector staticAngVel = {0, 0, 0};
        FVector staticPlayerVel = {0, 0, 0};
        FVector quarticAimPoint = QuarticPrediction(CameraCache, staticPlayerVel, projectileSpeed, projectileGravityScale, shipCoords, staticVel);
        if (quarticAimPoint.x != 0.f || quarticAimPoint.y != 0.f || quarticAimPoint.z != 0.f) {
            Coords screenPos = WorldToScreen(quarticAimPoint, CameraInfo, MonWidth, MonHeight);
            if (screenPos.x > 1 && screenPos.y > 1 && screenPos.x < MonWidth && screenPos.y < MonHeight) {
                ctx->draw_text(screenPos.x, screenPos.y, "[STATIC]", COLOR::YELLOW);
            }
        }
    }
}



///////////////// Cannon Prediction Functions //////////////////

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

// Helper to convert FRotator from degrees to radians
FRotator ToRadians(const FRotator& Rot) {
    return {
        Rot.Pitch * M_PI / 180.0,
        Rot.Yaw   * M_PI / 180.0,
        Rot.Roll  * M_PI / 180.0
    };
}

// Corrected RotateVectorByRotator
FVector RotateVectorByRotator(const FVector& V, const FRotator& R_deg) {
    FRotator R_rad = ToRadians(R_deg);
    double cy = cos(R_rad.Yaw);
    double sy = sin(R_rad.Yaw);
    double cp = cos(R_rad.Pitch);
    double sp = sin(R_rad.Pitch);
    double cr = cos(R_rad.Roll);
    double sr = sin(R_rad.Roll);

    FVector Rotated;
    Rotated.x = V.x * (cp * cy) + V.y * (sr * sp * cy - cr * sy) + V.z * (cr * sp * cy + sr * sy);
    Rotated.y = V.x * (cp * sy) + V.y * (sr * sp * sy + cr * cy) + V.z * (cr * sp * sy - sr * cy);
    Rotated.z = V.x * (-sp)     + V.y * (sr * cp)                  + V.z * (cr * cp);
    return Rotated;
}

// Corrected UnrotateVectorByRotator (using transposed matrix)
FVector UnrotateVectorByRotator(const FVector& V, const FRotator& R_deg) {
    FRotator R_rad = ToRadians(R_deg);
    double cy = cos(R_rad.Yaw);
    double sy = sin(R_rad.Yaw);
    double cp = cos(R_rad.Pitch);
    double sp = sin(R_rad.Pitch);
    double cr = cos(R_rad.Roll);
    double sr = sin(R_rad.Roll);

    // This is the transposed version of the rotation matrix above
    FVector Unrotated;
    Unrotated.x = V.x * (cp * cy) + V.y * (cp * sy) + V.z * (-sp);
    Unrotated.y = V.x * (sr * sp * cy - cr * sy) + V.y * (sr * sp * sy + cr * cy) + V.z * (sr * cp);
    Unrotated.z = V.x * (cr * sp * cy + sr * sy) + V.y * (cr * sp * sy - sr * cy) + V.z * (cr * cp);
    return Unrotated;
}

FVector CannonAimbot::RotationPredictionForPart(
    FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale,
    FVector ShipCenterCoords, FVector ShipLinearVel, FVector ShipAngularVel, FRotator ShipInitialRotation,
    FVector TargetPartGlobalCoords)
{
    // --- Step 1: Calculate the current offset of the part from the ship's center in world space. ---
    // This offset vector will be rotated to predict the part's future position.
    FVector globalOffset = TargetPartGlobalCoords - ShipCenterCoords;

    // --- Scaling for physics calculations ---
    double gravityM = static_cast<double>(ProjectileGravityScale * 981.0f) / 100.0;
    double projectileSpeedM = static_cast<double>(ProjectileSpeed) / 100.0;
    FVector cannonCoordsM = LPCam.POV.Location / 100.0;
    FVector shipCenterCoordsM = ShipCenterCoords / 100.0;
    FVector lpShipLinearVelM = LPLinearVel / 100.0;
    FVector globalOffsetM = globalOffset / 100.0; // Scale the offset too

    // We only care about turning left/right, which is Yaw (Z-axis angular velocity).
    double angularVelZ_deg = ShipAngularVel.z;
    double angularVelZ_rad = angularVelZ_deg * M_PI / 180.0;

    // Use the circular prediction model only if the ship is actually turning.
    bool isTurning = std::abs(angularVelZ_rad) > 1e-4;

    double linearVelX_M = ShipLinearVel.x / 100.0;
    double linearVelY_M = ShipLinearVel.y / 100.0;

    float tTime = 0.0f;
    const float tIterator = 0.05f;

    while (tTime < 20.0f) {
        FVector predictedShipCenterM;

        // --- Step 2: Predict the future position of the SHIP'S CENTER ---
        if (isTurning) {
            double speedMagnitudeM = std::sqrt(linearVelX_M * linearVelX_M + linearVelY_M * linearVelY_M);
            if (speedMagnitudeM > 0.1) { // Ensure ship is actually moving
                double diskRadius = speedMagnitudeM / angularVelZ_rad;
                double effectiveRadius = diskRadius * 0.98;
                double actualHeading = std::atan2(linearVelY_M, linearVelX_M);
                FVector turnCenter = {
                    static_cast<float>(effectiveRadius * std::cos(actualHeading + M_PI / 2.0) + shipCenterCoordsM.x),
                    static_cast<float>(effectiveRadius * std::sin(actualHeading + M_PI / 2.0) + shipCenterCoordsM.y),
                    shipCenterCoordsM.z
                };
                double angleTheta = actualHeading - M_PI / 2.0;

                double wTime = angularVelZ_rad * tTime;
                predictedShipCenterM.x = turnCenter.x + (effectiveRadius * std::cos(wTime + angleTheta));
                predictedShipCenterM.y = turnCenter.y + (effectiveRadius * std::sin(wTime + angleTheta));
                predictedShipCenterM.z = shipCenterCoordsM.z; // Z position of the center is assumed constant
            } else {
                 isTurning = false; // Not moving, so not turning in a circle. Fallback to linear.
            }
        }
        if (!isTurning) {
            // If not turning (or not moving), use simple linear prediction for the center.
            predictedShipCenterM.x = shipCenterCoordsM.x + (linearVelX_M * tTime);
            predictedShipCenterM.y = shipCenterCoordsM.y + (linearVelY_M * tTime);
            predictedShipCenterM.z = shipCenterCoordsM.z;
        }

        // Account for our own ship's movement.
        predictedShipCenterM = predictedShipCenterM - (lpShipLinearVelM * tTime);


        // --- Step 3: Predict the future position of the SHIP PART ---
        // This is the key change: We apply a simple 2D rotation to the initial offset.
        double deltaYaw_rad = angularVelZ_rad * tTime;
        double cos_yaw = std::cos(deltaYaw_rad);
        double sin_yaw = std::sin(deltaYaw_rad);

        // Rotate the horizontal components of the offset.
        FVector rotatedOffsetM;
        rotatedOffsetM.x = globalOffsetM.x * cos_yaw - globalOffsetM.y * sin_yaw;
        rotatedOffsetM.y = globalOffsetM.x * sin_yaw + globalOffsetM.y * cos_yaw;

        // Keep the vertical offset constant, as requested.
        rotatedOffsetM.z = globalOffsetM.z;

        // The final predicted part position is the predicted center + the rotated offset.
        FVector predictedPartPosM = predictedShipCenterM + rotatedOffsetM;


        // --- Step 4: Run ballistics calculation on the predicted part position ---
        double nDist = get_2d_distance(cannonCoordsM, predictedPartPosM);
        if (nDist < 1.0) { // Avoid division by zero if distance is negligible
             tTime += tIterator;
             continue;
        }
        double angleTan = get_launch_angle_tan(cannonCoordsM, predictedPartPosM, gravityM, projectileSpeedM);
        if (projectileSpeedM * std::cos(std::atan(angleTan)) == 0) {
             tTime += tIterator;
             continue;
        }
        double sTime = nDist / (projectileSpeedM * std::cos(std::atan(angleTan)));

        if (sTime < tTime) {
            predictedPartPosM.z = get_aim_z(cannonCoordsM, predictedPartPosM, gravityM, projectileSpeedM);
            return predictedPartPosM * 100.0; // Scale back to game units
        }
        tTime += tIterator;
    }
    return {0,0,0};
}

FVector CannonAimbot::QuarticPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel) {
    // All calculations and checks in this function use original game units.
    double distance = get_2d_distance(LPCam.POV.Location, ShipCoords);
    if (distance <= 5000.0 || distance >= 50000.0) {
        return {0,0,0};
    }

    const double gravity = static_cast<double>(ProjectileGravityScale * 981.0f);
    const FVector gravityVec = {0.0, 0.0, static_cast<float>(-gravity)};
    const FVector netVelVec = ShipLinearVel - LPLinearVel;
    const FVector netPosVec = ShipCoords - LPCam.POV.Location;

    auto dot = [](const FVector& a, const FVector& b) {return a.x*b.x+a.y*b.y+a.z*b.z;};

    // Check if the target is stationary
    bool isStatic = (std::abs(netVelVec.x) < 0.1 && std::abs(netVelVec.y) < 0.1 && std::abs(netVelVec.z) < 0.1);

    double bestRoot = -1.0; // Use -1 to indicate no solution found yet

    if (isStatic) {

        // Equation is: (0.25*g^2)*t^4 + (dot(p,g) - V^2)*t^2 + dot(p,p) = 0
        // Which is A*y^2 + B*y + C = 0 where y = t^2
        double A = dot(gravityVec, gravityVec) * 0.25;
        double B = dot(netPosVec, gravityVec) - (ProjectileSpeed * ProjectileSpeed);
        double C = dot(netPosVec, netPosVec);

        // Solve the quadratic equation for y = t^2
        double discriminant = B * B - 4 * A * C;

        if (discriminant >= 0) {
            double sqrt_discriminant = std::sqrt(discriminant);
            double y1 = (-B + sqrt_discriminant) / (2 * A);
            double y2 = (-B - sqrt_discriminant) / (2 * A);

            // We need a positive, real t, so we need a positive y
            if (y1 > 0) bestRoot = std::sqrt(y1);
            if (y2 > 0) bestRoot = std::min(bestRoot, std::sqrt(y2));
        }

    } else {

        const double c4 = dot(gravityVec, gravityVec) * 0.25;
        const double c3 = dot(netVelVec, gravityVec);
        const double c2 = dot(netPosVec, gravityVec) + dot(netVelVec, netVelVec) - (ProjectileSpeed * ProjectileSpeed);
        const double c1 = 2.0 * dot(netPosVec, netVelVec);
        const double c0 = dot(netPosVec, netPosVec);

        std::vector<std::complex<double>> coeffs = {{c0,0},{c1,0},{c2,0},{c3,0},{c4,0}};
        std::vector<std::complex<double>> roots(4);
        solve_quartic(coeffs, roots);

        bestRoot = std::numeric_limits<double>::max();
        for (int i = 0; i < 4; ++i) {
            if (roots[i].real() > 0.0 && std::abs(roots[i].imag()) < 1e-4) {
                bestRoot = std::min(bestRoot, roots[i].real());
            }
        }
        if (bestRoot == std::numeric_limits<double>::max()) bestRoot = -1.0; // Standardize "no solution"
    }


    if (bestRoot <= 0) {
        return {0,0,0};
    }

    FVector aimAt = ShipCoords + (netVelVec * bestRoot);
    aimAt.z = get_aim_z(LPCam.POV.Location, aimAt, gravity, ProjectileSpeed);
    return aimAt;
}

/////////////// Helper Functions //////////////

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

        //if distance is over 30m, skip
        if (get_2d_distance(ent.location, ShipActor.location) > 3000.0f) continue;

        if (ent.name == "BP_SmallShip_Mast_C" || ent.name == "BP_large_mast_mizzen_C" || ent.name == "BP_large_mast_main_C"
            || ent.name == "BP_medium_mast_fore_C" || ent.name == "BP_medium_mast_main_C") { //BP_SmallShip_Mast_C (//BP_large_mast_mizzen_C //BP_large_mast_main_C ) //BP_medium_mast_fore_C //BP_medium_mast_main_C
            outShipMasts.push_back(ent.location);
        } else if (ent.name.find("BP_Cannon_ShipPartMMC_") != std::string::npos) { //BP_Cannon_ShipPartMMC_b_C, BP_Cannon_ShipPartMMC_C (same)
            outCannonLocation.push_back(ent.location);
        } else if (ent.name.find("hipWheel_C") != std::string::npos) { //BP_SmallShipWheel_C //BP_LargeShipWheel_C //BP_MediumShipWheel_C
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


FVector CannonAimbot::StaticPrediction(FVector PlayerPos, FVector TargetPos, float ProjectileSpeed, float ProjectileGravityScale) {
    // 1. --- SETUP ---
    double distance = get_2d_distance(PlayerPos, TargetPos);
    // Optional: Filter out targets that are too close or too far.
    if (distance <= 100.0 || distance >= 50000.0) {
        return {0, 0, 0};
    }

    const double g_scalar = static_cast<double>(ProjectileGravityScale * 981.0f);
    const FVector gravity_vec = {0.0, 0.0, static_cast<float>(-g_scalar)};
    const FVector relative_pos = TargetPos - PlayerPos;

    // 2. --- SOLVE FOR TIME OF FLIGHT (t) ---
    // The physics equation simplifies to a quadratic equation in terms of t² (time squared).
    // Let y = t², the equation is: Ay² + By + C = 0
    // See the math explanation below for the derivation of these coefficients.

    // dot(gravity_vec, gravity_vec) is just g²
	auto dot = [](const FVector& a, const FVector& b) {return a.x*b.x+a.y*b.y+a.z*b.z;};
    double A = 0.25 * dot(gravity_vec, gravity_vec);
    // Note: The correct B coefficient is -(dot(relative_pos, gravity_vec) + ProjectileSpeed²).
    // The original code may have used a different sign convention or simplification.
    // This version uses the physically derived formula for a robust solution.
    double B = -(dot(relative_pos, gravity_vec) + (ProjectileSpeed * ProjectileSpeed));
    // dot(relative_pos, relative_pos) is the squared 3D distance to the target.
    double C = dot(relative_pos, relative_pos);

    // Solve the quadratic equation for y = t²
    double discriminant = B * B - 4 * A * C;
    if (discriminant < 0) {
        // No real solutions for t², meaning it's impossible to hit the target (e.g., out of range).
        return {0, 0, 0};
    }

    double sqrt_discriminant = std::sqrt(discriminant);
    double y1 = (-B + sqrt_discriminant) / (2 * A);
    double y2 = (-B - sqrt_discriminant) / (2 * A);

    // Find the smallest, positive, real time of flight 't'.
    // We need y (which is t²) to be positive.
    double time_of_flight = -1.0;
    if (y1 > 0) {
        time_of_flight = std::sqrt(y1);
    }
    if (y2 > 0) {
        double t2 = std::sqrt(y2);
        if (time_of_flight < 0 || t2 < time_of_flight) {
            time_of_flight = t2;
        }
    }

    if (time_of_flight <= 0) {
        // No positive time solution found.
        return {0, 0, 0};
    }

    // 3. --- CALCULATE AIM POINT ---
    // Now that we have the time 't', we can calculate the initial velocity vector
    // required for the projectile to land at the target's position.
    // V_launch = (TargetPos - PlayerPos - 0.5 * g * t²) / t
    FVector launch_velocity = (relative_pos - gravity_vec * 0.5 * time_of_flight * time_of_flight) / time_of_flight;

    // The function should return a point to aim at. By returning a point along the
    // required launch velocity vector, the cannon will aim in the correct direction.
    // PlayerPos + launch_velocity gives a point in the correct direction from the player.
    return PlayerPos + launch_velocity;
}