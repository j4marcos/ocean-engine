// scene_textures.cpp — Implementação do sistema de texturas para OpenGL 2.x
// Utiliza stb_image (LDR: JPG/PNG) e tinyexr (HDR: .exr) para carregamento.
// Todos os paths são absolutos, baseados no diretório de assets do projeto.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Usa zlib do sistema em vez de miniz bundled (miniz.h não incluído no repo)
#define TINYEXR_USE_MINIZ 0
#include <zlib.h>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include "scene_textures.h"

#include <GL/gl.h>
#include <GL/glx.h>   // glXGetProcAddressARB
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ─── Definição das variáveis globais declaradas no .h ─────────────────────────
GLuint gTexBrickDiffuse   = 0;
GLuint gTexBrickNormal    = 0;
GLuint gTexBrickDisp      = 0;
GLuint gTexTerrainDiffuse = 0;
GLuint gTexTerrainDetail  = 0;
GLuint gTexRockyDiffuse   = 0;
GLuint gTexRockyNormal    = 0;

// ─── Helpers internos ─────────────────────────────────────────────────────────

static bool sExtChecked      = false;
static bool sHasFloatTex     = false;
static bool sHasFBO          = false;
static bool sHasAnisotropic  = false;

static void checkExtensions() {
    if (sExtChecked) return;
    sExtChecked = true;
    const char* exts = (const char*)glGetString(GL_EXTENSIONS);
    if (!exts) return;
    sHasFloatTex    = (strstr(exts, "GL_ARB_texture_float")            != nullptr);
    sHasFBO         = (strstr(exts, "GL_EXT_framebuffer_object")        != nullptr);
    sHasAnisotropic = (strstr(exts, "GL_EXT_texture_filter_anisotropic") != nullptr);
}

// Gera mipmaps de forma compatível com OpenGL 2.x
// Deve ser chamado APÓS glTexImage2D.
static void generateMipmaps() {
    checkExtensions();
    if (sHasFBO) {
        // glGenerateMipmapEXT disponível via EXT_framebuffer_object
        typedef void (*GenMipFn)(GLenum);
        auto fn = (GenMipFn)glXGetProcAddressARB((const GLubyte*)"glGenerateMipmapEXT");
        if (fn) { fn(GL_TEXTURE_2D); return; }
    }
    // Fallback: driver gera mipmaps automaticamente no próximo upload
    // (setado ANTES do glTexImage2D via GL_GENERATE_MIPMAP)
}

// ─── Carregamento LDR ─────────────────────────────────────────────────────────

bool loadTextureLDR(const char* path, GLuint& outTex,
                    GLint wrapMode, bool doMips)
{
    checkExtensions();

    int w, h, channels;
    stbi_set_flip_vertically_on_load(false);

    unsigned char* data = stbi_load(path, &w, &h, &channels, 0);
    if (!data) {
        fprintf(stderr, "[Texture] ERRO: nao foi possivel carregar '%s': %s\n",
                path, stbi_failure_reason());
        outTex = 0;
        return false;
    }

    const GLenum fmt = (channels == 4) ? GL_RGBA :
                       (channels == 3) ? GL_RGB  :
                       (channels == 2) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    doMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    if (sHasAnisotropic) {
        float maxAniso = 1.0f;
        glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &maxAniso);
        if (maxAniso > 8.0f) maxAniso = 8.0f;
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/, maxAniso);
    }

    if (doMips && !sHasFBO) {
        // Fallback: pede ao driver para gerar mipmaps ao fazer upload
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0,
                 fmt, GL_UNSIGNED_BYTE, data);

    if (doMips && sHasFBO) {
        generateMipmaps();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    printf("[Texture] OK: %s (%dx%d, %dch)\n", path, w, h, channels);
    return true;
}

// ─── Carregamento EXR (normal map float) ──────────────────────────────────────

