/*
Variables, character visualization and game modes
*/

#define MENU 0
#define EASY 1
#define HARD 2
#define GAMEOVER 3

#define NORMAL 0
#define HAPPY 1
#define SAD 2
#define WORRIED 3

#define LED_ON LOW
#define LED_OFF HIGH

Arduboy arduboy;

byte gameState = MENU;
byte previousGameState;
byte emotionState = NORMAL;

int counter;
int enemySound;
int rnd;
int effectDuration;
int ledDuration;

float posY;
float playerRec;
float posXEnemy;
float posYEnemy;
float previousPosXEnemy;
float previousPosYEnemy;
float checkPosY;
float enemySpeed;

char enemyArray[] = {'<', 'H', 'I', '{','+','#','O'};
char player[] = ":]";
char playerHappy[] = ":D";
char playerWorried[] = ":O";
char playerSad[] = ":(";
char enemy;

boolean invertedControls;
boolean introTunePlayed = false;
boolean modeSelected = false;

void resetVariables(){
  counter = 0;
  effectDuration = 5;
  ledDuration = 0;
  enemySound = rand()%(1000)+100;

  emotionState = NORMAL;

  posY = 20;
  playerRec = 25;
  posXEnemy = 130;
  posYEnemy = rand()%(50-1)+1;
  enemySpeed = 3;

  enemy = enemyArray[rand()%(sizeof(enemyArray))];

  invertedControls = false;
  modeSelected = false;

  arduboy.flipHorizontal(false);  
  arduboy.invert(false);
}

