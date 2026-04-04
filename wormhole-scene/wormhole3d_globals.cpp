#include "wormhole3d_globals.h"

int gWindowWidth = 1280;
int gWindowHeight = 720;
bool gUseRaycast = false;
bool gRaycastGpuReady = false;
bool gUseGpuRaycast = true;

Wormhole3D gWormhole = {
    {{-1.6f, 0.45f, -4.2f}, 1.45f, 0.46f, 0.19f},
    {{1.9f, 0.15f, -9.2f}, 1.45f, 0.46f, 0.19f}
};

Camera gCamera = {{0.0f, 0.9f, 2.2f}, 0.0f, -0.15f, 58.0f};

std::vector<Sphere> gSpheres = {
    {{-2.2f, -0.15f, -6.2f}, 0.85f, {0.85f, 0.40f, 0.20f}},
    {{0.1f, -0.25f, -5.6f}, 0.75f, {0.25f, 0.72f, 0.92f}},
    {{2.4f, 0.00f, -7.5f}, 0.95f, {0.95f, 0.84f, 0.28f}}
};

std::vector<Aabb> gBoxes = {
    {{-0.9f, -0.30f, -3.9f}, {0.55f, 0.55f, 0.55f}, {0.90f, 0.30f, 0.28f}},
    {{1.1f, -0.50f, -6.4f}, {0.80f, 0.35f, 0.70f}, {0.38f, 0.88f, 0.40f}},
    {{3.1f, -0.60f, -10.0f}, {0.50f, 0.25f, 0.50f}, {0.74f, 0.74f, 0.80f}}
};

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
Vec3 P0 = {0.0f, 0.9f, 2.2f};
Vec3 P1 = {-1.6f, 0.45f, -1.0f};
Vec3 P2 = {-1.6f, 0.45f, -4.2f};
Vec3 P3 = {1.9f, 0.15f, -9.2f};

GLuint myTexture = 0;
GLuint gTexSky = 0;
GLUquadric* sphereQuadric = nullptr;
