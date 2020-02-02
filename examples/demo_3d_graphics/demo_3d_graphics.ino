#include <math.h>
#include <Arduino_3D.h> 


// For the Adafruit shield, these are the default.
#define TFT_DC  9
#define TFT_CS 10
#define TFT_RST 8

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
//Arduino_3D tft = Arduino_3D(&SPI, TFT_CS, TFT_DC, TFT_RST);
Arduino_3D tft = Arduino_3D(TFT_CS, TFT_DC, TFT_RST);

// INCLUDE MODEL DATA
//#include "sphere.h"
//#include "small_bunny.h"
//#include "bunny.h"
//#include "face.h"
#include "teapot.h"

#define AXLEN 0.6

#define POINT_CLOUD_RENDERING_MODE 0
#define EDGE_RENDERING_MODE        1
#define MESH_RENDERING_MODE        2
#define SILHOUETTE_RENDERING_MODE  3
#define FACET_RENDERING_MODE       4
#define SHADED_RENDERING_MODE      5
uint8_t rendering_mode = 4;

int16_t   touch_x,touch_y,touch_z;
uint8_t   color_choosing_mode = 0;
unsigned long rotation_benchmark_start;
int16_t   rotation_benchmark_counter = 0;
#define   BUTTON_SIZE 40
#include "color_picker.h"
#include "buttons.h"
#include "touch_routines.h"

void setup() {
  tft.begin();
  touch_begin();
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  tft.useFrameBuffer(true);
  drawButtons();
}


void loop() {
  tft.background_color = BLACK;
  clearDrawing();
  model(); // does not return
}

// need a little more contrast -- draw vertices twice
// once in foreground and once in inverted foreground?
void draw_bold_vertices(Model *M, int8_t *vbuff) {
    tft.foreground_color ^= 0xffff;
    tft.X0--;
    tft.drawVertices(M, vbuff);
    tft.updateScreen();
    tft.X0++;
    tft.foreground_color ^= 0xffff;
    tft.drawVertices(M, vbuff);
    tft.updateScreen();
}

void erase_bold_vertices(Model *M, int8_t *vbuff) {
    tft.X0--;
    tft.eraseVertices(M,vbuff);
    //tft.updateScreen();
    tft.X0++;
    tft.eraseVertices(M,vbuff);
    tft.updateScreen();
}
  
void model() {

  init_model();
  Model *M = &model_data;
  
  // Rotated versions of the points are buffered here
  int8_t buff[NVERTICES*2*3];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTICES*3];

  // Define 3D axis. We rotate this based on touch input
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3]; 
  tft.getScaleTransform(AXLEN,abuff1);
  tft.applyTransform(M,abuff1,vbuff1);
  tft.applyTransform(M,abuff1,vbuff2);

  // Initialize permutation. We keep this across frames
  // Because sorting a mostly ordered set is faster. 
  uint8_t permutation[NTRIANGLES];
  for (uint16_t i=0; i<NTRIANGLES; i++) permutation[i]=i;

  uint8_t previous_rendering_mode = rendering_mode;
  uint8_t active_rendering = 1;
  while (1) 
  {
    if (color_choosing_mode) {
        poll_touch();
        continue;
    }
    
    float dx=0,dy=0;
    if (rotation_benchmark_counter>1) {
        // perform rotation benchmark
        dy = 0.393;
        dx = 0;
        rotation_benchmark_counter--;
    } else if (rotation_benchmark_counter==1) {
        // Finish the benchmark: estimate frame rate and print it
        int fps = 16000000L / (micros() - rotation_benchmark_start);
        tft.setCursor(5, 5+BUTTON_SIZE);
        tft.setTextColor(tft.foreground_color,tft.background_color);
        tft.setTextSize(2);
        tft.print(fps);
        tft.print("   ");
        tft.updateScreen();
        // Wrap things up.
        rotation_benchmark_counter=0;
        // patch to get the last frame to render
        active_rendering=1;
    } else {
        // Get user input
        poll_touch();
        dx = DX;
        dy = DY;
    }

    // If there is a sufficient change, update the model
    if (active_rendering || dx<-MIND || dx>MIND || dy<-MIND || dy>MIND) {
        tft.rotateTransformXY(abuff1,dx,dy,abuff2);
        {float *temp2 = abuff1; abuff1 = abuff2; abuff2 = temp2;}
        tft.applyTransform(M,abuff1,vbuff1);
        {int8_t *temp = vbuff1; vbuff1 = vbuff2; vbuff2 = temp;}
    }
    
    // User input mode: render quickly
    if (model_drag) {
      // Just draw points for speed
      //draw_bold_vertices(M, vbuff2);
      //tft.updateScreen();
      // Depending on whether the previous frame was rendered
      // Quickly or not, either erase points or erase the bounding
      // box.
      //if (active_rendering==0) tft.eraseBoundingBox(M,vbuff1);
      //else erase_bold_vertices(M,vbuff1);
      //tft.updateScreen();
      tft.drawEdges(M, vbuff2);
      //tft.updateScreen();
      if (active_rendering==0) tft.eraseBoundingBox(M,vbuff1);
      else tft.eraseEdges(M, vbuff1);
      tft.updateScreen();
      active_rendering = 1;
      tft.flip_mask();
    }
    
    // Static render: render nicely
    else if (rotation_benchmark_counter || active_rendering || previous_rendering_mode != rendering_mode) {
        rendering_mode &= 0b1111111;
        switch (rendering_mode) {
            case POINT_CLOUD_RENDERING_MODE:
                draw_bold_vertices(M, vbuff2);
                tft.updateScreen();
                break;
            case EDGE_RENDERING_MODE:
            Serial.println("Edge Rendering");
                tft.drawEdges(M, vbuff2);
                tft.updateScreen();
                break;
            case MESH_RENDERING_MODE:
            Serial.println("Mesh Rendering");
                tft.drawMesh(M, vbuff2);
                tft.updateScreen();
                break;
            case SILHOUETTE_RENDERING_MODE:
            Serial.println("SILHOUETTE Rendering");
                tft.fillFaces(M,vbuff2,NULL,NULL); 
                tft.updateScreen();
                break;
            case FACET_RENDERING_MODE: {
            Serial.println("FACET Rendering");
                  uint8_t face_colors[NTRIANGLES];
                  tft.computeFaceLightingColors(M,abuff1,face_colors);
                  tft.fillFaces(M,vbuff2,face_colors,permutation);
                  tft.updateScreen();
                }
                break;
            // Leonardo is too small and slow to support shaded rendering
            // So this is enabled only for the Uno
            case SHADED_RENDERING_MODE: {
                  uint8_t normal_colors[NVERTICES];
                  tft.computeVertexLightingColors(M,abuff1,normal_colors);
                  tft.shadeFaces(M,vbuff2,normal_colors,permutation);
                  tft.updateScreen();
                }
                break;
            default: tft.updateScreen();
        }
        // Use the fastest erase method depending on the content of the 
        // previous frame
        switch (previous_rendering_mode) {
            case POINT_CLOUD_RENDERING_MODE:
                erase_bold_vertices(M, vbuff1);
                break;
            case EDGE_RENDERING_MODE:
                tft.eraseEdges(M, vbuff1);
                tft.updateScreen();
                break;
            case MESH_RENDERING_MODE:
                tft.eraseMesh(M, vbuff1);
                tft.updateScreen();
                break;
            default: 
              tft.eraseBoundingBox(M,vbuff1);
              break;
        }
        
        active_rendering = 0;
        previous_rendering_mode = rendering_mode;
        tft.flip_mask();
    }
  }
}
