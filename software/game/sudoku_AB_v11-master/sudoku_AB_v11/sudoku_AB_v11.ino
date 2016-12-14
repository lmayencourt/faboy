/* Neil Pierson - June 11, 2016
 *
 * sudoku_AB_v11.ino (started May 22, 2016)
 * 
 * Modified for Arbuboy
 * [solver functions from sudoku.c -- Neil Pierson - September 10, 2005]
 *
 * Solve Sudoku Puzzles
 *  http://www.sudoku.com/
 *
 * Complete a 9x9 grid so that every row, column and 3x3 box contains
 * the numbers 1 through 9 (no repeats).
 *
 *
 * 06/04/2016 v10
 * - menus and game play added
 *     New Game, Hint, Notes, Solve
 * 06/11/2016 v11
 * - implement Save & Restore menu items
 * - improved ScanPuzzle() to catch invalid full puzzles
 * - added custom puzzle manual entry
 * - added gameLogo() -- display SudokuLogo and falling puzzle tiles
 * 
 */

#include <EEPROM.h>
#include "Arduboy.h"
#include "bitmaps.h"
#include "puzzles.h"

#define EEPROM_SZ    1024 // EEPROM: max 1024 bytes on the ATmega328 (Arduino UNO & Leonardo)
                          // Note: Arduboy reserves the first 16 bytes (EEPROM_STORAGE_SPACE_START)

const char title[10] PROGMEM = "sudoku_AB";     // title length matches puzzle.id[10]
const char rev       PROGMEM = 0x11;            // (use title & rev to validate EEPROM read)
                                                // if the puzzletype structure changes, change the rev
                                                
#define MSGBUFFER_SZ 10       // large enough for copying PROGMEM strings (title, memuNN)
char msgBuffer[MSGBUFFER_SZ];

//#define DEBUG     // used in developing Sudoku Solver Functions, not enough sketch space to turn on anymore

#define FRAMERATE 15

#define GRIDSIZE  9 // 9x9 grid
#define BOXSIZE   3 // 3x3 box, square root of grid size

struct puzzletype {     // 15+81+108+6+8+1 = 219 bytes
  char id[10];          // "sudoku_AB"
  byte rev;             // 0x11  (use id & rev to validate EEPROM read)
  char player[4];       // player initials, null terminated
  
  byte spot[GRIDSIZE*GRIDSIZE];        // lower nibble: 9x9 grid of spots, 0's for empty spot
                                       // upper nibble: number of choices for blank spots (F = locked)
  long choiceset[GRIDSIZE*GRIDSIZE/3]; // set of possible numbers, 32 bit mask, 27 bits cover 3 spots
                                       // save memory, 108 bytes vs 729 bytes (9x9x9)
  int singles;          // track stats in solving the puzzle
  int unique;
  int prunes;

  byte num;            // loaded puzzle number from puzzle_input[] or custom (0xFF)
  byte notes;
  unsigned long timer; // msec, pauses when in menu mode
  int empty;
  
  byte checksum;       // checksum to validate EEPROM read
};

int SolvePuzzle(struct puzzletype *thispuzzle, boolean hint);
int ScanPuzzle(struct puzzletype *check, boolean hint);
int FindSingles(struct puzzletype *check, boolean hint);
int FindUniqueChoice(struct puzzletype *check, boolean hint);
int PruneChoices(struct puzzletype *check);
int GuessFromChoiceset(struct puzzletype *thispuzzle);
void ShowPuzzle(struct puzzletype show);
void ShowPuzzleStruct(struct puzzletype show, boolean allspots);

unsigned long guesscount = 0;

struct puzzletype puzzle;

// menus and game play
const char menu00[] PROGMEM = "New Game";
const char menu01[] PROGMEM = "Hint";
const char menu02[] PROGMEM = "Notes";
const char menu03[] PROGMEM = "Save";
const char menu04[] PROGMEM = "Restore";
const char menu05[] PROGMEM = "Solve";

const char *menu[] = { menu00, menu01, menu02, menu03, menu04, menu05 };


#define M_NEW      0  // menu indexes
#define M_HINT     1
#define M_NOTES    2
#define M_SAVE     3
#define M_RESTORE  4
#define M_SOLVE    5
#define M_MENU     6  // game play modes
#define M_CURSOR   7
#define M_SELECT   8
#define M_TRANS1   9
#define M_TRANS2  10

#define NOTES_OFF  0
#define NOTES_1    1
#define NOTES_ALL  2

#define ENTRIES    4  // max number of EEPROM stored puzzles (1024 - 16) / 219 = 4
#define NAMELEN    3  // three initials

int row = 0;
byte manual = 0;      // custom puzzle manual entry mode { 0 = off, M_CURSOR, M_SELECT }

byte save_entry = 0;
char save_name[ENTRIES][NAMELEN+1]; // three initials, null terminated
byte save_ltr;                      // save/restore menu save_name letter position (0..2)

int tileX   = 0;  // cursor box location (0..GRIDSIZE-1)
int tileY   = 0;
int x_off   = 32; // x offset for puzzle display

byte mode        = M_CURSOR;
byte menu_item   = 0;
byte select_spot = 0;

unsigned long frameCount   = 0;
unsigned long tpuzzle      = 0;

// make an instance of arduboy used for many functions
Arduboy arduboy;


// helper functions for Arduboy buttons
#define HOLDFRAMES 5
uint8_t buttonState[HOLDFRAMES];
uint8_t buttonIndex = 0;

void buttonCheck() { // call once per frame
  buttonIndex = (buttonIndex+1) % HOLDFRAMES;
  buttonState[buttonIndex] = arduboy.buttonsState(); // circular buffer of recent button states
}

boolean buttonJustPressed(uint8_t buttons) {
  uint8_t prevIndex = (buttonIndex > 0) ? buttonIndex-1 : HOLDFRAMES-1;
  return (!(buttonState[prevIndex] & buttons) && (buttonState[buttonIndex] & buttons));
}

boolean buttonHolding(uint8_t buttons) {
  uint8_t i,hold = 0xFF;
  for (i=0; i<HOLDFRAMES; i++) hold &= buttonState[i];
  return (hold & buttons);
}


int freeRAM() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
} // freeRAM()


void loadPuzzle(byte p_num) { // load puzzle from PROGMEM (puzzles.h) & scan puzzle
  byte i,j;
  
  Serial.print(F("Puzzle #"));
  Serial.println(p_num);
    
  for (j=0; j<GRIDSIZE; j++) {
    long rowEntry = pgm_read_dword(&puzzle_input[(int)p_num*GRIDSIZE+j]);
    Serial.println(rowEntry);
    for (i=0; i<GRIDSIZE; i++) {
      if (rowEntry % 10)
           puzzle.spot[j*GRIDSIZE+(GRIDSIZE-1-i)] = 0xF0 | (rowEntry % 10); // numchoices: upper nibble (locked spot)
      else puzzle.spot[j*GRIDSIZE+(GRIDSIZE-1-i)] = 0;
      rowEntry /= 10;
    }
  }

  tileX = tileY = GRIDSIZE/2;

  // scan puzzle, populate numchoices & choiceset
  puzzle.empty = ScanPuzzle(&puzzle);

} // loadPuzzle()


int savePuzzle(byte entry, struct puzzletype *sPtr) {    // write puzzle structure to EEPROM
  byte value;
  byte *ptr;

  int count = sizeof(struct puzzletype);
  int addr;   // EEPROM address

  // EEPROM_SZ: max 1024 bytes on the ATmega328 (Arduino UNO & Leonardo)
  // Note: Arduboy reserves the first 16 bytes (EEPROM_STORAGE_SPACE_START)
  
  //Serial.print(F("savePuzzle: "));
  //Serial.print(entry);
  //Serial.print(',');
  //Serial.print(count);
  //Serial.println(F(" bytes "));

  // byte entry is unsigned don't need to check for < 0
  if (entry >=  (EEPROM_SZ - EEPROM_STORAGE_SPACE_START) / count ) return(-1); // error, entry out of rannge
  
  // save to EEPROM
  sPtr->checksum = 0;
  ptr = (byte *)sPtr;
  addr = EEPROM_STORAGE_SPACE_START + (int)entry * count;

  do {
    value = *ptr++;
    EEPROM.write(addr++,value);
    sPtr->checksum += value;
  } while (--count > 1); // all but last byte of structure (checksum)
  
  sPtr->checksum = ~sPtr->checksum + 1; // 2's complement
  EEPROM.write(addr,sPtr->checksum);

  //Serial.println(sPtr->checksum,HEX);
  
  return(entry);
} // savePuzzle()


