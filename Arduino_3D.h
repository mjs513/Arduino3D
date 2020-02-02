#ifndef _Arduino_3D_H_
#define _Arduino_3D_H_
/**
 * Extends the UnoTFTLCD class with 3D drawing routines.
 * The base classes UnoTFTLCD and UnoGFX have been modified to support 
 * the specific drawing routines required for 3D drawing.
 * 
 * All drawing commands accept optional list of colors for the elements.
 * If list is NULL then the foreground color is used by default. 
 *
 * Triangle lists, edge lists, colors, and edge maps are to be stored in 
 * Program memory (PROGMEM). Triangles and edges are stored as tiplets and
 * pairs of uint8_t indecies into vertex arrays. Edge maps additionally 
 * store two uint8_t indecies into the triangle array. Colors are uint16_t
 * lists. 
 * 
 * Vertex lists for drawing are to be stored in RAM. Edge sets for drawing
 * are to be store in RAM. 
 * 
 * Raw vertex sets (before geometry transformaton) are to be stored in 
 * Program memory (PROGMEM) as int8_t lists. 
 *
 * IMPORTANT: there are no variable-length stack arrays in C++, but several
 * rendering routines require scratch space to store rotated vertices
 * or to compute the colors of faces and edges. The size of these buffers
 * depends on the size of the 3D model. For efficiency, this library
 * makes no attempt to initialize "scratch space" memory buffers. The
 * arrays for storing rotated vertices, axis transformations, vertex and
 * face colors, depth, and drawing order, must be managed by the client
 * code. The client code must ensure that these buffers are large enough
 * to store intermediate calculations. 
 *
 * A variety of rendering modes are supported
 * 
 * Point clouds: Render a pixel for each vertex. 
 * Color modes:
 *      Foreground color monochrome
 *      Depth-shaded monochrome
 *      User defined colors
 *      Vertex-normal shaded
 * Line modes:
 *      Solid
 *      Dashed
 * 
 * Mesh rendering: line-drawn triangles
 *      Full transparent mode: all triangles are drawn
 *      Partial surface mode: only triangles facing the camera are drawn
 *      Either mode may be "accelerated" by caching a set of edges to
 *          draw in advance. This is limited to very simple models as
 *          stack space is usually quite limited.
 * Color modes:
 *      Foreground color monochrome
 *      Depth shaded monochrome
 *      Face-normal shaded
 *      User define colors
 * Line modes:
 *      Solid
 *      Dashed
 *
 * Outline rendering: render the border of an object as a line
 *      Computed by detecting pairs of adjacent triangles across which the
 *      Z-component of the face normal changes sign. 
 *      If an edge map is available, searching for adjacent triangles is
 *      greatly accelerated.
 *      Additionally, the outline may be cached as a set of edges for
 *      faster redraw-erasing later. 
 *      Outlines only support drawing in foreground color
 *      but do support both soli and dashed lines. 
 *
 * Solid surface rendering: filled triangles
 *      Solid foreground color
 *      User-supplied per-face color
 *      Depth-shaded monochrome face color
 *      Face-normal shaded
 * 
 * Shaded surface rendering: filled triangles with interpolated colors
 *      User-supplied per-vertex color
 *      Depth-shaded per-vertex color
 *      Vertex-normal shaded
 * 
 * Closed convex surfaces do not overlap themselves, and will draw correctly 
 * as long as only triangles facing the camera are drawn. Non-convex 
 * surfaces may overlap themselves. This can be avoided by drawing the
 * triangles from front-to-back and avoiding drawing over regions that
 * have already been painted. This requires that a draw_order be supplied
 * to the rendering function. 
 * 
 */

#include <math.h>
#include <stdint.h>
#include "ILI9341_t3n.h"
#include <SPI.h>


#define PU8  const uint8_t  *
#define P8   const int8_t   * 
#define PU16 const uint16_t *
#define swapU8(a, b)  { uint8_t  t=a; a=b; b=t; }
#define swapU16(a, b) { uint16_t t=a; a=b; b=t; }

