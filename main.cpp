//Extern Headers
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>

//Local Headers
#include <cfloat>
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
    // std::cout << "[GetShipVelocityByDistance] rootComponent: 0x" << std::hex << rootComponent << std::dec << std::endl;
    FTransform componentToWorld = ReadMemory<FTransform>(rootComponent + 0x130);
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

FVector GetPlayerGlobalVelocitySloppy(uintptr_t playerPawnAddress, std::vector<Entity> ships) { //could be improved by checking AthenaCameraComponent->IsInsideShipHull
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

        // uintptr_t WorldSettings = ReadMemory<uintptr_t>(UWorld + 0x3A0);
        // WorldGravityZ = ReadMemory<float>(WorldSettings + 0x548);
        //
        // std::cout << "World Settings: " << std::hex << WorldSettings << std::dec << std::endl;
        // std::cout << "World Gravity Z: " << WorldGravityZ << std::endl;

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
            // else if (entity.name == "BP_Cannon_ShipPartMMC_C" || entity.name == "BP_Cannon_ShipPartMMC_b_C" || entity.name == "BP_SeaFort_Cannon_C" || entity.name == "BP_IslandCannon_C") {
            //     cannonEntities.push_back(entity);
            // }
            else if (entity.name == "BP_Rowboat_C" || entity.name == "BP_Rowboat_WithFrontCannon_C" || entity.name == "BP_Rowboat_WithFrontHarpoon_C") {
                entity.type = EntTypes::ROWBOAT;
                entity.rowboat.displayName = "Rowboat";
                if (entity.name == "BP_Rowboat_WithFrontCannon_C") entity.rowboat.displayName += " Cannon";
                if (entity.name == "BP_Rowboat_WithFrontHarpoon_C") entity.rowboat.displayName += " Harpoon";
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
        ctx->draw_line(MonWidth/2 - 10, MonHeight/2, MonWidth/2 + 10, MonHeight/2, 2, COLOR::ORANGE);
        ctx->draw_line(MonWidth/2, MonHeight/2 - 10, MonWidth/2, MonHeight/2 + 10, 2, COLOR::ORANGE);

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
        if (!WieldedItem) continue; /////////////////////////////////////////////////////////////////////////////////////////////////Very aimbot specific placement
        uintptr_t weaponParamsPtrPRE = ReadMemory<uintptr_t>(WieldedItem + 0x2d8);
        float bulletSpeed = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80 + 0x10); //ProjectileWeapons->WeaponParameters + AmmoParams + Velocity
        // float TimeBeforeApplyingGravity = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80 +         0x14);
        // float GravityFalloffTimeScalar = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80 +          0x18);
        // float DownForceVelocityFractionPerSecond = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80+ 0x1C);
        // float VelocityDampeningPerSecond = ReadMemory<float>(weaponParamsPtrPRE + 0x848 + 0x80 +        0x20);

        //Getting used cannon
        TArray<uintptr_t> LPChildren = ReadMemory<TArray<uintptr_t>>(LPawn + 0x150);
        uintptr_t cannonActor = 0x0;
        for (int i = 0; i < LPChildren.Length(); i++) {
            uintptr_t ChildActor = ReadMemory<uintptr_t>(LPChildren.GetAddress() + (i * sizeof(uintptr_t)));
            int ChildID = ReadMemory<int>(ChildActor + Offsets::ActorID);

            //name
            char interactable_name_cannon[68];
            {
                const auto chunk_offset = (ChildID / 0x4000) * 8;
                const auto chunk_index_offset = (ChildID % 0x4000) * 8;
                const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);
                const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);
                ReadMemoryBuffer(name_ptr + 0xC, interactable_name_cannon, sizeof(interactable_name_cannon));
                interactable_name_cannon[67] = '\0';
            }
            std::string PotentialCannonName = interactable_name_cannon;


            if (PotentialCannonName.find("IslandCannon") != std::string::npos || PotentialCannonName.find("Cannon_Ship") != std::string::npos) {
                cannonActor = ChildActor;
            }
        }

        if (cannonActor != 0x0) {
            //Get cannon location
            uintptr_t cannonSceneComponent = ReadMemory<uintptr_t>(cannonActor + Offsets::RootComponent);
            FTransform cannonTransform = ReadMemory<FTransform>(cannonSceneComponent + 0x130);
            FVector cannonLocation = cannonTransform.Translation;
            FVector cannonVelocity = GetShipVelocityByDistance(LPawn, shipEntities); //Velocity

            float projectileSpeed = ReadMemory<float>(cannonActor + 0x59C);
            float gravityScalar = ReadMemory<float>(cannonActor + 0x5A0);

            uintptr_t LoadableComponent = ReadMemory<uintptr_t>(cannonActor + 0x538);
            uintptr_t LoadedItem = ReadMemory<uintptr_t>(LoadableComponent + 0x178 + 0x8); // 1: LoadableComponent.LoadableComponentState and 2: LoadableComponentState.LoadedItem
            if (LoadedItem != 0x0) {
                char loadedItemName[68];
                const auto chunk_offset = (ReadMemory<int>(LoadedItem + Offsets::ActorID) / 0x4000) * 8;
                const auto chunk_index_offset = (ReadMemory<int>(LoadedItem + Offsets::ActorID) % 0x4000) * 8;
                const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);
                const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);
                ReadMemoryBuffer(name_ptr + 0xC, loadedItemName, sizeof(loadedItemName));
                loadedItemName[67] = '\0';

                std::string loadedItemStr = loadedItemName;
                if (loadedItemStr.find("Player") != std::string::npos) {
                    std::cout << "Loaded Item: Player: " << loadedItemName << std::endl;
                    gravityScalar = 1.3f;
                } else if (loadedItemStr.find("chain_shot") != std::string::npos) {
                    gravityScalar = 1.0f;
                }
            }


            for (int j = 0; j < shipEntities.size(); j++) {
                uintptr_t movementProxyComponent = ReadMemory<uintptr_t>(shipEntities[j].pawn + 0x638);
                uintptr_t movementProxyActor = ReadMemory<uintptr_t>(movementProxyComponent + 0x2d8); // UChildActorComponent::ChildActor
                FVector shipLinearVel = ReadMemory<FVector>(movementProxyActor +  0x3b0 + 0x0 + 0x0); //in subclass ShipMovementProxy, struct ReplicatedShipMovement, inside that struct at 0x0 is RepMovement
                FVector shipAngularVel = ReadMemory<FVector>(movementProxyActor + 0x3b0 + 0x0 + 0xC); //AngularVelocity
                FVector shipLocation = ReadMemory<FVector>(movementProxyActor +   0x3b0 + 0x0 + 0x18); //Location

                FRotator out_low, out_high;
                FVector AimAtPoint;

                int solutions = AimAtShip(shipLocation, shipLinearVel, shipAngularVel, //enemy
                    cannonLocation, cannonVelocity, //us
                    projectileSpeed, gravityScalar, //other info
                    out_low, out_high); //output`


                FVector aimDirection = RotatorToVector(out_low);
                FVector aimPointInSpace = cannonLocation + (aimDirection * 10000.f);

                if (solutions > 0) {
                    Coords AimAtCoords = WorldToScreen(aimPointInSpace, CameraCache.POV, MonWidth, MonHeight);
                    ctx->draw_text(AimAtCoords.x - 3, AimAtCoords.y, "X", COLOR::RED);
                }
            }


        }


        // for (int i = 0; i < cannonEntities.size(); i++) {
        //     std::cout << "Cannon Entity: " << cannonEntities[i].name << std::endl;
        //     Entity& entity = cannonEntities[i];
        //
        //     float ServerPitch = ReadMemory<float>(entity.pawn + 0x7A0);
        //     std::cout << "Server Pitch: " << ServerPitch << std::endl;
        //
        //     uintptr_t cannonSceneComponent = ReadMemory<uintptr_t>(entity.pawn + Offsets::RootComponent);
        //     FTransform cannonTransform = ReadMemory<FTransform>(cannonSceneComponent + 0x130);
        //     FVector cannonLocation = cannonTransform.Translation;
        //     std::cout << "Cannon Location: " << cannonLocation.x << ", " << cannonLocation.y << ", " << cannonLocation.z << std::endl;
        //
        //     uintptr_t CannonOwner = ReadMemory<uintptr_t>(entity.pawn + 0x190); //Get the owner of the cannon
        //     //read owner to get ship pawn
        //
        //     //display location
        //     Coords screenCoords = WorldToScreen(cannonLocation, CameraCache.POV, MonWidth, MonHeight);
        //     ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.name, COLOR::YELLOW);
        // }

        // uintptr_t CameraManager2 = ReadMemory<uintptr_t>(LPawn + 0x430);
        // std::cout << "CameraManager " << std::hex << CameraManager << std::dec << std::endl;
        // uintptr_t ViewTargetActor = ReadMemory<uintptr_t>(CameraManager2 + 0xF70);
        // std::cout << "ViewTargetActor: " << std::hex << ViewTargetActor << std::dec << std::endl;
        //
        // uintptr_t controlledPawn = ReadMemory<uintptr_t>(PlayerController + 0x410);
        // std::cout << "Controlled Pawn: " << std::hex << controlledPawn << std::dec << std::endl;
        // uintptr_t controlledID = ReadMemory<uintptr_t>(controlledPawn + Offsets::ActorID);
        //
        // char interactable_name_text[68];
        // std::cout << "Cannon ID: " << controlledID << std::endl;
        // {
        //         const auto chunk_offset = (controlledID / 0x4000) * 8;
        //         const auto chunk_index_offset = (controlledID % 0x4000) * 8;
        //         const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);
        //         const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);
        //         ReadMemoryBuffer(name_ptr + 0xC, interactable_name_text, sizeof(interactable_name_text));
        //         interactable_name_text[67] = '\0';
        // }
        // std::cout << "Interactable Name: " << interactable_name_text << std::endl;

        // --- Aimbot Configuration (Tweak these values!) ---
        const float AIMBOT_FOV = 250.0f;          // The radius in pixels from your crosshair to search for targets.
        const float THREAT_RADIUS = 750.0f;       // World distance (e.g., in units/cm) for emergency 360-degree targeting.

        // Dynamic Smoothing Parameters
        const float MIN_SMOOTHNESS = 3.0f;        // Hardest tracking (for closest targets). Lower is faster.
        const float MAX_SMOOTHNESS = 15.0f;       // Softest tracking (for farthest targets). Higher is smoother.
        const float MIN_SMOOTH_DIST = 500.0f;     // World distance where MIN_SMOOTHNESS is fully applied.
        const float MAX_SMOOTH_DIST = 8000.0f;    // World distance where MAX_SMOOTHNESS is fully applied.

        // Deadzone
        const float AIM_DEADZONE = 2.0f;          // Stop aiming when crosshair is this close (in pixels) to the target.


        // --- Find Best Target Loop (Refactored) ---

        // Initialize variables for the single best target
        float best_target_score = FLT_MAX; // Use the maximum possible float value
        FVector final_aim_target;
        float final_target_world_distance = 0.0f;
        bool target_found = false;

        FVector LPVelocityWithShip = GetPlayerGlobalVelocitySloppy(LPawn, shipEntities);

        for (int i = 0; i < playerEntities.size(); i++) {
            // --- Prediction ---
            FVector EnemyVelocityWithShip = GetPlayerGlobalVelocitySloppy(playerEntities[i].pawn, shipEntities);
            FVector toAimCoords = {0, 0, 0};
            bool worked = GetPlayerAimPosition_NoGravity(CameraCache.POV.Location, LPVelocityWithShip, playerEntities[i].location, EnemyVelocityWithShip, bulletSpeed, toAimCoords);

            // --- Convert to Screen Coords & Calculate Distances ---
            Coords screenCoords = WorldToScreen(toAimCoords, CameraCache.POV, MonWidth, MonHeight);

            if (screenCoords.x > 0 && screenCoords.x < MonWidth && screenCoords.y > 0 && screenCoords.y < MonHeight) {
                // Distance from crosshair (in pixels)
                float dx = screenCoords.x - (MonWidth / 2.0f);
                float dy = screenCoords.y - (MonHeight / 2.0f);
                float dist_from_crosshair = sqrt(dx * dx + dy * dy);

                // Distance from you (in world units)
                float dist_in_world = toAimCoords.Distance(CameraCache.POV.Location);

                // --- NEW: Scoring and Prioritization Logic ---
                bool is_in_threat_radius = dist_in_world < THREAT_RADIUS;
                bool is_in_fov = dist_from_crosshair < AIMBOT_FOV;

                // Only consider this player if they are a threat or in our FOV
                if (is_in_threat_radius || is_in_fov) {
                    float current_score;

                    if (is_in_threat_radius) {
                        // If they are an immediate threat, their score IS their world distance.
                        // This makes the physically closest person the highest priority.
                        current_score = dist_in_world;
                    } else {
                        // If they are just in the FOV, their score is based on crosshair distance.
                        // We add THREAT_RADIUS to ensure this score is ALWAYS higher (worse)
                        // than any possible score from a target inside the threat radius.
                        current_score = THREAT_RADIUS + dist_from_crosshair;
                    }

                    // If this target has a better score (lower is better), they are our new best target.
                    if (current_score < best_target_score) {
                        best_target_score = current_score;
                        final_aim_target = toAimCoords;
                        final_target_world_distance = dist_in_world;
                        target_found = true;
                    }
                }
            }
            // This is just for drawing, so it's okay to do it for everyone
            ctx->draw_box(screenCoords.x - 5, screenCoords.y - 5, 10, 10, 1.0f, COLOR::GREEN);
        }


        // --- Aimbot Logic (Refactored) ---
        if (inputManager.isKeyDown(KEY_2) && target_found) {
            Coords targetScreenCoords = WorldToScreen(final_aim_target, CameraCache.POV, MonWidth, MonHeight);

            float deltaX = targetScreenCoords.x - (MonWidth / 2.0f);
            float deltaY = targetScreenCoords.y - (MonHeight / 2.0f);

            // Check if we are already inside the deadzone
            if (std::abs(deltaX) > AIM_DEADZONE || std::abs(deltaY) > AIM_DEADZONE) {

                // --- NEW: Dynamic Smoothness Calculation ---
                float dynamic_smoothness;

                if (final_target_world_distance <= MIN_SMOOTH_DIST) {
                    dynamic_smoothness = MIN_SMOOTHNESS;
                } else if (final_target_world_distance >= MAX_SMOOTH_DIST) {
                    dynamic_smoothness = MAX_SMOOTHNESS;
                } else {
                    // Linearly interpolate the smoothness based on the target's distance.
                    // Calculate how far into the range [MIN_SMOOTH_DIST, MAX_SMOOTH_DIST] the target is.
                    float distance_ratio = (final_target_world_distance - MIN_SMOOTH_DIST) / (MAX_SMOOTH_DIST - MIN_SMOOTH_DIST);
                    // Apply that ratio to the smoothness range.
                    dynamic_smoothness = MIN_SMOOTHNESS + (MAX_SMOOTHNESS - MIN_SMOOTHNESS) * distance_ratio;
                }
                // --- End of Dynamic Smoothness ---

                // Calculate mouse movement using the new dynamic smoothness
                float moveX_float = deltaX / dynamic_smoothness;
                float moveY_float = deltaY / dynamic_smoothness;

                // Convert to integer for the mouse function
                int moveX = static_cast<int>(moveX_float);
                int moveY = static_cast<int>(moveY_float);

                // Minimum Movement Logic (ensures the mouse always moves if needed)
                if (moveX == 0 && std::abs(deltaX) > AIM_DEADZONE) {
                    moveX = (deltaX > 0) ? 1 : -1;
                }
                if (moveY == 0 && std::abs(deltaY) > AIM_DEADZONE) {
                    moveY = (deltaY > 0) ? 1 : -1;
                }

                // Move the mouse
                if (moveX != 0 || moveY != 0) {
                    inputManager.moveMouseRelative(moveX, moveY);
                }
            }
        }

        ctx->end_frame();
    }


    delete ctx;
    return 0;
}