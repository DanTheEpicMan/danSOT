//Extern Headers
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>
#include <cfloat>
#include <chrono>
#include <linux/input-event-codes.h>

//Local Headers
// #include "utils/ProcessUtils.h"
// #include "memory/Memory.h"
// #include "utils/LocalStructs.h"
// #include "offsets.h"
// #include "utils/GameStructs.h"
// #include "overlay/drawing.h" //overlay/shared_data.h imported by this
// #include "utils/tables.h"
// #include "hacks/Aimbot.h"
// #include "hacks/InputManager.h"
// #include "hacks/InputManager.h"

#include "aimbot/CannonAimbot.h"
 #include "aimbot/PlayerAimbot.h"
#include "esp/ESP.h"
#include "IO/InputManager.h"
#include "memory/Memory.h"
#include "memory/ProcessUtils.h"
#include "overlay/drawing.h"
#include "overlay/shared_data.h"
#include "utils/GameData.h"
#include "utils/LocalData.h"
#include "utils/LocalFunctions.h"
#include "config.h"
#include "offsets.h"

//Drawing stuff
DrawingContext* ctx = nullptr;
void cleanup_shm(int signum) {
    std::cout << "\nCaught signal " << signum << ". Cleaning up..." << std::endl;
    delete ctx;
    ctx = nullptr;
    exit(0);
}

#define ptr uintptr_t //Makes it a little more readable

CannonAimbot cannonAimbot;
PlayerAimbot playerAimbot;
ESP esp;
InputManager inputManager;


int main() {
    if (getuid() != 0) {
        std::cerr << "Must run as root (sudo)" << std::endl;
        return 1;
    }

    if (!inputManager.isVirtualMouseInitialized()) {
        std::cerr << "Could not create virtual mouse. Aborting." << std::endl;
        return 1;
    }

    signal(SIGINT, cleanup_shm); //for drawing
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

    int loopIndex = 0;
    while (true) {
        loopIndex++;
        std::cout << "LN: " << loopIndex << std::endl;

        ptr UWorld = ReadMemory<ptr>(BaseAddress + Offsets::UWorld);
        ptr GameState = ReadMemory<ptr>(UWorld + Offsets::GameState);
        ptr PersistentLevel = ReadMemory<ptr>(UWorld + Offsets::PresistentLevel);
        ptr GameInstance = ReadMemory<ptr>(UWorld + Offsets::OwnerGameInstance);
        ptr LocalPlayers = ReadMemory<ptr>(GameInstance + Offsets::LocalPlayers);
        ptr LocalPlayer = ReadMemory<ptr>(LocalPlayers);
        ptr PlayerController = ReadMemory<ptr>(LocalPlayer + Offsets::PlayerController);
        ptr LPawn = ReadMemory<ptr>(PlayerController + Offsets::LocalPawn);
        ptr GNames = ReadMemory<ptr>(BaseAddress + Offsets::GName);


        std::vector<int> myCrewIDs = getMyCrewIDs(GameState, PlayerController);

        std::vector<Entity> EnemyPlayers, TeamPlayers, EnemyShips, otherEntities;

        TArray<uintptr_t> PlayersTArray = ReadMemory<TArray<uintptr_t>>(PersistentLevel + Offsets::OwningActor);
        int playerCount = PlayersTArray.Length();
        uintptr_t PlayerStateArray = PlayersTArray.GetAddress();
        for (int i = 0; i < playerCount; i++) {
            Entity ent;
            ptr entPawn = ReadMemory<ptr>(PlayerStateArray + (i * sizeof(ptr)));
            if (entPawn == 0) continue;

            ent.pawn = entPawn;

            //Get name
            ent.name = getNameFromPawn(ent.pawn, GNames);

            //Get Location
            auto ActorRootComponent = ReadMemory<uintptr_t>(entPawn + Offsets::RootComponent);
            if (ActorRootComponent == 0x0) continue;
            ent.location = ReadMemory<FVector>(ActorRootComponent + Offsets::RelativeLocation);

            //TODO: ID buring blade in this.
            if (ent.name.find("ShipTemplate_C") != std::string::npos ||
                ent.name.find("ShipNetProxy_C") != std::string::npos ||
                ent.name.find("AggressiveGhostShip") != std::string::npos) {
                EnemyShips.push_back(ent);
            } else if (ent.name == "BP_PlayerPirate_C"/* || ent.name.find("SkeletonPawnBase") != std::string::npos*/) { //uncomment for testing on skeleton

                ptr playerEntityRootComponent  = ReadMemory<ptr>(ent.pawn + Offsets::RootComponent);
                FTransform playerEntityComponentToWorld = ReadMemory<FTransform>(playerEntityRootComponent + Offsets::ComponentToWorld);
                ent.location = playerEntityComponentToWorld.Translation;

                //Weeding out people on same team
                ptr PlayerStateFromPawn = ReadMemory<ptr>(entPawn + Offsets::PlayerState);
                int PlayerID_entity = ReadMemory<int>(PlayerStateFromPawn + Offsets::PlayerId);
                bool isMyCrewMember = false;
                for (int crewMateId : myCrewIDs) {
                    if (crewMateId == PlayerID_entity) {
                        isMyCrewMember = true;
                        break;
                    }
                }
                if (isMyCrewMember) {
                    TeamPlayers.push_back(ent);
                } else {
                    EnemyPlayers.push_back(ent);
                }
            } else {
                otherEntities.push_back(ent);
            }
        } //for i

        ctx->begin_frame();
        ctx->draw_text_uncentered(10, 15, "DanSOT", COLOR::WHITE);

        esp.Run(LPawn, PlayerController, EnemyPlayers, TeamPlayers, EnemyShips, otherEntities, ctx);
        playerAimbot.Run(LPawn, PlayerController, EnemyPlayers, EnemyShips, ctx, &inputManager);
        cannonAimbot.Run(GNames, LPawn, PlayerController, EnemyShips, EnemyPlayers, otherEntities, ctx, &inputManager);

        ctx->end_frame();
    }//while true

    delete ctx;
    return 0;
}//main