/* Model struct. Pointers to PROGMEM stored vertex, 
   edge, triangle, and edge-map arrays. Numbers denoting
   the length of said arrays. In practice, any number of 
   these pointser may be null. At minimum, a list of vertex
   points is needed to render a model.
*/ 
typedef struct Model {
    uint16_t NVertices;
    uint16_t NEdges;
    uint16_t NFaces;
    P8    vertices;
    PU8   edges;
    PU8   faces;
    P8    vertexNormals;
    P8    faceNormals;
} Model;


// Assign human-readable names to some common 16-bit color values:
// Using values with identical high and low bytes gives faster
// Rendering of filled regions
#define BLACK   0b0000000000000000
#define BLUE    0b0001000000010000
#define RED     0b1110000011100000
#define GREEN   0b0000010000000100
#define CYAN    0b0001010000010100
#define PURPLE  0b0111000001110000
#define MAGENTA 0b1111000011110000
#define LIME    0b0110011101100111
#define YELLOW  0b1110011011100110
#define ORANGE  0b1110010011100100
#define WHITE   0b1111011111110111
#define GREY    0b0110001101101011
//#define MASK   ~0b1111011111110111

	
#define FRAME_ID_BIT 3
#define QUICK_READ_MASK   0b11111100
// Macros for masked rendering -- set FRAME_ID_BIT based on platform
#define FRAME_ID_FLAG16   ((uint16_t)0x100<<FRAME_ID_BIT)
#define FRAME_ID_MASK16  ~FRAME_ID_FLAG16
#define FRAME_ID_FLAG8    ((uint8_t)1<<FRAME_ID_BIT)
#define FRAME_ID_MASK8   ~FRAME_ID_FLAG8
#define QUICK_COLOR_MASK  (QUICK_READ_MASK^FRAME_ID_FLAG8)

// If there's space, we can turn on software clipping to the screen
// boundaries. This isn't essential, and the fast drawing routines
// don't use it, but it could be nice in some cases. 
#define DO_CLIP

#ifdef __cplusplus

class Arduino_3D : public ILI9341_t3n
{

public:
  //Arduino_3D(SPIClass *SPIWire, uint8_t _CS, uint8_t _DC, uint8_t _RST = 255, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);
  Arduino_3D(uint8_t _CS, uint8_t _DC, uint8_t _RST = 255, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);

  uint16_t X0 = 240/2;
  uint16_t Y0 = 320/2;
  
  uint8_t color_map[16];
	// State variables for controlling masked and overdrawn rendering
	uint8_t  mask_flag = 0;
	uint8_t  do_masking = 0;  
	uint8_t  do_overdraw = 0;  
	uint16_t background_color = 0;
	uint16_t foreground_color = WHITE;

  
  void     setColorMap(uint8_t cmap);
   
  void     setLocation(uint16_t x0, uint16_t y0);
  
  void     eraseRegion(uint8_t x0, uint16_t y0, uint8_t x1, uint16_t y1);
  void     eraseBoundingBox(Model *M, int8_t *vertices);
  void     eraseBoundingBox(int8_t *vertices,uint16_t nv);
  
  // Main 3D drawing commands. Can draw points, meshes as specified by
  // edges or by triangles, filled areas using solid colors, or shaded
  // using face or vertex normals. 
  void     drawVertices( Model *M, int8_t *vertices);
  void     drawVertices( Model *M, int8_t *vertices, uint16_t color);
  void     eraseVertices(Model *M, int8_t *vertices);

  void     drawEdges( Model *M, int8_t *vertices);
  void     drawEdges( Model *M, int8_t *vertices, uint16_t color);
  void     eraseEdges(Model *M, int8_t *vertices);

  // face_colors may be NULL, to use the model-specified colors, or the 
  // current foreground color if those are not available.
  // set dashed to 0 to draw solid lines. If nonzero, it behaves
  // like a bitmask, and only points numbers that mask to 0 are drawn
  void     drawMesh( Model *M, int8_t *vertices);
  void     drawMesh( Model *M, int8_t *vertices, uint16_t color);
  void     eraseMesh(Model *M, int8_t *vertices);
  
