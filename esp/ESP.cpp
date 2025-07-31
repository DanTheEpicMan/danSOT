#include "ESP.h"

#include <algorithm>

void ESP::Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> TeamPlayers,
              std::vector<Entity> EnemyShips, std::vector<Entity> otherEntities, DrawingContext *ctx) {

    ShipsHolesPos.clear();
    CannonBalls.clear();
    //Should move to contructor
    this->ScrWidth = MonWidth, this->ScrHeight = MonHeight;
    this->draw = ctx;

    // std::cout << "ESP: Start" << std::endl;
    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);
    // std::cout << "ESP: CameraManager" << std::endl;
    DrawCrosshair(10/*radius*/, ctx, COLOR::ORANGE);
    // std::cout << "ESP: Drew Crosshair" << std::endl;
    DrawEnemies(EnemyPlayers, CameraCache.POV, COLOR::ORANGE, COLOR::RED);
    // std::cout << "ESP: Drew Enemies" << std::endl;
    DrawTeam(TeamPlayers, CameraCache.POV, COLOR::BLUE);
    // std::cout << "ESP: Drew Team" << std::endl;
    DrawShip(EnemyShips, otherEntities, CameraCache.POV, COLOR::YELLOW);
    // std::cout << "ESP: Drew Ship" << std::endl;



    if (this->drawMerms || this->shipwrecks || this->drawWorldEvents || this->drawRowboats || this->drawStorm ||
        this->drawGoodItems || this->drawGoodLoots || this->drawProjectiles || this->drawAIEnemies ||
        this->drawGoldHoardersLoot || this->drawOrderOfSoulsLoot || this->drawMerchantAllianceLoot || this->drawAthenaLoot || this->drawAllFactionLoot || this->drawMicsLoot) {
        this->CannonBalls.clear();
        for (int i = 0; i < otherEntities.size(); i++) {
            Entity &entity = otherEntities[i];
            std::string itemName = "";
            std::string distanceString = " - " + std::to_string((int)(CameraCache.POV.Location.Distance(entity.location) / 100.f /*m*/)) + "m";
            if (entity.location.x == 0) continue;

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
            // itemName = getDisplayName(entity.name, Tables::projectiles);
            if (this->drawProjectiles && entity.name.find("_Projectile_") != std::string::npos) {
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
                itemName = "Mermaid";
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
    // std::cout << "ESP: Other entities esp" << std::endl;

    if (this->drawRadar) {
        ctx->draw_line(radarX, radarY - radarRadius, radarX, radarY + radarRadius, 1.0f, COLOR::TransparentLightWhite);
        ctx->draw_line(radarX - radarRadius, radarY, radarX + radarRadius, radarY, 1.0f, COLOR::TransparentLightWhite);

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
            if (cannonBallPos.z < 100) continue; //skip when below water
            plot_on_radar(cannonBallPos, COLOR::ORANGE, 7.f);
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
    // std::cout << "ESP: Draw Radar" << std::endl;

    if (this->drawLocalRadar) {
        float cursorY = 150.f;
        float radX = 50.f;
        const float localRadarSize = 200.f;
        const float localRadarWorldRadius = 5000.f; // 50m in-game units
        const float textHeight = 15.f;
        const float radarPadding = 35.f;

        // Make a mutable copy to sort ships by distance
        std::vector<Entity> sortedShips = EnemyShips;
        std::sort(sortedShips.begin(), sortedShips.end(), [&](const Entity& a, const Entity& b) {
            return CameraCache.POV.Location.Distance(a.location) < CameraCache.POV.Location.Distance(b.location);
        });


        auto plot_on_local_radar = [&](const FVector& entityPos, const FVector& shipCenterPos, float radarX, float radarY, COLOR::Color color, float dotSize) {
            FVector relativePos = entityPos - shipCenterPos;

            if (abs(relativePos.x) > localRadarWorldRadius || abs(relativePos.y) > localRadarWorldRadius) {
                return;
            }

            float scale = localRadarSize / (localRadarWorldRadius * 2.f);
            float radarPointX = radarX + (localRadarSize / 2.f) + (relativePos.y * scale);
            float radarPointY = radarY + (localRadarSize / 2.f) - (relativePos.x * scale);

            ctx->draw_line(radarPointX - (dotSize / 2.f), radarPointY, radarPointX + (dotSize / 2.f), radarPointY, dotSize, color);
        };

        // --- Iterate through the sorted list of ships ---
        for (const auto& ship : sortedShips) {
            // Stop drawing if we run out of screen space
            if (cursorY > MonHeight - 50.f) break;

            FVector shipPos = ship.location;
            float distance = CameraCache.POV.Location.Distance(shipPos);
            std::string shipDisplayName = GetShipBaseName(ship.name);
            std::string displayText = shipDisplayName + " " + std::to_string((int)(distance / 100.f)) + "m";

            if (distance < 150000.f) {

                draw->draw_text_uncentered(radX, cursorY, displayText, COLOR::WHITE);

                float radarTopY = cursorY + textHeight;

                if (radarTopY > MonHeight - (localRadarSize + 20.f)) break;

                ctx->draw_box(radX, radarTopY, localRadarSize, localRadarSize, 1.f, COLOR::TransparentLightWhite);
                ctx->draw_line(radX + localRadarSize / 2.f - 5.f, radarTopY + localRadarSize / 2.f, radX + localRadarSize / 2.f + 5.f, radarTopY + localRadarSize / 2.f, 1.f, COLOR::TransparentLightWhite);
                ctx->draw_line(radX + localRadarSize / 2.f, radarTopY + localRadarSize / 2.f - 5.f, radX + localRadarSize / 2.f, radarTopY + localRadarSize / 2.f + 5.f, 1.f, COLOR::TransparentLightWhite);

                for (const auto& shipHole: this->ShipsHolesPos) {
                    plot_on_local_radar(shipHole, shipPos, radX, radarTopY, COLOR::GREEN, 4.f);
                }
                for (const auto& player : EnemyPlayers) {
                    plot_on_local_radar(player.location, shipPos, radX, radarTopY, COLOR::RED, 5.f);
                }
                for (const auto& player : TeamPlayers) {
                    plot_on_local_radar(player.location, shipPos, radX, radarTopY, COLOR::CYAN, 5.f);
                }
                for (const auto& cannonBall: this->CannonBalls) {
                    if (cannonBall.z < 100) continue;
                    plot_on_local_radar(cannonBall, shipPos, radX, radarTopY, COLOR::ORANGE, 7.f);
                }

                cursorY = radarTopY + localRadarSize + radarPadding;

            } else {
                draw->draw_text_uncentered(radX, cursorY, displayText, COLOR::WHITE);
                cursorY += textHeight;
            }//else
        }//for ship
    }// if drawLocalRadar
    // std::cout << "ESP: Local Radar" << std::endl;
}

void ESP::DrawPredictedShipMovement(const FVector& currentPos, FVector& linearVel, const FVector& angularVel, float predictionSeconds, const COLOR::Color pathColor, DrawingContext* ctx, const FMinimalViewInfo& CameraInfo, int MonWidth, int MonHeight)
{
    // return;
    if (predictionSeconds <= 0.f || !ctx) return;
    linearVel.z = 0;

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
            if (p1.x == 0) continue;
            if (p2.x == 0) continue;

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

void ESP::DrawCrosshair(int radius, DrawingContext *ctx, COLOR::Color color) {
    if (!this->drawCrosshair) return;
    ctx->draw_line(ScrWidth/2 - radius, ScrHeight/2, ScrWidth/2 + radius, ScrHeight/2, 2, color);
    ctx->draw_line(ScrWidth/2, ScrHeight/2 - radius, ScrWidth/2, ScrHeight/2 + radius, 2, color);
}

void ESP::DrawEnemies(std::vector<Entity> EnemyPlayers, FMinimalViewInfo CamPOV, COLOR::Color tracerColor, COLOR::Color enemyBoxColor) {
    if (!(this->drawTracers || this->drawEnemiesBox || this->drawEnemiesHealth || this->drawEnemiesNames || this->drawEnemiesBones)) return;
    for (int i = 0; i < EnemyPlayers.size(); i++) {
        Entity &entity = EnemyPlayers[i];

        Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CamPOV, ScrWidth, ScrHeight);
        Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CamPOV, ScrWidth, ScrHeight);
        int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
        int width = (int)(height * 0.5f);

        if (this->drawTracers) { //Need a way to accurately get snapline

        }

        if (this->drawEnemiesBox) {
            draw->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, enemyBoxColor);
        }

        if (this->drawEnemiesHealth) {
            uintptr_t entHealthComp = ReadMemory<uintptr_t>(entity.pawn + Offsets::HealthComponent); //HealthComponent
            int EntityHealth = ReadMemory<float>(entHealthComp + Offsets::Health);
            float healthPercentage = EntityHealth / 100.f;
            int healthHeight = (int)(height * healthPercentage);
            int missingHealthHeight = height - healthHeight;
            int healthBarX = screenCoordsHead.x - (width / 2) - 5;
            draw->draw_line(healthBarX, screenCoordsHead.y, healthBarX, screenCoordsFeet.y, 5.0f, COLOR::RED);
            draw->draw_line(healthBarX, screenCoordsFeet.y, healthBarX, screenCoordsFeet.y - healthHeight, 5.0f, COLOR::GREEN);
        }

        if (this->drawEnemiesNames) {
            // Display name above the enemy player
        }

        // Render skeleton if enabled
        if (this->drawEnemiesBones) {
            //RenderSkeleton(ctx, entity.player.meshComponentPtr, CameraCache, MonWidth, MonHeight);
        }
    }
}

void ESP::DrawTeam(std::vector<Entity> TeamPlayers, FMinimalViewInfo CamPOV, COLOR::Color teamBoxColor) {
    if (!(this->drawTeamBox || this->drawTeamHealth || this->drawTeamNames || this->drawTeamBox)) return;
    for (int i = 0; i < TeamPlayers.size(); i++) {
        Entity &entity = TeamPlayers[i];

        Coords screenCoordsFeet = WorldToScreen({entity.location.x, entity.location.y, entity.location.z - 100.0f}, CamPOV, ScrWidth, ScrHeight);
        Coords screenCoordsHead = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 100.0f}, CamPOV, ScrWidth, ScrHeight);
        int height = (int)(screenCoordsFeet.y - screenCoordsHead.y);
        int width = (int)(height * 0.5f);

        if (this->drawTeamBox) {
            draw->draw_box(screenCoordsHead.x - width / 2, screenCoordsHead.y, width, height, 2, teamBoxColor);
        }

        if (this->drawTeamHealth) {
            uintptr_t entHealthComp = ReadMemory<uintptr_t>(entity.pawn + Offsets::HealthComponent); //HealthComponent
            int EntityHealth = ReadMemory<float>(entHealthComp + Offsets::Health);
            float healthPercentage = EntityHealth / 100.f;
            int healthHeight = (int)(height * healthPercentage);
            int missingHealthHeight = height - healthHeight;
            int healthBarX = screenCoordsHead.x - (width / 2) - 5;
            draw->draw_line(healthBarX, screenCoordsHead.y, healthBarX, screenCoordsFeet.y, 3.0f, COLOR::RED);
            draw->draw_line(healthBarX, screenCoordsFeet.y, healthBarX, screenCoordsFeet.y - healthHeight, 3.0f, COLOR::GREEN);
        }

        if (this->drawTeamNames) {
            // Display name above the team player
        }

        if (this->drawTeamBones) {
            //RenderSkeleton(ctx, entity.player.meshComponentPtr, CameraCache, MonWidth, MonHeight);
        }
    } //for team
}

