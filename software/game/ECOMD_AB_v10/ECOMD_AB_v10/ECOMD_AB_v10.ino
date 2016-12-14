/*
 EPIC CRATES OF MASS DESTRUCTION : http://www.team-arg.org/ecomd-manual.html

 Arduboy version 1.0:  http://www.team-arg.org/ecomd-downloads.html

 MADE by TEAM a.r.g. : http://www.team-arg.org/more-about.html

 2015 - DRAGULA96 - JO3RI  (Firepit animation by @JUSTIN_CYR)

 Game License: MIT : https://opensource.org/licenses/MIT

 */

//determine the game
#define GAME_ID 33

#include <SPI.h>
#include <EEPROM.h>
#include "Arglib.h"
#include "menu_bitmap.h"
#include "physics.h"
#include "Player.h"
#include "Crate.h"
#include "Weapons.h"
#include "Enemy.h"
#include "score.h"
#include "inputs.h"
#include "background.h"

//define menu states (on main menu)
#define STATE_MENU_INTRO         0
#define STATE_MENU_MAIN          1
#define STATE_MENU_HELP          2
#define STATE_MENU_PLAY          3
#define STATE_MENU_INFO          4
#define STATE_MENU_SOUNDFX       5

//define game states (on main menu)
#define STATE_GAME_PLAYING       8
#define STATE_GAME_PAUSE         9
#define STATE_GAME_OVER          10


Arduboy arduboy;
SimpleButtons buttons (arduboy);
Sprites sprites(arduboy);

unsigned char gameState = STATE_MENU_MAIN;
boolean soundYesNo;
int menuSelection;
byte counter = 0;

byte walkingFrame;
byte waitingFrame;
int scorePlayer;


void setup()
{
  arduboy.start();
  arduboy.setFrameRate(60);
  if (EEPROM.read(EEPROM_AUDIO_ON_OFF)) soundYesNo = true;
  arduboy.initRandomSeed();
  gameState = STATE_MENU_INTRO;
  menuSelection = STATE_MENU_PLAY;
  Serial.begin(9600);
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  buttons.poll();
  if (soundYesNo == true) arduboy.audio.on();
  else arduboy.audio.off();
  arduboy.clearDisplay();
  switch (gameState)
  {
    case STATE_MENU_INTRO:
      arduboy.drawCompressed(0, 4, TEAMarg2, WHITE);
      counter++;
      if (counter > 40) gameState = STATE_MENU_MAIN;
      break;
    case STATE_MENU_MAIN:
      // show the splash art
      arduboy.drawCompressed(0, 0, title_bitmap, WHITE);
      sprites.drawSelfMasked(20, 56, mainmenu_is_mask, menuSelection - 2);
      if (buttons.justPressed(RIGHT_BUTTON) && (menuSelection < 5))menuSelection++;
      if (buttons.justPressed(LEFT_BUTTON) && (menuSelection > 2))menuSelection--;
      if (buttons.justPressed(A_BUTTON | B_BUTTON)) gameState = menuSelection;
      break;
    case STATE_MENU_HELP: // QR code
      arduboy.drawCompressed(32, 0, qrcode_bitmap, WHITE);
      if (buttons.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
      break;
    case STATE_MENU_INFO: // infoscreen
      arduboy.drawCompressed(20, 0, info_bitmap, WHITE);
      if (buttons.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
      break;
    case STATE_MENU_SOUNDFX: // soundconfig screen
      arduboy.drawCompressed(0, 0, title_bitmap, WHITE);
      sprites.drawSelfMasked(22, 56, soundYesNo_is_mask, soundYesNo);
      if (buttons.justPressed(RIGHT_BUTTON)) soundYesNo = true;
      if (buttons.justPressed(LEFT_BUTTON)) soundYesNo = false;
      if (buttons.justPressed(A_BUTTON | B_BUTTON))
      {
        arduboy.audio.save_on_off();
        gameState = STATE_MENU_MAIN;
      }
      if (soundYesNo == true) arduboy.audio.on();
      else arduboy.audio.off();
      break;
    case STATE_MENU_PLAY:
      setupGame();
      gameState = STATE_GAME_PLAYING;
      break;
    case STATE_GAME_PLAYING:
      checkInputs();
      gravityEffect();
      checkCrateCollisions();
      checkEnemyCollisions();
      updateEnemies();
      updateWeapon();
      
      drawBackDrop();
      drawPlayer();
      drawBullets();
      drawFirePit();
      drawEnemies();
      
      sprites.drawPlusMask(crate.x, crate.y, crate_plus_mask, 0);
      drawSurrounding();
      drawScore(98, 6, SCORE_SMALL_FONT);
      sprites.drawSelfMasked(11, 6, guns, CURRENT_WEAPON);
      if (gameState == STATE_GAME_OVER) delay(500);
      break;
    case STATE_GAME_OVER:
      arduboy.drawCompressed(0, 0, gameover_bitmap, WHITE);
      drawScore(14, 32, SCORE_BIG_FONT);
      if (buttons.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
      break;
    case STATE_GAME_PAUSE:
      arduboy.drawCompressed(0, 0, pause_bitmap, WHITE);
      if (buttons.justPressed(UP_BUTTON)) gameState = STATE_GAME_PLAYING;
      break;
  }
  arduboy.display();
}

void setupGame()
{
  scorePlayer = 0;
  //set starting  weapon type
  CURRENT_WEAPON = DEFAULT_GUN;
  weapon.weaponSet(0); //set to default gun specs
  crate.randomizeCrate();
  player.xSpeed = 1;
  player.ySpeed = 3;
  player.x = 24;
  player.y = 38;

  enemy[0].enemyType = 0; //set to big enemy
  enemy[1].enemyType = 1;
  enemy[2].enemyType = 1; //set to midium size enemy;

  for (byte i = 0; i < 8; i++)
  {
    enemy[i].reset();
    enemyWalkingFrame = 0;
  }

  topHole.width = 16;
  topHole.height = 4;
  topHole.x = 56;
  topHole.y = 0;

  bottomHole.width = 16;
  bottomHole.height = 4;
  bottomHole.x = 56;
  bottomHole.y = 63;

  //platform left
  wall[0].x = 0;
  wall[0].y =  31;
  wall[0].width = 55;
  wall[0].height = 2;

  //platform right
  wall[1].x = 72;
  wall[1].y = 31;
  wall[1].width = 55;
  wall[1].height = 2;

  //platform mid top
  wall[2].x = 40;
  wall[2].y = 15;
  wall[2].width = 47;
  wall[2].height = 2;

  //platform mid bottom
  wall[3].x = 40;
  wall[3].y = 47;
  wall[3].width = 47;
  wall[3].height = 2;

  walkingFrame = 0;
  waitingFrame = 0;
  waitingTimer = 0;
}