int restorePuzzle(byte entry, struct puzzletype *sPtr) {  // read puzzle structure from EEPROM
  byte sum;
  byte value;
  byte *ptr;

  int count = sizeof(struct puzzletype);
  int addr;   // EEPROM address

  // EEPROM_SZ: max 1024 bytes on the ATmega328 (Arduino UNO & Leonardo)
  // Note: Arduboy reserves the first 16 bytes (EEPROM_STORAGE_SPACE_START)
  
  //Serial.print(F("restorePuzzle: "));
  //Serial.print(entry);
  //Serial.print(',');
  //Serial.print(count);
  //Serial.println(F(" bytes "));

  // byte entry is unsigned don't need to check for < 0  
  if (entry >=  (EEPROM_SZ - EEPROM_STORAGE_SPACE_START) / count ) return(-1); // error, entry out of rannge
  
  sum = 0;
  ptr = (byte *)sPtr;
  addr = EEPROM_STORAGE_SPACE_START + (int)entry * count;
  //if (addr%16 != 0) {
  //  byte n = addr%16;
  //  if (addr < 0x100) Serial.print('0');
  //  if (addr < 0x010) Serial.print('0');
  //  Serial.print(addr,HEX);
  //  Serial.print(':');
  //  while (n--) Serial.print(F("   "));
  //}
  
  do {
    //if (addr%16 == 0) {
    //  Serial.println();
    //  if (addr < 0x100) Serial.print('0');
    //  if (addr < 0x010) Serial.print('0');
    //  Serial.print(addr,HEX);
    //  Serial.print(F(": "));
    //} else Serial.print(' ');
 
    value = EEPROM.read(addr++);
    *ptr++ = value;
    sum += value;   // sum each byte, last byte is a 2's complement, will sum to 0x00

    //if (value < 0x10) Serial.print('0');
    //Serial.print(value,HEX);

  } while (--count > 0);
  //Serial.println();

  strcpy_P(msgBuffer,title);
  if (strcmp(sPtr->id,msgBuffer) != 0 || sPtr->rev != pgm_read_byte(&rev) || sum) return(-1); // invalid EEPROM entry

  return(entry);
} // restorePuzzle()


void gameLogo() { // display SudokuLogo and falling puzzle tiles
  byte i,j;
  int k;
  long rowEntry;

  for (k=5*GRIDSIZE-1; k >= 0; k--) { // falling puzzle tiles (use 2*5 = 10 puzzles, 5*9 = 45 iterations, ~1.5 sec)
    arduboy.clear();

    for (j=0; j<GRIDSIZE; j++) {
      rowEntry = (k+j > GRIDSIZE) ? pgm_read_dword(&puzzle_input[k+j]) : 0;
      for (i=0; i<GRIDSIZE; i++) {
        arduboy.drawBitmap(1+i*TILE_SZ,j*TILE_SZ,num_bitmaps[rowEntry % 10],TILE_SZ,TILE_SZ,WHITE);
        rowEntry /= 10;
      }
      rowEntry = (k+j > GRIDSIZE) ? pgm_read_dword(&puzzle_input[k+j+(5*GRIDSIZE)]) : 0;
      for (i=0; i<GRIDSIZE; i++) {
        arduboy.drawBitmap(64+i*TILE_SZ,j*TILE_SZ,num_bitmaps[rowEntry % 10],TILE_SZ,TILE_SZ,WHITE);
        rowEntry /= 10;
      }
    }    
    
    arduboy.drawBitmap(21,16,sudokuLogo,LOGO_W,LOGO_H,WHITE);
    arduboy.display();
    delay(13); // tuned for ~30 fps
  }
    
} // gameLogo()


// ***********************************************************************************************************
// *
// *                            Power Up Init.
// *
// *
// ***********************************************************************************************************

void setup() {
  byte i;
  
  // initiate arduboy instance
  arduboy.beginNoLogo();        // beginNoLogo() saves 350 bytes of program space

  // here we set the framerate to 15, we do not need to run at
  // default 60 and it saves us battery life
  arduboy.setFrameRate(FRAMERATE);

  // initialize button tracking variables for buttonCheck(), buttonJustPressed() & buttonHolding()
  for (i=0; i<HOLDFRAMES; i++) buttonState[i] = 0; 
  buttonIndex = 0;
    
  Serial.begin(115200);
  Serial.print(F("\n== sudoku_AB_v11.ino [by Neil Pierson, 11 Jun 2016]\n"));

  //tpuzzle = millis();
  
  gameLogo(); // display SudokuLogo and falling puzzle tiles, ~1.5 sec
  
  //Serial.print(millis() - tpuzzle);
  //Serial.println(F(" msec"));
  
  // restore puzzle structures from EEPROM to populate save_name[] strings (unused entries can fail)
  for (i=0; i<ENTRIES; i++) {
    if (restorePuzzle(i,&puzzle) < 0) strcpy(save_name[i],"   ");
    else strcpy(save_name[i],puzzle.player);
  }
  
  
  // initialize puzzle
  strcpy_P(puzzle.id,title);
  puzzle.rev = pgm_read_byte(&rev);
  strcpy(puzzle.player,"   ");
  puzzle.notes = NOTES_OFF;
  
  puzzle.num = 0;
  loadPuzzle(puzzle.num); // load puzzle from PROGMEM (puzzles.h) & scan puzzle

  delay(2000);
  
  puzzle.timer = 0;
  tpuzzle = millis();
} // setup()


// ***********************************************************************************************************
// *
// *                            Main Loop 
// *
// *
// ***********************************************************************************************************

