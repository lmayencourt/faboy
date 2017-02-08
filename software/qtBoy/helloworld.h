#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include "game.h"

class HelloWorld : public Game
{
public:
    HelloWorld();

    void setup();
    void loop();
};

#endif // HELLOWORLD_H