std::string ESP::GetShipBaseName(std::string gameName) {
    std::string shipDisplayName = "";
    if (gameName == "BP_SmallShipTemplate_C" || gameName == "BP_SmallShipNetProxy_C") shipDisplayName = "Sloop";
    if (gameName == "BP_MediumShipTemplate_C" || gameName == "BP_MediumShipNetProxy_C") shipDisplayName = "Brigantine";
    if (gameName == "BP_LargeShipTemplate_C" || gameName == "BP_LargeShipNetProxy_C") shipDisplayName = "Galleon";
    if (gameName == "BP_AISmallShipTemplate_C" || gameName == "BP_AISmallShipNetProxy_C") shipDisplayName = "AI Sloop";
    if (gameName == "BP_AILargeShipTemplate_C" || gameName == "BP_AILargeShipNetProxy_C") shipDisplayName = "AI Galleon";
    if (gameName == "BP_AggressiveGhostShip_C") shipDisplayName = "Ghost Ship";
    if (gameName == "NetProxy_C") shipDisplayName = "Far Ship";
    if (shipDisplayName == "") shipDisplayName = "Burning Blade"; //this should be replaced with real code that checks for burning blade specifically
    return shipDisplayName;
}

std::string ESP::DrawHolesAndUpdateName(uintptr_t shipAddr, FMinimalViewInfo CamPOV, std::string shipName, std::vector<Entity> otherObj) {
    ptr hullDamage = ReadMemory<ptr>(shipAddr + Offsets::HullDamage);
    TArray<ptr> DamageZones = ReadMemory<TArray<ptr>>(hullDamage + Offsets::ActiveHullDamageZones);
    if (this->drawHoles) {
        int DZLength = DamageZones.Length();
        if (DZLength > 50) return shipName;
        std::cout << "Length Active: " << DZLength << std::endl;
        for (int j = 0; j < DZLength; j++) {
            uintptr_t DamageZone = ReadMemory<ptr>(DamageZones.GetAddress() + (j * sizeof(uintptr_t)));
            ptr ShipSceneComponent = ReadMemory<ptr>(DamageZone + Offsets::SceneRootComponent);
            FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
            drawX(draw, WorldToScreen(location, CamPOV, MonWidth, MonHeight), 5, COLOR::GREEN);
        }

        TArray<ptr> DamageZonesAllHoles = ReadMemory<TArray<ptr>>(hullDamage + Offsets::DamageZones);
        int DZLengthAll = DamageZonesAllHoles.Length();
        std::cout << "Length Inactive: " << DZLengthAll << std::endl;
        if (DZLengthAll > 50 || DZLength < 0) return shipName;
        for (int j = 0; j < DZLengthAll; j++) {
            uintptr_t DamageZone = ReadMemory<ptr>(DamageZonesAllHoles.GetAddress() + (j * sizeof(uintptr_t)));
            ptr ShipSceneComponent = ReadMemory<ptr>(DamageZone + Offsets::SceneRootComponent);
            FVector location = ReadMemory<FVector>(ShipSceneComponent + Offsets::ActorCoordinates);
            ShipsHolesPos.push_back(location);
        }
    }

    if (this->drawShipHoleCount) {
        shipName += " " + std::to_string(DamageZones.Length()) + "H";
    }

    return shipName;
}




