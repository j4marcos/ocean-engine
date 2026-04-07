#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include "wormhole3d_types.h"
#include <array>
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

inline constexpr int kMaxPointLights = 1000;
extern std::vector<PointLight> gPointLights;

/** Preenchido em `sceneBuild()` — esferas em curvas Bézier (render só vê SDF). */
extern std::array<BezierPath4, 3> gBezierMovingSpheres;
/** Curva da praia para os carros (`carBeachMotionSample`); deve ter 3 faixas Z em código. */
extern BezierPath4 gBezierCarBeach;
/** Barcos animados: índices em `gBoxes`/`gSpheres` (3 barcos). */
extern std::array<int, 3> gBoatHullBoxIndex;
extern std::array<int, 3> gBoatPoleBoxIndex;
extern std::array<int, 3> gBoatBulbSphereIndex;

/** Esferas Bézier / carros / farol: slots em `gSpheres`/`gBoxes`, preenchidos em `syncDynamicPrimitivesToScene`. */
extern std::array<int, 3> gMovingBezierSphereIndex;
extern std::array<int, 3> gCarRearBoxIndex;
extern std::array<int, 3> gCarFrontBoxIndex;
extern int gLighthouseTowerBoxIndex;
/** Placas da cabeça (paralelepípedos em mundo); mesma ideia que os AABBs dos carros. */
extern std::array<int, 5> gLighthouseHeadPlateBoxIndex;

extern std::vector<unsigned char> gRaycastPixels;

extern int gFpsLastMs;
extern int gFpsFrameAccum;
extern float gFpsDisplay;

extern bool gAnimatingCamera;
/** Carros em movimento + barcos (geometria e luzes pontuais). Tecla `V` no demo. */
extern bool gSceneVehiclesEnabled;
extern float gCameraT;
extern Vec3 P0, P1, P2, P3;

extern float gSceneTimeSec;
/** Tempo absoluto da app (ms), para ciclo dia/noite. */
extern int gSceneTimeMs;
/** Duração de um ciclo completo dia→noite→dia (ms). */
extern int gDayNightCycleMs;
/** Se true, a fase do sol segue o relógio da app; se false, só `gDayNightEffectiveMs` (teclas `[` `]`). */
extern bool gDayNightAuto;
/** Instante usado na fase do ciclo (ms); em modo auto é igual a `gSceneTimeMs` a cada frame. */
extern int gDayNightEffectiveMs;
/** Passo ao segurar `[` ou `]` (ms). */
extern int gDayNightStepMs;

/** Luz direcional global (sol) — usados em `computeDayNightLighting` (raycast + raster). */
extern float gSunDirWobbleAmp;
extern float gSunDiffuseScale;
extern float gAmbientBase;
extern float gAmbientDayScale;
extern float gSkyDayHorizonBias;
extern float gSkyDaySpan;
extern float gPointLightNightMix;

extern GLUquadric* sphereQuadric;

// BVH e bounding sphere — definições em scene/bvh.cpp, declarações em scene/bvh.h
#include "scene/bvh.h"

/** Se true, usa early-exit pela bounding sphere e BVH no ray tracing CPU. */
extern bool gUseBvh;
