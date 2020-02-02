#include <math.h>
#include <Arduino_3D.h> 


// For the Adafruit shield, these are the default.
#define TFT_DC  9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//Arduino_3D tft = Arduino_3D(&SPI, TFT_CS, TFT_DC, 8);
Arduino_3D tft = Arduino_3D(TFT_CS, TFT_DC, 8);

uint16_t half_height, half_width;

void setup() {
  tft.begin();
  tft.background_color = BLACK;
  tft.foreground_color = WHITE;
  tft.fillScreen(BLACK);
  tft.setRotation(3);
  tft.overdraw_off();
  tft.flip_mask();

  half_height =tft.height()/2;
  half_width = tft.width()/2;
}

#define DELAY 500

void loop() { 
  tft.fastFillTriangle( 0,0,   half_width,0,  0,half_height,  YELLOW);
  tft.fastFillTriangle( tft.width(),0, half_width,0,  tft.width() ,half_height, ORANGE);
  tft.fastFillTriangle( tft.width(),tft.height(), half_width,tft.height(), tft.width(),half_height, GREEN);
  tft.flip_mask();
  tft.fastFillTriangle( 0,tft.height(),  half_width,tft.height(),   0,half_height, LIME);
  delay(DELAY);
  tft.eraseRegion(0,0,tft.width(),tft.height());
  delay(DELAY);
  tft.flip_mask();
}
