//Used for passing data between threads, does not contain structs from the game
#ifndef LOCALSTRUCTS_H
#define LOCALSTRUCTS_H

typedef struct {

} Player;

typedef struct {

} Ship;

typedef struct {

} Island;

typedef struct {
    Overlay ov; // Overlay instance for drawing
} CheatData;

extern CheatData GlobalCheatData;



#endif //LOCALSTRUCTS_H