void loop() {
  int i,j;
  boolean tileMotion = false;
  
  // pause render until it's time for the next frame
  if (!(arduboy.nextFrame()))
    return;

  frameCount++;
  buttonCheck(); // call once per frame

 
  // clear screen to black
  arduboy.clear();

  switch (mode) {
    case M_CURSOR:
      // if the right button is pressed move 1 tile to the right every frame
      if ((buttonJustPressed(RIGHT_BUTTON) || buttonHolding(RIGHT_BUTTON)) && (tileX < GRIDSIZE-1)) {
        tileX++;
        tileMotion = true;
      }

      // if the left button is pressed move 1 tile to the left every frame
      if ((buttonJustPressed(LEFT_BUTTON) || buttonHolding(LEFT_BUTTON)) && (tileX > 0)) {
        tileX--;
        tileMotion = true;
      }

      // if the up button is pressed move 1 tile up every frame
      if ((buttonJustPressed(UP_BUTTON) || buttonHolding(UP_BUTTON)) && (tileY > 0)) {
        tileY--;
        tileMotion = true;
      }

      // if the down button is pressed move 1 tile down every frame
      if ((buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) && (tileY < GRIDSIZE-1)) {
        tileY++;
        tileMotion = true;
      }

      if (buttonJustPressed(A_BUTTON)) mode = M_TRANS1; // transition to menu 
      if (buttonJustPressed(B_BUTTON)) { 
        select_spot = puzzle.spot[tileY*GRIDSIZE+tileX]; 
        if ((select_spot & 0xF0) != 0xF0) mode = M_SELECT; // only enter select mode if spot is not locked
      }

      break;
      
    case M_SELECT:
      // select_spot locked if upper nibble = 0xF
      
      if (buttonJustPressed(RIGHT_BUTTON)) puzzle.notes |=  NOTES_1;
      if (buttonJustPressed( LEFT_BUTTON)) puzzle.notes &= ~NOTES_1;
      
      if ((buttonJustPressed(UP_BUTTON)   || buttonHolding(UP_BUTTON))   && (select_spot & 0x0F) < GRIDSIZE) select_spot++;
      if ((buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) && (select_spot & 0x0F) > 0)        select_spot--;
      
      if (buttonJustPressed(A_BUTTON)) {
        mode = M_CURSOR; // back: don't commit number selection
        puzzle.notes &= ~NOTES_1;
      }
      
      if (buttonJustPressed(B_BUTTON)) { 
        mode = M_CURSOR; 
        puzzle.notes &= ~NOTES_1;
        puzzle.spot[tileY*GRIDSIZE+tileX] = select_spot;
        
        // scan puzzle, populate numchoices & choiceset
        puzzle.empty = ScanPuzzle(&puzzle);
      }

      break;

    case M_MENU:
      // menu display
      for (j=0; j<M_MENU; j++) {
        arduboy.setCursor(3,j*10 + 2);
        if (j == M_SOLVE && puzzle.empty < 0)       arduboy.print(F("invalid"));
        else if (j == M_SOLVE && puzzle.empty == 0) arduboy.print(F("DONE"));
        else arduboy.print(strcpy_P(msgBuffer,menu[j]));
        
        if (j == M_NOTES) {
          if (puzzle.notes == NOTES_ALL) arduboy.print(F(" on"));
          else                           arduboy.print(F(" off"));
        }
      }

      arduboy.drawRoundRect(0,menu_item*10,60,11,3,WHITE); // 60 = 3 + 9 characters * 6 + 3

      if (buttonJustPressed(LEFT_BUTTON)) { gameLogo(); delay(1000); }
      
      if (buttonJustPressed(UP_BUTTON) || buttonHolding(UP_BUTTON)) 
        menu_item = (menu_item > 0) ? menu_item-1 : 0;
        
      if (buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) 
        menu_item = (menu_item < M_MENU-1) ? menu_item+1: M_MENU-1;
        
      if (buttonJustPressed(A_BUTTON)) mode = M_TRANS2; // transition back to game
      if (buttonJustPressed(B_BUTTON)) {
        if (menu_item == M_NEW) {
          if (puzzle.num == PZ_CUSTOM) {
            for (byte r=0; r<GRIDSIZE; r++)
              for (byte c=0; c<GRIDSIZE; c++) 
                if ((puzzle.spot[r*GRIDSIZE+c] & 0xF0) != 0xF0) puzzle.spot[r*GRIDSIZE+c] = 0; // clear unlocked spots

            // scan puzzle, populate numchoices & choiceset
            puzzle.empty = ScanPuzzle(&puzzle);
              
            manual = M_SELECT; // start in manual entry select mode
          } else manual = 0;
        } else if (menu_item == M_SAVE) {
          save_ltr = NAMELEN; // flag, start with picking save entry
          
          if (strcmp(puzzle.player,"   ") == 0) // blank puzzle player
            for (j=0; j<ENTRIES; j++) 
              if (strcmp(save_name[j],"   ") == 0) { save_entry = j; break; } // point to first blank save_name entry
              
        }
        
        mode = menu_item;
      }

      break;

    case M_NEW:
      { // new game menu
        byte old_num = puzzle.num;
        
        arduboy.setCursor(3,2);
        arduboy.print(strcpy_P(msgBuffer,menu[M_NEW]));

        if (puzzle.num == PZ_CUSTOM) {
          
          if (buttonJustPressed(RIGHT_BUTTON) || buttonHolding(RIGHT_BUTTON))
            if (tileX < GRIDSIZE-1) {
              tileX++; 
              tileMotion = true; 
            } else { // roll down one row (special scroll for manual entry, can stay in M_SELECT mode)
              tileX = 0;
              tileY = (tileY+1) % GRIDSIZE;
              tileMotion = true;
            }

          if (buttonJustPressed(LEFT_BUTTON) || buttonHolding(LEFT_BUTTON))
            if (tileX > 0) {
              tileX--;
              tileMotion = true;
            } else { // roll up one row (special scroll for manual entry, can stay in M_SELECT mode)
              tileX = GRIDSIZE-1;
              tileY = (tileY == 0) ? GRIDSIZE-1 : tileY-1;
              tileMotion = true;
            }
            
        }

        if (buttonJustPressed(UP_BUTTON) || buttonHolding(UP_BUTTON)) {
          if (!manual) {
            if (puzzle.num == 0)              puzzle.num = PZ_CUSTOM;
            else if (puzzle.num == PZ_CUSTOM) puzzle.num = PUZZLES-1; 
            else puzzle.num = (puzzle.num > 0) ? puzzle.num-1 : 0;
          } else {
            if (manual == M_CURSOR) {
              if (tileY > 0) { tileY--; tileMotion = true; }
            } else {                                                    // manual == M_SELECT
              puzzle.spot[tileY*GRIDSIZE+tileX] &= 0x0F;                // unlock
              if (puzzle.spot[tileY*GRIDSIZE+tileX] < GRIDSIZE) 
                puzzle.spot[tileY*GRIDSIZE+tileX]++;                    // increment spot, don't lock yet
              
              // scan puzzle, populate numchoices & choiceset
              puzzle.empty = ScanPuzzle(&puzzle);
            }
          }
        }
        
        if (buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) {
          if (!manual) {
            if (puzzle.num == PUZZLES-1)      puzzle.num = PZ_CUSTOM; 
            else if (puzzle.num == PZ_CUSTOM) puzzle.num = 0;
            else puzzle.num = (puzzle.num < PUZZLES-1) ? puzzle.num+1 : PUZZLES-1;
          } else {
            if (manual == M_CURSOR) {
              if (tileY < GRIDSIZE-1) { tileY++; tileMotion = true; }
            } else {                                                    // manual == M_SELECT
              puzzle.spot[tileY*GRIDSIZE+tileX] &= 0x0F;                // unlock
              if (puzzle.spot[tileY*GRIDSIZE+tileX] > 0)
                puzzle.spot[tileY*GRIDSIZE+tileX]--;                    // decrement spot, don't lock yet

              // scan puzzle, populate numchoices & choiceset
              puzzle.empty = ScanPuzzle(&puzzle);
            }
          }
        }

        arduboy.setCursor(3,12);
        if (puzzle.num < PZ_MEDIUM) {
          j = 1;
          if (puzzle.num < 10) arduboy.print('0');
          arduboy.print(puzzle.num);
        } else arduboy.print(F("  "));
        arduboy.print(F(" Easy"));

        arduboy.setCursor(3,22);
        if (puzzle.num >= PZ_MEDIUM && puzzle.num < PZ_HARD) { j = 2; arduboy.print(puzzle.num); }
        else arduboy.print(F("  "));
        arduboy.print(F(" Medium"));
      
        arduboy.setCursor(3,32);
        if (puzzle.num >= PZ_HARD && puzzle.num < PUZZLES)   { j = 3; arduboy.print(puzzle.num); }
        else arduboy.print(F("  "));
        arduboy.print(F(" Hard"));
      
        arduboy.setCursor(3,42);
        if (puzzle.num == PZ_CUSTOM) { 
          j = 4; 
          if (manual == M_SELECT)      arduboy.print(F(">>")); // indicate manual entry mode
          else if (manual == M_CURSOR) arduboy.print(F(">-"));
          else                         arduboy.print(F("--")); 
        } else arduboy.print(F("  "));
        arduboy.print(F(" Custom"));

        arduboy.drawRoundRect(18,j*10,42,11,3,WHITE); // 42 = 3 + 6 characters * 6 + 3

        if (puzzle.num < PUZZLES && old_num != puzzle.num) {
          loadPuzzle(puzzle.num); // load puzzle from PROGMEM (puzzles.h) & scan puzzle
          
          // scan puzzle, populate numchoices & choiceset
          puzzle.empty = ScanPuzzle(&puzzle);
          
        } else { // load custom puzzle from serial input OR manual entry

          if (old_num != puzzle.num) {
            for (byte r=0; r<GRIDSIZE; r++)
              for (byte c=0; c<GRIDSIZE; c++) puzzle.spot[r*GRIDSIZE+c] = 0; // clear puzzle

            // scan puzzle, populate numchoices & choiceset
            puzzle.empty = ScanPuzzle(&puzzle);

            manual = M_SELECT;
            tileX = tileY = row = 0;
            
            Serial.print(F("Enter row["));
            Serial.print(row);
            Serial.println(']');          
          }
        
          // if there's any serial available, read it for puzzle input rows:
          while (Serial.available() > 0) {
            long rowEntry = Serial.parseInt();

            for (int col = GRIDSIZE-1; col >= 0; col--) {
              puzzle.spot[row*GRIDSIZE+col] = rowEntry % 10; // don't lock spot yet
              rowEntry /= 10;
            }

            tileX = 0;
            tileY = row;
            
            // scan puzzle, populate numchoices & choiceset
            puzzle.empty = ScanPuzzle(&puzzle);

            row++;
            row %= GRIDSIZE;
          
            Serial.print(F("Enter row["));
            Serial.print(row);
            Serial.println(']');    
          }

        } // load custom puzzle
      } // track old_num
      
          
      if (buttonJustPressed(A_BUTTON)) {
        if (puzzle.num != PZ_CUSTOM) mode = M_MENU;
        else switch (manual) { // cyle back through manual entry modes { 0 <- M_CURSOR <- M_SELECT <- 0 }
          case M_SELECT: manual = M_CURSOR; break;
          case M_CURSOR: manual = 0;        break;
          case 0:        manual = M_SELECT; break;
          default:       manual = M_SELECT;
        }
      }

      if (buttonJustPressed(B_BUTTON) && puzzle.empty > 0) { // if valid puzzle, lock the spots and play game
        for (byte r=0; r<GRIDSIZE; r++)
          for (byte c=0; c<GRIDSIZE; c++) 
            if ((puzzle.spot[r*GRIDSIZE+c] & 0x0F) > 0) puzzle.spot[r*GRIDSIZE+c] |= 0xF0; // numchoices: upper nibble (locked spot)
              
        mode = M_TRANS2; // transition back to game
      }

      puzzle.timer = 0;
      tpuzzle = millis();

      if (puzzle.empty < 0) {
        arduboy.setCursor(3,52);
        arduboy.print(F("invalid"));
      }

      break;

    case M_HINT:
      puzzle.empty = SolvePuzzle(&puzzle,true); // hint == true, solve one step
      mode = M_MENU; // transition back to game
    
      break;

    case M_NOTES:
      // toggle all notes on/off
      if (puzzle.notes == NOTES_OFF) puzzle.notes = NOTES_ALL;
      else                           puzzle.notes = NOTES_OFF;
      
      mode = M_TRANS2; // transition back to game
    
      break;

    case M_SAVE:
      { // save menu
        
        arduboy.setCursor(3,2);
        arduboy.print(strcpy_P(msgBuffer,menu[M_SAVE]));

        if (buttonJustPressed(RIGHT_BUTTON) || buttonHolding(RIGHT_BUTTON))
          save_ltr = (save_ltr < NAMELEN) ? save_ltr+1 : 0; // NAMELEN == entry pick, letter position  (0..2) three initials
        
        if (buttonJustPressed(LEFT_BUTTON) || buttonHolding(LEFT_BUTTON)) 
          save_ltr = (save_ltr > 0) ? save_ltr-1 : NAMELEN; // NAMELEN == entry pick, letter position  (0..2) three initials

        if (buttonJustPressed(UP_BUTTON) || buttonHolding(UP_BUTTON)) {
          if (save_ltr > NAMELEN-1) save_entry = (save_entry > 0) ? save_entry-1 : ENTRIES-1;
          else {
            char c = puzzle.player[save_ltr];
            switch (c) { //   up one letter _,A..Z,a..z,0..9
              case ' ': c = 'A'; break;
              case 'Z': c = 'a'; break;
              case 'z': c = '0'; break;
              case '9': c = ' '; break;
              default: c++;
            }
            puzzle.player[save_ltr] = c; 
            puzzle.player[NAMELEN]  = 0; // safety, null terminated
          }
        }

        if (buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) {
          if (save_ltr > NAMELEN-1) save_entry = (save_entry < ENTRIES-1) ? save_entry+1 : 0;
          else {
            char c = puzzle.player[save_ltr];
            switch (c) { // down one letter _,A..Z,a..z,0..9
              case '0': c = 'z'; break;
              case 'a': c = 'Z'; break;
              case 'A': c = ' '; break;
              case ' ': c = '9'; break;
              default: c--;
            }
            puzzle.player[save_ltr] = c; 
            puzzle.player[NAMELEN]  = 0; // safety, null terminated
          }
        }


        for (j=0; j<ENTRIES; j++) {
          arduboy.setCursor(9,j*10+12);
          arduboy.print(j+1);
          arduboy.print(' ');
          if (j == save_entry && strcmp(puzzle.player,"   ") != 0) 
               arduboy.print(puzzle.player); // non-blank puzzle player
          else arduboy.print(save_name[j]);  // blank puzzle player transparent to see entry save_name
        }

        arduboy.drawRect(6,save_entry*10 + 10,11,11,WHITE);
        if (save_ltr >= 0 && save_ltr < NAMELEN) {
          arduboy.drawFastHLine(save_ltr*6 + 21,save_entry*10 + 10,6,WHITE); //  overline editing letter
          arduboy.drawFastHLine(save_ltr*6 + 21,save_entry*10 + 20,6,WHITE); // underline editing letter
        }

        if (buttonJustPressed(A_BUTTON)) mode = M_MENU; // cancel save
        
        if (buttonJustPressed(B_BUTTON)) {              // commit save
          if (strcmp(puzzle.player,"   ") == 0) 
            strcpy(puzzle.player,save_name[save_entry]);      // blank puzzle player, use entry save_name
            
          if (savePuzzle(save_entry,&puzzle) < 0) {
            Serial.print(F("savePuzzle: entry "));
            Serial.print(save_entry);
            Serial.println(F(" failed"));
          } else {
            strcpy(save_name[save_entry],puzzle.player); // success, assign player initials to the entry
            mode = M_TRANS2;                             // transition back to game
          }

        } // B_BUTTON

      }
      
      break;

    case M_RESTORE:
      { // restore menu

        arduboy.setCursor(3,2);
        arduboy.print(strcpy_P(msgBuffer,menu[M_RESTORE]));

        if (buttonJustPressed(UP_BUTTON) || buttonHolding(UP_BUTTON)) 
          save_entry = (save_entry > 0) ? save_entry-1 : ENTRIES-1;
        
        if (buttonJustPressed(DOWN_BUTTON) || buttonHolding(DOWN_BUTTON)) 
          save_entry = (save_entry < ENTRIES-1) ? save_entry+1 : 0;

        for (j=0; j<ENTRIES; j++) {
          arduboy.setCursor(9,j*10+12);
          arduboy.print(j+1);
          arduboy.print(' ');
          arduboy.print(save_name[j]);
        }

        arduboy.drawRect(6,save_entry*10 + 10,11,11,WHITE);

        if (buttonJustPressed(A_BUTTON)) mode = M_MENU; // cancel restore
        
        if (buttonJustPressed(B_BUTTON)) {              // commit restore
          byte old_num = puzzle.num;
          char old_player[4];

          strcpy(old_player,puzzle.player);
          
          if (restorePuzzle(save_entry,&puzzle) < 0) {
            Serial.print(F("restorePuzzle: entry "));
            Serial.print(save_entry);
            Serial.println(F(" failed"));
                      
            // re-initialize puzzle
            strcpy_P(puzzle.id,title);
            puzzle.rev = pgm_read_byte(&rev);
            strcpy(puzzle.player,old_player);
            puzzle.notes = NOTES_OFF;
          
            puzzle.num = old_num;
            loadPuzzle((puzzle.num < PUZZLES)? puzzle.num: 0);
          
            puzzle.timer = 0;  
          } else {              // restore success
            tpuzzle = millis(); // new sample (timer restored)
            mode = M_TRANS2;    // transition back to game
          }
          
        } // B_BUTTON
        
      }

      break;

    case M_SOLVE:
      puzzle.singles    = 0;
      puzzle.unique     = 0;
      puzzle.prunes     = 0;
      guesscount = 0;

      puzzle.empty = SolvePuzzle(&puzzle,false); // hint == false, fully solve
      if (puzzle.empty == 0) {
        Serial.println(F("Puzzle solution found:"));
        ShowPuzzle(puzzle);

        Serial.print(F("  [stats: singles="));
        Serial.print(puzzle.singles);
        Serial.print(F(", unique="));
        Serial.print(puzzle.unique);
        Serial.print(F(", prunes="));
        Serial.print(puzzle.prunes);
        Serial.print(F("] guess count="));
        Serial.println(guesscount);

      } else if (puzzle.empty < 0) {
        Serial.print(F("Error: Invalid Puzzle (empty="));
        Serial.print(puzzle.empty);
        Serial.println(F(")."));
        Serial.println(F("  Negative empty: bad spot or empty spot(s) with no valid choices."));

#ifdef DEBUG
        // show structure, debug
        ShowPuzzleStruct(puzzle,true);
#endif
      } else {
        Serial.print(F("Error: empty = "));
        Serial.print(puzzle.empty);
        Serial.println(F(", unexpected."));

#ifdef DEBUG
        // show structure, debug
        ShowPuzzleStruct(puzzle,true);
#endif
      }

      mode = M_MENU;
      menu_item = 0;
      
      break;
      
    case M_TRANS1:
      // transition to menu, animate for 10 frames
      // x_off 32 .. 64, increment (64-32)/10 = 3
      if (x_off < 64) x_off += (64 - x_off) >= 3 ? 3 : (64 - x_off); // one extra frame for remainder
      else { mode = M_MENU; menu_item = 0; }
      
      break;

    case M_TRANS2:
      // transition to game, animate for 10 frames
      // x_off 64 .. 32, decrement (64-32)/10 = 3
      if (x_off > 32) x_off -= (x_off - 32) >= 3 ? 3 : (x_off - 32); // one extra frame for remainder
      else mode = M_CURSOR;

      break;
      
    default:
      Serial.print(F("Error: unknown mode: "));
      Serial.println(mode);
      mode = M_CURSOR;
  } // switch (mode)


  if (mode == M_CURSOR || mode == M_SELECT) {
    if (puzzle.empty != 0) {
      unsigned long t = millis();
      if (t < tpuzzle) 
        puzzle.timer += 0xFFFFFFFF - tpuzzle + t; // millis() rollover, 32bit unsigned
      else puzzle.timer += t - tpuzzle;
      tpuzzle = t;
    }
  
    // left sidebar
    arduboy.setCursor(0,0);

    if (puzzle.num == PZ_CUSTOM) arduboy.print(F("--"));
    else {
      if (puzzle.num < 10) arduboy.print('0');
      arduboy.print(puzzle.num);
    }

    if (puzzle.empty < 0) {
      arduboy.setCursor(3,24);
      arduboy.print(F("bad"));
      arduboy.setCursor(0,32);
      arduboy.print(F("spot"));
    } else if (puzzle.empty == 0) {
      arduboy.setCursor(3,28);
      arduboy.print(F("DONE"));
      arduboy.drawRoundRect(0,26,30,11,3,WHITE); // 30 = 3 + 4 characters * 6 + 3
    }
        
    arduboy.setCursor(0,56);
    if (puzzle.num < PZ_MEDIUM)    arduboy.print(F("Easy"));
    else if (puzzle.num < PZ_HARD) arduboy.print(F("Med."));
    else if (puzzle.num < PUZZLES) arduboy.print(F("Hard")); 

    // right sidebar
    arduboy.setCursor(97,0);
    i = (int)(puzzle.timer / 60000);   // minutes
    if (i < 10) arduboy.print(' ');
    arduboy.print(i);
    arduboy.print(':');
    i = (int)(puzzle.timer/1000 % 60); // seconds
    if (i < 10) arduboy.print('0');
    arduboy.print(i);

    if ((puzzle.spot[tileY*GRIDSIZE+tileX] & 0xF0) == 0xF0) { arduboy.setCursor(111,28); arduboy.print('L'); } // locked
    else if (puzzle.notes != NOTES_OFF) {
      long mask = (puzzle.choiceset[(tileY*GRIDSIZE/3+(tileX/3))] >> (GRIDSIZE*(tileX%3))) & (long)0x1FF; // check 9 bits
      byte num = 1;
      i = 103;
      j =  20;
      while (mask) {
        if (mask & (long)1) { // check (num-1) bit in choiceset mask
          arduboy.setCursor(i,j);
          arduboy.print(num);
          
        }
        mask >>= 1;
        num++;
        // 8x8 pixel spacing
        if (num == 4)      { i = 103; j = 28; }
        else if (num == 7) { i = 103; j = 36; }
        else i += 8;
      }
    }
    
    arduboy.setCursor(109,46);
    arduboy.setTextSize(2);
    if (mode == M_SELECT) {
      arduboy.drawRect(105,44,18,20,WHITE);
           i = select_spot & 0x0F;
    } else i = puzzle.spot[tileY*GRIDSIZE+tileX] & 0x0F;
    if (i == 0) arduboy.print('_');
    else        arduboy.print(i);   // magnified current tile
    arduboy.setTextSize(1);
    
  } else tpuzzle = millis(); // puzzle.timer paused, let the sample rise

  // display the sudoku puzzle
  for (j=0; j<GRIDSIZE; j++) {
    for (i=0; i<GRIDSIZE; i++) {
      if ((puzzle.spot[j*GRIDSIZE+i] & 0x0F) == 0 && (puzzle.notes & NOTES_ALL)) { // blank spot, show all notes
        long mask = (puzzle.choiceset[(j*GRIDSIZE/3+(i/3))] >> (GRIDSIZE*(i%3))) & (long)0x1FF; // check 9 bits
        byte num = 1;
        int x = x_off + i*TILE_SZ + 1;
        int y =         j*TILE_SZ + 1;
        while (mask) {
          if (mask & (long)1) arduboy.drawPixel(x,y,WHITE); // check (num-1) bit in choiceset mask
          mask >>= 1;
          num++;
          if (num == 4)      { x = x_off + i*TILE_SZ + 1; y += 2; }
          else if (num == 7) { x = x_off + i*TILE_SZ + 1; y += 2; }
          else x += 2;
        }
      } else arduboy.drawBitmap(x_off+i*TILE_SZ,j*TILE_SZ,num_bitmaps[puzzle.spot[j*GRIDSIZE+i] & 0x0F],TILE_SZ,TILE_SZ,WHITE);
    }
    
  }

  arduboy.drawFastHLine(x_off,3*TILE_SZ,GRIDSIZE*TILE_SZ,WHITE);
  arduboy.drawFastHLine(x_off,6*TILE_SZ,GRIDSIZE*TILE_SZ,WHITE);
  arduboy.drawFastVLine(x_off+3*TILE_SZ,0,GRIDSIZE*TILE_SZ,WHITE);
  arduboy.drawFastVLine(x_off+6*TILE_SZ,0,GRIDSIZE*TILE_SZ,WHITE);

  // draw cursor box, 30 frames = 2 sec, 2/3 time ON
  if (frameCount % (FRAMERATE*2) >= (FRAMERATE*2/3) || tileMotion) 
    arduboy.drawRect(x_off+tileX*TILE_SZ,tileY*TILE_SZ,TILE_SZ,TILE_SZ,WHITE);
  
  // then we finaly we tell the arduboy to display what we just wrote to the display
  arduboy.display();
  
} // loop()


