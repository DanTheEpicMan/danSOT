#pragma once


#include "GameData.h"
#include "../offsets.h"
#include "../memory/Memory.h"
#include "LocalData.h"
#include <vector>

#define ptr uintptr_t

inline std::vector<int> getMyCrewIDs(uintptr_t gameState, uintptr_t playerController) {
    std::vector<int> my_crew_actor_ids; // Vector to store ActorIDs
    my_crew_actor_ids.clear();


    ptr CrewService = ReadMemory<uintptr_t>(gameState + Offsets::CrewService);
    if (CrewService) {
        ptr LocalPlayerState = ReadMemory<uintptr_t>(playerController + Offsets::PlayerState);
        if (LocalPlayerState) {
            TArray<ptr> Crews = ReadMemory<TArray<ptr>>(CrewService + Offsets::Crews);
            ptr CrewsArrayData = Crews.GetAddress();
            int CrewCount = Crews.Length();

            for (int i = 0; i < CrewCount; i++) {
                ptr CrewStructAddress = CrewsArrayData + (i * 0xA0); //Size of struct is 0xA0
                TArray<ptr> CrewMatesPlayerStates = ReadMemory<TArray<ptr>>(CrewStructAddress + Offsets::CrewMatesPlayerStates); //Offset for List of PlayerStates

                bool isMyCrew = false;
                for (int j = 0; j < CrewMatesPlayerStates.Length(); j++) {
                    ptr CrewMatePlayerState = ReadMemory<ptr>(CrewMatesPlayerStates.GetAddress() + (j * sizeof(ptr)));
                    if (CrewMatePlayerState == LocalPlayerState) {
                        isMyCrew = true;
                        break;
                    }
                }

                if (isMyCrew) {
                    for (int j = 0; j < CrewMatesPlayerStates.Length(); j++) {
                        ptr CrewMatePlayerState = ReadMemory<ptr>(CrewMatesPlayerStates.GetAddress() + (j * sizeof(ptr)));
                        if (!CrewMatePlayerState) continue;

                        // Get the ActorID from the Pawn and store it
                        int State_PlayerID = ReadMemory<int>(CrewMatePlayerState + Offsets::PlayerId);
                        my_crew_actor_ids.push_back(State_PlayerID);
                    }
                    break;
                }
            }
        }
    }

    return my_crew_actor_ids;
}

inline std::string getNameFromPawn(uintptr_t pawn, uintptr_t GNames) {
    auto actorID = ReadMemory<int>(pawn + Offsets::ActorID);
    char name_text[68];
    {
        const auto chunk_offset = (actorID / 0x4000) * 8;
        const auto chunk_index_offset = (actorID % 0x4000) * 8;
        const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);
        const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);
        ReadMemoryBuffer(name_ptr + 0xC, name_text, sizeof(name_text));
        name_text[67] = '\0';
    }
    return name_text;
}

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

inline bool coordsOnScreen(Coords coords, float ScreenWidth, float ScreenHeight) {
    return coords.x >= 0 && coords.x <= ScreenWidth && coords.y >= 0 && coords.y <= ScreenHeight;
}

inline void drawX(DrawingContext *ctx, Coords coords, int size, COLOR::Color color) {
    ctx->draw_line(coords.x - size, coords.y - size, coords.x + size, coords.y + size, 2, color);
    ctx->draw_line(coords.x - size, coords.y + size, coords.x + size, coords.y - size, 2, color);
}

