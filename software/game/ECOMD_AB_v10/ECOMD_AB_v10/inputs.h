#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>

//define game states (on main menu)
#define STATE_GAME_PLAYING              8
#define STATE_GAME_PAUSE                9
#define STATE_GAME_OVER                 10

#define STAGE_LEFT                      0
#define STAGE_RIGHT                     1
#define STAGE_TOP                       2
#define STAGE_BOTTOM                    3


extern Arduboy arduboy;
extern SimpleButtons buttons;
extern unsigned char gameState;
extern boolean isGrounded;
extern byte jumpTimer;

byte waitingTimer;


void checkInputs()
{
  if (buttons.justPressed(UP_BUTTON)) gameState = STATE_GAME_PAUSE;

  if (buttons.pressed(RIGHT_BUTTON))
  {
    playerFacing = PLAYER_FACING_RIGHT;
    waitingTimer = 0;
    waitingFrame = 0;
    if (arduboy.everyXFrames(6))
    {
      walkingFrame++;
      if (walkingFrame > 3) walkingFrame = 0;
    }
    for (byte i  = 0; i < player.xSpeed; i++)
    {
      if (!checkCollisions()) player.x ++;
    }
  }

  if (buttons.pressed(LEFT_BUTTON))
  {
    playerFacing = PLAYER_FACING_LEFT;
    waitingTimer = 0;
    waitingFrame = 0;
    if (arduboy.everyXFrames(6))
    {
      walkingFrame++;
      if (walkingFrame > 3) walkingFrame = 0;
    }
    for (byte i  = 0; i < player.xSpeed; i++)
    {
      if (!checkCollisions()) player.x --;
    }
  }

  if (buttons.notPressed(LEFT_BUTTON && RIGHT_BUTTON))
  {
    if (arduboy.everyXFrames(60)) waitingTimer++;
    if (waitingTimer > 5)
    {
      playerFacing = PLAYER_FACING_FRONT;
      waitingTimer = 6;
      if (arduboy.everyXFrames(15))
      {
        waitingFrame++;
        if (waitingFrame > 35) waitingFrame = 0;
      }
    }

  }

  if (buttons.pressed(A_BUTTON))
  {
    waitingTimer = 0;             //resetwaiting timer becauause you are shooting
    shoot();                      //BANG! BANG!
  }

  if (buttons.justPressed(B_BUTTON) && isGrounded)
  {
    if (player.y > 4) player.y --;
    jumpTimer = 6;
    isGrounded = false;
    arduboy.tunes.tone(60, 100);
  }
}

#endif
