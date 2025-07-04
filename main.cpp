//Extern Headers
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>

//Local Headers
#include <chrono>
#include <linux/input-event-codes.h>

#include "utils/ProcessUtils.h"
#include "memory/Memory.h"
#include "utils/LocalStructs.h"
#include "offsets.h"
#include "utils/GameStructs.h"
#include "overlay/drawing.h" //overlay/shared_data.h imported by this
#include "utils/tables.h"
#include "hacks/Aimbot.h"
#include "hacks/InputManager.h"
#include "hacks/InputManager.h"

#define ptr uintptr_t //Makes it a little more readable
using BYTE = unsigned char;

//HARDCODED VALUES-CHANGE
int MonWidth = 2560; // Width of the monitor, this does not controller the overlay size but is used for calculations
int MonHeight = 1440; // Height of the monitor, this does not controller the overlay size but is used for calculations
//END OF HARDCODED VALUES

DrawingContext* ctx = nullptr;

void cleanup_shm(int signum) {
    std::cout << "\nCaught signal " << signum << ". Cleaning up..." << std::endl;
    delete ctx;
    ctx = nullptr;
    exit(0);
}

const BYTE bodyHead[] = { 4, 5, 6, 51, 7, 6, 80, 7, 8, 9 };
const BYTE neckHandR[] = { 80, 81, 82, 83, 84 };
const BYTE neckHandL[] = { 51, 52, 53, 54, 55 };
const BYTE bodyFootR[] = { 4, 111, 112, 113, 114 };
const BYTE bodyFootL[] = { 4, 106, 107, 108, 109 };

const std::pair<const BYTE*, const BYTE> skeleton[] = {
    {bodyHead, 10},
    {neckHandR, 5},
    {neckHandL, 5},
    {bodyFootR, 5},
    {bodyFootL, 5}
};

void RenderSkeleton(DrawingContext* ctx, uintptr_t meshComponent, const FCameraCacheEntry& cameraCache, int monWidth, int monHeight) {
    std::cout << "Mesh: " << std::hex << meshComponent << std::dec << std::endl;
    if (!meshComponent) return;

    // Read the ComponentToWorld transform from the mesh
    FTransform CompToWorld = ReadMemory<FTransform>(meshComponent + /*Offsets::ComponentToWorld*/0x130);
    const D3DMATRIX Comp2WorldMatrix = CompToWorld.ToMatrixWithScale();
    std::cout << "Component to World Matrix: " << std::hex << Comp2WorldMatrix.m[3][0] << ", "
              << Comp2WorldMatrix.m[3][1] << ", " << Comp2WorldMatrix.m[3][2] << std::dec << std::endl;

    // Read the bone array TArray structure, then the pointer to the bones
    TArray<FTransform> boneArrayT = ReadMemory<TArray<FTransform>>(meshComponent + /*Offsets::SpaceBasesArray*/0x5D8); //0x758
    uintptr_t boneArray = boneArrayT.GetAddress();
    std::cout << "Bone array address: 0x" << std::hex << boneArray << std::dec << std::endl;
    if (!boneArray) return;

    // The tutorial has 5 hardcoded paths, so we use std::size
    std::cout << "Rendering skeleton with " << std::size(skeleton) << " paths." << std::endl;
    for (size_t s = 0; s < std::size(skeleton); ++s) {
        const auto& boneInfo = skeleton[s]; // e.g., {neckHandR, 5}
        const BYTE* bonePath = boneInfo.first;
        const BYTE pathSize = boneInfo.second;

        Coords previousBoneScreenCoords = {0, 0};
        std::cout << "Processing bone path of size " << static_cast<int>(pathSize) << std::endl;
        for (int i = 0; i < pathSize; ++i) {
            // Get the bone ID for the current point in the path
            BYTE boneId = bonePath[i];

            // Read the FTransform for this specific bone
            FTransform boneTransform = ReadMemory<FTransform>(boneArray + (boneId * sizeof(FTransform)));
            // std::cout << "Bone ID: " << static_cast<int>(boneId) << " Transform x: " << boneTransform.Translation.x << std::endl;

            // Convert bone transform to a matrix
            D3DMATRIX boneMatrix = boneTransform.ToMatrixWithScale();

            // Multiply with the component's world matrix to get the final world position
            D3DMATRIX worldMatrix = boneMatrix * Comp2WorldMatrix;

            // Extract the position from the final matrix
            FVector boneWorldPos = { worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };
            // std::cout << "Bone ID: " << static_cast<int>(boneId) << " World Position: ("
            //           << boneWorldPos.x << ", " << boneWorldPos.y << ", " << boneWorldPos.z << ")" << std::endl;

            // Project the 3D world position to 2D screen coordinates
            Coords currentBoneScreenCoords = WorldToScreen(boneWorldPos, cameraCache.POV, monWidth, monHeight);

            // If the point is on screen (WorldToScreen usually returns 0,0 for off-screen points)
            if (currentBoneScreenCoords.x > 0 && currentBoneScreenCoords.y > 0) {
                // If this is not the first bone in the path, draw a line from the previous bone to this one
                if (i > 0 && previousBoneScreenCoords.x > 0) {
                    ctx->draw_line(previousBoneScreenCoords.x, previousBoneScreenCoords.y,
                                   currentBoneScreenCoords.x, currentBoneScreenCoords.y, 1.5f, COLOR::WHITE);
                }
            }
            // Update the previous bone's screen position for the next line segment
            previousBoneScreenCoords = currentBoneScreenCoords;
        }
    }
}

bool isShip(const std::string& actorName) {
    return actorName == "BP_SmallShipTemplate_C" || actorName == "BP_SmallShipNetProxy_C" ||
           actorName == "BP_MediumShipTemplate_C" || actorName == "BP_MediumShipNetProxy_C" ||
           actorName == "BP_LargeShipTemplate_C" || actorName == "BP_LargeShipNetProxy_C" ||
           actorName == "BP_AISmallShipTemplate_C" || actorName == "BP_AISmallShipNetProxy_C" ||
           actorName == "BP_AILargeShipTemplate_C" || actorName == "BP_AILargeShipNetProxy_C";
}