inline FVector GetShipVelocityByDistance(uintptr_t playerPawnAddress, std::vector<Entity> ships) {
    uintptr_t rootComponent  = ReadMemory<uintptr_t>(playerPawnAddress + Offsets::RootComponent);
    // std::cout << "[GetShipVelocityByDistance] rootComponent: 0x" << std::hex << rootComponent << std::dec << std::endl;
    FTransform componentToWorld = ReadMemory<FTransform>(rootComponent + Offsets::ComponentToWorld);
    FVector Location = componentToWorld.Translation;
    // std::cout << "[GetShipVelocityByDistance] Player Location: (" << Location.x << ", " << Location.y << ", " << Location.z << ")" << std::endl;

    int shortestDistance = 3000; //Largest Dist to consider
    int shortestShipIndex = -1;
    FVector closestShipPos = {0, 0, 0};
    for (int i = 0; i < ships.size(); i++) {
        float dist = ships[i].location.Distance(Location);
        // std::cout << "[GetShipVelocityByDistance] Ship " << i << " at (" << ships[i].location.x << ", " << ships[i].location.y << ", " << ships[i].location.z << ") distance: " << dist << std::endl;
        if (dist < shortestDistance) {
            shortestShipIndex = i;
            shortestDistance = dist;
            closestShipPos = ships[i].location;
        }
    }

    // std::cout << "[GetShipVelocityByDistance] Found ship at index: " << shortestShipIndex << " with distance: " << shortestDistance << std::endl;

    if (shortestDistance != 3000 && shortestShipIndex != -1) {
        uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(ships[shortestShipIndex].pawn + Offsets::ShipMovementProxyComponent);
        uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + Offsets::ChildActor); // UChildActorComponent::ChildActor
        FVector global = ReadMemory<FVector>(movementProxyActor + Offsets::ShipMovement +
            Offsets::ReplicatedShipMovement_Movement + Offsets::LinearVelocity);

        return global;
    }
    // std::cout << "[GetShipVelocityByDistance] No ship found within distance, returning (0,0,0)" << std::endl;
    return {0, 0, 0}; // No ship found within distance
}

//it works but is hard to improve
inline FVector GetPlayerGlobalVelocitySloppy(uintptr_t playerPawnAddress, std::vector<Entity> ships) { //could be improved by checking AthenaCameraComponent->IsInsideShipHull
    //Get Velocity
    uintptr_t characterMovement  = ReadMemory<uintptr_t>(playerPawnAddress + Offsets::CharacterMovement);
    FVector PlayerVelocity = ReadMemory<FVector>(characterMovement  + Offsets::MovementComponent_Velocity);

    //Get Location
    FVector location = ReadMemory<FVector>(ReadMemory<ptr>(playerPawnAddress + Offsets::RootComponent) + Offsets::RelativeLocation);

    //Get BaseComponent
    uintptr_t baseComponentAddr = ReadMemory<uintptr_t>(playerPawnAddress + Offsets::BasedMovement + Offsets::MovementBase);

    uintptr_t attachedParentComp = ReadMemory<uintptr_t>(baseComponentAddr + Offsets::AttachParent);
    uintptr_t childActor = ReadMemory<uintptr_t>(attachedParentComp + Offsets::ChildActor);
    uintptr_t parentActor = ReadMemory<uintptr_t>(childActor + Offsets::ParentComponentActor + 0x0); //Reads struct were 0x0 points to actor

    if (baseComponentAddr == 0 && location.Distance({0, 0, 0}) < 3000 && PlayerVelocity.Distance(FVector(0, 0, 0)) < 1) { //On ladder or interacting with something on a ship
        //Read by getting the closest ship;
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity; //Add player velocity to ship velocity
    } else if (baseComponentAddr == 0) { //in water
        return PlayerVelocity;
    } else if (childActor == 0) { //on island or specific part of ship
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity;
    } else if (childActor > 0x4931d0000000 && parentActor == 0) { //on stairs or on lower deck
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity;
    } else { //standing on the ship normally
        while (true) {
            uintptr_t nextParentActor = ReadMemory<uintptr_t>(parentActor + Offsets::ParentComponentActor);
            if (nextParentActor == 0 || nextParentActor == parentActor) {
                break; // No more parents or reached the end of the chain
            }
            parentActor = nextParentActor;
        }

        uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(parentActor + Offsets::ShipMovementProxyComponent);
        uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + Offsets::ChildActor);
        FVector global = ReadMemory<FVector>(movementProxyActor + Offsets::ShipMovement +
            Offsets::ReplicatedShipMovement_Movement + Offsets::LinearVelocity);

        return global + PlayerVelocity;
    }
    std::cout << "No valid ship found, returning player velocity." << std::endl;
    return {0, 0, 0}; // Fallback, should not reach here
}