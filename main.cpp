//Extern Headers
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>

//Local Headers
#include "utils/ProcessUtils.h"
#include "memory/Memory.h"
#include "utils/LocalStructs.h"
#include "offsets.h"
#include "utils/GameStructs.h"
#include "overlay/drawing.h" //overlay/shared_data.h imported by this

#define ptr uintptr_t //Makes it a little more readable

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
        if (loopCount % 100 == 0) {
            UWorld = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld);
            GameState = ReadMemory<uintptr_t>(UWorld + Offsets::GameState);
            Persistentlevel = ReadMemory<uintptr_t>(UWorld + Offsets::PresistentLevel);
            GameInstance = ReadMemory<uintptr_t>(UWorld + Offsets::OwnerGameInstance);
            LocalPlayers = ReadMemory<uintptr_t>(GameInstance + Offsets::LocalPlayers);
            LocalPlayer = ReadMemory<uintptr_t>(LocalPlayers);
            PlayerController = ReadMemory<uintptr_t>(LocalPlayer + Offsets::PlayerController);
            LPawn = ReadMemory<uintptr_t>(PlayerController + Offsets::LocalPawn);
            GNames = ReadMemory<ptr>(BaseAddress + Offsets::GName);

            // std::cout << "UWorld: 0x" << std::hex << UWorld << std::dec << std::endl;
            // std::cout << "GameState: 0x" << std::hex << GameState << std::dec << std::endl;
            // std::cout << "PersistentLevel: 0x" << std::hex << Persistentlevel << std::dec << std::endl;
            // std::cout << "GameInstance: 0x" << std::hex << GameInstance << std::dec << std::endl;
            // std::cout << "LocalPlayers: 0x" << std::hex << LocalPlayers << std::dec << std::endl;
            // std::cout << "LocalPlayer: 0x" << std::hex << LocalPlayer << std::dec << std::endl;
            // std::cout << "PlayerController: 0x" << std::hex << PlayerController << std::dec << std::endl;
            // std::cout << "LPawn: 0x" << std::hex << LPawn << std::dec << std::endl;
            // std::cout << "GNames: 0x" << std::hex << GNames << std::dec << std::endl;

        }

        if (!LPawn) {
            ctx->end_frame();
            usleep(100000);
            continue;
        }

        ptr CameraManager = ReadMemory<ptr>(PlayerController + Offsets::PlayerCameraManager);
        FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);
        TArray<uintptr_t> PlayersTArray = ReadMemory<TArray<uintptr_t>>(Persistentlevel + Offsets::OwningActor);
        int playerCount = PlayersTArray.Length();
        uintptr_t PlayerStateArray = PlayersTArray.GetAddress();

        uintptr_t HealthComp = ReadMemory<uintptr_t>(LPawn + Offsets::HealthComponent); //HealthComponent
        float Health = ReadMemory<float>(HealthComp + Offsets::Health); //CurrentHealthInfo (0xd4 + 0x0)
        //std::cout << "Health: " << Health << std::endl << std::endl;

        std::vector<Entity> goodEntities; //Stores useful stuff like players, boats, mermades, etc.
        int goodEntitiesCount = 0;

        for (int i = 0; i < playerCount; i++) {
            Entity entity;
            ptr PlayerPawn = ReadMemory<ptr>(PlayerStateArray + (i * sizeof(ptr))); //PlayerPawn does not have to be player, could be ship or anything else
            if (PlayerPawn == 0 || PlayerPawn == LPawn) continue;

            //------Finding Location------
            auto ActorRootComponent = ReadMemory<uintptr_t>(PlayerPawn + Offsets::RootComponent);
            if (ActorRootComponent == 0x0) continue;
            entity.location = ReadMemory<FVector>(ActorRootComponent + Offsets::RelativeLocation);
            if (entity.location.x == 0) continue;

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

                if (CameraCache.POV.Location.Distance(entity.location) > 1750.f) { //If the ship is too far away, skip it
                    goto EndConditions; // Always wanted to use goto for something
                }

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
                    uintptr_t ShipSceneComponent = ReadMemory<uintptr_t>(DamageZone + Offsets::SceneRootComponent);
                    FTransform comTrans = ReadMemory<FTransform>(ShipSceneComponent + 0x2e0 /*also tried 0x240*/);
                    entity.ship.holeLocations[j] = comTrans.Translation; // Get the hole location from the transform

                    std::cout << "Hole " << j << " Location: " << entity.ship.holeLocations[j].x << ", "
                              << entity.ship.holeLocations[j].y << ", "
                              << entity.ship.holeLocations[j].z << std::endl;
                    // entity.ship.holeLocations[j] = ReadMemory<FVector>(ShipSceneComponent + Offsets::RelativeLocation);

                    //Everying reads 0, 0, 0 for the position of the holes but the amount reads correct
                }

            } else if (entity.name == "BP_Mermade_C" || entity.name == "BP_LootStorage_Retrieve_C") {
                entity.type = EntTypes::MERMADE;
                entity.mermade.displayName = "Mermade";
                if (entity.name == "BP_LootStorage_Retrieve_C") entity.mermade.displayName = "Loot Mermade"; //Holds loot when in shrines
            } else if (entity.name == "BP_PlayerPirate_C") {
                entity.type = EntTypes::PLAYER;
                entity.player.displayName = "Player";
            } else {
                continue; // Skip unknown entities
                std::cout << entity.name << std::endl;
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


            if (entity.type == EntTypes::NETPROXYSHIP) {
                Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x, screenCoords.y, entity.netProxyShip.displayName, COLOR::YELLOW);

            } else if (entity.type == EntTypes::SHIP) {
                Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CameraCache.POV, MonWidth, MonHeight);
                int waterPercentage = static_cast<int>(entity.ship.WaterAmount * 100);
                if (waterPercentage == 0 && entity.ship.holeCount == 0) {
                    ctx->draw_text(screenCoords.x, screenCoords.y, entity.ship.displayName, COLOR::GREEN);
                } else {
                    ctx->draw_text(screenCoords.x, screenCoords.y, entity.ship.displayName + " (" + std::to_string(entity.ship.holeCount) + " H - " + std::to_string(static_cast<int>(entity.ship.WaterAmount* 100)) + "%)", COLOR::GREEN);
                }
                for (int j = 0; j < entity.ship.holeCount; j++) {
                    Coords holeCoords = WorldToScreen(entity.ship.holeLocations[j], CameraCache.POV, MonWidth, MonHeight);
                    ctx->draw_box(holeCoords.x - 5, holeCoords.y - 5, 10, 10, 1.0f, COLOR::YELLOW);
                }

            } else if (entity.type == EntTypes::MERMADE) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x, screenCoords.y, entity.mermade.displayName, COLOR::MAGENTA);

            } else if (entity.type == EntTypes::PLAYER) {
                Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
                ctx->draw_text(screenCoords.x, screenCoords.y, entity.player.displayName, COLOR::RED);
            }
        }

        ctx->end_frame();
    }


    delete ctx;
    return 0;
}