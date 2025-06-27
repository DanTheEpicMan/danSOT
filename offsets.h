#pragma once

#include <cstdint>
#define ptr uintptr_t

namespace Offsets {
    ptr UWorld = 0x8A19E30; //Or GWorld, same thing if your updating
        ptr GameState = 0x40; //Read forom UWorld
            ptr PlayerArray = 0x3C8; //Read from GameState
        ptr OwnerGameInstance = 0x250; //Reads from Uworld
            ptr LocalPlayers = 0x38; //Reads from GameInstance, reads array, need to read result to get first local player instance
                ptr PlayerController = 0x30; //read from LocalPlayer
                    ptr LocalPawn = 0x410; //Read from PlayerController, gets inherited by "AthenaCharacter" so all stuff under there is accessible from this Pawn

                        //Pawn offsets (both local and enemy)
                        ptr HealthComponent = 0x898; //Read rom Pawn (part of AthenaCharacter)
                            ptr Health = 0xD4; //Reads to CurrentHealthInfo struct, Health is first part of that struct
                        ptr RootComponent = 0x160;
                            ptr RelativeLocation = 0xF8;
                            ptr RelativeRotation = 0x104;
                            ptr RelativeScale3D = 0x110;
                        ptr ActorID = 0x24; //Part of actor class but get from Pawn, used for name
                        ptr Mesh = 0x418; //SkeletalMeshComponent, used for bones. Part of Character witch is subclass of Pawn

                    //PlayerController Offsets
                    ptr PlayerCameraManager = 0x430; //Read from PlayerController
                        ptr CameraCachePrivate = 0x410; //Read from PlayerCameraManager, contains FCameraCacheEntry struct


        ptr PresistentLevel = 0x60; // Part of UWorld
            ptr OwningActor = 0xA0; //Part of Object part of Level (IDK what this is based on but this value probs not changing)

    ptr GName = 0x8948EA8;

    // ship (pawn when itterating through array)
    ptr HullDamage = 0xF80;
        ptr ActiveHullDamageZones = 0x450; //part of HullDamage
            ptr SceneRootComponent = 0x450; //part of DamageZone, gives scene component
                // RelativeLocation = 0xF8; //part of SceneComponent, gives location of anything, in this case damage zone

    ptr ShipInternalWaterComponent = 0x630; //Part of ship
        ptr ChildActor = 0x2D8; //Part of ShipInternalWaterComponent, gives ShipInternalWaterActor
            ptr InternalWaterParams = 0x3B8;  ptr MaxWaterAmount = 0x8; //Part of ShipInternalWaterActor, gives ShipInternalWaterParams struct
            ptr WaterAmount = 0x3F4; //Part of ShipInternalWaterActor, gives water amount in float, 0-1
}
