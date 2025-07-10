#include <vector>
#include "../utils/GameData.h"
#include "../utils/LocalData.h"
#include "overlay/drawing.h"

#ifndef ESP_H
#define ESP_H
#include "../config.h"
#include "../offsets.h"
#include "../memory/Memory.h"
#include "../overlay/drawing.h"
#include "../utils/LocalFunctions.h"
#include "tables.h"


class ESP {
public:
    void Run(uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> EnemyPlayers, std::vector<Entity> TeamPlayers,
              std::vector<Entity> EnemyShips, std::vector<Entity> otherEntities, DrawingContext *ctx);

private:
    bool drawCrosshair = true;
    bool drawRadar = true;
    bool drawShipList = true;
    bool drawSinkInfo = true;


    bool drawTracers = true;
    bool drawEnemiesBox = true;
    bool drawEnemiesHealth = true;
    bool drawEnemiesNames = true;
    bool drawEnemiesBones = true;

    bool drawTeamBox = false;
    bool drawTeamHealth = false;
    bool drawTeamNames = false;
    bool drawTeamBones = false;

    bool drawShip = true;
    bool drawShipDist = true;
    bool drawShipBox = false;
    bool drawHoles = true;
    bool drawShipHoleCount = true;
    bool drawShipFloodCount = true;
    bool drawShipVelocity = true;
    bool drawShipAngleArrow = true;
    bool drawCloseShipWaterInfo = true;

    bool drawMerms = true;
    bool shipwrecks = true;
    bool drawWorldEvents = true;
    bool drawRowboats = true;
    bool drawStorm = true;

    bool drawGoodItems = true;
    bool drawGoodLoots = true;
    bool drawProjectiles = true;
    bool drawAIEnemies = true;

    bool drawGoldHoardersLoot = false;
    bool drawOrderOfSoulsLoot = false;
    bool drawMerchantAllianceLoot = false;
    bool drawAthenaLoot = false;
    bool drawAllFactionLoot = false;
    bool drawMicsLoot = false;

};



#endif //ESP_H
