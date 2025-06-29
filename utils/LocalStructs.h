//Used for passing data between threads, does not contain structs from the game
#ifndef LOCALSTRUCTS_H
#define LOCALSTRUCTS_H
#pragma once
#include "GameStructs.h"
#include "../overlay/drawing.h" //for colors
#include <array>


enum EntTypes {
    NETPROXYSHIP,
    SHIP,
    ROWBOAT,
    STORM,

    PLAYER,
    MERMADE,

    BASICENTITY
};

typedef struct {
    std::string displayName = "Player";
    int distance = 0;

    int health = 100;
    bool sameTeam = false; //if true, the player is on the same team as you

    uintptr_t meshComponentPtr;

    GUID shipID;
} Player;


struct NetProxyShipData {
    std::string displayName = "Far Ship";
    int distance = 0;
};

typedef struct {
    std::string displayName = "Rowboat";
    int distance = 0;
} Rowboat;

typedef struct {
    int distance = 0;
} Storm;

struct MermadeData {
    std::string displayName = "Mermade";
    int distance = 0;
};

struct BasicEntity {
    std::string displayName = "Item";
    int distance = 0;
    COLOR::Color color = COLOR::WHITE;
};

typedef struct { //read everything from pawn
    float health;

    std::string displayName = "Ship";
    double distance = 0;
    float WaterAmount = 0; //0-1, 1 is full
    std::array<FVector, 30> holeLocations;
    int holeCount = 0;

    bool onShip = false;

    //trajectory

} Ship;

typedef struct {

} Island;


typedef struct {
    std::string name;
    FVector location;

    EntTypes type; //determins what type to use


    //memory inefficient but its negligible
    NetProxyShipData netProxyShip;
    MermadeData mermade;
    Ship ship;
    Player player;
    Rowboat rowboat;
    Storm storm;

    BasicEntity basicEntity;

} Entity;



typedef struct {
    //Overlay ov; // Overlay instance for drawing
    uintptr_t CameraCachePointer;
    int localHealth;
} CheatData;

typedef struct {
   int x, y = -1;
} Coords;

extern CheatData GlobalCheatData;

inline Coords WorldToScreen(FVector TargetLocation, FMinimalViewInfo CameraInfo, float ScreenWidth, float ScreenHeight) {
    Coords ScreenLocation = {-1, -1}; // Default to invalid coordinates

    FVector Rotation = CameraInfo.Rotation;
    D3DMATRIX tempMatrix = Matrix(Rotation, FVector(0, 0, 0));

    FVector vAxisX = FVector(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
    FVector vAxisY = FVector(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
    FVector vAxisZ = FVector(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

    FVector vDelta = TargetLocation - CameraInfo.Location;
    FVector vTransformed = FVector(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

    // If the object is behind the player, don't draw it.
    if (vTransformed.z < 1.f) {
        return ScreenLocation;
    }

    float FovAngle = CameraInfo.FOV;
    float ScreenCenterX = ScreenWidth / 2.0f;
    float ScreenCenterY = ScreenHeight / 2.0f;

    // Use the correct projection formula
    float scale = ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.0f);
    ScreenLocation.x = ScreenCenterX + vTransformed.x * scale / vTransformed.z;
    ScreenLocation.y = ScreenCenterY - vTransformed.y * scale / vTransformed.z;

    return ScreenLocation;
}

// Calculates the end-point for a tracer line, clamping it to the screen edges if the target is off-screen.
inline Coords CalculateTracerEndPoint(FVector TargetLocation, FMinimalViewInfo CameraInfo, float ScreenWidth, float ScreenHeight) {
    Coords TracerEndPoint = {-1, -1};

    // --- 1. 3D to 2D Projection (same as W2S) ---
    FVector Rotation = CameraInfo.Rotation;
    D3DMATRIX tempMatrix = Matrix(Rotation, FVector(0, 0, 0));

    FVector vAxisX = FVector(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
    FVector vAxisY = FVector(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
    FVector vAxisZ = FVector(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

    FVector vDelta = TargetLocation - CameraInfo.Location;
    FVector vTransformed = FVector(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

    // --- 2. Handle Targets Behind The Camera ---
    // If the target is behind, we "flip" its projected position so it is clamped to the correct edge.
    // The original W2S would just ignore it, but for a tracer, we want to know the direction.
    if (vTransformed.z < 1.f) {
        vTransformed.x *= -1.f;
        vTransformed.y *= -1.f;
    }

    float FovAngle = CameraInfo.FOV;
    const float ScreenCenterX = ScreenWidth / 2.0f;
    const float ScreenCenterY = ScreenHeight / 2.0f;

    // Project the 3D position to 2D screen space
    float scale = ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.0f);
    TracerEndPoint.x = ScreenCenterX + vTransformed.x * scale / (vTransformed.z < 1.f ? 1.f : vTransformed.z);
    TracerEndPoint.y = ScreenCenterY - vTransformed.y * scale / (vTransformed.z < 1.f ? 1.f : vTransformed.z);

    // --- 3. Check if the point is on-screen ---
    if (TracerEndPoint.x > 0 && TracerEndPoint.x < ScreenWidth &&
        TracerEndPoint.y > 0 && TracerEndPoint.y < ScreenHeight) {
        // The target is on-screen, so the tracer ends directly on them.
        return TracerEndPoint;
    }

    // --- 4. Clamp the Off-Screen Point to the Screen Edge ---
    // The point is off-screen, so we calculate the intersection with the screen edge.

    // Start with the calculated (off-screen) point
    Coords OffScreenPoint = TracerEndPoint;

    // Vector from the center of the screen to the off-screen point
    float vec_x = OffScreenPoint.x - ScreenCenterX;
    float vec_y = OffScreenPoint.y - ScreenCenterY;

    // Calculate the slope of the line from the center to the off-screen point.
    // Handle the case of a vertical line to avoid division by zero.
    float slope = (vec_x != 0.0f) ? (vec_y / vec_x) : 0.0f;

    // We use the properties of similar triangles to find the intersection point.
    // Determine which edge (top/bottom or left/right) the line will intersect first.
    if (fabsf(vec_x) > fabsf(vec_y)) {
        // Intersects with left or right edge
        if (vec_x > 0) { // Right edge
            TracerEndPoint.x = ScreenWidth -1; // -1 to keep it just inside
            TracerEndPoint.y = ScreenCenterY + slope * (TracerEndPoint.x - ScreenCenterX);
        } else { // Left edge
            TracerEndPoint.x = 1; // +1 to keep it just inside
            TracerEndPoint.y = ScreenCenterY + slope * (TracerEndPoint.x - ScreenCenterX);
        }
    } else {
        // Intersects with top or bottom edge
        if (vec_y > 0) { // Bottom edge
            TracerEndPoint.y = ScreenHeight - 1;
            if (slope != 0.0f) { // Avoid division by zero if it was a vertical line
                 TracerEndPoint.x = ScreenCenterX + (TracerEndPoint.y - ScreenCenterY) / slope;
            }
        } else { // Top edge
            TracerEndPoint.y = 1;
             if (slope != 0.0f) {
                 TracerEndPoint.x = ScreenCenterX + (TracerEndPoint.y - ScreenCenterY) / slope;
            }
        }
    }

    // Finally, ensure the calculated point is truly on the screen (handles corners).
    TracerEndPoint.x = std::max(1.f, std::min(ScreenWidth - 1.f, (float)TracerEndPoint.x));
    TracerEndPoint.y = std::max(1.f, std::min(ScreenHeight - 1.f, (float)TracerEndPoint.y));

    return TracerEndPoint;
}


#endif //LOCALSTRUCTS_H