// A helper to get actor name, assuming you have a way to do this (like in your main loop)
std::string getActorName(uintptr_t actorAddress, uintptr_t GNames) {
    if (!actorAddress) return "";
    int actorId = ReadMemory<int>(actorAddress + Offsets::ActorID);
    if (actorId <= 0) return "";

    char name_text[68];
    const auto chunk_offset = (actorId / 0x4000) * 8;
    const auto chunk_index_offset = (actorId % 0x4000) * 8;
    const auto name_chunk = ReadMemory<uintptr_t>(GNames + chunk_offset);
    if (!name_chunk) return "";
    const auto name_ptr = ReadMemory<uintptr_t>(name_chunk + chunk_index_offset);
    if (!name_ptr) return "";
    ReadMemoryBuffer(name_ptr + 0xC, name_text, sizeof(name_text));
    name_text[67] = '\0';
    return std::string(name_text);
}


FVector GetPlayerGlobalVelocity(uintptr_t playerPawnAddress) {
    // 1. Get the player's own linear velocity.
    uintptr_t LPCharicterMovement = ReadMemory<uintptr_t>(playerPawnAddress + 0x420);
    FVector globalVelocity = ReadMemory<FVector>(LPCharicterMovement + 0xCC); //Velocityc
    FVector location = ReadMemory<FVector>(ReadMemory<ptr>(playerPawnAddress + Offsets::RootComponent) + Offsets::RelativeLocation);

    // 2. Find the object the player is standing on.
    // ACharacter has a BasedMovement member (at 0x430) which contains info about the base.
    uintptr_t baseComponentAddr = ReadMemory<uintptr_t>(playerPawnAddress + 0x430 + 0x0);

    if (baseComponentAddr != 0) {
        // 3. If standing on a component, get its owning Actor (the ship).
        // The owner of a UActorComponent is at a standard, but assumed, offset.
        uintptr_t attachedParentComp = ReadMemory<uintptr_t>(baseComponentAddr + 0xD0); //USceneComponent //ATTACH_PARENT
        if (attachedParentComp != 0) {
            uintptr_t childActor = ReadMemory<uintptr_t>(attachedParentComp + 0x2D8); // //UPrimitiveComponent, it's the first value //CHILD_ACTOR
            uintptr_t parentActor = ReadMemory<uintptr_t>(childActor + 0x190); //PARENT_COMPONENT //0x190 + 0x0, points to actor
            while (true) {
                uintptr_t nextParentActor = ReadMemory<uintptr_t>(parentActor + 0x190); //PARENT_COMPONENT
                if (nextParentActor == 0 || nextParentActor == parentActor) {
                    break; // No more parents or reached the end of the chain
                }
                parentActor = nextParentActor;
            }
            //parentActor now represents ship
            uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(parentActor + 0x638);
            uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
            FVector global = ReadMemory<FVector>(movementProxyActor + 0x3b0);
            return global + globalVelocity;


        }
    }
    return globalVelocity;
}

// FVector GetShipVelocityByDistance(uintptr_t playerPawnAddress, std::vector<Entity> ships) {
//     uintptr_t rootComponent  = ReadMemory<uintptr_t>(playerPawnAddress + Offsets::RootComponent);
//     FTransform componentToWorld = ReadMemory<FTransform>(rootComponent + 0x130);
//     FVector Location = componentToWorld.Translation;
//
//     int shortestDistance = 3000; //Largest Dist to consider
//     int shortestShipIndex = -1;
//     FVector closestShipPos = {0, 0, 0};
//     for (int i = 0; i < ships.size(); i++) {
//         if (ships[i].location.Distance(Location) > shortestDistance) {
//             shortestShipIndex = i;
//             shortestDistance = ships[i].location.Distance(Location);
//             closestShipPos = ships[i].location;
//         }
//     }
//
//     std::cout << "Found ship at index: " << shortestShipIndex << " with distance: " << shortestDistance << std::endl;
//
//     if (shortestDistance != 3000) {
//         uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(ships[shortestShipIndex].pawn + 0x638);
//         uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
//         FVector global = ReadMemory<FVector>(movementProxyActor + 0x3b0);
//
//         return global;
//     }
//     return {0, 0, 0}; // No ship found within distance
// }
FVector GetShipVelocityByDistance(uintptr_t playerPawnAddress, std::vector<Entity> ships) {
    uintptr_t rootComponent  = ReadMemory<uintptr_t>(playerPawnAddress + Offsets::RootComponent);
    std::cout << "[GetShipVelocityByDistance] rootComponent: 0x" << std::hex << rootComponent << std::dec << std::endl;
    FTransform componentToWorld = ReadMemory<FTransform>(rootComponent + 0x130);
    FVector Location = componentToWorld.Translation;
    std::cout << "[GetShipVelocityByDistance] Player Location: (" << Location.x << ", " << Location.y << ", " << Location.z << ")" << std::endl;

    int shortestDistance = 3000; //Largest Dist to consider
    int shortestShipIndex = -1;
    FVector closestShipPos = {0, 0, 0};
    for (int i = 0; i < ships.size(); i++) {
        float dist = ships[i].location.Distance(Location);
        std::cout << "[GetShipVelocityByDistance] Ship " << i << " at (" << ships[i].location.x << ", " << ships[i].location.y << ", " << ships[i].location.z << ") distance: " << dist << std::endl;
        if (dist < shortestDistance) {
            shortestShipIndex = i;
            shortestDistance = dist;
            closestShipPos = ships[i].location;
        }
    }

    std::cout << "[GetShipVelocityByDistance] Found ship at index: " << shortestShipIndex << " with distance: " << shortestDistance << std::endl;

    if (shortestDistance != 3000 && shortestShipIndex != -1) {
        uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(ships[shortestShipIndex].pawn + 0x638);
        // std::cout << "[GetShipVelocityByDistance] movementProxyComponent: 0x" << std::hex << movementProxyComponent << std::dec << std::endl;
        uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
        // std::cout << "[GetShipVelocityByDistance] movementProxyActor: 0x" << std::hex << movementProxyActor << std::dec << std::endl;
        FVector global = ReadMemory<FVector>(movementProxyActor + 0x3b0);
        // std::cout << "[GetShipVelocityByDistance] Ship velocity: (" << global.x << ", " << global.y << ", " << global.z << ")" << std::endl;

        return global;
    }
    // std::cout << "[GetShipVelocityByDistance] No ship found within distance, returning (0,0,0)" << std::endl;
    return {0, 0, 0}; // No ship found within distance
}

