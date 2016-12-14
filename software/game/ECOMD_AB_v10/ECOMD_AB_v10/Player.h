#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include "physics.h"


//define player facing states (in game)
#define PLAYER_FACING_RIGHT       0
#define PLAYER_FACING_LEFT        1
#define PLAYER_FACING_FRONT       2

#define STAGE_LEFT                      0
#define STAGE_RIGHT                     1
#define STAGE_TOP                       2
#define STAGE_BOTTOM                    3


extern Sprites sprites;
extern Physics physics;
extern byte walkingFrame;
extern byte waitingFrame;

byte playerFacing = PLAYER_FACING_RIGHT;
boolean isGrounded = false;
byte jumpTimer = 0;
const byte gravityDelay = 4;
byte gravityCounter = gravityDelay;

PROGMEM const byte playerWalks[] = { 1, 2, 3, 2, 4, 5, 6, 5,};
PROGMEM const byte playerWaiting[] = {0, 7, 0, 7, 0, 0, 0, 0, 0, 8, 9, 8, 0, 0, 0, 0, 1, 1, 1, 1, 0, 4, 4, 4, 4, 0, 0, 0, 7, 0, 7, 0, 0, 0, 0, 0};

PROGMEM const unsigned char player_plus_mask[] = {
  // width, height
  8, 8,
  // frame 0
  0x00, 0x00, 0x00, 0x7E, 0x62, 0xFF, 0xAB, 0xFF, 0x63, 0xFF,
  0xAB, 0xFF, 0x62, 0xFF, 0x00, 0x7E,
  // frame 1
  0x38, 0x7E, 0x7E, 0xFF, 0xA3, 0xFF, 0x6B, 0xFF, 0x63, 0xFF,
  0xEA, 0xFF, 0x30, 0xFE, 0x10, 0x38,
  // frame 2
  0x38, 0x7E, 0x7E, 0xFF, 0x63, 0xFF, 0xAB, 0xFF, 0xE3, 0xFF,
  0x6A, 0xFF, 0x30, 0xFE, 0x10, 0x38,
  // frame 3
  0x38, 0x7E, 0x7E, 0xFF, 0xE3, 0xFF, 0x6B, 0xFF, 0x23, 0xFF,
  0xEA, 0xFF, 0x30, 0xFE, 0x10, 0x38,
  // frame 4
  0x10, 0x38, 0x30, 0xFE, 0xEA, 0xFF, 0x63, 0xFF, 0x6B, 0xFF,
  0xA3, 0xFF, 0x7E, 0xFF, 0x38, 0x7E,
  // frame 5
  0x10, 0x38, 0x30, 0x7E, 0x6A, 0xFF, 0xE3, 0xFF, 0xAB, 0xFF,
  0x63, 0xFF, 0x7E, 0xFF, 0x38, 0x7E,
  // frame 6
  0x10, 0x38, 0x30, 0xFE, 0xEA, 0xFF, 0x23, 0xFF, 0x6B, 0xFF,
  0xE3, 0xFF, 0x7E, 0xFF, 0x38, 0x7E,
  // frame 7
  0x00, 0x00, 0x00, 0x7E, 0x62, 0xFF, 0xA3, 0xFF, 0x63, 0xFF,
  0xA3, 0xFF, 0x62, 0xFF, 0x00, 0x7E,
  // frame 8
  0x00, 0x00, 0x00, 0x7E, 0x42, 0xFF, 0xEB, 0xFF, 0x63, 0xFF,
  0xEB, 0xFF, 0x42, 0xFF, 0x00, 0x7E,
  // frame 9
  0x00, 0x00, 0x10, 0x7E, 0x62, 0xFF, 0xEB, 0xFF, 0x63, 0xFF,
  0xEB, 0xFF, 0x62, 0xFF, 0x10, 0x7E,
};




class GameObject {
  public:
    byte x, y;
    int8_t xSpeed, ySpeed;
};

GameObject player;



boolean checkCollisions()
{
  playerRect.x = player.x;
  playerRect.y = player.y - 1 ;
  playerRect.width = 8;
  playerRect.height = 8;
  for (byte i = 0; i < 4; i++)
  {
    if (physics.collide(playerRect, wall[i])) return true;
  }
  return false;
}


boolean checkIfGrounded()
{
  playerRect.x = player.x + 1;
  playerRect.y = player.y + 1 ;
  playerRect.width = 6;
  playerRect.height = 8;
  for (byte i = 0; i < 4; i++)
  {
    if (physics.collide(playerRect, wall[i]))
    {
      isGrounded = true;
      return true;
    }
    if (player.y + 8 > 62)
    {
      isGrounded = true;
      player.y = 63 - 8;
      return true;
    }
  }
  return false;
}


void gravityEffect()
{
  if (jumpTimer > 0)
  {
    jumpTimer--;
    for (byte i = 0; i < 5; i++)
    {
      if (!checkCollisions() && (player.y > 2))
        player.y --;
    }
  }

  gravityCounter--;
  if (gravityCounter < 2)
  {
    gravityCounter = gravityDelay;
    for (byte i = 0; i < 3; i++)
    {
      if (!checkIfGrounded())player.y++;
    }
  }
}


void drawPlayer()
{
  if (player.x < 8) player.x = 8;
  if (player.x > 112) player.x = 112;
  switch (playerFacing)
  {
    case PLAYER_FACING_FRONT:
      sprites.drawPlusMask(player.x, player.y, player_plus_mask, pgm_read_byte(&playerWaiting[waitingFrame]));
      break;
    default:
      sprites.drawPlusMask(player.x, player.y, player_plus_mask, pgm_read_byte(&playerWalks[walkingFrame + (4 * playerFacing)]));
      break;
  }
}


#endif
