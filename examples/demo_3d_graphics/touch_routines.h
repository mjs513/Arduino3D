// CONFIGURE TOUCH SCREEN STUFF
#include <XPT2046_Touchscreen.h>
#include <SPI.h>


//#define TFT_LED        8
// MOSI=11, MISO=12, SCK=13

#define CS_PIN  7
//XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  6
XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

#define TS_MINX 323  //140
#define TS_MINY 323
#define TS_MAXX 3894 //3827  
#define TS_MAXY 3776 //3520
#define MINPRESSURE 600
#define MAXPRESSURE 2500


// debouncing coefficient for buttons
// if SAVE_SPACE is active, macros have been replaced by
// functions, so the refresh rate is a little slower, 
// we adjust the debounce times to compensate

  #define TOUCH_DEBOUNCE 100
  #define MODEL_DEBOUNCE 30

uint16_t  touch_down = 0;

void touch_begin() {
  ts.begin();
  ts.setRotation(0);
}

uint8_t get_touch(int16_t *x, int16_t *y, int16_t *z) {

    TS_Point p = ts.getPoint();

    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        *x = map(p.x, TS_MINX, TS_MAXX, tft.width() , 0);
        *y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
        *z = p.z;
        return 1;
    }
    return 0;
}

// damping coefficients for rotation
#define MAXD 1.5
#define MIND 0.01
#define ALPHA 0.8
#define BETA (1.0-ALPHA)
// Drag time-out: initial position reset once this counts to zero
uint16_t model_drag = 0;
TS_Point p; // current touch location if touch down, else unchanged.
TS_Point q; // most recent touch location
// Using chained exponential damping
float ddx=0,ddy=0,DX=0,DY=0;
void damped_rotation() {
    float dx, dy;
    dx = (p.x-q.x)*0.02;
    dy = (p.y-q.y)*0.02;
    if (dx<-MAXD) dx=-MAXD;
    if (dy<-MAXD) dy=-MAXD;
    if (dx> MAXD) dx=MAXD;
    if (dy> MAXD) dy=MAXD;
    q.x=p.x;
    q.y=p.y;
    // add damping
    ddx  = ddx *ALPHA+BETA*dx;
    ddy  = ddy *ALPHA+BETA*dy;
    DX   = DX  *ALPHA+BETA*ddx;
    DY   = DY  *ALPHA+BETA*ddy;
}

void poll_touch() {
    if (get_touch(&touch_x,&touch_y,&touch_z)) {
        if (color_choosing_mode) {
            color_chooser_touch();
            touch_down=TOUCH_DEBOUNCE;
        } else {
            button_touch(); 
            if (touch_y<=BUTTON_SIZE || touch_y>=320-BUTTON_SIZE) {
                // touch is outside rendering area. might be a button press
                touch_down=TOUCH_DEBOUNCE;
            } else {
                // touch is within rendering area. update damped rotation model.
                p.x = touch_x;
                p.y = touch_y;
                if (model_drag==0) {
                    q.x=p.x;
                    q.y=p.y;
                    ddx = ddy = DX = DY = 0;
                }
                model_drag=MODEL_DEBOUNCE;
            }
        }
    } else {
        // possibly send a touch-up event for the buttons 
        if (touch_down>0) {
            touch_down --;
            if (touch_down==0) {
                if (color_choosing_mode) color_chooser_event();    
                else button_event();
            }
        }
        if (model_drag>0) model_drag --;
    }
    damped_rotation();
}
