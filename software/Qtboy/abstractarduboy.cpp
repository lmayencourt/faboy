#include "abstractarduboy.h"

#define _BV(b) (1UL << (b))
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define abs(a) (((a) < 0) ? -(a) : (a))
#define pgm_read_byte(a) 1

AbstractArduboy::AbstractArduboy()
{}

////////////////////////////////////////////////
//          default virtual function
////////////////////////////////////////////////
void AbstractArduboy::idle()
{}

void AbstractArduboy::saveMuchPower()
{}

////////////////////////////////////////////////
//              frame management
////////////////////////////////////////////////
void AbstractArduboy::setFrameRate(uint8_t rate)
{
  frameRate = rate;
  eachFrameMillis = 1000 / rate;
}

bool AbstractArduboy::everyXFrames(uint8_t frames)
{
  return frameCount % frames == 0;
}

bool AbstractArduboy::nextFrame()
{
  long now = getTime();
  uint8_t remaining;

  // post render
  if (post_render) {
    lastFrameDurationMs = now - lastFrameStart;
    frameCount++;
    post_render = false;
  }

  // if it's not time for the next frame yet
  if (now < nextFrameStart) {
    remaining = nextFrameStart - now;
    // if we have more than 1ms to spare, lets sleep
    // we should be woken up by timer0 every 1ms, so this should be ok
    if (remaining > 1)
      idle();
    return false;
  }

  // pre-render

  // technically next frame should be last frame + each frame but if we're
  // running a slow render we would constnatly be behind the clock
  // keep an eye on this and see how it works.  If it works well the
  // lastFrameStart variable could be eliminated completely
  nextFrameStart = now + eachFrameMillis;
  lastFrameStart = now;
  post_render = true;
  return post_render;
}

////////////////////////////////////////////////
//                    info
////////////////////////////////////////////////
// returns the load on the CPU as a percentage
// this is based on how much of the time your app is spends rendering
// frames.  This number can be higher than 100 if your app is rendering
// really slowly.
int AbstractArduboy::cpuLoad()
{
  return lastFrameDurationMs * 100 / eachFrameMillis;
}


////////////////////////////////////////////////
//                  buttons
////////////////////////////////////////////////

void AbstractArduboy::poll()
{
  previousButtonState = currentButtonState;
  currentButtonState = getInput();
}

// returns true if the button mask passed in is pressed
//
//   if (pressed(LEFT_BUTTON + A_BUTTON))
bool AbstractArduboy::pressed(uint8_t buttons)
{
  uint8_t button_state = getInput();
  return (button_state & buttons) == buttons;
}

// returns true if the button mask passed in not pressed
//
//   if (not_pressed(LEFT_BUTTON))
bool AbstractArduboy::notPressed(uint8_t buttons)
{
  uint8_t button_state = getInput();
  return (button_state & buttons) == 0;
}

// returns true if a button has just been pressed
// if the button has been held down for multiple frames this will return
// false.  You should only use this to poll a single button.
bool AbstractArduboy::justPressed(uint8_t button)
{
  uint8_t button_state = getInput();
  return (!(previousButtonState & button) && (currentButtonState & button));
}

////////////////////////////////////////////////
//                  graphics
////////////////////////////////////////////////

void AbstractArduboy::blank()
{
  fillScreen(0);
}

void AbstractArduboy::clearDisplay()
{
    fillScreen(0);
}

void AbstractArduboy::drawPixel(int x, int y, uint8_t color)
{
#ifdef PIXEL_SAFE_MODE
  if (x < 0 || x > (WIDTH - 1) || y < 0 || y > (HEIGHT - 1))
  {
    return;
  }
#endif

  uint8_t row = (uint8_t)y / 8;
  if (color)
  {
    sBuffer[(row * WIDTH) + (uint8_t)x] |=   _BV((uint8_t)y % 8);
  }
  else
  {
    sBuffer[(row * WIDTH) + (uint8_t)x] &= ~ _BV((uint8_t)y % 8);
  }
}

uint8_t AbstractArduboy::getPixel(uint8_t x, uint8_t y)
{
  uint8_t row = y / 8;
  uint8_t bit_position = y % 8;
  return (sBuffer[(row * WIDTH) + x] & _BV(bit_position)) >> bit_position;
}

void AbstractArduboy::drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0 + r, color);
  drawPixel(x0, y0 - r, color);
  drawPixel(x0 + r, y0, color);
  drawPixel(x0 - r, y0, color);

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void AbstractArduboy::drawCircleHelper
(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (cornername & 0x4)
    {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2)
    {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8)
    {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1)
    {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void AbstractArduboy::fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void AbstractArduboy::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint8_t color)
{
  // used to do circles and roundrects!
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (cornername & 0x1)
    {
      drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
      drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
    }

    if (cornername & 0x2)
    {
      drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
      drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
    }
  }
}

void AbstractArduboy::drawLine
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  // bresenham's algorithm - thx wikpedia
   bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      drawPixel(y0, x0, color);
    }
    else
    {
      drawPixel(x0, y0, color);
    }

    err -= dy;
    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}