// ***********************************************************************************************************
// *
// *                            Sudoku Solver Functions
// *
// *
// ***********************************************************************************************************

int SolvePuzzle(struct puzzletype *thispuzzle, boolean hint) {
  // find single, unique choices & prune choices                    
  // then guess, which will make recursive calls                    
  // return number of empty spots, negative empty if invalid puzzle 
  // hint == true, solve one step


  int empty, oldempty, prunes;
  int findone;

#ifdef DEBUG
    Serial.print(F("freeRAM() = "));
    Serial.println(freeRAM());
#endif

  // scan puzzle
  empty = ScanPuzzle(thispuzzle);
  if (empty < 1) return(empty);

  do {
    oldempty = empty;
    prunes = 0;

    // find easy spots (single choice)
    do {
      findone = FindSingles(thispuzzle,hint);
#ifdef DEBUG
      Serial.print(F("found "));
      Serial.print(findone);
      Serial.println(F(" singles"));
#endif
      if (findone) {
        // scan to clean up numchoices & choiceset
        empty = ScanPuzzle(thispuzzle);
        if (empty < 1) return(empty);
      }
      if (hint && findone) return(empty); // solve one step
    } while (findone);

    // check for unique numbers in choiceset
    do {
      findone = FindUniqueChoice(thispuzzle,hint);
#ifdef DEBUG
      Serial.print(F("found "));
      Serial.print(findone);
      Serial.println(F(" unique choices"));
#endif
      if (findone) {
        // scan to clean up numchoices & choiceset
        empty = ScanPuzzle(thispuzzle);
        if (empty < 1) return(empty);
      }
      if (hint && findone) return(empty); // solve one step
    } while (findone);

#ifdef DEBUG
    // debug, show puzzle & empty spots
    Serial.print(F("empty= "));
    Serial.println(empty);
    ShowPuzzle(*thispuzzle);
    ShowPuzzleStruct(*thispuzzle,false); // only show empty spots
#endif

    // prune choices where choicesets narrow options in other spaces
    do {
      findone = PruneChoices(thispuzzle);
      prunes += findone;
#ifdef DEBUG
      Serial.print(F("found "));
      Serial.print(findone);
      Serial.println(F(" pruned choices"));
#endif
      // no scan, PruneChoices() does not fill spots
    } while (findone);
    
  } while (empty < oldempty || prunes);


  // try combinations from the choiceset
  do {
    oldempty = empty;

#ifdef DEBUG
    Serial.print(F("freeRAM() = "));
    Serial.println(freeRAM());
    Serial.print(F("sizeof(struct puzzletype) = "));
    Serial.println(sizeof(struct puzzletype));
#endif

    // WARNING: GuessFromChoiceset() allocates another puzzle structure, need to make sure there is enough memory
    if (freeRAM() > (sizeof(struct puzzletype) + 307)) {
    
      empty = GuessFromChoiceset(thispuzzle,hint);
      if (empty <= 0) return(empty);

#ifdef DEBUG
      // debug, show puzzle & empty spots
      Serial.print(F("empty= "));
      Serial.println(empty);
      ShowPuzzle(*thispuzzle);
      ShowPuzzleStruct(*thispuzzle,false); // only show empty spots
#endif
      if (hint && empty < oldempty) return(empty); // solve one step
    }
#ifdef DEBUG 
      else Serial.println(F("Not enough memory, can't call GuessFromChoiceset()"));
#endif

  } while (empty < oldempty);

  return(empty);
} // end SolvePuzzle()