FVector GetPlayerGlobalVelocitySloppy(uintptr_t playerPawnAddress, std::vector<Entity> ships) {
    //Get Velocity
    uintptr_t characterMovement  = ReadMemory<uintptr_t>(playerPawnAddress + 0x420);
    FVector PlayerVelocity = ReadMemory<FVector>(characterMovement  + 0xCC);

    //Get Location
    FVector location = ReadMemory<FVector>(ReadMemory<ptr>(playerPawnAddress + Offsets::RootComponent) + Offsets::RelativeLocation);

    //Get BaseComponent
    uintptr_t baseComponentAddr = ReadMemory<uintptr_t>(playerPawnAddress + 0x430 + 0x0);

    uintptr_t attachedParentComp = ReadMemory<uintptr_t>(baseComponentAddr + 0xD0); //USceneComponent //ATTACH_PARENT
    uintptr_t childActor = ReadMemory<uintptr_t>(attachedParentComp + 0x2D8); // //UPrimitiveComponent, it's the first value //CHILD_ACTOR
    uintptr_t parentActor = ReadMemory<uintptr_t>(childActor + 0x190); //PARENT_COMPONENT //0x190 + 0x0, points to actor
    // std::cout << "Base Component Address: 0x" << std::hex << baseComponentAddr << std::dec << std::endl;
    // std::cout << "Location from center: " << location.Distance(FVector(0, 0, 0)) << std::endl;
    // std::cout << "Player Velocity: " << PlayerVelocity.x << ", " << PlayerVelocity.y << ", " << PlayerVelocity.z << std::endl;

    // std::cout << "Child Actor Address: 0x" << std::hex << childActor << std::dec << std::endl;
    // std::cout << "Parent Actor Address: 0x" << std::hex << parentActor << std::dec << std::endl;

    if (baseComponentAddr == 0 && location.Distance({0, 0, 0}) < 3000 && PlayerVelocity.Distance(FVector(0, 0, 0)) < 1) { //On ladder or interacting with something on a ship
        //Read by getting closest ship
        // std::cout << "On ladder or interacting with something on a ship, returning ship velocity." << std::endl;
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity; //Add player velocity to ship velocity
    } else if (baseComponentAddr == 0) { //in water
        // std::cout << "In water, returning player velocity." << std::endl;
        return PlayerVelocity;
    } else if (childActor == 0) { //on island or specific part of ship
        // std::cout << "On island, returning player velocity." << std::endl;
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity;
    } else if (childActor > 0x4931d0000000 && parentActor == 0) { //on stairs or on lower deck
        // std::cout << "On stairs or on lower deck, returning ship velocity." << std::endl;
        return GetShipVelocityByDistance(playerPawnAddress, ships) + PlayerVelocity;
    } else { //standing on the ship normally
        // std::cout << "Standing on the ship normally, calculating ship velocity." << std::endl;
        while (true) {
            uintptr_t nextParentActor = ReadMemory<uintptr_t>(parentActor + 0x190); //PARENT_COMPONENT
            if (nextParentActor == 0 || nextParentActor == parentActor) {
                break; // No more parents or reached the end of the chain
            }
            parentActor = nextParentActor;
        }

        uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(parentActor + 0x638);
        uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
        FVector global = ReadMemory<FVector>(movementProxyActor + 0x3b0);

        return global + PlayerVelocity;
    }
    std::cout << "No valid ship found, returning player velocity." << std::endl;
    return {0, 0, 0}; // Fallback, should not reach here
}


