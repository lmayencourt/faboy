#ifndef GAME_H
#define GAME_H

#include "arduboy.h"

class Game
{
public:
    Game();

    virtual void setup() = 0;
    virtual void loop() = 0;

protected:
    Arduboy arduboy;
};

#endif // GAME_H
