#ifndef ABSTRACTARDUBOY_H
#define ABSTRACTARDUBOY_H

#include <stdint.h>

#define WIDTH 128
#define HEIGHT 64

#define WHITE 1
#define BLACK 0

#define COLUMN_ADDRESS_END (WIDTH - 1) & 0x7F
#define PAGE_ADDRESS_END ((HEIGHT/8)-1) & 0x07

class AbstractArduboy
{
public:
    AbstractArduboy();

    // pure virtual function
    virtual void start() = 0;

    virtual long getTime() = 0;

    virtual void LCDDataMode() = 0;
    virtual void LCDCommandMode() = 0;
    virtual void drawScreen(const unsigned char *image) = 0;

    virtual uint8_t getInput() = 0;

    // virtual
    virtual void idle();
    virtual void saveMuchPower();

    // frame management
    void setFrameRate(uint8_t rate);
    bool nextFrame();
    bool everyXFrames(uint8_t frames);

    // info
    int cpuLoad();

    // buttons
    void poll();
    bool pressed(uint8_t buttons);
    bool notPressed(uint8_t buttons);
    bool justPressed(uint8_t buttons);

    // graphics
    void blank();
    void clearDisplay();
    void display();
    void drawPixel(int x, int y, uint8_t color);
    uint8_t getPixel(uint8_t x, uint8_t y);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint8_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint8_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
    void fillScreen(uint8_t color);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color);
    void drawCompressed(int16_t sx, int16_t sy, const uint8_t *bitmap, uint8_t color);

    static inline void swap(int16_t &a, int16_t &b);

protected:
    uint8_t frameRate = 60;
    uint16_t frameCount = 0;
    uint8_t eachFrameMillis = 1000 / 60;
    long lastFrameStart = 0;
    long nextFrameStart = 0;
    bool post_render = false;
    uint8_t lastFrameDurationMs = 0;

    uint8_t currentButtonState = 0;
    uint8_t previousButtonState = 0;

    uint8_t sBuffer[(HEIGHT * WIDTH) / 8];
};

#endif // ABSTRACTARDUBOY_H
