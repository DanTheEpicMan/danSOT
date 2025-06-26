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


    uintptr_t UWorld, GNames, PlayerController, LPawn, GameInstance, Persistentlevel, GameState, LocalPlayers, LocalPlayer;
    int loopCount = 0;
    int x = 10;
    while (true) {
        loopCount++;
        if (x > 1000) x = 10;
        x++;

        //std::cout << "Loop count: " << loopCount << std::endl; // Removed for performance

        ctx->begin_frame();
        ctx->draw_text(x, 10, "DanSOT", COLOR::WHITE);

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

        for (int i = 0; i < playerCount; i++) {
            Entity entity;
            ptr PlayerPawn = ReadMemory<ptr>(PlayerStateArray + (i * sizeof(ptr)));
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
            entity.name = name_text;

            entity.



            //

            // Coords screenLoc = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
            //
            // if (screenLoc.x == -1) continue;
            //
            // ctx->draw_text(screenLoc.x, screenLoc.y, name_text, COLOR::WHITE);

            entity.invalid = false;

        }

        ctx->end_frame();
        //usleep(8000);
    }

    delete ctx;
    return 0;
}