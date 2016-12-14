/*
YOU WON'T SURVIVE - A simple arcade game for Arduboy!
Made by Andre Hellwig 2016-10-26
Facebook: https://facebook.com/hellwig404
Twitter: https://twitter.com/Andre_Hellwig
Website: https://andrehellwig.de
More games: http://gamejolt.com/@LoCoMaMa

Game License: MIT : https://opensource.org/licenses/MIT
*/

#include <Arduboy.h>
#include "conf.h"
#include "draw.h"
#include "update.h"
#include "led.h"

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.display();
  arduboy.clear();
  drawIntro();
  setupLEDs();
}


void updateState(){
  switch(gameState){
    case MENU:
      stopLEDs();
      menuHandler();
      resetVariables();
      break;
    case EASY:
      stopLEDs();
      selectMode();
      updatePlayer();
      updateEnemy();
      break;
     case HARD:
      stopLEDs();
      updatePlayer();
      updateEnemy();
      break;
    case GAMEOVER:
      gameoverHandler();
      startLEDs();
      break;
  } 
}

void drawState(){
  arduboy.clear();
  switch(gameState){
    case MENU:
      drawMenu();
      drawMenuEffect();
      playTune();
      break;
    case EASY:
      drawPlayer();
      drawEnemy();
      drawScore();
      break;
     case HARD:
      drawPlayer();
      drawEnemy();
      drawScore();
      break;
    case GAMEOVER:
      drawGameover();
      break;
  }
  arduboy.display(); 
}


void loop() {
  if (!(arduboy.nextFrame())) return;
  updateState();
  drawState();
}