int main() {
    InputManager inputManager;

    if (!inputManager.isVirtualMouseInitialized()) {
        std::cerr << "Could not create virtual mouse. Aborting." << std::endl;
        return 1;
    }


    if (getuid() != 0) {
        std::cerr << "Must run as root (sudo)" << std::endl;
        return 1;
    }

    signal(SIGINT, cleanup_shm);

    try {
        ctx = new DrawingContext();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error initializing DrawingContext: " << e.what() << std::endl;
        return 1;
    }

    ProcessId = FindProcess("SotGame.exe");
    if (ProcessId == 0) {
        std::cerr << "Game Not found. Name might need to be updated" << std::endl;
        delete ctx;
        return 1;
    }
    std::cout << "Game is running, process id " << ProcessId << std::endl;


    BaseAddress = FindBaseImage(ProcessId, "SotGame.exe");
    if (BaseAddress == 0) {
        std::cerr << "Could not find game's base address." << std::endl;
        delete ctx;
        return 1;
    }
    std::cout << "Found base address: 0x" << std::hex << BaseAddress << std::dec << std::endl;

    uintptr_t uworld = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld);
    std::cout << "UWorld read test 0x " << std::hex << uworld << std::dec << std::endl;
    if (uworld > 0x10000000000 || uworld == 0x0) {
        std::cout << "UWorld is out of bounds." << std::endl;
        std::cout << "Procede with program? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice != 'y' && choice != 'Y') {
            delete ctx;
            return 2;
        }
    }


    uintptr_t UWorld, GNames, PlayerController, LPawn, GameInstance, Persistentlevel, GameState, LocalPlayers, LocalPlayer;
    float WorldGravityZ = -981.0f;

    int loopCount = -1;
    while (true) {
        loopCount++;
        //////////////////////////////////////////////////////////////////////////////////////////////
        //================================Start of Game Data Loop Area==============================//
        //////////////////////////////////////////////////////////////////////////////////////////////
        UWorld = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld);
        GameState = ReadMemory<uintptr_t>(UWorld + Offsets::GameState);
        Persistentlevel = ReadMemory<uintptr_t>(UWorld + Offsets::PresistentLevel);
        GameInstance = ReadMemory<uintptr_t>(UWorld + Offsets::OwnerGameInstance);
        if (GameInstance == 0x0) {
            ctx->end_frame();
            continue; // If GameInstance is not found, skip the rest of the loop
        }
        LocalPlayers = ReadMemory<uintptr_t>(GameInstance + Offsets::LocalPlayers);
        LocalPlayer = ReadMemory<uintptr_t>(LocalPlayers);
        PlayerController = ReadMemory<uintptr_t>(LocalPlayer + Offsets::PlayerController);
        LPawn = ReadMemory<uintptr_t>(PlayerController + Offsets::LocalPawn);
        GNames = ReadMemory<ptr>(BaseAddress + Offsets::GName);

        if (!LPawn) {
            ctx->end_frame();
            continue;
        }

        uintptr_t WorldSettings = ReadMemory<uintptr_t>(UWorld + 0x3A0);
        WorldGravityZ = ReadMemory<float>(WorldSettings + 0x548);

        std::cout << "World Settings: " << std::hex << WorldSettings << std::dec << std::endl;
        std::cout << "World Gravity Z: " << WorldGravityZ << std::endl;

        ptr CameraManager = ReadMemory<ptr>(PlayerController + Offsets::PlayerCameraManager);
        FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);
        uintptr_t LPCharicterMovement = ReadMemory<uintptr_t>(LPawn + 0x420);
        FVector LPVelocity = ReadMemory<FVector>(LPCharicterMovement + 0xCC); //Velocityc
        // std::cout << "Velocity: " << LPVelocity.x << " " << LPVelocity.y << " " << LPVelocity.z << std::endl;


        TArray<uintptr_t> PlayersTArray = ReadMemory<TArray<uintptr_t>>(Persistentlevel + Offsets::OwningActor);
        int playerCount = PlayersTArray.Length();
        uintptr_t PlayerStateArray = PlayersTArray.GetAddress();

        //std::cout << "LP ID: " << ReadMemory<int>(PlayerController + 0xC) << std::endl;
        //GetPlayerGlobalVelocity(LPawn);
        // GetShipVelocity(LPawn);

        //uintptr_t HealthComp = ReadMemory<uintptr_t>(LPawn + Offsets::HealthComponent); //HealthComponent
        //float Health = ReadMemory<float>(HealthComp + Offsets::Health); //CurrentHealthInfo (0xd4 + 0x0)
        //std::cout << "Health: " << Health << std::endl << std::endl;

        ///////////////////Reading Crew Data/////////////////////
        std::vector<int> my_crew_actor_ids; // Vector to store ActorIDs
        my_crew_actor_ids.clear();
        int AllCrewCount = 0;

        uintptr_t CrewService = ReadMemory<uintptr_t>(GameState + Offsets::CrewService);
        if (CrewService) {
            uintptr_t LocalPlayerState = ReadMemory<uintptr_t>(PlayerController + Offsets::LocalPlayerState);
            if (LocalPlayerState) {
                TArray<uintptr_t> Crews = ReadMemory<TArray<uintptr_t>>(CrewService + Offsets::Crews);
                uintptr_t CrewsArrayData = Crews.GetAddress();
                int CrewCount = Crews.Length(); AllCrewCount = CrewCount;

                for (int i = 0; i < CrewCount; i++) {
                    uintptr_t CrewStructAddress = CrewsArrayData + (i * 0xA0); //Size of struct is 0xA0
                    TArray<uintptr_t> CrewMatesPlayerStates = ReadMemory<TArray<uintptr_t>>(CrewStructAddress + Offsets::CrewMatesPlayerStates); //Offset for List of PlayerStates

                    bool isMyCrew = false;
                    for (int j = 0; j < CrewMatesPlayerStates.Length(); j++) {
                        uintptr_t CrewMatePlayerState = ReadMemory<uintptr_t>(CrewMatesPlayerStates.GetAddress() + (j * sizeof(ptr)));
                        if (CrewMatePlayerState == LocalPlayerState) {
                            isMyCrew = true;
                            break;
                        }
                    }

                    if (isMyCrew) {
                        for (int j = 0; j < CrewMatesPlayerStates.Length(); j++) {
                            uintptr_t CrewMatePlayerState = ReadMemory<uintptr_t>(CrewMatesPlayerStates.GetAddress() + (j * sizeof(ptr)));
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
        ////////////////End of Reading Crew Data/////////////////////



        std::vector<Entity> goodEntities; //Stores useful stuff like players, boats, mermades, etc.
        std::vector<Entity> playerEntities; //Used for aimbot.
        std::vector<Entity> shipEntities; //Used for aimbot.
        int goodEntitiesCount = 0;

        ptr meshPtr = ReadMemory<ptr>(LPawn + Offsets::RootComponent); //SkeletalMeshComponent
        FTransform Comp = ReadMemory<FTransform>(meshPtr + 0x130); //ComponentToWorld
        // std::cout << "ComponentToWorld: " << std::hex << Comp.ToMatrixWithScale().m[3][0] << ", "
        //           << Comp.ToMatrixWithScale().m[3][1] << ", " << Comp.ToMatrixWithScale().m[3][2] << std::dec << std::endl;
        //RenderSkeleton(ctx, meshPtr, CameraCache, MonWidth, MonHeight);

        for (int i = 0; i < playerCount; i++) {
            Entity entity;
            ptr PlayerPawn = ReadMemory<ptr>(PlayerStateArray + (i * sizeof(ptr))); //PlayerPawn does not have to be player, could be ship or anything else
            if (PlayerPawn == 0 /*|| PlayerPawn == LPawn*/) {
                continue;
            }
            entity.pawn = PlayerPawn; // Store the pawn address in the entity

            //------Finding Location------
            auto ActorRootComponent = ReadMemory<uintptr_t>(PlayerPawn + Offsets::RootComponent);
            if (ActorRootComponent == 0x0) continue;
            entity.location = ReadMemory<FVector>(ActorRootComponent + Offsets::RelativeLocation);
            if (entity.location.x == 0) continue;

            //------Weed out Local Player like things------
            // if (entity.location.Distance(CameraCache.POV.Location) < 100.f) {
            //     // Skip entities that are too close to the player
            //     continue;
            // }

            //------Finding Name------
            auto actorID = ReadMemory<int>(PlayerPawn + Offsets::ActorID);
            char name_text[68];
            {
                const auto chunk_offset = (actorID / 0x4000) * 8;
                const auto chunk_index_offset = (actorID % 0x4000) * 8;
                const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);
                const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);
                ReadMemoryBuffer(name_ptr + 0xC, name_text, sizeof(name_text));
                name_text[67] = '\0';
            }
            entity.name = name_text; // Store the name in the entity

            // std::cout << "EntName: " << entity.name << std::endl;
            // Coords entityScreenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
            // ctx->draw_text(entityScreenCoords.x, entityScreenCoords.y, entity.name.c_str(), COLOR::WHITE);

            if (entity.name == "NetProxy_C") { //Far off ship
                entity.type = EntTypes::NETPROXYSHIP;
                entity.netProxyShip.displayName = "Far Ship";

            } else if (entity.name == "BP_SmallShipTemplate_C" || entity.name == "BP_SmallShipNetProxy_C" ||
                       entity.name == "BP_MediumShipTemplate_C" || entity.name == "BP_MediumShipNetProxy_C" ||
                       entity.name == "BP_LargeShipTemplate_C" || entity.name == "BP_LargeShipNetProxy_C" ||
                       entity.name == "BP_AISmallShipTemplate_C" || entity.name == "BP_AISmallShipNetProxy_C" ||
                       entity.name == "BP_AILargeShipTemplate_C" || entity.name == "BP_AILargeShipNetProxy_C") {

                entity.type = EntTypes::SHIP;//type
                if (entity.name == "BP_SmallShipTemplate_C" || entity.name == "BP_SmallShipNetProxy_C") entity.ship.displayName = "Sloop";      //name
                if (entity.name == "BP_MediumShipTemplate_C" || entity.name == "BP_MediumShipNetProxy_C") entity.ship.displayName = "Brigantine";
                if (entity.name == "BP_LargeShipTemplate_C" || entity.name == "BP_LargeShipNetProxy_C") entity.ship.displayName = "Galleon";
                if (entity.name == "BP_AISmallShipTemplate_C" || entity.name == "BP_AISmallShipNetProxy_C") entity.ship.displayName = "AI Sloop";
                if (entity.name == "BP_AILargeShipTemplate_C" || entity.name == "BP_AILargeShipNetProxy_C") entity.ship.displayName = "AI Galleon";


                // std::cout << "Ship: " << PlayerPawn << std::endl;

                if (CameraCache.POV.Location.Distance(entity.location) > 175000.f) { //If the ship is too far away, skip it
                    goto EndConditions; // Always wanted to use goto for something
                }

                if (CameraCache.POV.Location.Distance(entity.location) < 3000) entity.ship.onShip = true;

                // auto shipVel = ReadMemory<FVector>(PlayerPawn + 0x90);
                // std::cout << "Ship Vel: " << shipVel.x << " " << shipVel.y << " " << shipVel.z << std::endl;

                uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(PlayerPawn + 0x638);
                uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
                entity.ship.moveInfo = ReadMemory<RepMovement>(movementProxyActor + 0x3b0);

                //Get water amount
                uintptr_t pShipInternalWaterComponent = ReadMemory<uintptr_t>(PlayerPawn + Offsets::ShipInternalWaterComponent);
                uintptr_t pShipInternalWaterActor = ReadMemory<uintptr_t>(pShipInternalWaterComponent + Offsets::ChildActor);
                float waterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::WaterAmount);
                float maxWaterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::InternalWaterParams + Offsets::MaxWaterAmount);
                if (maxWaterAmount > 0.f) {
                    entity.ship.WaterAmount = waterAmount / maxWaterAmount; // Normalize
                } else {
                    entity.ship.WaterAmount = 0.f;
                }


                //Get Holes
                uintptr_t hullDamage = ReadMemory<uintptr_t>(PlayerPawn + Offsets::HullDamage);
                TArray<uintptr_t> DamageZones = ReadMemory<TArray<uintptr_t>>(hullDamage + Offsets::ActiveHullDamageZones);
                entity.ship.holeCount = DamageZones.Length();
                for (int j = 0; j < entity.ship.holeCount; j++) {
                    uintptr_t DamageZone = ReadMemory<uintptr_t>(DamageZones.GetAddress() + (j * sizeof(uintptr_t)));
                    ptr ShipSceneComponent = ReadMemory<uintptr_t>(DamageZone + Offsets::SceneRootComponent);
                    FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
                    entity.ship.holeLocations[j] = location; // Get the hole location from the transform
                }


                shipEntities.push_back(entity);
            } else if (entity.name == "BP_Mermaid_C" || entity.name == "BP_LootStorage_Retrieve_C") {
                entity.type = EntTypes::MERMADE;
                entity.mermade.displayName = "Mermade";
                if (entity.name == "BP_LootStorage_Retrieve_C") entity.mermade.displayName = "Loot Mermade"; //Holds loot when in shrines
            } else if (entity.name == "BP_PlayerPirate_C"
) {
//            || entity.name == "BP_PhantomPawnBase_C" || entity.name == "BP_SkeletonPawnBase_C") { ///////////////////////////////////////////////////////////////////Skeleton/Phanton - temp///////////////////////////////////
                uintptr_t rootComponentEnemy  = ReadMemory<uintptr_t>(PlayerPawn + Offsets::RootComponent);
                FTransform componentToWorldEnemy = ReadMemory<FTransform>(rootComponentEnemy + 0x130);
                entity.location = componentToWorldEnemy.Translation;
                std::cout << "Entity Location: " << entity.location.x << ", " << entity.location.y << ", " << entity.location.z << std::endl;


                entity.type = EntTypes::PLAYER;
                entity.player.displayName = "Player";

                //Getting health
                uintptr_t entHealthComp = ReadMemory<uintptr_t>(PlayerPawn + Offsets::HealthComponent); //HealthComponent
                entity.player.health = ReadMemory<float>(entHealthComp + Offsets::Health);

                uintptr_t Mesh = ReadMemory<uintptr_t>(PlayerPawn + Offsets::Mesh);
                // uintptr_t SpaceBaseArray = ReadMemory<TArray<uintptr_t>>(Mesh + 0x5D8);
                entity.player.meshComponentPtr = Mesh;

                //Rooting out people on same team
                uintptr_t PlayerStateFromPawn = ReadMemory<uintptr_t>(PlayerPawn + Offsets::LocalPlayerState);
                int PlayerID_entity = ReadMemory<int>(PlayerStateFromPawn + Offsets::PlayerId);
                bool isMyCrewMember = false;
                for (int crewMateId : my_crew_actor_ids) {
                    if (crewMateId == PlayerID_entity) {
                        isMyCrewMember = true;
                        break;
                    }
                }
                if (isMyCrewMember) { continue; }

                uintptr_t EntityCharacterMovement = ReadMemory<uintptr_t>(PlayerPawn + 0x420);
                entity.player.velocity = ReadMemory<FVector>(EntityCharacterMovement + 0xCC);



                playerEntities.push_back(entity); //used for aimbot.
            }
            else if (entity.name == "BP_Rowboat_C" || entity.name == "BP_Rowboat_WithFrontCannon_C" || entity.name == "BP_Rowboat_WithFrontHarpoon_C") {
                entity.type = EntTypes::ROWBOAT;
                entity.rowboat.displayName = "Rowboat";
                if (entity.name == "BP_Rowboat_WithFrontCannon_C") entity.rowboat.displayName += "Cannon ";
                if (entity.name == "BP_Rowboat_WithFrontHarpoon_C") entity.rowboat.displayName += "Harpoon ";
            } else if (entity.name == "BP_Storm_C") {
                entity.type = EntTypes::STORM;
            }
            else {
                //check if the name is in table of important loot
                std::string itemName = getDisplayName(entity.name, Tables::goodItems);
                if (!itemName.empty()) {
                    entity.type = EntTypes::BASICENTITY;
                    entity.basicEntity.displayName = "[" + itemName + "]";
                    entity.basicEntity.color = COLOR::TransparentLightRed;
                    goto EndConditions;
                }
                itemName = getDisplayName(entity.name, Tables::goodLoot);
                if (!itemName.empty()) {
                    entity.type = EntTypes::BASICENTITY;
                    entity.basicEntity.displayName = itemName;
                    entity.basicEntity.color = COLOR::TransparentLightGold;
                    goto EndConditions;
                }
                itemName = getDisplayName(entity.name, Tables::goodItems);
                if (!itemName.empty()) {
                    entity.type = EntTypes::BASICENTITY;
                    entity.basicEntity.displayName = itemName;
                    entity.basicEntity.color = COLOR::TransparentOrange;
                    goto EndConditions;
                }
                itemName = getDisplayName(entity.name, Tables::projectiles);
                if (!itemName.empty()) {
                    entity.type = EntTypes::BASICENTITY;
                    entity.basicEntity.displayName = itemName;
                    entity.basicEntity.color = COLOR::RED;
                    goto EndConditions;
                }
                itemName = getDisplayName(entity.name, Tables::enemyEntity);
                if (!itemName.empty()) {
                    entity.type = EntTypes::BASICENTITY;
                    entity.basicEntity.displayName = itemName;
                    entity.basicEntity.color = COLOR::TransparentLightRed;
                    goto EndConditions;
                }

                //Non essential ESP, uncomment if you want to see it
                // itemName = getDisplayName(entity.name, Tables::gold_hoarders_loot);
                // if (!itemName.empty()) {
                //     std::cout << "Gold Hoarders Loot found: " << itemName << std::endl;
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightRed;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::order_of_souls_loot);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightPurple;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::merchants_loot);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightBlue;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::athena_loot);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightGreen;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::anyFactionLoot);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightWhite;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::misc);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightWhite;
                //     goto EndConditions;
                // }
                // itemName = getDisplayName(entity.name, Tables::misc);
                // if (!itemName.empty()) {
                //     entity.type = EntTypes::BASICENTITY;
                //     entity.basicEntity.displayName = itemName;
                //     entity.basicEntity.color = COLOR::TransparentLightWhite;
                //     goto EndConditions;
                // }


                entity.basicEntity.color = COLOR::TransparentLightPink; //Default color for world events
                entity.type = EntTypes::BASICENTITY; //Default type for world events
                entity.basicEntity.distance = entity.location.Distance(CameraCache.POV.Location);
                if (entity.name == "BP_Shipwreck_01_a_NetProxy_C" || entity.name == "BP_Seagulls_Barrels_C" || entity.name == "BP_Seagulls_Barrels_BarrelsOfPlenty_C") {
                    entity.basicEntity.displayName = "ShipWreck";
                    if (entity.name == "BP_Seagulls_Barrels_C") {
                        entity.basicEntity.displayName = "Seagulls Barrels";
                    } else if (entity.name == "BP_Seagulls_Barrels_BarrelsOfPlenty_C") {
                        entity.basicEntity.displayName = "Barrels of Plenty";
                    }
                    entity.basicEntity.color = COLOR::WHITE;
                } else if (entity.name == "BP_SkellyFort_RitualSkullCloud_C") {
                    entity.basicEntity.displayName = "Fort of the Damned";
                    goto EndConditions;
                } else if (entity.name == "BP_LegendSkellyFort_SkullCloud_C") {
                    entity.basicEntity.displayName = "Fort of Fortune";
                    goto EndConditions;
                } else if (entity.name == "BP_GhostShips_Signal_Flameheart_NetProxy_C" || entity.name == "BP_GhostShip_TornadoCloud_C") {
                    entity.basicEntity.displayName = "Flameheart/Ghost Fleet";
                    goto EndConditions;
                } else if (entity.name == "BP_SkellyFort_SkullCloud_C") {
                    entity.basicEntity.displayName = "Skeleton Fort";
                    goto EndConditions;
                } else if (entity.name == "BP_SkellyShip_ShipCloud_C") {
                    entity.basicEntity.displayName = "Skeleton Fleet";
                    goto EndConditions;
                } else if (entity.name == "BP_AshenLord_SkullCloud_C") {
                    entity.basicEntity.displayName = "Ashen Winds";
                    goto EndConditions;
                } else if (entity.name == "BP_ReaperTributeShipNetProxy_C" || entity.name == "BP_ReapersTributeShipTemplate_C") {
                    entity.basicEntity.displayName = "Burning Blade";
                    goto EndConditions;
                }
                //std::cout << "Entity: " << entity.name << std::endl;
                continue; // If the entity is not one of the important types, skip it
            }

            EndConditions:

            // Coords thing = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
            //
            // ctx->draw_text(thing.x, thing.y, entity.name, COLOR::WHITE);
            goodEntities.push_back(entity);
            goodEntitiesCount++;

            }

        //////////////////////////////////////////////////////////////////////////////////////////////
        //=================================End of Game Data Loop Area===============================//
        //================================Start of Rendering Loop Area==============================//
        //////////////////////////////////////////////////////////////////////////////////////////////

        ctx->begin_frame();
        ctx->draw_text(10, 15, "DanSOT", COLOR::WHITE);

        for (int i = 0; i < goodEntitiesCount; i++) {
            Entity& entity = goodEntities[i];

            //Display Crew Count
            ctx->draw_text(MonWidth - 75, 15, "Crews #: " + std::to_string(AllCrewCount), COLOR::WHITE);

            if (entity.type == EntTypes::NETPROXYSHIP) {
                Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.netProxyShip.displayName, COLOR::YELLOW);

            } else if (entity.type == EntTypes::SHIP) {
                Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CameraCache.POV, MonWidth, MonHeight);
                int waterPercentage = static_cast<int>(entity.ship.WaterAmount * 100);
                if (waterPercentage == 0 && entity.ship.holeCount == 0) {
                    ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.ship.displayName + "-" + std::to_string(static_cast<int>(entity.location.Distance(CameraCache.POV.Location)/100)*100), COLOR::YELLOW);
                } else {
                    ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.ship.displayName + " (" + std::to_string(entity.ship.holeCount) + " H - " + std::to_string(static_cast<int>(entity.ship.WaterAmount* 100)) + "%)", COLOR::YELLOW);
                }
                for (int j = 0; j < entity.ship.holeCount; j++) {
                    Coords holeCoords = WorldToScreen(entity.ship.holeLocations[j], CameraCache.POV, MonWidth, MonHeight);
                    ctx->draw_box(holeCoords.x - 5, holeCoords.y - 5, 10, 10, 1.0f, COLOR::GREEN);
                }

                if (entity.ship.onShip && (entity.ship.holeCount > 0 || entity.ship.WaterAmount > 0.f)) {
                    ctx->draw_text(MonWidth/2 - 25, MonHeight/2 + 150, std::to_string(entity.ship.holeCount) + " " + std::to_string((int)(entity.ship.WaterAmount * 100)) + "%" , COLOR::YELLOW);
                }

            } else if (entity.type == EntTypes::MERMADE) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.mermade.displayName, COLOR::MAGENTA);

            } else if (entity.type == EntTypes::PLAYER) {
                Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CameraCache.POV, MonWidth, MonHeight);
                Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CameraCache.POV, MonWidth, MonHeight);
                int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
                int width = (int)(height * 0.5f);

                ctx->draw_box(screenCoordsHead.x-width/2, screenCoordsHead.y, width, height, 2, COLOR::RED);

                //RenderSkeleton(ctx, entity.player.meshComponentPtr, CameraCache, MonWidth, MonHeight);

                //Tracers
                Coords tracers = CalculateTracerEndPoint(entity.location, CameraCache.POV, MonWidth, MonHeight);
                if (tracers.x != -1) {
                    ctx->draw_line(MonWidth/2, MonHeight/2, tracers.x, tracers.y, 2.0f, COLOR::ORANGE); //Tracer line from center of screen to entity
                }

                //Draw Health
                float healthPercentage = entity.player.health / 100.f;
                int healthHeight = (int)(height * healthPercentage);
                int missingHealthHeight = height - healthHeight;
                int healthBarX = screenCoordsHead.x - (width/2) - 5;
                ctx->draw_line(healthBarX, screenCoordsHead.y, healthBarX, screenCoordsFeet.y, 3.0f, COLOR::RED); //could do screenCoordsHead.y + missingHealthHeight for x2
                ctx->draw_line(healthBarX, screenCoordsFeet.y, healthBarX, screenCoordsFeet.y - healthHeight, 3.0f, COLOR::GREEN);

            } else if (entity.type == EntTypes::ROWBOAT) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x-40, screenCoords.y, entity.rowboat.displayName, COLOR::BLUE);
            } else if (entity.type == EntTypes::STORM) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x-30, screenCoords.y, "Storm - " + std::to_string((int)(entity.location.Distance(CameraCache.POV.Location)/1000)) + "k", COLOR::TransparentLightPink);
            } else if (entity.type == EntTypes::BASICENTITY) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                if (entity.basicEntity.distance > 0) {
                    ctx->draw_text(screenCoords.x-40, screenCoords.y, entity.basicEntity.displayName + " (" + std::to_string((int)(entity.basicEntity.distance/1000)) + "k)", entity.basicEntity.color);
                } else {
                    ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.basicEntity.displayName, entity.basicEntity.color);
                }
            }
        }

        //Bullet speed
        uintptr_t WieldedItem = ReadMemory<uintptr_t>(LPawn + 0x870); //ProjectileWeapon, ProjectileWeaponParameters,
        if (!WieldedItem) continue;
        uintptr_t weaponParamsPtrPRE = ReadMemory<uintptr_t>(WieldedItem + 0x2d8);
        float bulletSpeed = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80 + 0x10);
        // if (!weaponParamsPtr) continue;
        // uintptr_t ammoParamsPtr = weaponParamsPtr + 0x80;
        // float bulletSpeed = ReadMemory<float>(ammoParamsPtr + 0x10);


        // --- Find Best Targets Loop ---

        // Initialize with a very large number
        float closest_dist_from_crosshair = 99999.0f;
        float closest_dist_in_world = 99999.0f;

        FVector best_pos_crosshair; // Target closest to the crosshair on screen
        FVector best_pos_world;     // Target closest to you in 3D space

        bool is_best_pos_crosshair_valid = false;
        bool is_best_pos_world_valid = false;

        FVector LPVelocityWithShip = GetPlayerGlobalVelocitySloppy(LPawn, shipEntities);

        for (int i = 0; i < playerEntities.size(); i++) {
            // --- Prediction (This part seems okay) ---
            FVector EnemyVelocityWithShip = GetPlayerGlobalVelocitySloppy(playerEntities[i].pawn, shipEntities);
            FVector toAimCoords = {0, 0, 0};
            bool worked = GetPlayerAimPosition_NoGravity(CameraCache.POV.Location, LPVelocityWithShip, playerEntities[i].location, EnemyVelocityWithShip, bulletSpeed, toAimCoords);

            // --- Convert to Screen Coordinates ---
            Coords screenCoords = WorldToScreen(toAimCoords, CameraCache.POV, MonWidth, MonHeight);

            // Check if the target is actually on the screen before considering it
            if (screenCoords.x > 0 && screenCoords.x < MonWidth && screenCoords.y > 0 && screenCoords.y < MonHeight) {

                // --- BUG FIX: Calculate distance from the CENTER of the screen ---
                float dx = screenCoords.x - (MonWidth / 2.0f);
                float dy = screenCoords.y - (MonHeight / 2.0f);
                float dist_from_crosshair = sqrt(dx * dx + dy * dy);

                if (dist_from_crosshair < closest_dist_from_crosshair) {
                    closest_dist_from_crosshair = dist_from_crosshair;
                    best_pos_crosshair = toAimCoords;
                    is_best_pos_crosshair_valid = true;
                }

                // --- Find the physically closest target in the 3D world ---
                float dist_in_world = toAimCoords.Distance(CameraCache.POV.Location);
                if (dist_in_world < closest_dist_in_world) {
                    closest_dist_in_world = dist_in_world;
                    best_pos_world = toAimCoords;
                    is_best_pos_world_valid = true;
                }
            }

            // This is just for drawing, so it's okay to do it for everyone
            ctx->draw_box(screenCoords.x - 5, screenCoords.y - 5, 10, 10, 1.0f, COLOR::GREEN);
        }


        // --- Aimbot Logic ---
        if (inputManager.isKeyDown(KEY_2)) {
            FVector final_aim_target;
            bool should_aim = false;

            // --- TARGET PRIORITIZATION (from previous code) ---
            if (is_best_pos_world_valid && closest_dist_in_world < 500.0f) {
                final_aim_target = best_pos_world;
                should_aim = true;
            }
            else if (is_best_pos_crosshair_valid) {
                final_aim_target = best_pos_crosshair;
                should_aim = true;
            }

            if (should_aim) {
                Coords targetScreenCoords = WorldToScreen(final_aim_target, CameraCache.POV, MonWidth, MonHeight);

                // Calculate the precise floating-point distance from the crosshair
                float deltaX = targetScreenCoords.x - (MonWidth / 2.0f);
                float deltaY = targetScreenCoords.y - (MonHeight / 2.0f);

                // --- NEW: Dead Zone Implementation ---
                // If the target is within this many pixels of the crosshair, stop moving.
                const float aim_deadzone = 5.0f;

                if (std::abs(deltaX) < aim_deadzone) {
                    deltaX = 0.0f;
                }
                if (std::abs(deltaY) < aim_deadzone) {
                    deltaY = 0.0f;
                }

                // --- End of Dead Zone Logic ---


                // Only proceed if there's still a delta after the dead zone check
                if (deltaX != 0.0f || deltaY != 0.0f) {
                    // Define a smoothing factor
                    const float aim_smoothness = 20.0f;

                    // Calculate the ideal (but potentially fractional) mouse movement
                    float moveX_float = deltaX / aim_smoothness;
                    float moveY_float = deltaY / aim_smoothness;

                    // Convert to integer for the mouse.moveRelative function
                    int moveX = static_cast<int>(moveX_float);
                    int moveY = static_cast<int>(moveY_float);

                    // --- Minimum Movement Logic (from previous code) ---
                    // This now only triggers if the target is OUTSIDE the dead zone
                    // but the smoothed movement rounded to zero.
                    if (moveX == 0 && deltaX != 0.0f) {
                        moveX = (deltaX > 0) ? 1 : -1;
                    }
                    if (moveY == 0 && deltaY != 0.0f) {
                        moveY = (deltaY > 0) ? 1 : -1;
                    }

                    // Move the mouse
                    inputManager.moveMouseRelative(moveX, moveY);
                }
            }
        }

        ctx->end_frame();
    }


    delete ctx;
    return 0;
}