bool loadEXRNormalMap(const char* exrPath, const char* fallbackJpgPath,
                      GLuint& outTex)
{
    checkExtensions();

    float* rgba = nullptr;
    int w = 0, h = 0;
    const char* err = nullptr;

    int ret = LoadEXR(&rgba, &w, &h, exrPath, &err);
    if (ret != TINYEXR_SUCCESS) {
        fprintf(stderr, "[EXR] Falha em '%s': %s — usando fallback JPG.\n",
                exrPath, err ? err : "erro desconhecido");
        FreeEXRErrorMessage(err);
        return loadTextureLDR(fallbackJpgPath, outTex, GL_REPEAT, true);
    }

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (sHasFloatTex) {
        // Empacota RGB32F (descarta canal alpha do EXR)
        std::vector<float> rgb3(static_cast<size_t>(w * h * 3));
        for (int i = 0; i < w * h; ++i) {
            rgb3[static_cast<size_t>(i * 3 + 0)] = rgba[static_cast<size_t>(i * 4 + 0)];
            rgb3[static_cast<size_t>(i * 3 + 1)] = rgba[static_cast<size_t>(i * 4 + 1)];
            rgb3[static_cast<size_t>(i * 3 + 2)] = rgba[static_cast<size_t>(i * 4 + 2)];
        }
        // GL_RGB32F_ARB = 0x8815
        glTexImage2D(GL_TEXTURE_2D, 0, 0x8815, w, h, 0,
                     GL_RGB, GL_FLOAT, rgb3.data());
        printf("[EXR] Float32 normal map: %s (%dx%d)\n", exrPath, w, h);
    } else {
        // Fallback 8-bit: converte float→uchar (perde precisão, mas funciona)
        fprintf(stderr, "[EXR] GL_ARB_texture_float ausente; convertendo EXR para 8-bit.\n");
        std::vector<unsigned char> rgb8(static_cast<size_t>(w * h * 3));
        for (int i = 0; i < w * h; ++i) {
            auto clamp01 = [](float v) -> float {
                return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            };
            rgb8[static_cast<size_t>(i * 3 + 0)] = (unsigned char)(clamp01(rgba[static_cast<size_t>(i * 4 + 0)]) * 255.0f);
            rgb8[static_cast<size_t>(i * 3 + 1)] = (unsigned char)(clamp01(rgba[static_cast<size_t>(i * 4 + 1)]) * 255.0f);
            rgb8[static_cast<size_t>(i * 3 + 2)] = (unsigned char)(clamp01(rgba[static_cast<size_t>(i * 4 + 2)]) * 255.0f);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, rgb8.data());
        printf("[EXR] 8-bit fallback normal map: %s (%dx%d)\n", exrPath, w, h);
    }

    generateMipmaps();
    glBindTexture(GL_TEXTURE_2D, 0);
    free(rgba);
    return true;
}

// ─── Carregamento completo da cena ────────────────────────────────────────────

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

void loadSceneTextures() {
    printf("[Texture] Carregando texturas da cena...\n");

    const std::string brickDiffuse = joinAssetPath("red_brick_03_1k.blend/textures/red_brick_03_diff_1k.jpg");
    const std::string brickNormalExr = joinAssetPath("red_brick_03_1k.blend/textures/red_brick_03_nor_gl_1k.exr");
    const std::string brickDisp = joinAssetPath("red_brick_03_1k.blend/textures/red_brick_03_disp_1k.png");
    const std::string terrainColor = joinAssetPath("Terrain002_2K-JPG/Terrain002_2K_Color.jpg");
    const std::string terrainDetail = joinAssetPath("Terrain002_2K-JPG/Terrain002_2K_Details.jpg");
    const std::string rockyDiffuse = joinAssetPath("rocky_terrain_02_1k/textures/rocky_terrain_02_diff_1k.jpg");
    const std::string rockyNormalExr = joinAssetPath("rocky_terrain_02_1k/textures/rocky_terrain_02_nor_gl_1k.exr");

    // ── Prédios (tijolos vermelhos) ──────────────────────────────────────────
    loadTextureLDR(
        brickDiffuse.c_str(),
        gTexBrickDiffuse, GL_REPEAT, true);

    loadEXRNormalMap(
        brickNormalExr.c_str(),
        brickDiffuse.c_str(),
        gTexBrickNormal);

    loadTextureLDR(
        brickDisp.c_str(),
        gTexBrickDisp, GL_REPEAT, true);

    // ── Montanhas / terreno (Terrain002 2K) ──────────────────────────────────
    loadTextureLDR(
        terrainColor.c_str(),
        gTexTerrainDiffuse, GL_REPEAT, true);

    loadTextureLDR(
        terrainDetail.c_str(),
        gTexTerrainDetail, GL_REPEAT, true);

    // ── Terreno pedregoso (rocky_terrain 1k) ─────────────────────────────────
    loadTextureLDR(
        rockyDiffuse.c_str(),
        gTexRockyDiffuse, GL_REPEAT, true);

    loadEXRNormalMap(
        rockyNormalExr.c_str(),
        rockyDiffuse.c_str(),
        gTexRockyNormal);

    printf("[Texture] Carregamento concluido.\n");
}

// ─── Liberação ────────────────────────────────────────────────────────────────

void freeSceneTextures() {
    GLuint ids[] = {
        gTexBrickDiffuse, gTexBrickNormal,  gTexBrickDisp,
        gTexTerrainDiffuse, gTexTerrainDetail,
        gTexRockyDiffuse,  gTexRockyNormal
    };
    glDeleteTextures(7, ids);
    gTexBrickDiffuse   = 0;
    gTexBrickNormal    = 0;
    gTexBrickDisp      = 0;
    gTexTerrainDiffuse = 0;
    gTexTerrainDetail  = 0;
    gTexRockyDiffuse   = 0;
    gTexRockyNormal    = 0;
    printf("[Texture] Texturas liberadas.\n");
}
