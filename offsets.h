#pragma once

#include <cstdint>
#define ptr uintptr_t

namespace Offsets {
    ptr UWorld = 0x8972940; //Or GWorld, same thing if your updating
    ptr GameState = 0x40; //Read forom UWorld
    ptr PlayerArray = 0x3C8; //Read from GameState


}
