#pragma once

#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>
#define ptr uintptr_t

namespace Offsets {
    inline ptr UWorld = 0x8A19E30; //Or GWorld, same thing if your updating
        inline ptr GameState = 0x40; //Read forom UWorld
            inline ptr PlayerArray = 0x3C8; //Read from GameState
            inline ptr CrewService = 0x5D0;
                inline ptr Crews = 0x478; //Read TArray from CrewService, contains Crew structs
                    inline ptr CrewMatesPlayerStates = 0x20; // In Crew struct, called "Players". Read TArray from Crew struct, contains PlayerState structs

        inline ptr OwnerGameInstance = 0x250; //Reads from Uworld
            inline ptr LocalPlayers = 0x38; //Reads from GameInstance, reads array, need to read result to get first local player instance
                inline ptr PlayerController = 0x30; //read from LocalPlayer
                    inline ptr LocalPawn = 0x410; //Read from PlayerController, gets inherited by "AthenaCharacter" so all stuff under there is accessible from this Pawn

                        //Pawn offsets (both local and enemy)
                        inline ptr HealthComponent = 0x898; //Read rom Pawn (part of AthenaCharacter)
                            inline ptr Health = 0xD4; //Reads to CurrentHealthInfo struct, Health is first part of that struct
                        inline ptr RootComponent = 0x160;
                            inline ptr RelativeLocation = 0xF8;
                            inline ptr RelativeRotation = 0x104;
                            inline ptr RelativeScale3D = 0x110;
                            inline ptr ComponentToWorld = 0x130;
                        inline ptr ActorID = 0x24; //Part of actor class but get from Pawn, used for name
                        inline ptr Mesh = 0x418; //SkeletalMeshComponent, used for bones. Part of Character witch is subclass of Pawn
                        inline ptr WieldedItemComponent = 0x870; //WieldableItemComponent, used for wielded item
                            inline ptr ReplicatedCurrentlyWieldedItem = 0x2d8; //Gives actor (subclass ProjectileWeapon) from WieldedItemComponent
                                inline ptr WeaponParameters = 0x848; //Reads a struct ProjectileWeaponParameters
                                    inline ptr ProjectileWeaponParameters_AmmoParams = 0x80; //AmmoParams in struct ProjectileWeaponParameters, reads struct WeaponProjectileParams
                                        inline ptr WeaponProjectileParams_Velocity = 0x10; //Velocity in struct WeaponProjectileParams, gives float value of projectile speed (read float)
                    //PlayerController Offsets
                    inline ptr PlayerCameraManager = 0x430; //Read from PlayerController
                        inline ptr CameraCachePrivate = 0x410; //Read from PlayerCameraManager, contains FCameraCacheEntry struct

                    //Player State
                    inline ptr PlayerState = 0x3C0;
                        inline ptr PlayerId = 0x3C8; //Read from PlayerState, used for crew member check

        inline ptr PresistentLevel = 0x60; // Part of UWorld
            inline ptr OwningActor = 0xA0; //Part of Object part of Level (IDK what this is based on but this value probs not changing)

    inline ptr GName = 0x8948EA8;

    // ship (pawn when itterating through array)
    inline ptr HullDamage = 0xF80;
        inline ptr ActiveHullDamageZones = 0x450; //part of HullDamage
            inline ptr SceneRootComponent = 0x450; //part of DamageZone, gives scene component
                inline ptr ActorCoordinates = 0x11c; //part of SceneComponent, gives location of anything, in this case damage zone

    inline ptr ShipInternalWaterComponent = 0x630; //Part of ship
        inline ptr ChildActor = 0x2D8; //Part of ShipInternalWaterComponent, gives ShipInternalWaterActor
            inline ptr InternalWaterParams = 0x3B8;  inline ptr MaxWaterAmount = 0x8; //Part of ShipInternalWaterActor, gives ShipInternalWaterParams struct
            inline ptr WaterAmount = 0x3F4; //Part of ShipInternalWaterActor, gives water amount in float, 0-1
}

#endif // OFFSETS_H