void playTune(){
  if(introTunePlayed == false){
     arduboy.tunes.tone(200, 400);
     introTunePlayed = true;
  }
}

void selectMode(){
  if(modeSelected == false){
  rnd = rand()%(10);
  
  if(rnd == 0){
    arduboy.flipHorizontal(true);
  }
  if(rnd == 1){
    arduboy.invert(true);
  }
  if(rnd >= 2){
  }
  modeSelected = true; 
  }
}

void menuHandler(){
  if(arduboy.pressed(A_BUTTON) == true) {
    gameState = EASY;
  }
  if(arduboy.pressed(B_BUTTON) == true) {
    gameState = HARD;
  }
  previousGameState = gameState;
}

void updateScore(){
   counter++;
   effectDuration++;
   if(gameState == HARD){
     spiceUp();
   }
}

void newSpeed(){
  if(gameState == EASY){
    enemySpeed = rand()%(1)+2.6;
  } else {
    enemySpeed = rand()%(1)+3.1;
  }
  arduboy.tunes.tone(enemySound, 20);
}

//Player movement
void updatePlayer(){
  if(posY < HEIGHT - 12){
if( arduboy.pressed(DOWN_BUTTON) == true && invertedControls == false) {
    posY = posY+0.5;
    playerRec = playerRec+0.5;
}
if( arduboy.pressed(DOWN_BUTTON) == true && invertedControls == true) {
    posY = posY-0.5;
    playerRec = playerRec-0.5;
}
  }
  if(posY > 0)
  {
if( arduboy.pressed(UP_BUTTON) == true && invertedControls == false) {
    posY = posY-0.5;
     playerRec = playerRec-0.5;
}
if( arduboy.pressed(DOWN_BUTTON) == true && invertedControls == true) {
    posY = posY+0.5;
    playerRec = playerRec+0.5;
}
  }
  if(posY-7 <= posYEnemy && playerRec >= posYEnemy && posXEnemy <= 5){
    arduboy.tunes.tone(200, 160);
    posXEnemy = rand()%(170-140) + 140;
    gameState = GAMEOVER;
  }
}

//Enemey movement
void updateEnemy(){
  if(posXEnemy < -20){
    checkPosY = rand()%(50)+1;
    posXEnemy = rand()%(170-140) + 140;

    while(checkPosY == previousPosYEnemy){
      checkPosY = rand()%(50)+1;
    }
    posYEnemy = checkPosY;
    previousPosYEnemy = checkPosY;
    updateScore();
    newSpeed();
  }
  posXEnemy = posXEnemy-enemySpeed;
}

void gameoverHandler(){
 if(arduboy.pressed(A_BUTTON) == true) {
        arduboy.tunes.tone(600, 40);
  delay(160);
  resetVariables();
    gameState = previousGameState;
}
 if(arduboy.pressed(B_BUTTON) == true) {
    delay(222);
    gameState = MENU;
}
}