  // Either face_colors or vertex_colors may be NULL, to use the model-
  // specified colors, or the current foreground color if those are not
  // available.
  // Draw order may be NUL, but if it is provided triangles are sorted
  // from front to back and overdraw avoidance is used. 
  void     fillFaces( Model *M, int8_t *vertices, uint8_t *face_colors  , uint8_t *draw_order);
  void     shadeFaces(Model *M, int8_t *vertices, uint8_t *vertex_colors, uint8_t *draw_order);
  
  // Routines for creating, rotating, and applying axis transformations 
  void     getScaleTransform(float scale, float *output_transform);
  void     rotateTransformXY(float *input_transform, float dx, float dy, float *output_transform);
  void     applyTransform(Model *M, float *transform, int8_t *vertices);
  
  // Routines for generating vertex and face colors 
  // from lights or from depth-shading
  void     computeVertexLightingColors(Model *M, float *transform, uint8_t *vertex_colors);
  void     computeFaceLightingColors(  Model *M, float *transform, uint8_t *face_colors);
  void     computeVertexDepthColors(Model *M, int8_t *vertices, uint8_t *vertex_colors);
  void     computeFaceDepthColors(  Model *M, int8_t *vertices, uint8_t *face_colors);

  // Basic triangle shader functions. 
  // Interpolated color triangles are drawn similarly to regular triangles
  // In that they are broken into horizontal scanlines. For this we need
  // a fast horizontal interpolated line function. Additionally, we 
  // optionally support overdraw, which prevents pixels from the current
  // frame from being over-drawn. This requires skipping some segments
  // of the triangle.
  void     interpolateFlood(uint16_t x, uint16_t y, uint16_t i, uint16_t stop, uint16_t length, uint8_t color1, uint8_t color2);
  void     interpolateFastHLine(int16_t x0, int16_t y0, uint8_t w, uint8_t color1, uint8_t color2);
  void     shadeTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color0, uint8_t color1, uint8_t color2);
  uint16_t interpolate(uint8_t color1, uint8_t color2, uint8_t weight);
  	void fastFlood(uint8_t c, uint16_t l);
	void flood(uint16_t color, uint32_t len);
	// Functions for controlling masked and overdrawn rendering
	void     overdraw_on();
	void     overdraw_off();
	void     masking_on();
	void     masking_off();
	void     flip_mask();
	void fastFillScreen(uint8_t color);
	void fastPixel(uint8_t x, uint16_t y, uint8_t color);
	void fastFillRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c);
	void fastDrawRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c);
	void fastDrawTriangle(
		int16_t x0, int16_t y0, 
		int16_t x1, int16_t y1, 
		int16_t x2, int16_t y2, uint16_t color);
	void fastFillTriangle(
		int16_t _x0, int16_t _y0, 
		int16_t _x1, int16_t _y1, 
		int16_t _x2, int16_t _y2, uint16_t color);

  // Non-convex 3D surfaces can overlap themselvs. Sorting triangles from
  // front to back and checking to make sure we don't draw on top of areas
  // that have already been drawn can avoid overlap artefacts. To support
  // maintaining sortes lists of polygons across frames, theses functions
  // accept a permutation list for the triangle drawing order. The 
  // permutation is updated to reflect the current z-order. 
  // Alternate versions: use previously tranformed vertices
  // Helper routine for sorting triangles

  void     computeTriangleDepths(Model *M, int8_t *vertices, uint8_t *draw_order, uint8_t *depths);
  void     updateDrawingOrder(Model *M, int8_t *vertices, uint8_t *draw_order);
  void get_triangle_points(Model *M, int8_t *vertices, uint8_t i, int8_t **p, int8_t **q, int8_t **r); 
  uint8_t facing_camera(int8_t *p, int8_t *q, int8_t *r);

private: 

};

#endif

#endif





