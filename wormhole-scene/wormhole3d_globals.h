#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include "wormhole3d_types.h"
#include <vector>

// Dimensão interna do buffer de raycast (CPU e FBO GPU)
inline constexpr int kRaycastWidth = 400;
inline constexpr int kRaycastHeight = 300;

extern int gWindowWidth;
extern int gWindowHeight;
extern bool gUseRaycast;
extern bool gRaycastGpuReady;
extern bool gUseGpuRaycast;

extern Wormhole3D gWormhole;
extern Camera gCamera;
extern std::vector<Sphere> gSpheres;
extern std::vector<Aabb> gBoxes;

extern std::vector<unsigned char> gRaycastPixels;

extern int gFpsLastMs;
extern int gFpsFrameAccum;
extern float gFpsDisplay;

extern bool gAnimatingCamera;
extern float gCameraT;
extern Vec3 P0, P1, P2, P3;

extern GLuint myTexture;
extern GLuint gTexSky;
extern GLUquadric* sphereQuadric;