int ScanPuzzle(struct puzzletype *check) {
  // pass in puzzle, scan puzzle and populate structure             
  // return number of empty spots, negative empty if invalid puzzle 

  int empty=0;
  int validpuzzle=1;
  byte row, col, boxrow, boxcol;
  byte num, r_used, c_used, b_used;
  byte r,c;
  long mask;

//#ifdef DEBUG
//  Serial.println(F("start ScanPuzzle()"));
//  ShowPuzzleStruct(puzzle,true);
//#endif

  for (row=0; row<GRIDSIZE; row++) {
    for (col=0; col<GRIDSIZE; col++) {

      // numchoices: using the upper nibble (0xF0 = locked)
      if ((check->spot[row*GRIDSIZE+col] & 0xF0) != 0xF0) 
        check->spot[row*GRIDSIZE+col] &= 0x0F; // zero out the upper nibble
        
      check->choiceset[(row*GRIDSIZE/3+(col/3))] &= ~((long)0x1FF << (GRIDSIZE*(col%3))); // clear 9 bits
      
      { // scan all spots to catch invalid filled spots
        
        if ((check->spot[row*GRIDSIZE+col] & 0x0F) < 1) empty++; // count empty spots (lower nibble)
        
        mask = 0; // for building choiceset

        // check for unused numbers (row, col, box)
        for (num=1; num<=GRIDSIZE; num++) {
          r_used = c_used = b_used = 0;

          // row check
          for (c=0; c<GRIDSIZE; c++)
            if ((check->spot[row*GRIDSIZE+c] & 0x0F) == num) r_used++; // lower nibble

          // col check
          for (r=0; r<GRIDSIZE; r++)
            if ((check->spot[r*GRIDSIZE+col] & 0x0F) == num) c_used++; // lower nibble

          // box check
          boxrow = row/BOXSIZE * BOXSIZE;
          boxcol = col/BOXSIZE * BOXSIZE;

          for (r=boxrow; r<boxrow+BOXSIZE; r++) 
            for (c=boxcol; c<boxcol+BOXSIZE; c++) 
              if ((check->spot[r*GRIDSIZE+c] & 0x0F) == num) b_used++; // lower nibble

          if (r_used > 1 || c_used > 1 || b_used > 1)
            validpuzzle = -1; // invalid puzzle, no solution possible
        
          if (r_used + c_used + b_used == 0 && (check->spot[row*GRIDSIZE+col] & 0x0F) < 1) {  // only for empty spot
            // number not used in row, col or box
            mask |= ((long)1 << (num-1)); // set bit
            check->spot[row*GRIDSIZE+col] += 0x10;      // numchoices: upper nibble
          }
        } // num
        
        if ((check->spot[row*GRIDSIZE+col] & 0x0F) < 1)                                       // only for empty spot
          check->choiceset[(row*GRIDSIZE/3+(col/3))] |= (mask << (GRIDSIZE*(col%3)));         // set 9 bit mask
          
      } // scan all spots to catch invalid filled spots

      if ((check->spot[row*GRIDSIZE+col] & 0x0F) < 1 && 
          (check->spot[row*GRIDSIZE+col] & 0xF0) == 0)  // numchoices: upper nibble
          validpuzzle = -1; // invalid puzzle, no solution possible

    } // col
  } // row

//#ifdef DEBUG
//  Serial.println(F("end ScanPuzzle()"));
//  Serial.print(F("empty= "));
//  Serial.println(empty);
//  Serial.print(F("validpuzzle= "));
//  Serial.println(validpuzzle);
//  ShowPuzzleStruct(puzzle,true);
//#endif

  if (empty == 0 && validpuzzle == -1) return(-1);
  else return(empty*validpuzzle);

} // end ScanPuzzle()


