#include <string>

#include "GameData.h"

#ifndef LOCALDATA_H
#define LOCALDATA_H

struct Entity {
    uintptr_t pawn;
    std::string name;
    FVector location;
};

typedef struct {
    int x, y = -1;
} Coords;;


#endif //LOCALDATA_H
