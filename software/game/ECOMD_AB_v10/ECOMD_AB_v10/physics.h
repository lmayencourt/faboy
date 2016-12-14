#ifndef PHYSICS_H
#define PHYSICS_H

#include <Arduino.h>

struct Rect
{
public:
    int x;
    int y;
    uint8_t width;
    uint8_t height;
};

Rect topHole;
Rect bottomHole;
Rect wall[4];
Rect playerRect;


class Physics
{
  public:
    bool static collide(Rect rect, Rect rect2);
};


bool Physics::collide(Rect rect1, Rect rect2)
{
  return !( rect2.x                 >=  rect1.x+rect1.width    ||
            rect2.x + rect2.width   <=  rect1.x                ||
            rect2.y                 >=  rect1.y + rect1.height ||
            rect2.y + rect2.height  <=  rect1.y);
}


#endif