int FindSingles(struct puzzletype *check, boolean hint) {
  // find easy spots (single choice)
  // hint == true, solve one step

  int findone=0;
  byte row,col,num;

  for (row=0; row<GRIDSIZE; row++) {
    for (col=0; col<GRIDSIZE; col++) {
      if ((check->spot[row*GRIDSIZE+col] & 0xF0) == 0x10) { // numchoices: upper nibble
        long mask = (check->choiceset[(row*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3))) & (long)0x1FF; // check 9 bits
        num = 1;
        while (mask) {
          if (mask & (long)1) { // check (num-1) bit in choiceset mask
            check->spot[row*GRIDSIZE+col] = num; // also sets numchoices = 0 (upper nibble)
            // clear structure for filled spot
            check->choiceset[(row*GRIDSIZE/3+(col/3))] &= ~((long)0x1FF << (GRIDSIZE*(col%3))); // clear 9 bits

            check->singles++;
            findone++;
            if (hint && findone) return(findone); // solve one step
            break;            
          }
          mask >>= 1;
          num++;
        }
      } // single choice
    }
  }
  return(findone);

} // FindSingles()


int FindUniqueChoice(struct puzzletype *check, boolean hint) {
  // find unique numbers in choiceset (exclusive choice)
  // hint == true, solve one step

  int findone=0;
  byte row, col, boxrow, boxcol;
  byte num, r_unique, c_unique, b_unique;
  byte r,c;

//#ifdef DEBUG
//  Serial.println(F("start FindUniqueChoice()"));
//  ShowPuzzleStruct(puzzle,true);
//#endif

  for (row=0; row<GRIDSIZE; row++) {
    for (col=0; col<GRIDSIZE; col++) {
      if ((check->spot[row*GRIDSIZE+col] & 0x0F) == 0) { // check empty spots (lower nibble)
        // a number choice unique if exclusive to this spot
        for (num=1; num<=GRIDSIZE; num++) {
          if (!((check->choiceset[(row*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3)+(num-1))) & (long)1)) // check bit
            continue; // number not in spot choiceset, check next num

          // check (row, col, box)
          r_unique = c_unique = b_unique = 1; // assume unique

          // row check
          for (c=0; c<GRIDSIZE; c++){
            if (c==col) continue;
            if ((check->choiceset[(row*GRIDSIZE/3+(c/3))] >> (GRIDSIZE*(c%3)+(num-1))) & (long)1) { // check bit
              // not unique
              r_unique=0;
              break;
            }
          }

          // col check
          for (r=0; r<GRIDSIZE; r++){
            if (r==row) continue;
            if ((check->choiceset[(r*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3)+(num-1))) & (long)1) { // check bit
              // not unique
              c_unique=0;
              break;
            }
          }

          /* box check */
          boxrow = row/BOXSIZE * BOXSIZE;
          boxcol = col/BOXSIZE * BOXSIZE;

          for (r=boxrow; r<boxrow+BOXSIZE; r++) {
            if (!b_unique) break;
            for (c=boxcol; c<boxcol+BOXSIZE; c++) {
              if (r==row && c==col) continue;
              if ((check->choiceset[(r*GRIDSIZE/3+(c/3))] >> (GRIDSIZE*(c%3)+(num-1))) & (long)1) { // bit check
                // not unique
                b_unique=0;
                break;
              }
            }
          }
        
          if (r_unique || c_unique || b_unique) { 
            // number exclusive for this spot
            check->spot[row*GRIDSIZE+col] = num; // also sets numchoices = 0 (upper nibble)
            // clear structure for filled spot
            check->choiceset[(row*GRIDSIZE/3+(col/3))] &= ~((long)0x1FF << (GRIDSIZE*(col%3))); // clear 9 bits

            // keep num in choiceset, avoid bogus uniques before ScanPuzzle()
            check->choiceset[(row*GRIDSIZE/3+(col/3))] |= ((long)1 << (GRIDSIZE*(col%3)+(num-1))); // set bit

            check->unique++;
            findone++;
            if (hint && findone) return(findone); // solve one step
            break;
          }
        } // num
      } // unique check
    }
  }

//#ifdef DEBUG
//  Serial.println(F("end FindUniqueChoice()"));
//  Serial.print(F("findone= "));
//  Serial.println(findone);
//  ShowPuzzleStruct(puzzle,true);
//#endif

  return(findone);
} // FindUniqueChoice()


