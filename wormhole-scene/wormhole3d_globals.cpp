#include "wormhole3d_globals.h"

int gWindowWidth = kRaycastWidth * 2;
int gWindowHeight = kRaycastHeight * 2;
bool gUseRaycast = false;
bool gRaycastGpuReady = false;
bool gUseGpuRaycast = true;

// Posições de A/B preenchidas em sceneBuildCrossingQuarter().
Wormhole3D gWormhole = {
    {{0.0f, 0.3f, -5.0f}, 1.7f, 0.46f, 0.19f},
    {{2.2f, 11.0f, -7.5f}, 1.7f, 0.46f, 0.19f}
};

// Olha para -Z (buraco B ao longe); costas para +Z (rua com buraco A, prédios, montanhas).
Camera gCamera = {{0.0f, 0.95f, -9.0f}, 0.0f, -0.12f, 58.0f};

std::vector<Sphere> gSpheres;
std::vector<Aabb> gBoxes;
std::vector<PointLight> gPointLights;

std::array<BezierPath4, 3> gBezierMovingSpheres = {};
BezierPath4 gBezierCarBeach = {};
std::array<int, 3> gBoatHullBoxIndex = {-1, -1, -1};
std::array<int, 3> gBoatPoleBoxIndex = {-1, -1, -1};
std::array<int, 3> gBoatBulbSphereIndex = {-1, -1, -1};

// ilha (superfície 3d com heightmap usando real-bumpmap)

// predios (array de objetos predio) (bloco 3d com textura de paredes)

// arvores (array de objetos arvore) (tronco 3d)

// folhas (array de objetos folha) (textura de folha em 8 direções juntas)

// poste (aste 3d com ponto de luz no topo)

// agua do mar (superfície gigante 3d com heightmap usando real-bumpmap) (totalmente refletiva, mas deixa o raio um pouco azul)





std::vector<unsigned char> gRaycastPixels(kRaycastWidth * kRaycastHeight * 3, 0);

int gFpsLastMs = 0;
int gFpsFrameAccum = 0;
float gFpsDisplay = 0.0f;

bool gAnimatingCamera = false;
float gCameraT = 0.0f;
Vec3 P0 = {0.0f, 0.95f, -9.0f};
Vec3 P1 = {-1.4f, 0.5f, -11.0f};
Vec3 P2 = {1.2f, 0.55f, -13.0f};
Vec3 P3 = {0.0f, 11.0f, -50.0f};

float gSceneTimeSec = 0.0f;
int gSceneTimeMs = 0;
int gDayNightCycleMs = 120000;
bool gDayNightAuto = true;
int gDayNightEffectiveMs = 0;
int gDayNightStepMs = 5000;

float gSunDirWobbleAmp = 0.24f;
float gSunDiffuseScale = 0.54f;
float gAmbientBase = 0.032f;
float gAmbientDayScale = 0.075f;
float gSkyDayHorizonBias = 0.12f;
float gSkyDaySpan = 1.12f;
float gPointLightNightMix = 0.9f;

GLUquadric* sphereQuadric = nullptr;
