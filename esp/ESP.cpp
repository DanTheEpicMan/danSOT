#include "ESP.h"

void DrawPredictedShipMovement(const FVector& currentPos, const FVector& linearVel, const FVector& angularVel, float predictionSeconds, const COLOR::Color pathColor, DrawingContext* ctx, const FMinimalViewInfo& CameraInfo, int MonWidth, int MonHeight)
{
    if (predictionSeconds <= 0.f || !ctx) return;

    const int numSteps = 20; // Use a fixed number of segments for the path
    const float timeStep = predictionSeconds / numSteps;
    FVector lastPathPoint = currentPos;

    const float w_deg = angularVel.z;

    // Use linear prediction if the target isn't turning significantly or is moving too slow.
    if (std::abs(w_deg) < 1.0f || FVector(linearVel.x, linearVel.y, 0.0f).Size() < 50.f)
    {
        // --- Linear Prediction Path ---
        for (int i = 1; i <= numSteps; ++i)
        {
            float t = i * timeStep;
            FVector currentPathPoint = currentPos + (linearVel * t);

            Coords p1 = WorldToScreen(lastPathPoint, CameraInfo, MonWidth, MonHeight);
            Coords p2 = WorldToScreen(currentPathPoint, CameraInfo, MonWidth, MonHeight);

            // Only draw if one of the points is valid/on-screen
            if ((p1.x > 1 && p1.y > 1 && p1.x < MonWidth && p1.y < MonHeight) || (p2.x > 1 && p2.y > 1 && p2.x < MonWidth && p2.y < MonHeight)) {
                ctx->draw_line(p1.x, p1.y, p2.x, p2.y, 2.0f, pathColor);
            }
            lastPathPoint = currentPathPoint;
        }
    }
    else
    {
        // --- Rotational Prediction Path ---
        const float w_rad = w_deg * (M_PI / 180.f);
        if (std::abs(w_rad) < 1e-6) return; // Avoid division by zero
        const float r_turn = FVector(linearVel.x, linearVel.y, 0.0f).Size() / std::abs(w_rad);

        // Calculate the center of the turning circle
        FVector vel_dir_2d = FVector(linearVel.x, linearVel.y, 0.f).unit();
        FVector to_center_dir = (w_rad > 0) ? FVector(-vel_dir_2d.y, vel_dir_2d.x, 0.f) : FVector(vel_dir_2d.y, -vel_dir_2d.x, 0.f);
        FVector turn_center = currentPos + (to_center_dir * r_turn);

        // Calculate the initial angle of the ship relative to the turn center
        float initial_angle = atan2f(currentPos.y - turn_center.y, currentPos.x - turn_center.x);

        for (int i = 1; i <= numSteps; ++i)
        {
            float t = i * timeStep;
            float current_angle = initial_angle + (w_rad * t);

            FVector currentPathPoint = turn_center + FVector(
                r_turn * cosf(current_angle),
                r_turn * sinf(current_angle),
                currentPos.z + (linearVel.z * t) // Also account for vertical movement (e.g., waves)
            );

            Coords p1 = WorldToScreen(lastPathPoint, CameraInfo, MonWidth, MonHeight);
            Coords p2 = WorldToScreen(currentPathPoint, CameraInfo, MonWidth, MonHeight);

            if ((p1.x > 1 && p1.y > 1 && p1.x < MonWidth && p1.y < MonHeight) || (p2.x > 1 && p2.y > 1 && p2.x < MonWidth && p2.y < MonHeight)) {
                ctx->draw_line(p1.x, p1.y, p2.x, p2.y, 2.0f, pathColor);
            }
            lastPathPoint = currentPathPoint;
        }
    }
}

