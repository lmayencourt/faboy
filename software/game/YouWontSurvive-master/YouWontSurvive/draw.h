void drawIntro(){
  arduboy.setCursor(25,22);
  arduboy.setTextSize(4);
  arduboy.print("YOU");
  arduboy.display();
  arduboy.tunes.tone(400, 400);
  delay(800);
  arduboy.clear();
  arduboy.setCursor(9,22);
  arduboy.print("WON'T");
  arduboy.display();
  arduboy.tunes.tone(300, 400);
  delay(800);
}

void drawMenu(){
  byte menuDrawOffset = 34;

  arduboy.setCursor(13,52);
  arduboy.setTextSize(1);
  arduboy.print("A: EASY  B: HARD");
  arduboy.setCursor(36,10);
  arduboy.print("You Won't");
  if (arduboy.everyXFrames(2)){
    arduboy.setCursor(20,22);
    arduboy.setTextSize(2);
    arduboy.print("SURVIVE");
  }
}

void drawMenuEffect(){
   if (arduboy.everyXFrames(30)){
    arduboy.setCursor(rand()%(48-1)+1,rand()%(48-1)+1);
    arduboy.setTextSize(2);
    arduboy.print("SURVIVE");
   }
}

void drawPlayer(){
  arduboy.setTextSize(1);
  arduboy.setCursor(5, posY);
  
  switch(emotionState){
    case NORMAL:
        arduboy.print(player);
      break;
    case HAPPY:
        arduboy.print(playerHappy);
      break;
     case SAD:
        arduboy.print(playerSad);
      break;
    case WORRIED:
        arduboy.print(playerWorried);
      break;
  }
}

void drawEnemy(){
  arduboy.setTextSize(1);
  arduboy.setCursor(posXEnemy, posYEnemy);
  arduboy.print(enemy);
}

void drawGameover(){
  arduboy.flipHorizontal(false);  
  arduboy.invert(false);
  invertedControls = false;
  arduboy.setCursor(43, 1);
  arduboy.print("SCORE: ");
  arduboy.print(counter);

  if(counter >= 30){
    arduboy.setCursor(26, 20);
    if (arduboy.everyXFrames(2)){
      arduboy.print("WOW! Good job!");
    }
  } else {
    arduboy.setCursor(27, 20);
    if (arduboy.everyXFrames(2)){
     arduboy.print("I told you...");
    }
  }
  arduboy.setCursor(11,52);
  arduboy.print("A: AGAIN  B: MENU");
}

void drawScore(){
  arduboy.setCursor(117, 1);
  arduboy.print(counter);
}

void spiceUp(){
  rnd = rand()%(20);

  if(effectDuration == 5){
    arduboy.tunes.tone(333, 400);
    arduboy.flipHorizontal(false);  
    arduboy.invert(false);
    invertedControls = false;
    emotionState = HAPPY;
  }
  if(effectDuration == 6){
        emotionState = NORMAL;
  }
  if(rnd == 9 && effectDuration > 5){
    arduboy.tunes.tone(1000, 400);
    arduboy.flipHorizontal(true);
    effectDuration = 0;
    emotionState = SAD;
  }
  if(rnd == 10 && effectDuration > 5){
    arduboy.tunes.tone(1000, 400);
    arduboy.invert(true);
    effectDuration = 0;
    emotionState = WORRIED;
  }
}