int PruneChoices(struct puzzletype *check) {
  // prune choices where choicesets narrow options in other spaces  
  // i.e. two spaces with same double choiceset eliminate choiceset 
  //      for other spaces in same row, col or box                  

  int findone=0;
  byte matchesneeded, matches;
  byte matchedspots[GRIDSIZE];
  byte row, col, boxrow, boxcol;
  byte r,c;
  long mask, pmask;

//#ifdef DEBUG
//  Serial.println(F("start PruneChoices()"));
//  ShowPuzzleStruct(puzzle,true);
//#endif

  for (row=0; row<GRIDSIZE; row++) {
    for (col=0; col<GRIDSIZE; col++) {
      if ((check->spot[row*GRIDSIZE+col] & 0x0F) == 0) { // check empty spots (lower nibble)

        // numchoice dictates how many choiceset matches needed to trigger pruning possibility
        matchesneeded = check->spot[row*GRIDSIZE+col] >> 4; // numchoices: upper nibble
        
        mask = ((check->choiceset[(row*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3))) & (long)0x1FF); // check 9 bits
        
        // scan row for spaces with same choiceset
        matchedspots[col]=1;
        matches = 1;
        for (c=0; c<GRIDSIZE; c++) {
          if (c==col) continue;
          if ((check->spot[row*GRIDSIZE+c  ] & 0xF0) == 
              (check->spot[row*GRIDSIZE+col] & 0xF0) ) { // numchoices: upper nibble

            // check for matching choiceset
            if (((check->choiceset[(row*GRIDSIZE/3+(c  /3))] >> (GRIDSIZE*(c  %3))) & (long)0x1FF) == mask) { // check 9 bits
              matchedspots[c]=1;
              matches++;
            } else matchedspots[c]=0;
          } else matchedspots[c]=0;
        }

        if (matches == matchesneeded) {
          for (c=0; c<GRIDSIZE; c++){
            if (!matchedspots[c]) { // prune matched choices from other spots
              pmask = ((check->choiceset[(row*GRIDSIZE/3+(c  /3))] >> (GRIDSIZE*(c  %3))) & (long)0x1FF); // check 9 bits

              if (mask & pmask) { // there are bits to clear (prune choices)
                check->choiceset[(row*GRIDSIZE/3+(c  /3))] &= ~(mask << (GRIDSIZE*(c  %3)));
                pmask &= mask; // check how many choices were pruned
#ifdef DEBUG
                Serial.print(F("prune row scan: row="));
                Serial.print(row+1);
                Serial.print(F(",col="));
                Serial.print(col+1);
                Serial.print(F(",matches="));
                Serial.print(matches);
                Serial.print(F(" ->     c="));
                Serial.print(c+1);
                Serial.print(F(", pmask "));
                { long m = 0x100;
                  while (pmask < m && m > 0x01) { Serial.print('0'); m >>=1; } // leading zeros for BINary number
                }
                Serial.println(pmask,BIN);
#endif

                while (pmask) {
                  if (pmask & (long)1) {
                    check->spot[row*GRIDSIZE+c  ] -= 0x10; // numchoices: upper nibble
                    check->prunes++;
                    findone++;
                  }
                  pmask >>= 1;                  
                }
              }
            }
          }
        } // found matchesneeded in row


        // scan col for spaces with same choiceset
        matchedspots[row]=1;
        matches = 1;
        for (r=0; r<GRIDSIZE; r++){
          if (r==row) continue;
          if ((check->spot[r  *GRIDSIZE+col] & 0xF0) == 
              (check->spot[row*GRIDSIZE+col] & 0xF0) ) { // numchoices: upper nibble
                
            // check for matching choiceset
            if (((check->choiceset[(r  *GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3))) & (long)0x1FF) == mask) { // check 9 bits
              matchedspots[r]=1;
              matches++;
            } else matchedspots[r]=0;
          } else matchedspots[r]=0;
        }

        if (matches == matchesneeded) {
          for (r=0; r<GRIDSIZE; r++){
            if (!matchedspots[r]) { // prune matched choices from other spots
              pmask = ((check->choiceset[(r  *GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3))) & (long)0x1FF); // check 9 bits

              if (mask & pmask) { // there are bits to clear (prune choices)
                check->choiceset[(r  *GRIDSIZE/3+(col/3))] &= ~(mask << (GRIDSIZE*(col%3)));
                pmask &= mask; // check how many choices were pruned
#ifdef DEBUG
                Serial.print(F("prune col scan: row="));
                Serial.print(row+1);
                Serial.print(F(",col="));
                Serial.print(col+1);
                Serial.print(F(",matches="));
                Serial.print(matches);
                Serial.print(F(" -> r="));
                Serial.print(r+1);
                Serial.print(F("    , pmask "));
                { long m = 0x100;
                  while (pmask < m && m > 0x01) { Serial.print('0'); m >>=1; } // leading zeros for BINary number
                }
                Serial.println(pmask,BIN);
#endif
                while (pmask) {
                  if (pmask & (long)1) {
                    check->spot[r  *GRIDSIZE+col] -= 0x10; // numchoices: upper nibble
                    check->prunes++;
                    findone++;
                  }
                  pmask >>= 1;                  
                }
              }
            }
          }
        } // found matchesneeded in col

        // scan box for spaces with same choiceset
        boxrow = row/BOXSIZE * BOXSIZE;
        boxcol = col/BOXSIZE * BOXSIZE;
        matchedspots[(row-boxrow)*BOXSIZE+col-boxcol]=1;
        matches = 1;
        for (r=boxrow; r<boxrow+BOXSIZE; r++) {
          for (c=boxcol; c<boxcol+BOXSIZE; c++) {
            if (r==row && c==col) continue;
            if ((check->spot[r  *GRIDSIZE+c  ] & 0xF0) == 
                (check->spot[row*GRIDSIZE+col] & 0xF0) ) { // numchoices: upper nibble

              // check for matching choiceset
              if (((check->choiceset[(r  *GRIDSIZE/3+(c  /3))] >> (GRIDSIZE*(c  %3))) & (long)0x1FF) == mask) { // check 9 bits
                matchedspots[(r-boxrow)*BOXSIZE+c-boxcol]=1;
                matches++;
              } else matchedspots[(r-boxrow)*BOXSIZE+c-boxcol]=0;
            } else matchedspots[(r-boxrow)*BOXSIZE+c-boxcol]=0;
          }
        }

        if (matches == matchesneeded) {
          for (r=boxrow; r<boxrow+BOXSIZE; r++) {
            for (c=boxcol; c<boxcol+BOXSIZE; c++) {
              if (r==row && c==col) continue;
              if (!matchedspots[(r-boxrow)*BOXSIZE+c-boxcol]) { // prune matched choices from other spots
                pmask = ((check->choiceset[(r  *GRIDSIZE/3+(c  /3))] >> (GRIDSIZE*(c  %3))) & (long)0x1FF); // check 9 bits

                if (mask & pmask) { // there are bits to clear (prune choices)
                  check->choiceset[(r  *GRIDSIZE/3+(c  /3))] &= ~(mask << (GRIDSIZE*(c  %3)));
                  pmask &= mask; // check how many choices were pruned
#ifdef DEBUG
                  Serial.print(F("prune box scan: row="));
                  Serial.print(row+1);
                  Serial.print(F(",col="));
                  Serial.print(col+1);
                  Serial.print(F(",matches="));
                  Serial.print(matches);
                  Serial.print(F(" -> r="));
                  Serial.print(r+1);
                  Serial.print(F(",c="));
                  Serial.print(c+1);
                  Serial.print(F(", pmask "));
                  { long m = 0x100;
                    while (pmask < m && m > 0x01) { Serial.print('0'); m >>=1; } // leading zeros for BINary number
                  }
                  Serial.println(pmask,BIN);
#endif
                  while (pmask) {
                    if (pmask & (long)1) {
                      check->spot[r  *GRIDSIZE+c  ] -= 0x10; // numchoices: upper nibble
                      check->prunes++;
                      findone++;
                    }
                    pmask >>= 1;                  
                  }
                }
              }
            }
          }
        } // found matchesneeded in box

      } // prune check
    }
  }

//#ifdef DEBUG
//  Serial.println(F("end PruneChoices()"));
//  Serial.print(F("findone= "));
//  Serial.println(findone);
//  ShowPuzzleStruct(puzzle,true);
//#endif

  return(findone);
} // PruneChoices()


