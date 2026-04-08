// cpu_texture.cpp — Implementação do sampler CPU para o ray tracer

#include "cpu_texture.h"
#include "stb_image.h"   // já tem STB_IMAGE_IMPLEMENTATION em scene_textures.cpp

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>

// ─── Globals ──────────────────────────────────────────────────────────────────
CpuTexture gCpuBrickDiffuse;
CpuTexture gCpuTerrainDiffuse;

// ─── Carregamento ─────────────────────────────────────────────────────────────

static const char* getAssetsBase() {
    const char* env = std::getenv("WORMHOLE_ASSETS_DIR");
    return (env && env[0]) ? env : "assets";
}

static std::string joinAssetPath(const char* relativePath) {
    std::string path = getAssetsBase();
    if (!path.empty() && path.back() != '/') {
        path.push_back('/');
    }
    path += relativePath;
    return path;
}

static bool loadCpuTex(const char* path, CpuTexture& out) {
    int w, h, ch;
    stbi_set_flip_vertically_on_load(false);
    uint8_t* raw = stbi_load(path, &w, &h, &ch, 3); // força 3 canais RGB
    if (!raw) {
        fprintf(stderr, "[CpuTex] ERRO: '%s': %s\n", path, stbi_failure_reason());
        return false;
    }
    out.width    = w;
    out.height   = h;
    out.channels = 3;
    out.data.assign(raw, raw + static_cast<size_t>(w * h * 3));
    stbi_image_free(raw);
    printf("[CpuTex] OK: %s (%dx%d)\n", path, w, h);
    return true;
}

void loadCpuTextures() {
    const std::string brickDiffuse = joinAssetPath("red_brick_03_1k.blend/textures/red_brick_03_diff_1k.jpg");
    const std::string terrainColor = joinAssetPath("Terrain002_2K-JPG/Terrain002_2K_Color.jpg");

    loadCpuTex(
        brickDiffuse.c_str(),
        gCpuBrickDiffuse);
    loadCpuTex(
        terrainColor.c_str(),
        gCpuTerrainDiffuse);
}

void freeCpuTextures() {
    gCpuBrickDiffuse  = CpuTexture{};
    gCpuTerrainDiffuse = CpuTexture{};
}

// ─── Sampler bilinear ─────────────────────────────────────────────────────────

CpuTexSample sampleTexBilinear(const CpuTexture& tex, float u, float v) {
    if (!tex.loaded()) return {1.0f, 1.0f, 1.0f};

    // GL_REPEAT
    u = u - std::floor(u);
    v = v - std::floor(v);

    const float px = u * static_cast<float>(tex.width  - 1);
    const float py = v * static_cast<float>(tex.height - 1);
    const int   x0 = static_cast<int>(px);
    const int   y0 = static_cast<int>(py);
    const int   x1 = (x0 + 1) % tex.width;
    const int   y1 = (y0 + 1) % tex.height;
    const float fx = px - static_cast<float>(x0);
    const float fy = py - static_cast<float>(y0);

    auto fetch = [&](int x, int y) -> CpuTexSample {
        const size_t idx = static_cast<size_t>((y * tex.width + x) * 3);
        return {
            tex.data[idx + 0] / 255.0f,
            tex.data[idx + 1] / 255.0f,
            tex.data[idx + 2] / 255.0f
        };
    };

    const CpuTexSample c00 = fetch(x0, y0);
    const CpuTexSample c10 = fetch(x1, y0);
    const CpuTexSample c01 = fetch(x0, y1);
    const CpuTexSample c11 = fetch(x1, y1);

    // Bilinear interpolation
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    return {
        lerp(lerp(c00.r, c10.r, fx), lerp(c01.r, c11.r, fx), fy),
        lerp(lerp(c00.g, c10.g, fx), lerp(c01.g, c11.g, fx), fy),
        lerp(lerp(c00.b, c10.b, fx), lerp(c01.b, c11.b, fx), fy)
    };
}

// ─── UV mapping por face de AABB ──────────────────────────────────────────────
//
// Detecta a face pelo componente dominante de (p - center) / halfSize.
// Face Y: topo/base → UV no plano XZ
// Face X: lateral   → UV no plano ZY
// Face Z: frontal   → UV no plano XY
// O tiling é proporcional à dimensão real da face dividida por tileM.

void aabbSurfaceUV(
    float px, float py, float pz,
    float cx, float cy, float cz,
    float hx, float hy, float hz,
    float tileM,
    float& outU, float& outV)
{
    // Coordenada local normalizada [-1, 1]
    const float lx = (hx > 1e-6f) ? (px - cx) / hx : 0.0f;
    const float ly = (hy > 1e-6f) ? (py - cy) / hy : 0.0f;
    const float lz = (hz > 1e-6f) ? (pz - cz) / hz : 0.0f;

    const float ax = std::fabs(lx);
    const float ay = std::fabs(ly);
    const float az = std::fabs(lz);

    float u, v;

    if (ax >= ay && ax >= az) {
        // Face X: mapeia Z e Y
        u = (lz + 1.0f) * 0.5f * (hz * 2.0f / tileM);
        v = (ly + 1.0f) * 0.5f * (hy * 2.0f / tileM);
    } else if (ay >= ax && ay >= az) {
        // Face Y: mapeia X e Z
        u = (lx + 1.0f) * 0.5f * (hx * 2.0f / tileM);
        v = (lz + 1.0f) * 0.5f * (hz * 2.0f / tileM);
    } else {
        // Face Z: mapeia X e Y
        u = (lx + 1.0f) * 0.5f * (hx * 2.0f / tileM);
        v = (ly + 1.0f) * 0.5f * (hy * 2.0f / tileM);
    }

    outU = u;
    outV = v;
}
