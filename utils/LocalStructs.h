//Used for passing data between threads, does not contain structs from the game
#ifndef LOCALSTRUCTS_H
#define LOCALSTRUCTS_H
#pragma once
#include "GameStructs.h"
#include <array>


enum EntTypes {
    NETPROXYSHIP,
    SHIP,
    ROWBOAT,

    PLAYER,
    MERMADE
};

typedef struct {
    std::string displayName = "Player";
    int distance = 0;
} Player;


struct NetProxyShipData {
    std::string displayName = "Far Ship";
    int distance = 0;
};

struct MermadeData {
    std::string displayName = "Mermade";
    int distance = 0;
};

typedef struct { //read everything from pawn
    float health;

    std::string displayName = "Ship";
    double distance = 0;
    float WaterAmount = 0; //0-1, 1 is full
    std::array<FVector, 30> holeLocations;
    int holeCount = 0;



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


#endif //LOCALSTRUCTS_H
