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

    uintptr_t world = ReadMemory<uintptr_t>(BaseAddress + 0x8972940); //Read the UWorld address
    std::cout << "UWorld Address: " << world  << std::endl;


    // //Defining the overlay
    // Overlay ov = Overlay();
    // ov.initialize("Sea of Thieves");
    // ov.update(); ov.clearBackBuffer();
    // GlobalCheatData.ov = ov; //Give overlay to global data

    return 1;

    uintptr_t UWorld, PlayerController, GameInstance, GameState, LocalPlayer, PlayerStateArray;
    int loopCount = 0; // Used for reading some things more rarly
    while (true) { loopCount++; // Main loop, loopCount increases every time
        if (loopCount % 100000 == 0) { //every 1000 loops
            UWorld = ReadMemory<uintptr_t>(BaseAddress + Offsets::UWorld);
            GameState = ReadMemory<uintptr_t>(UWorld + Offsets::GameState);
            PlayerStateArray = ReadMemory<uintptr_t>(GameState + Offsets::PlayerArray);
            std::cout << "UWorld: 0x" << UWorld  << std::endl;
            std::cout << "GameState: 0x" << std::hex << GameState << std::dec << std::endl;
            std::cout << "PlayerStateArray: 0x" << std::hex << PlayerStateArray << std::dec << std::endl;


        }

        int32_t actorCount = ReadMemory<ptr>(PlayerStateArray + 0x8 /*Get length of the TArray*/);
        //std::cout << "Actor Count: " << actorCount << std::endl;

        for (int i = 0; i < actorCount; i++) {
            ptr PlayerState = ReadMemory<ptr>(PlayerStateArray + 0x10 + (i * sizeof(ptr))); //Get the player state
            if (PlayerState == 0) continue; //If the player state is null, skip it

            auto PlayerNameLength = ReadMemory<int32_t>(PlayerState + 0x3A8 + 0x8 /*Offset to FString*/);
            std::cout << "Name lenght: " << PlayerNameLength << std::endl;
        }
    }


}