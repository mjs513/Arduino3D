#include <math.h>
#include "ILI9488_t3.h"
//#include "Arduino_3D.h" 
#include "colors.h"

ILI9488_t3 tft = ILI9488_t3(&SPI, 10, 9, 8);

void setup() {
  tft.begin();
  tft.background_color = BLACK;
  tft.foreground_color = WHITE;
  tft.fillScreen(BLACK);
  tft.overdraw_off();
  tft.flip_mask();
}

#define DELAY 500

void loop() { 
  tft.fastFillTriangle( 0,0,   160,0,  0,240,  YELLOW);
  tft.fastFillTriangle( 310,0, 160,0,  310,240, ORANGE);
  tft.fastFillTriangle( 310,460, 160,460, 310,240, GREEN);
  tft.flip_mask();
  tft.fastFillTriangle( 0,460,  160,460,   0,240, LIME);
  delay(DELAY);
  //tft.eraseRegion(0,0,240,320);
  delay(DELAY);
  tft.flip_mask();
}