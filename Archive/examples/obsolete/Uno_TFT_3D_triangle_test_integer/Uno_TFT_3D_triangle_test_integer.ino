#include <Uno_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!secret
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 920
#define TS_MAXY 900
#define MINPRESSURE 0
#define MAXPRESSURE 1000
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Uno_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  cube();
}
// Test the masked HLINE functionality
void loop() {
}


#include <math.h>
#define NVERTS 8
#define NEDGES 8
#define NTRIANGLES 10

#define MAXD 0.4
#define ALPHA 0.5
#define BETA (1.0-ALPHA)


PROGMEM const int8_t vertices[NVERTS*3]={
  1,1,1,
  1,1,-1,
  1,-1,1,
  1,-1,-1,
  -1,1,1,
  -1,1,-1,
  -1,-1,1,
  -1,-1,-1};

PROGMEM const uint8_t edges[NEDGES*2]={
  0,2,
  0,4,
  1,3,
  1,5,
  2,6,
  4,5,
  3,7,
  6,7};

PROGMEM const uint8_t triangles[NTRIANGLES*4]={
  0,2,6,CYAN,
  0,6,4,CYAN,
  0,4,5,RED,
  0,5,1,RED,
  0,3,2,BLUE,
  0,1,3,BLUE,
  1,7,3,YELLOW,
  1,5,7,YELLOW,
  2,3,7,GREEN,
  2,7,6,GREEN,
};

void cube() {
  
  int8_t buff[NVERTS*2*2];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTS*2];
  uint16_t color = WHITE;
  uint16_t x0    = 120;
  uint16_t y0    = 160;
  tft.fillScreen(BLACK);
  tft.mask_flag = 0;
  TSPoint q;

  // Define 3D axis. We're going to rotate this around
  // Based on user inputs
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3];  
  abuff1[0] = 30;
  abuff1[1] = 0;
  abuff1[2] = 0;
  abuff1[3] = 0;
  abuff1[4] = 30;
  abuff1[5] = 0;
  abuff1[6] = 0;
  abuff1[7] = 0;
  abuff1[8] = 30;

  float ddx=0,ddy=0,dddx=0,dddy=0,ddddx=0,ddddy=0;
  while (1) 
  {
    // Do touch screen stuff
    PORTC=0b11111111;
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    } else {
      p.x=q.x;
      p.y=q.y;
    }
    float dx = (p.x-q.x)*0.02;
    float dy = (p.y-q.y)*0.02;
    q.x=p.x;
    q.y=p.y;
    if (dx<-MAXD) dx=-MAXD;
    if (dy<-MAXD) dy=-MAXD;
    if (dx>MAXD)  dx=MAXD;
    if (dy>MAXD)  dy=MAXD;
    // add some damping to the cursor movement
    ddx  = ddx *ALPHA+BETA*dx;
    ddy  = ddy *ALPHA+BETA*dy;
    dddx = dddx*ALPHA+BETA*ddx;
    dddy = dddy*ALPHA+BETA*ddy;
    ddddx = ddddx*ALPHA+BETA*dddx;
    ddddy = ddddy*ALPHA+BETA*dddy;
   
    // Draw next frame
    // Flip the color and masking bit being used
    tft.mask_flag ^= 0b1000;

    // Draw the axis.
    // First, we perform a rotation on the axis. 
    float cdx = cos(ddddx);
    float sdx = sin(ddddx);
    float cdy = cos(ddddy);
    float sdy = sin(ddddy);
    uint16_t colors[3] = {RED,GREEN,BLUE};
    for (int j=0; j<3; j++) {
      // Get the current axis orientation
      float *a = &abuff1[j*3];
      float *b = &abuff2[j*3];
      float x = a[0];
      float y = a[1];
      float z = a[2];
      float nx,ny,nz;
      nz   = cdx*z  - sdx*x;      
      b[0] = nx = cdx*x  + sdx*z;
      b[1] = ny = cdy*y  + sdy*nz;
      b[2] = nz = cdy*nz - sdy*y;
    }
    
    // Rotate vertices into place
    int i;
    for (i=0; i<NVERTS; i++) {
      int8_t *q = &vbuff1[i*2];
      const int8_t *v = &vertices[i*3];
      float  x = (float)((int8_t)pgm_read_byte(&v[0]));
      float  y = (float)((int8_t)pgm_read_byte(&v[1]));
      float  z = (float)((int8_t)pgm_read_byte(&v[2]));
      float  w = x*abuff1[2]+y*abuff1[5]+z*abuff1[8];
      w = 200/(200-w);
      q[0] = (int8_t)((x*abuff1[0]+y*abuff1[3]+z*abuff1[6])*w);
      q[1] = (int8_t)((x*abuff1[1]+y*abuff1[4]+z*abuff1[7])*w);
    }

    // Draw the next frame of the cube
    
    // Triangles
    tft.masking_on = 0;
    for (i=0; i<NTRIANGLES; i++) {
      // Draw the new triangle
      const uint8_t *t = &triangles[i*4];
      int8_t *p = &vbuff1[pgm_read_byte(&t[0])*2];
      int8_t *q = &vbuff1[pgm_read_byte(&t[1])*2];
      int8_t *r = &vbuff1[pgm_read_byte(&t[2])*2];
      if ((int)(r[0]-p[0])*(q[1]-p[1])>(int)(q[0]-p[0])*(r[1]-p[1])) {
        color = (pgm_read_byte(&t[3])&0b11110111 | tft.mask_flag)*0x0101;
        tft.fillTriangle(p[0]+x0,p[1]+y0,q[0]+x0,q[1]+y0,r[0]+x0,r[1]+y0,color);
      }
    }
    tft.masking_on = 1;    
    for (i=0; i<NTRIANGLES; i++) {
      // Erase the old line using the masking feature
      const uint8_t *t = &triangles[i*4];
      int8_t *p = &vbuff2[pgm_read_byte(&t[0])*2];
      int8_t *q = &vbuff2[pgm_read_byte(&t[1])*2];
      int8_t *r = &vbuff2[pgm_read_byte(&t[2])*2];
      if ((int)(r[0]-p[0])*(q[1]-p[1])>(int)(q[0]-p[0])*(r[1]-p[1])) {
        tft.fillTriangle(p[0]+x0,p[1]+y0,q[0]+x0,q[1]+y0,r[0]+x0,r[1]+y0,BLACK);
      }
    }
    
    // Flip vertex buffers
    int8_t *temp;
    temp = vbuff1;
    vbuff1 = vbuff2;
    vbuff2 = temp;

    // Flip axis buffers
    float *temp2 = abuff1;
    abuff1 = abuff2;
    abuff2 = temp2;

  }
}