void ESP::DrawShip(std::vector<Entity> ShipList, std::vector<Entity> OtherEntities, FMinimalViewInfo CamPOV, COLOR::Color ShipDisplayInfo) {
    if (!(this->drawShip || this->drawShipBox || this->drawHoles || this->drawShipHoleCount || this->drawShipFloodCount || this->drawShipVelocity || this->drawShipAngleArrow || this->drawCloseShipWaterInfo || this->drawSinkInfo)) return;
    for (int i = 0; i < ShipList.size(); i++) {
        Entity &entity = ShipList[i];

        Coords screenCoords = WorldToScreen({entity.location.x, entity.location.y, entity.location.z + 200.f}, CamPOV, ScrWidth, ScrHeight);
        int distance = (int)(CamPOV.Location.Distance(entity.location) / 100.f /*m*/);

        std::string shipDisplayName = GetShipBaseName(entity.name);

        if (this->drawShipBox) {
            // ctx->draw_box(screenCoords.x - 50, screenCoords.y - 20, 100, 40, 2, COLOR::YELLOW);
        }

        if (this->drawShipDist) {
            shipDisplayName += " " + std::to_string(distance) + "m";
        }
        if (distance < 1500 * 100) {
            std::cout << "ESP:DRAWSHIP: Start" << std::endl;
            shipDisplayName = DrawHolesAndUpdateName(entity.pawn, CamPOV, shipDisplayName, OtherEntities);
            std::cout << "ESP:DRAWSHIP: End func" << std::endl;
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

        if (this->drawShipMovement) {
            ptr movementProxyComponent = ReadMemory<ptr>(entity.pawn + Offsets::ShipMovementProxyComponent);
            ptr movementProxyActor = ReadMemory<ptr>(movementProxyComponent + Offsets::ChildActor);
            ptr RepMovement_Address = movementProxyActor + Offsets::ShipMovement + Offsets::ReplicatedShipMovement_Movement;
            FVector ShipLinearVel = ReadMemory<FVector>(RepMovement_Address + Offsets::LinearVelocity);
            FVector ShipAngularVel = ReadMemory<FVector>(RepMovement_Address + Offsets::AngularVelocity);
            DrawPredictedShipMovement(entity.location, ShipLinearVel, ShipAngularVel, 30.f /*sec of traj*/,
                COLOR::WHITE, draw, CamPOV, MonWidth, MonHeight);
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
            draw->draw_text(screenCoords.x, screenCoords.y, shipDisplayName, ShipDisplayInfo);
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
                draw->draw_text(MonWidth/2, (MonHeight/2) + 80, std::to_string(DamageZones.Length()) + " " + std::to_string((int)(waterAmount / maxWaterAmount * 100)) + "%", COLOR::MAGENTA);
            }
        }
    }
}