int GuessFromChoiceset(struct puzzletype *thispuzzle, boolean hint) {
  // scan empty spots, guess from the choiceset to try combinations 
  // try one guess per level, recursive calls to SolvePuzzle ()  
  // optimize guessing by picking from the smallest choiceset first (numchoices)   
  // hint == true, solve one step

  byte row,col,num,numchoices;
  int empty;
  struct puzzletype guesspuzzle; // WARNING: large data structure, make sure there is enough memory before calling.

  for (numchoices=2; numchoices<=GRIDSIZE; numchoices++) { // singles already removed, guess from lowest numchoices
    for (row=0; row<GRIDSIZE; row++) {
      for (col=0; col<GRIDSIZE; col++) {
        
        if ((thispuzzle->spot[row*GRIDSIZE+col] & 0x0F) != 0) continue;       // filled, skip (lower nibble)
        if ((thispuzzle->spot[row*GRIDSIZE+col] >> 4) > numchoices) continue; // skip higher numchoices (upper nibble)
        
        for (num=1; num<=GRIDSIZE; num++) {
          if (!((thispuzzle->choiceset[(row*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3)+(num-1))) & (long)1)) // check bit
            continue; // num not in choiceset, skip

          // copy the puzzle before guessing, recursive call to SolvePuzzle()
          guesspuzzle = *thispuzzle;

          // apply a guess, trying combinations
          guesspuzzle.spot[row*GRIDSIZE+col] = num; // also sets numchoices = 0 (upper nibble)
          // clear structure for filled spot
          guesspuzzle.choiceset[(row*GRIDSIZE/3+(col/3))] &= ~((long)0x1FF << (GRIDSIZE*(col%3))); // clear 9 bits

          // increment global guess counter
          guesscount++;


#ifdef DEBUG
          Serial.print(F("[guess count: "));
          Serial.print(guesscount);
          Serial.println(']');
          Serial.print(F("GuessFromChoiceset(): row="));
          Serial.print(row+1);
          Serial.print(F(",col="));
          Serial.print(col+1);
          Serial.print(F(",num="));
          Serial.println(num);
#endif

          if (!hint) {
            //  recursive call to SolvePuzzle()
            empty = SolvePuzzle(&guesspuzzle,hint);
          } else empty = ScanPuzzle(&guesspuzzle); // guess, solve one step
          
          if (empty >= 0) { // guess helped
            *thispuzzle = guesspuzzle;
            return(empty);
          }

          // empty < 0, invalid/deadend, continue to next choice
#ifdef DEBUG
          Serial.print(F("GuessFromChoiceset(): row="));
          Serial.print(row+1);
          Serial.print(F(",col="));
          Serial.print(col+1);
          Serial.print(F(",num="));
          Serial.print(num);
          Serial.println(F("-> DEADEND"));
#endif

        } // walk choicset
      } // col
    } // row
  } // numchoices (2..GRIDSIZE)

#ifdef DEBUG
  Serial.print(F("GuessFromChoiceset(): ran out of combinations to try.\n"));
#endif
  return (-1); // invalid puzzle, ran out of choices before solution

} // end GuessFromChoiceset()


void ShowPuzzle(struct puzzletype show) {

  byte row,col;

  for (row=0; row<GRIDSIZE; row++) {
    Serial.print(F("   "));
    for (col=0; col<GRIDSIZE-1; col++) {
      Serial.print(show.spot[row*GRIDSIZE+col] & 0x0F); // lower nibble
      Serial.print(F(", "));
    }
    Serial.println(show.spot[row*GRIDSIZE+col] & 0x0F); // lower nibble
  } // row
  Serial.println();

  return;
} // ShowPuzzle()



void ShowPuzzleStruct(struct puzzletype show, boolean allspots) {

  byte row,col,tmp;
  int choices;
  long mask, m;

  Serial.println(F("Show puzzle structure:"));
  Serial.print(F("  [stats: singles="));
  Serial.print(show.singles);
  Serial.print(F(", unique="));
  Serial.print(show.unique);
  Serial.print(F(", prunes="));
  Serial.print(show.prunes);
  Serial.println(F("]\n"));

  Serial.print(F("  row,col: spot  num[choiceset]"));
  if (allspots) Serial.println(F(" all spots"));
  else          Serial.println(F(" empty spots"));
  Serial.println(F("  --- ---  ---   --- 987654321"));
  choices = 0;
  for (row=0; row<GRIDSIZE; row++) {
    for (col=0; col<GRIDSIZE; col++) {
      if (!allspots && (show.spot[row*GRIDSIZE+col] & 0x0F) != 0) continue; // skip filled spots (lower nibble)
      Serial.print(F("   "));
      Serial.print(row+1);
      Serial.print(F(" , "));
      Serial.print(col+1);
      Serial.print(F(" :  "));
      tmp = show.spot[row*GRIDSIZE+col];
      Serial.print(tmp & 0x0F);     // lower nibble
      Serial.print(F("     "));
      tmp >>= 4;                    // numchoices: upper nibble
      Serial.print(tmp,HEX);
      if (tmp != 0x0F) choices += tmp;
      Serial.print(F(" ["));
      mask = (show.choiceset[(row*GRIDSIZE/3+(col/3))] >> (GRIDSIZE*(col%3))) & (long)0x1FF;
      m = 0x100;
      while (mask < m && m > 0x01) { Serial.print('0'); m >>=1; } // leading zeros for BINary number
      Serial.print(mask,BIN);
      Serial.println(']');
    } // col
  } // row
  Serial.println(F("                 ---"));
  Serial.print(F("                 "));
  Serial.print(choices);
  Serial.println(F("  choices remain"));

  return;
} // end ShowPuzzleStruct()


