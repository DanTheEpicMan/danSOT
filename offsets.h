#pragma once

#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>
#define ptr uintptr_t

namespace Offsets {
    inline ptr UWorld = 0x8AAED30; //Or GWorld, same thing if your updating
        inline ptr GameState = 0x40; //Read forom UWorld
            inline ptr PlayerArray = 0x3C8; //Read from GameState
            inline ptr CrewService = 0x5D0; //Athena_Classes
                inline ptr Crews = 0x478; //Read TArray from CrewService, contains Crew structs
                    inline ptr CrewMatesPlayerStates = 0x20; // In Crew struct, called "Players". Read TArray from Crew struct, contains PlayerState structs

        inline ptr OwnerGameInstance = 0x250; //Reads from Uworld
            inline ptr LocalPlayers = 0x38; //Reads from GameInstance, reads array, need to read result to get first local player instance
                inline ptr PlayerController = 0x30; //read from LocalPlayer
                    inline ptr LocalPawn = 0x410; //Read from PlayerController, gets inherited by "AthenaCharacter" so all stuff under there is accessible from this Pawn

                        //Pawn offsets (both local and enemy)
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
                        inline ptr Children = 0x150; //TArray of Actors from Pawn

                        //Humanoids (players, skeletons, etc.)
                        inline ptr HealthComponent = 0x898; //Read rom Pawn (part of AthenaCharacter)
                            inline ptr Health = 0xD4; //Reads to CurrentHealthInfo struct, Health is first part of that struct
                        inline ptr CharacterMovement = 0x420; //Read from Pawn, gives CharacterMovementComponent
                            inline ptr MovementComponent_Velocity = 0xCC; //Read from CharacterMovement (super of CharacterMovementComponent), gives FVector of velocity
                        inline ptr BasedMovement = 0x430; inline ptr MovementBase = 0x0; //Read from Pawn, gives struct BasedMovementInfo, at 0x0 in that is MovementBase (class PrimitiveComponent)
                            inline ptr AttachParent = 0xD0; //in SceneComponent, super of PrimitiveComponent, gives SceneComponent (Actually is ChildActorComponent)
                                inline ptr ChildActor = 0x2D8; //Read from ChildActorComponent, Read ChildActor of type Actor
                                    inline ptr ParentComponentActor = 0x190; //Read from class Actor, type struct ActorPtr witch just holds pointer to Actor

                        //Pawn for canons
                        inline ptr ProjectileSpeed = 0x59C; //Read from Pawn, gives float value of projectile speed
                        inline ptr ProjectileGravityScale = 0x5A0; //Read from Pawn, gives float value of projectile gravity scale
                        inline ptr LoadableComponent = 0x538; //Read from Pawn, gives LoadableComponent struct
                            inline ptr LoadableComponentState = 0x178; inline ptr LoadedItem = 0x8; //Read from LoadableComponent, gives LoadableComponentState struct, 0x8 at that struct gives Object of loaded item

                    //PlayerController Offsets
                    inline ptr PlayerCameraManager = 0x430; //Read from PlayerController
                        inline ptr CameraCachePrivate = 0x410; //Read from PlayerCameraManager, contains FCameraCacheEntry struct

                    //Player State
                    inline ptr PlayerState = 0x3C0;
                        inline ptr PlayerId = 0x3C8; //Read from PlayerState, used for crew member check

        inline ptr PresistentLevel = 0x60; // Part of UWorld
            inline ptr OwningActor = 0xA0; //Part of Object part of Level (IDK what this is based on but this value probs not changing)

    inline ptr GName = 0x89DDCB8;

    // ship (pawn when itterating through array)
    inline ptr HullDamage = 0xF90;
        inline ptr DamageZones = 0x440; //part of HullDamage
            inline ptr DamageLevel = 0x754; //part of DamageZone, gives int value of damage level
            // inline ptr ActorCoordinates = 0x11c; //part of SceneComponent, gives location of anything, in this case damage zone
        inline ptr ActiveHullDamageZones = 0x450; //part of HullDamage
            inline ptr SceneRootComponent = 0x450; //part of DamageZone, gives scene component
                inline ptr ActorCoordinates = 0x11c; //part of SceneComponent, gives location of anything, in this case damage zone

    inline ptr ShipMovementProxyComponent = 0x638; //Part of ship pawn, class it reads is ChildActorComponent
        //defined above //inline ptr ChildActor = 0x2D8; //Part of ChildActorComponent, gives ShipMovementProxy
            inline ptr ShipMovement = 0x3B0; inline ptr ReplicatedShipMovement_Movement = 0x0; //Part of ShipMovementProxy, gives struct ReplicatedShipMovement, at 0x0 in that struct is another struct: RepMovement
                //All of these are still part of RepMovement so when your trying to read them, do: Read<type>(ShipPawn + ShipMovementProxyComponent + ShipMovement + ReplicatedShipMovement_Movement + <offset below>)
                //All part if RepMovement struct
                inline ptr LinearVelocity = 0x0; //FVector, gives ship velocity
                inline ptr AngularVelocity = 0xC; //FVector, gives ship angular velocity
                inline ptr Location = 0x18; //FVector, gives ship location
                inline ptr Rotation = 0x24; //FRotator, gives ship rotation

    inline ptr ShipInternalWaterComponent = 0x630; //Part of ship
        //defined above //inline ptr ChildActor = 0x2D8; //Part of ShipInternalWaterComponent, gives ShipInternalWaterActor
            inline ptr InternalWaterParams = 0x3B8;  inline ptr MaxWaterAmount = 0x8; //Part of ShipInternalWaterActor, gives ShipInternalWaterParams struct
            inline ptr WaterAmount = 0x3F4; //Part of ShipInternalWaterActor, gives water amount in float, 0-1
}

#endif // OFFSETS_H