void AbstractArduboy::drawRect
(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void AbstractArduboy::drawFastVLine
(int16_t x, int16_t y, int16_t h, uint8_t color)
{
  int end = y + h;
  for (int a = max(0, y); a < min(end, HEIGHT); a++)
  {
    drawPixel(x, a, color);
  }
}

void AbstractArduboy::drawFastHLine
(int16_t x, int16_t y, int16_t w, uint8_t color)
{
  int end = x + w;
  for (int a = max(0, x); a < min(end, WIDTH); a++)
  {
    drawPixel(a, y, color);
  }
}

void AbstractArduboy::fillRect
(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  // stupidest version - update in subclasses if desired!
  for (int16_t i = x; i < x + w; i++)
  {
    drawFastVLine(i, y, h, color);
  }
}

void AbstractArduboy::fillScreen(uint8_t color)
{
    // C version :
    if(color != 0) color = 0xFF;  //change any nonzero argument to b11111111 and insert into screen array.
        for(int16_t i=0; i<1024; i++)  { sBuffer[i] = color; }  //sBuffer = (128*64) = 8192/8 = 1024 bytes.
}

void AbstractArduboy::drawRoundRect
(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color)
{
  // smarter version
  drawFastHLine(x + r, y, w - 2 * r, color); // Top
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
  drawFastVLine(x, y + r, h - 2 * r, color); // Left
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void AbstractArduboy::fillRoundRect
(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color)
{
  // smarter version
  fillRect(x + r, y, w - 2 * r, h, color);

  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void AbstractArduboy::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color) {
  // no need to dar at all of we're offscreen
  if (x + w < 0 || x > WIDTH - 1 || y + h < 0 || y > HEIGHT - 1)
    return;

  int yOffset = abs(y) % 8;
  int sRow = y / 8;
  if (y < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h / 8;
  if (h % 8 != 0) rows++;
  for (int a = 0; a < rows; a++) {
    int bRow = sRow + a;
    if (bRow > (HEIGHT / 8) - 1) break;
    if (bRow > -2) {
      for (int iCol = 0; iCol < w; iCol++) {
        if (iCol + x > (WIDTH - 1)) break;
        if (iCol + x >= 0) {
          if (bRow >= 0) {
            if (color) this->sBuffer[ (bRow * WIDTH) + x + iCol  ]  |= pgm_read_byte(bitmap + (a * w) + iCol) << yOffset;
            else this->sBuffer[ (bRow * WIDTH) + x + iCol  ]  &= ~(pgm_read_byte(bitmap + (a * w) + iCol) << yOffset);
          }
          if (yOffset && bRow < (HEIGHT / 8) - 1 && bRow > -2) {
            if (color) this->sBuffer[ ((bRow + 1)*WIDTH) + x + iCol  ] |= pgm_read_byte(bitmap + (a * w) + iCol) >> (8 - yOffset);
            else this->sBuffer[ ((bRow + 1)*WIDTH) + x + iCol  ] &= ~(pgm_read_byte(bitmap + (a * w) + iCol) >> (8 - yOffset));
          }
        }
      }
    }
  }
}



typedef struct CSESSION {
  int byte;
  int bit;
  const uint8_t *src;
  int src_pos;
} CSESSION;
static CSESSION cs;

static int getval(int bits)
{
  int val = 0;
  int i;
  for (i = 0; i < bits; i++)
  {
    if (cs.bit == 0x100)
    {
      cs.bit = 0x1;
      cs.byte = pgm_read_byte(&cs.src[cs.src_pos]);
      cs.src_pos ++;
    }
    if (cs.byte & cs.bit)
      val += (1 << i);
    cs.bit <<= 1;
  }
  return val;
}

void AbstractArduboy::drawCompressed(int16_t sx, int16_t sy, const uint8_t *bitmap, uint8_t color)
{
  int bl, len;
  int col;
  int i;
  int a, iCol;
  int x, y;
  int byte = 0;
  int bit = 0;
  int w, h;

  // set up decompress state

  cs.src = bitmap;
  cs.bit = 0x100;
  cs.byte = 0;
  cs.src_pos = 0;

  // read header

  w = getval(8) + 1;
  h = getval(8) + 1;

  col = getval(1); // starting colour

  // no need to draw at all if we're offscreen
  if (sx + w < 0 || sx > WIDTH - 1 || sy + h < 0 || sy > HEIGHT - 1)
    return;

  // sy = sy - (frame*h);

  int yOffset = abs(sy) % 8;
  int sRow = sy / 8;
  if (sy < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h / 8;
  if (h % 8 != 0) rows++;

  a = 0; // +(frame*rows);
  iCol = 0;

  byte = 0; bit = 1;
  while (a < rows) // + (frame*rows))
  {
    bl = 1;
    while (!getval(1))
      bl += 2;

    len = getval(bl) + 1; // span length

    // draw the span


    for (i = 0; i < len; i++)
    {
      if (col)
        byte |= bit;
      bit <<= 1;

      if (bit == 0x100) // reached end of byte
      {
        // draw

        int bRow = sRow + a;

        //if (byte) // possible optimisation
        if (bRow <= (HEIGHT / 8) - 1)
          if (bRow > -2)
            if (iCol + sx <= (WIDTH - 1))
              if (iCol + sx >= 0) {

                if (bRow >= 0)
                {
                  if (color)
                    this->sBuffer[ (bRow * WIDTH) + sx + iCol] |= byte << yOffset;
                  else
                    this->sBuffer[ (bRow * WIDTH) + sx + iCol] &= ~(byte << yOffset);
                }
                if (yOffset && bRow < (HEIGHT / 8) - 1 && bRow > -2)
                {
                  if (color)
                    this->sBuffer[((bRow + 1)*WIDTH) + sx + iCol] |= byte >> (8 - yOffset);
                  else
                    this->sBuffer[((bRow + 1)*WIDTH) + sx + iCol] &= ~(byte >> (8 - yOffset));
                }

              }

        // iterate
        iCol ++;
        if (iCol >= w)
        {
          iCol = 0;
          a ++;
        }

        // reset byte
        byte = 0; bit = 1;
      }
    }

    col = 1 - col; // toggle colour for next span
  }
}

void AbstractArduboy::swap(int16_t &a, int16_t &b)
{
    int16_t t = a;

    a = b;
    b = t;
}

void AbstractArduboy::display()
{
  this->drawScreen(sBuffer);
}
