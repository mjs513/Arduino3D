#include <math.h>
#include <Arduino_3D.h> 
// CONFIGURE TOUCH SCREEN STUFF
#include <XPT2046_Touchscreen.h>
#include <SPI.h>


// For the Adafruit shield, these are the default.
#define TFT_DC  9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//Arduino_3D tft = Arduino_3D(&SPI, TFT_CS, TFT_DC, 8);
Arduino_3D tft = Arduino_3D(TFT_CS, TFT_DC, 8);


//#define TFT_LED        8
// MOSI=11, MISO=12, SCK=13

#define CS_PIN  7
//XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  6
XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

#define TS_MINX 140
#define TS_MINY 210
#define TS_MAXX 3827
#define TS_MAXY 3520
#define MINPRESSURE 600
#define MAXPRESSURE 2500
#define NDAMP 4
float dampx[NDAMP];
float dampy[NDAMP];
TS_Point q;

void setup() {
  tft.begin();
  tft.fillScreen(BLACK);
  ts.begin();
  tft.setRotation(0);
  tft.useFrameBuffer(true);
  ts.setRotation(0);
  q.x = 102;
  q.y = 210;
}

#define ALPHA 0.9
#define BETA (1.0-ALPHA)
void getTouch(TS_Point *d) {
    TS_Point p = ts.getPoint();

    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      q.x = d->x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      q.y = d->y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    }
    dampx[0] = q.x;
    dampy[0] = q.y;
    for (int i=1; i<NDAMP; i++) {
      dampx[i] = dampx[i]*ALPHA + BETA*dampx[i-1]; 
      dampy[i] = dampy[i]*ALPHA + BETA*dampy[i-1]; 
    }
    d->x = dampx[NDAMP-1];
    d->y = dampy[NDAMP-1];   
}
  
void loop() {
  TS_Point p;
  uint16_t color;
  int32_t x0 = -2000;
  int32_t y0 = 15609;
  for (int16_t y=-128; y<1; y++) {
    x0 += -2*y-1;
    y0 += -121;
    
    getTouch(&p);
    tft.setCursor(0,0);
    tft.setTextColor(WHITE,BLACK);
    tft.println(p.x);
    tft.println(p.y);
    int32_t DX = p.y-tft.height()/2;
    int32_t DY = p.x-tft.width()/2;
    
    int32_t xi = x0+(DX<<6);
    int32_t yi = y0+(DY<<5);
    for (int16_t x=-tft.width()/2; x<tft.width()/2; x++) {
      xi += 2*x+1;
      yi += y;
      int16_t xx = xi>>6;
      int16_t yy = yi>>5;
      yy += tft.width()/2;
      xx += tft.height()/2;
      if (xx>=32&&yy>=0&&xx<288&&yy<240) {
		uint16_t pixel_color = tft.readPixel(yy,xx);
        uint8_t R = (pixel_color & 0xF800) >> 11; // Get red channel data ( 3rd bit is mask flag )
        if (R<0b11111) R++;
        color = R*0b0000100001000001;
      } else color=BLACK;
	  tft.drawPixel(-x+tft.width()/2, y+tft.height()/2,color);
	  tft.drawPixel( x+tft.width()/2,-y+tft.height()/2,color);
    }
  }
      tft.updateScreen();

}
