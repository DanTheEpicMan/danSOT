#include "ESP.h"


void ESP::Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> TeamPlayers,
              std::vector<Entity> EnemyShips, std::vector<Entity> otherEntities, DrawingContext *ctx) {

    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);

    if (this->drawCrosshair) {
        ctx->draw_line(MonWidth/2 - 10, MonHeight/2, MonWidth/2 + 10, MonHeight/2, 2, COLOR::ORANGE);
        ctx->draw_line(MonWidth/2, MonHeight/2 - 10, MonWidth/2, MonHeight/2 + 10, 2, COLOR::ORANGE);
    }

    if (this->drawRadar) {
        // Draw a radar circle in the center of the screen
    }

    if (this->drawShipList) {
        //sort EnemyShips by distance to player
        std::vector<Entity> SortedShips; //Closest to farthest
        for (int i = 0; i < EnemyShips.size(); i++) {
            Entity &entity = EnemyShips[i];

            if (!(entity.name == "BP_SmallShipTemplate_C" || entity.name == "BP_SmallShipNetProxy_C" ||
                entity.name == "BP_MediumShipTemplate_C" || entity.name == "BP_MediumShipNetProxy_C"||
                entity.name == "BP_LargeShipTemplate_C" || entity.name == "BP_LargeShipNetProxy_C")) {
                //if not a real ship
                continue;
            }

            int distance = (int)(entity.location.Distance(CameraCache.POV.Location) / 100); //Distance in m
            for (int j = 0; j < SortedShips.size(); j++) {
                if (distance < (int)(SortedShips[j].location.Distance(CameraCache.POV.Location) / 100)) {
                    SortedShips.insert(SortedShips.begin() + j, entity);
                    break;
                }
            }
        }

        int YCursor = 70;
        for (int i = 0; i < SortedShips.size(); i++) {
            Entity &entity = SortedShips[i];
            int distance = (int)(entity.location.Distance(CameraCache.POV.Location) / 100); //Distance in m
            std::string distanceInKmString = " - " + std::to_string(((int)(distance / 100))/10) + "m"; //Distance in km, keeps once decimal

            std::string shipDisplayName = "";
            if (entity.name == "BP_SmallShipTemplate_C" || entity.name == "BP_SmallShipNetProxy_C") shipDisplayName = "Sloop";
            if (entity.name == "BP_MediumShipTemplate_C" || entity.name == "BP_MediumShipNetProxy_C") shipDisplayName = "Brigantine";
            if (entity.name == "BP_LargeShipTemplate_C" || entity.name == "BP_LargeShipNetProxy_C") shipDisplayName = "Galleon";

            if (distance < 1000) {
                //Close - Red
                ctx->draw_text_uncentered(10, YCursor, shipDisplayName + distanceInKmString, COLOR::RED);
            } else if (distance < 2200) {
                //Medium - Yellow
                ctx->draw_text_uncentered(10, YCursor, shipDisplayName + distanceInKmString, COLOR::YELLOW);
            } else {
                //Far - Green
                ctx->draw_text_uncentered(10, YCursor, shipDisplayName + distanceInKmString, COLOR::GREEN);
            }
            YCursor += 20;
        }
    }


    if (this->drawTracers || this->drawEnemiesBox || this->drawEnemiesHealth || this->drawEnemiesNames || this->drawEnemiesBones) {
        for (int i = 0; i < EnemyPlayers.size(); i++) {
            Entity &entity = EnemyPlayers[i];

            Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CameraCache.POV, MonWidth, MonHeight);
            Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CameraCache.POV, MonWidth, MonHeight);
            int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
            int width = (int)(height * 0.5f);

            if (this->drawTracers) {
                Coords tracerEndPoint = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);

                if (coordsOnScreen(tracerEndPoint, MonWidth, MonHeight)) {
                    ctx->draw_line(MonWidth / 2, MonHeight / 2, tracerEndPoint.x, tracerEndPoint.y, 2.0f, COLOR::ORANGE);
                }
            }

            if (this->drawEnemiesBox) {
                ctx->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, COLOR::RED);
            }

            if (this->drawEnemiesHealth) {
                uintptr_t entHealthComp = ReadMemory<uintptr_t>(entity.pawn + Offsets::HealthComponent); //HealthComponent
                int EntityHealth = ReadMemory<float>(entHealthComp + Offsets::Health);
                float healthPercentage = EntityHealth / 100.f;
                int healthHeight = (int)(height * healthPercentage);
                int missingHealthHeight = height - healthHeight;
                int healthBarX = screenCoordsHead.x - (width / 2) - 5;
                ctx->draw_line(healthBarX, screenCoordsHead.y, healthBarX, screenCoordsFeet.y, 3.0f, COLOR::RED);
                ctx->draw_line(healthBarX, screenCoordsFeet.y, healthBarX, screenCoordsFeet.y - healthHeight, 3.0f, COLOR::GREEN);
            }

            if (this->drawEnemiesNames) {
                // Display name above the enemy player
            }

            // Render skeleton if enabled
            if (this->drawEnemiesBones) {
                //RenderSkeleton(ctx, entity.player.meshComponentPtr, CameraCache, MonWidth, MonHeight);
            }
        } //for enemy
    } //if enemy

    if (this->drawTeamBox || this->drawTeamHealth || this->drawTeamNames || this->drawTeamBox) {
        for (int i = 0; i < TeamPlayers.size(); i++) {
            Entity &entity = TeamPlayers[i];

            Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CameraCache.POV, MonWidth, MonHeight);
            Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CameraCache.POV, MonWidth, MonHeight);
            int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
            int width = (int)(height * 0.5f);

            if (this->drawTeamBox) {
                ctx->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, COLOR::BLUE);
            }

            if (this->drawTeamHealth) {
                uintptr_t entHealthComp = ReadMemory<uintptr_t>(entity.pawn + Offsets::HealthComponent); //HealthComponent
                int EntityHealth = ReadMemory<float>(entHealthComp + Offsets::Health);
                float healthPercentage = EntityHealth / 100.f;
                int healthHeight = (int)(height * healthPercentage);
                int missingHealthHeight = height - healthHeight;
                int healthBarX = screenCoordsHead.x - (width / 2) - 5;
                ctx->draw_line(healthBarX, screenCoordsHead.y, healthBarX, screenCoordsFeet.y, 3.0f, COLOR::RED);
                ctx->draw_line(healthBarX, screenCoordsFeet.y, healthBarX, screenCoordsFeet.y - healthHeight, 3.0f, COLOR::GREEN);
            }

            if (this->drawTeamNames) {
                // Display name above the team player
            }

            if (this->drawTeamBones) {
                //RenderSkeleton(ctx, entity.player.meshComponentPtr, CameraCache, MonWidth, MonHeight);
            }
        } //for team
    } //if team

    if (this->drawShip || this->drawShipBox || this->drawHoles || this->drawShipHoleCount || this->drawShipFloodCount || this->drawShipVelocity || this->drawShipAngleArrow || this->drawCloseShipWaterInfo || this->drawSinkInfo) {
        for (int i = 0; i < EnemyShips.size(); i++) {
            Entity &entity = EnemyShips[i];

            Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CameraCache.POV, MonWidth, MonHeight);
            int distance = (int)(CameraCache.POV.Location.Distance(entity.location) / 100.f /*m*/);

            std::string shipDisplayName = "";
            if (entity.name == "BP_SmallShipTemplate_C" || entity.name == "BP_SmallShipNetProxy_C") shipDisplayName = "Sloop";
            if (entity.name == "BP_MediumShipTemplate_C" || entity.name == "BP_MediumShipNetProxy_C") shipDisplayName = "Brigantine";
            if (entity.name == "BP_LargeShipTemplate_C" || entity.name == "BP_LargeShipNetProxy_C") shipDisplayName = "Galleon";
            if (entity.name == "BP_AISmallShipTemplate_C" || entity.name == "BP_AISmallShipNetProxy_C") shipDisplayName = "AI Sloop";
            if (entity.name == "BP_AILargeShipTemplate_C" || entity.name == "BP_AILargeShipNetProxy_C") shipDisplayName = "AI Galleon";
            if (entity.name == "BP_AggressiveGhostShip_C") shipDisplayName = "Ghost Ship";
            if (entity.name == "NetProxy_C") shipDisplayName = "Far Ship";

            if (shipDisplayName == "") continue; // Skip if no ship name is found

            if (this->drawShipBox) {
                // ctx->draw_box(screenCoords.x - 50, screenCoords.y - 20, 100, 40, 2, COLOR::YELLOW);
            }

            if (this->drawShipDist) {
                shipDisplayName += " " + std::to_string(distance) + "m";
            }

            if (distance < 1750) { //wont work at this range anyway
                if (this->drawHoles || this->drawShipHoleCount) {
                    ptr hullDamage = ReadMemory<ptr>(entity.pawn + Offsets::HullDamage);
                    TArray<ptr> DamageZones = ReadMemory<TArray<ptr>>(hullDamage + Offsets::ActiveHullDamageZones);
                    if (this->drawHoles) {
                        for (int j = 0; j < DamageZones.Length(); j++) {
                            uintptr_t DamageZone = ReadMemory<ptr>(DamageZones.GetAddress() + (j * sizeof(uintptr_t)));
                            ptr ShipSceneComponent = ReadMemory<ptr>(DamageZone + Offsets::SceneRootComponent);
                            FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
                            drawX(ctx, WorldToScreen(location, CameraCache.POV, MonWidth, MonHeight), 5, COLOR::GREEN);
                        }
                    }

                    if (this->drawShipHoleCount) {
                        shipDisplayName += " " + std::to_string(DamageZones.Length()) + "H";
                    }
                }

                if (this->drawShipFloodCount) {
                    uintptr_t pShipInternalWaterComponent = ReadMemory<uintptr_t>(entity.pawn + Offsets::ShipInternalWaterComponent);
                    uintptr_t pShipInternalWaterActor = ReadMemory<uintptr_t>(pShipInternalWaterComponent + Offsets::ChildActor);
                    float waterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::WaterAmount);
                    float maxWaterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::InternalWaterParams + Offsets::MaxWaterAmount);

                    if (maxWaterAmount > 0.f) {
                        shipDisplayName += " " + std::to_string((int)(waterAmount / maxWaterAmount * 100)) + "%";
                    }
                }
            }

            if (this->drawShipVelocity || this->drawShipAngleArrow) {
                if (this->drawShipVelocity) {
                    // Display ship velocity
                    // FVector shipVelocity = ReadMemory<FVector>(entity.pawn + Offsets::Velocity); // Assuming you have an offset for ship velocity
                }

                if (this->drawShipAngleArrow) {
                    //angluler velocity arrow
                }
            }

            if (this->drawShip) {
                ctx->draw_text(screenCoords.x, screenCoords.y, shipDisplayName, COLOR::YELLOW);
            }

            if (this->drawCloseShipWaterInfo && distance < 30) {
                // Display water amount and holes
                uintptr_t pShipInternalWaterComponent = ReadMemory<uintptr_t>(entity.pawn + Offsets::ShipInternalWaterComponent);
                uintptr_t pShipInternalWaterActor = ReadMemory<uintptr_t>(pShipInternalWaterComponent + Offsets::ChildActor);
                float waterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::WaterAmount);
                float maxWaterAmount = ReadMemory<float>(pShipInternalWaterActor + Offsets::InternalWaterParams + Offsets::MaxWaterAmount);

                ptr hullDamage = ReadMemory<ptr>(entity.pawn + Offsets::HullDamage);
                TArray<ptr> DamageZones = ReadMemory<TArray<ptr>>(hullDamage + Offsets::ActiveHullDamageZones);

                if (waterAmount / maxWaterAmount * 100 > 1.f || DamageZones.Length() > 0) {
                    ctx->draw_text(MonWidth/2, (MonHeight/2) + 80, std::to_string(DamageZones.Length()) + " " + std::to_string((int)(waterAmount / maxWaterAmount * 100)) + "%", COLOR::MAGENTA);
                }
            }
        } //for ship
    } //if ship

    if (this->drawMerms || this->shipwrecks || this->drawWorldEvents || this->drawRowboats || this->drawStorm ||
        this->drawGoodItems || this->drawGoodLoots || this->drawProjectiles || this->drawAIEnemies ||
        this->drawGoldHoardersLoot || this->drawOrderOfSoulsLoot || this->drawMerchantAllianceLoot || this->drawAthenaLoot || this->drawAllFactionLoot || this->drawMicsLoot) {
        for (int i = 0; i < otherEntities.size(); i++) {
            Entity &entity = otherEntities[i];
            std::string itemName = "";
            std::string distanceString = " - " + std::to_string((int)(CameraCache.POV.Location.Distance(entity.location) / 100.f /*m*/)) + "m";


            itemName = getDisplayName(entity.name, Tables::enemyEntity);
            if (this->drawAIEnemies && !itemName.empty()) {
                Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CameraCache.POV, MonWidth, MonHeight);
                Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CameraCache.POV, MonWidth, MonHeight);
                int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
                int width = (int)(height * 0.5f);
                if (itemName == "Skeleton" || itemName == "Siren" || itemName == "Phantom" || itemName == "Siren Leader" || itemName == "Ashen Lord") {
                    ctx->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, COLOR::GREEN);
                }
                ctx->draw_text(screenCoordsHead.x, screenCoordsHead.y + 10, itemName, COLOR::GREEN);
                continue;
            }
            itemName = getDisplayName(entity.name, Tables::projectiles);
            if (this->drawProjectiles && !itemName.empty()) {
                Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 20.0f}, CameraCache.POV, MonWidth, MonHeight);
                Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 20.0f}, CameraCache.POV, MonWidth, MonHeight);
                int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
                int width = (int)(height * 0.5f);
                ctx->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, COLOR::ORANGE);
                continue;
            }

            COLOR::Color drawColor = COLOR::WHITE;
            if (this->drawMerms && (entity.name == "BP_Mermaid_C" || entity.name == "BP_LootStorage_Retrieve_C")) {
                itemName = "BP_Mermaid_C";
                if (entity.name == "BP_LootStorage_Retrieve_C") itemName = "Loot Mermade"; //Holds loot when in shrines
                drawColor = COLOR::MAGENTA;
                itemName += distanceString;
            } else if (this->drawRowboats && (entity.name == "BP_Rowboat_C" || entity.name == "BP_Rowboat_WithFrontCannon_C" || entity.name == "BP_Rowboat_WithFrontHarpoon_C")) {
                itemName = "Rowboat";
                if (entity.name == "BP_Rowboat_WithFrontCannon_C") itemName += " Cannon";
                if (entity.name == "BP_Rowboat_WithFrontHarpoon_C") itemName += " Harpoon";
                itemName += distanceString;
                drawColor = COLOR::BLUE;
            } else if (this->shipwrecks && (entity.name.find("Shipwreck") != std::string::npos || entity.name.find("Seagulls_Barrels") != std::string::npos)) {
                if (entity.name.find("Circling") != std::string::npos) continue; //if the seagulls, dont highlight (will do it twice then)

                itemName = "Shipwreck/Barrels" + distanceString;
                drawColor = COLOR::TransparentLightPink;
            } else if (this->drawStorm && entity.name == "BP_Storm_C") {
                itemName = "Storm" + distanceString;
                drawColor = COLOR::TransparentLightPink;
            }
             else {
                itemName = getDisplayName(entity.name, Tables::world_events);
                if (this->drawWorldEvents && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightPink;
                    itemName += distanceString;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::usefulItems);
                if (this->drawGoodItems && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightRed;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::goodLoot);
                if (this->drawGoodLoots && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightGold;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::gold_hoarders_loot);
                if (this->drawGoldHoardersLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightGold;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::order_of_souls_loot);
                if (this->drawOrderOfSoulsLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightPurple;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::merchants_loot);
                if (this->drawMerchantAllianceLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightBlue;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::athena_loot);
                if (this->drawAthenaLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightGreen;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::any_faction_loot);
                if (this->drawAllFactionLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightGreen;
                    goto writeName;
                }
                itemName = getDisplayName(entity.name, Tables::misc_items);
                if (this->drawMicsLoot && !itemName.empty()) {
                    drawColor = COLOR::TransparentLightWhite;
                    goto writeName;
                }
                //Not in any list, not a valid entity to draw
                // std::cout << "Unregistared entity: " << entity.name << std::endl;
                continue;
            } // else (not Merms, Wrecks, WorldEvents, Rowboats, Storms)
            writeName: // On a valid name, draw here
            Coords screenCoords = WorldToScreen(entity.location, CameraCache.POV, MonWidth, MonHeight);
            if (coordsOnScreen(screenCoords, MonWidth, MonHeight)) {
                ctx->draw_text(screenCoords.x, screenCoords.y, itemName, drawColor);
            }
        } //for otherEntities
    } // if otherEntities
}
