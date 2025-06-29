//Extern Headers
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>

//Local Headers
#include <chrono>

#include "utils/ProcessUtils.h"
#include "memory/Memory.h"
#include "utils/LocalStructs.h"
#include "offsets.h"
#include "utils/GameStructs.h"
#include "overlay/drawing.h" //overlay/shared_data.h imported by this
#include "utils/tables.h"

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
    TArray<FTransform> boneArrayT = ReadMemory<TArray<FTransform>>(meshComponent + /*Offsets::SpaceBasesArray*/0x5D8);
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


int main() {
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

        ptr CameraManager = ReadMemory<ptr>(PlayerController + Offsets::PlayerCameraManager);
        FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);
        TArray<uintptr_t> PlayersTArray = ReadMemory<TArray<uintptr_t>>(Persistentlevel + Offsets::OwningActor);
        int playerCount = PlayersTArray.Length();
        uintptr_t PlayerStateArray = PlayersTArray.GetAddress();

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

            //------Finding Location------
            auto ActorRootComponent = ReadMemory<uintptr_t>(PlayerPawn + Offsets::RootComponent);
            if (ActorRootComponent == 0x0) continue;
            entity.location = ReadMemory<FVector>(ActorRootComponent + Offsets::RelativeLocation);
            if (entity.location.x == 0) continue;

            //------Weed out Local Player like things------
            if (entity.location.Distance(CameraCache.POV.Location) < 10.f) {
                // Skip entities that are too close to the player
                continue;
            }

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


                if (CameraCache.POV.Location.Distance(entity.location) > 175000.f) { //If the ship is too far away, skip it
                    goto EndConditions; // Always wanted to use goto for something
                }

                if (CameraCache.POV.Location.Distance(entity.location) < 3000) entity.ship.onShip = true;


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





            } else if (entity.name == "BP_Mermaid_C" || entity.name == "BP_LootStorage_Retrieve_C") {
                entity.type = EntTypes::MERMADE;
                entity.mermade.displayName = "Mermade";
                if (entity.name == "BP_LootStorage_Retrieve_C") entity.mermade.displayName = "Loot Mermade"; //Holds loot when in shrines
            } else if (entity.name == "BP_PlayerPirate_C") {
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
                if (isMyCrewMember) { std::cout << "Ending" << std::endl; continue; }
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
                    ctx->draw_text(screenCoords.x-30, screenCoords.y, entity.ship.displayName + "-" + std::to_string(entity.location.Distance(CameraCache.POV.Location)), COLOR::YELLOW);
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

        ctx->end_frame();
    }


    delete ctx;
    return 0;
}