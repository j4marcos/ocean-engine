#pragma once

#include <cstddef>
#include <vector>

#include <GL/gl.h>

// Textura 2D float RGBA: 3 linhas x kSceneDataWidth colunas (uma coluna por objeto).
/** Colunas na textura (esferas primeiro, depois caixas). Aumentado para caber cena + primitivas dinâmicas. */
inline constexpr int kSceneDataWidth = 128;
inline constexpr int kSceneDataHeight = 3;

struct SceneGpuPacked {
    int objectCount = 0;
    std::vector<float> pixels; // (kSceneDataWidth * kSceneDataHeight * 4) floats
};

// Preenche pixels a partir de gSpheres / gBoxes (fonte única em wormhole3d_globals).
void packSceneObjectsForGpu(SceneGpuPacked& out);

// Upload RGBA32F (requer GL_ARB_texture_float).
void uploadSceneDataTexture(GLuint tex, const float* pixels);
