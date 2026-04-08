// cpu_texture.h — Texturas CPU-side para o ray tracer (modo 2 — CPU)
// Carregamento via stb_image; sample bilinear com GL_REPEAT implícito.
// Usado em simulation.cpp::sceneColorAtBvh() para colorir prédios e montanhas.

#pragma once

#include <vector>
#include <cstdint>

struct CpuTexture {
    std::vector<uint8_t> data;
    int width    = 0;
    int height   = 0;
    int channels = 0;

    bool loaded() const { return width > 0 && height > 0 && !data.empty(); }
};

// ─── Texturas globais CPU-side ─────────────────────────────────────────────────
// Carregadas uma vez em loadCpuTextures() — espelhos das GPU textures.
extern CpuTexture gCpuBrickDiffuse;    // red_brick_03_diff_1k.jpg
extern CpuTexture gCpuTerrainDiffuse;  // Terrain002_2K_Color.jpg

// Carrega todas as texturas CPU (deve ser chamado junto com loadSceneTextures()).
void loadCpuTextures();
void freeCpuTextures();

// ─── Sampler bilinear com GL_REPEAT ───────────────────────────────────────────
// u,v podem estar em qualquer range (repeat automático via fmod).
// Retorna {r,g,b} no range [0,1].
struct CpuTexSample { float r, g, b; };
CpuTexSample sampleTexBilinear(const CpuTexture& tex, float u, float v);

// ─── UV mapping para AABB ──────────────────────────────────────────────────────
// Calcula (u,v) para um ponto `p` na superfície de uma AABB com centro `center`
// e halfSize `hs`. `tileM`: metros por tile UV (para repetição proporcional).
// Detecta a face automaticamente pelo componente dominante da normal SDF.
void aabbSurfaceUV(
    float px, float py, float pz,
    float cx, float cy, float cz,
    float hx, float hy, float hz,
    float tileM,
    float& outU, float& outV
);