void ESP::Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> TeamPlayers,
              std::vector<Entity> EnemyShips, std::vector<Entity> otherEntities, DrawingContext *ctx) {

    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);

    if (this->drawCrosshair) {
        ctx->draw_line(MonWidth/2 - 10, MonHeight/2, MonWidth/2 + 10, MonHeight/2, 2, COLOR::ORANGE);
        ctx->draw_line(MonWidth/2, MonHeight/2 - 10, MonWidth/2, MonHeight/2 + 10, 2, COLOR::ORANGE);
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
        this->ShipsHolesPos.clear();
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
                            this->ShipsHolesPos.push_back(location);
                            drawX(ctx, WorldToScreen(location, CameraCache.POV, MonWidth, MonHeight), 5, COLOR::GREEN);
                        }

                        TArray<ptr> DamageZonesAllHoles = ReadMemory<TArray<ptr>>(hullDamage + Offsets::DamageZones);
                        for (int j = 0; j < DamageZonesAllHoles.Length(); j++) {
                            uintptr_t DamageZone = ReadMemory<ptr>(DamageZonesAllHoles.GetAddress() + (j * sizeof(uintptr_t)));
                            ptr ShipSceneComponent = ReadMemory<ptr>(DamageZone + Offsets::SceneRootComponent);
                            FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
                            ShipsHolesPos.push_back(location);
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

                if (this->drawShipMovement) {
                    ptr movementProxyComponent = ReadMemory<ptr>(entity.pawn + Offsets::ShipMovementProxyComponent);
                    ptr movementProxyActor = ReadMemory<ptr>(movementProxyComponent + Offsets::ChildActor);
                    ptr RepMovement_Address = movementProxyActor + Offsets::ShipMovement + Offsets::ReplicatedShipMovement_Movement;
                    FVector ShipLinearVel = ReadMemory<FVector>(RepMovement_Address + Offsets::LinearVelocity);
                    FVector ShipAngularVel = ReadMemory<FVector>(RepMovement_Address + Offsets::AngularVelocity);
                    DrawPredictedShipMovement(entity.location, ShipLinearVel, ShipAngularVel, 30.f /*sec of traj*/,
                        COLOR::WHITE, ctx, CameraCache.POV, MonWidth, MonHeight);
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
        this->CannonBalls.clear();
        for (int i = 0; i < otherEntities.size(); i++) {
            Entity &entity = otherEntities[i];
            std::string itemName = "";
            std::string distanceString = " - " + std::to_string((int)(CameraCache.POV.Location.Distance(entity.location) / 100.f /*m*/)) + "m";
            if (entity.location.z == 0) continue;

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
                // if (itemName.find("ball") != std::string::npos)
                this->CannonBalls.push_back(entity.location);

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

    if (this->drawRadar) {
        const float radarX = MonWidth / 2.f;
        const float radarY = 170.f;
        const float radarRadius = 150.f;

        ctx->draw_line(radarX, radarY - radarRadius, radarX, radarY + radarRadius, 1.0f, COLOR::TransparentLightWhite);
        ctx->draw_line(radarX - radarRadius, radarY, radarX + radarRadius, radarY, 1.0f, COLOR::TransparentLightWhite);
        ctx->draw_text(radarX, radarY - radarRadius - 15, "N", COLOR::WHITE);

        const float playerYawRad = CameraCache.POV.Rotation.y * (M_PI / 180.f);
        const float cosYaw = cosf(playerYawRad);
        const float sinYaw = sinf(playerYawRad);

        // Define the lambda HERE, inside the function scope.
        auto plot_on_radar = [&](const FVector& entityPos, COLOR::Color color, float dotSize) {
            FVector relativePos = entityPos - CameraCache.POV.Location;
            float localX = relativePos.x * cosYaw + relativePos.y * sinYaw;
            float localY = relativePos.x * -sinYaw + relativePos.y * cosYaw;

            float entityDist = sqrtf(localX * localX + localY * localY) / 100.f;
            if (entityDist > this->radarScale) return;

            float scale = radarRadius / this->radarScale;
            float radarPointX = radarX + (localY / 100.f) * scale;
            float radarPointY = radarY - (localX / 100.f) * scale;

            FVector radarVec = {radarPointX - radarX, radarPointY - radarY, 0};
            if (radarVec.Size() > radarRadius) {
                radarVec = radarVec.unit() * radarRadius;
            }

            radarPointX -= dotSize/2;
            radarPointY -= dotSize/2;

            ctx->draw_line(radarPointX, radarPointY, radarPointX + dotSize, radarPointY, dotSize, color);

        };

        for (const auto& cannonBallPos: CannonBalls) {
            if (cannonBallPos.z < -100) continue; //skip when below water
            plot_on_radar(cannonBallPos, COLOR::ORANGE, 4.f);
        }

        for (const auto& shipsHole: ShipsHolesPos) {
            plot_on_radar(shipsHole, COLOR::BLUE, 3.f);
        }

        for (const auto& ship : EnemyShips) {
            plot_on_radar(ship.location, COLOR::BLUE, 3.f);
        }
        for (const auto& player : EnemyPlayers) {
            plot_on_radar(player.location, COLOR::RED, 4.f);
        }
        for (const auto& player : TeamPlayers) {
            plot_on_radar(player.location, COLOR::CYAN, 4.f);
        }



    }
    //Relative radar (shows every ship, not adjusted for player rotation, just north being on top)
    //Loop through every ship thats in range and on the list (if the local player is within 30 m, put that one on top)
    //Loop through and make a grid on the left, every grid id 100px by 100px, witch represents a 50m radius centered on the ship.
    //Draw all holes within 30m of the ship.
    //Draw the players on the ship (within 50m)
    //Draw cannonballs near the ship (within 50m, dont draw when below 0 z)
    if (this->drawLocalRadar) {
            float cursorY = 150.f;
            float radX = 50.f;
            const float localRadarSize = 100.f;
            const float localRadarWorldRadius = 5000.f; // 50m in-game units

            // A helper lambda to plot a world position onto a specific ship's local radar
            auto plot_on_local_radar = [&](const FVector& entityPos, const FVector& shipCenterPos, COLOR::Color color, float dotSize) {
                // Calculate the entity's position relative to the ship
                FVector relativePos = entityPos - shipCenterPos;

                // Check if the entity is within the radar's 50m square area
                if (abs(relativePos.x) > localRadarWorldRadius || abs(relativePos.y) > localRadarWorldRadius) {
                    return; // Not on this ship's radar
                }

                // Calculate the scale to map world coordinates (50m radius) to screen coordinates (100px size)
                // We use localRadarWorldRadius * 2 because the total width/height of the world area is 100m.
                float scale = localRadarSize / (localRadarWorldRadius * 2.f);

                // Convert relative world coordinates to radar screen coordinates.
                // We treat the ship's forward direction (+X) as "Up" on the radar.
                // We map relative.y to the radar's X-axis and relative.x to the radar's Y-axis.
                float radarPointX = radX + (localRadarSize / 2.f) + (relativePos.y * scale);
                float radarPointY = cursorY + (localRadarSize / 2.f) - (relativePos.x * scale); // Subtract because screen Y is top-to-bottom

                // Draw the entity as a small square dot
                ctx->draw_line(radarPointX - (dotSize / 2.f), radarPointY, radarPointX + (dotSize / 2.f), radarPointY, dotSize, color);
            };

            for (const auto& ship : EnemyShips) {
                // Stop drawing radars if we run out of screen space
                if (cursorY > MonHeight - (localRadarSize + 20.f)) break;

                FVector shipPos = ship.location;

                // --- Draw the radar grid for this ship ---
                ctx->draw_box(radX, cursorY, localRadarSize, localRadarSize, 1.f, COLOR::TransparentLightWhite);
                // Draw a small cross in the center to represent the ship's mast/center point
                ctx->draw_line(radX + localRadarSize / 2.f - 5.f, cursorY + localRadarSize / 2.f, radX + localRadarSize / 2.f + 5.f, cursorY + localRadarSize / 2.f, 1.f, COLOR::TransparentLightWhite);
                ctx->draw_line(radX + localRadarSize / 2.f, cursorY + localRadarSize / 2.f - 5.f, radX + localRadarSize / 2.f, cursorY + localRadarSize / 2.f + 5.f, 1.f, COLOR::TransparentLightWhite);

                // --- Plot entities on this ship's radar ---

                // Plot Ship Holes (from the global list of all holes)
                for (const auto& shipHole: this->ShipsHolesPos) {
                    plot_on_local_radar(shipHole, shipPos, COLOR::GREEN, 4.f);
                }

                // Plot Enemy Players
                for (const auto& player : EnemyPlayers) {
                    plot_on_local_radar(player.location, shipPos, COLOR::RED, 5.f);
                }

                // Plot Team Players
                for (const auto& player : TeamPlayers) {
                    plot_on_local_radar(player.location, shipPos, COLOR::CYAN, 5.f);
                }

                // Plot Cannonballs
                for (const auto& cannonBall: this->CannonBalls) {
                    if (cannonBall.z < -100) continue; // Skip if it's deep underwater
                    plot_on_local_radar(cannonBall, shipPos, COLOR::ORANGE, 3.f);
                }

                // Move the Y cursor down for the next radar, with 20px padding
                cursorY += localRadarSize + 20.f;
            }
        }


}
