//Extern Headers
#include <iostream>
#include <unistd.h>

//Local Headers
#include "utils/ProcessUtils.h"
#include "memory/Memory.h"
#include "overlay/Overlay.h"
#include "utils/LocalStructs.h"
#include "offsets.h"
#include "utils/GameStructs.h"
#define ptr uintptr_t //Makes it a little more readable

CheatData GlobalCheatData; // Global cheat data structure



int main() {
    if (getuid() != 0) {std::cerr << "Must run as root (sudo)"; return 1;}

    ProcessId = FindProcess("SotGame.exe");
    if (ProcessId == 0) {std::cerr << "Game Not found. Name might need to be updated" << std::endl; return 1;}
    std::cout << "Game is running, process id " << ProcessId << std::endl;

    BaseAddress = FindBaseImage(ProcessId, "SotGame.exe");
    if (BaseAddress == 0) {std::cerr << "Could not find game's base address." << std::endl; return 1;}
    std::cout << "Found base address: 0x" << std::hex << BaseAddress << std::dec << std::endl;

    //Main thread, intened for computing stuff once and passing it to the other threads

    uintptr_t world = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld); //Read the UWorld address
    std::cout << "UWorld Address: " << std::hex << world  << std::dec << std::endl;

    Overlay ov;

    if (!ov.initialize()) {
        std::cerr << "Fatal: Overlay initialization failed. See debug output for details." << std::endl;
        return 1;
    }


    uintptr_t UWorld, GNames, PlayerController, LPawn, GameInstance, Persistentlevel, GameState, LocalPlayers, LocalPlayer;
    int loopCount = 0; // Used for reading some things more rarly
    while (true) { loopCount++; // Main loop, loopCount increases every time
        ov.clearBackBuffer();
        //usleep(1000);
        ov.drawText(5, 18, "danSOT", Overlay::WHITE, 16);


        if (loopCount % 100 == 0) { //every 1000 loops
            UWorld = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld);
            GameState = ReadMemory<uintptr_t>(UWorld + Offsets::GameState);
            Persistentlevel = ReadMemory<uintptr_t>(UWorld + Offsets::PresistentLevel);

            // std::cout << "UWorld: 0x" << std::hex << UWorld << std::dec << std::endl;
            // std::cout << "GameState: 0x" << std::hex << GameState << std::dec << std::endl;

            GameInstance = ReadMemory<uintptr_t>(UWorld + Offsets::OwnerGameInstance); //OwningGameInstance
            LocalPlayers = ReadMemory<uintptr_t>(GameInstance + Offsets::LocalPlayers); //LocalPlayers
            LocalPlayer = ReadMemory<uintptr_t>(LocalPlayers); //getting out of list
            PlayerController = ReadMemory<uintptr_t>(LocalPlayer + Offsets::PlayerController); //PlayerController
            LPawn = ReadMemory<uintptr_t>(PlayerController + Offsets::LocalPawn); //Pawn

            // std::cout << "GameInstance: 0x" << std::hex << GameInstance << std::dec << std::endl;
            // std::cout << "LocalPlayer: 0x" << std::hex << LocalPlayer << std::dec << std::endl;
            // std::cout << "Localplayer: 0x" << std::hex << LocalPlayer << std::dec << std::endl;
            // std::cout << "PlayerController: 0x" << std::hex << PlayerController << std::dec << std::endl;
            // std::cout << "LPawn: 0x" << std::hex << LPawn << std::dec << std::endl;

            GNames = ReadMemory<ptr>(BaseAddress + Offsets::GName); //GNames
        }

        //Skip cheat stuff if in game
        if (!LPawn) continue;

        //----------- Local Player Stuff -----------------
        //Getting health
        uintptr_t HealthComp = ReadMemory<uintptr_t>(LPawn + Offsets::HealthComponent); //HealthComponent
        float Health = ReadMemory<float>(HealthComp + Offsets::Health); //CurrentHealthInfo (0xd4 + 0x0)
        std::cout << "Health: " << Health << std::endl << std::endl;

        //uintptr_t RootComponent = ReadMemory<uintptr_t>(LPawn + Offsets::RootComponent);

        ptr CameraManager = ReadMemory<ptr>(PlayerController + Offsets::PlayerCameraManager); //PlayerCameraManager
        FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate); //CameraCache
        //std::cout << "CameraCache test: " << CameraCache.POV.Location.x << std::endl;

        //----------- Other Player(and objects) Stuff -----------------
        TArray<uintptr_t> PlayersTArray = ReadMemory<TArray<uintptr_t>>(Persistentlevel + Offsets::OwningActor);
        int playerCount = PlayersTArray.Length();
        uintptr_t PlayerStateArray = PlayersTArray.GetAddress();

        for (int i = 0; i < playerCount; i++) {
            ptr PlayerPawn = ReadMemory<ptr>(PlayerStateArray + (i * sizeof(ptr))); //Get the player state
            if (PlayerPawn == 0 || PlayerPawn == LPawn) continue; //If the player state is null or is self, skip it

            auto actorID = ReadMemory<int>(PlayerPawn + Offsets::ActorID); //Get the actor ID (this is unique for each object)

            auto Mesh = ReadMemory<uintptr_t>(PlayerPawn + Offsets::Mesh);

            auto ActorRootComponent = ReadMemory<uintptr_t>(PlayerPawn + Offsets::RootComponent);
            if (ActorRootComponent == 0x0) continue;

            //if (!Mesh) continue;

            //Getting the name
            char name_text[68];
            {
                const auto chunk_offset = (actorID / 0x4000) * 8;
                const auto chunk_index_offset = (actorID % 0x4000) * 8;
                const auto name_chunk = ReadMemory<ptr>(GNames + chunk_offset);//data->intel_read<uintptr_t>(Cache::GNames + chunk_offset);
                const auto name_ptr = ReadMemory<ptr>(name_chunk + chunk_index_offset);//data->intel_read<uintptr_t>(name_chunk + chunk_index_offset);

                ReadMemoryBuffer(name_ptr + 0xC, name_text, sizeof(name_text));
                name_text[67] = '\0';

                //std::cout << "Name: " << name_text << std::endl;
            }

            FVector Location = ReadMemory<FVector>(ActorRootComponent + 0xF8); //RelativeLocation
            if (Location.x == 0) continue; //If the location is null, skip it


            //std::cout << "Location: " << std::hex << Location.x << " " << Location.y << " " << Location.z << std::dec << std::endl;
            //std::cout << "CameraCache test: " << CameraCache.POV.Location.x << std::endl;

            Coords screenLoc = WorldToScreen(Location, CameraCache.POV, ov.getWidth(), ov.getHeight());
            //std::cout << "Screen Location: " << screenLoc.x << " " << screenLoc.y << std::endl << std::endl;
            if (screenLoc.x == -1) continue;
            ov.drawText(screenLoc.x, screenLoc.y, name_text, Overlay::WHITE, 10);


            // Check if the actor is on screen before drawing
            if (screenLoc.x != -1) {

                // ===================================================================
                // ====== THIS IS THE FILTERING LOGIC YOU NEED TO ADD ================
                // ===================================================================

                c
            }
        }
        ov.swapBuffers();
        usleep(10);
    }


}