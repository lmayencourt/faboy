void setupLEDs(){
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
}

void setLEDs(uint8_t red, uint8_t green, uint8_t blue) {
  digitalWrite(RED_LED, red);
  digitalWrite(GREEN_LED, green);
  digitalWrite(BLUE_LED, blue);
}

void startLEDs(){
  if(counter >= 30){
    if(ledDuration <= 20){
      setLEDs(LED_ON, LED_OFF, LED_OFF);
    }
    if(ledDuration >= 21 && ledDuration <=40){
      setLEDs(LED_OFF, LED_ON, LED_OFF);
    }
    if(ledDuration >= 41 && ledDuration <=60){
      setLEDs(LED_OFF, LED_OFF, LED_ON);
    }
    if(ledDuration >= 61 && ledDuration <=80){
      setLEDs(LED_ON, LED_ON, LED_OFF);
    }
    if(ledDuration >= 81 && ledDuration <=100){
      setLEDs(LED_ON, LED_OFF, LED_ON);
    }
    if(ledDuration >= 101 && ledDuration <=120){
      setLEDs(LED_OFF, LED_ON, LED_ON);
    }
    ledDuration++;
    if(ledDuration >= 121){
      ledDuration = 0;
    }
  }
}
void stopLEDs(){
  setLEDs(LED_OFF, LED_OFF, LED_OFF);
}

