// scene_textures.h — Carregamento e gerenciamento de texturas para OpenGL 2.x
// Usa stb_image (já disponível em texture/stb_image.h) e tinyexr para .exr

#pragma once

#include <GL/gl.h>
#include <string>

// ─── IDs das texturas globais ─────────────────────────────────────────────────
// Texturas para prédios (tijolos)
extern GLuint gTexBrickDiffuse;   // red_brick_03_diff_1k.jpg
extern GLuint gTexBrickNormal;    // red_brick_03_nor_gl_1k.exr  (fallback: .png)
extern GLuint gTexBrickDisp;      // red_brick_03_disp_1k.png

// Texturas para montanhas / terreno
extern GLuint gTexTerrainDiffuse; // Terrain002_2K_Color.jpg
extern GLuint gTexTerrainDetail;  // Terrain002_2K_Details.jpg
extern GLuint gTexRockyDiffuse;   // rocky_terrain_02_diff_1k.jpg
extern GLuint gTexRockyNormal;    // rocky_terrain_02_nor_gl_1k.exr (fallback)

// ─── Funções públicas ─────────────────────────────────────────────────────────

/**
 * Carrega uma textura LDR (JPEG / PNG) com stb_image e envia para a GPU.
 * Parâmetros:
 *   path          — caminho absoluto do arquivo
 *   outTex        — id de textura OpenGL gerado (0 em caso de erro)
 *   wrapMode      — GL_REPEAT (tiling) ou GL_CLAMP_TO_EDGE
 *   generateMips  — true para glGenerateMipmapEXT / fallback manual
 * Retorna true em sucesso.
 */
bool loadTextureLDR(const char* path, GLuint& outTex,
                    GLint wrapMode = GL_REPEAT,
                    bool generateMips = true);

/**
 * Carrega um normal map .exr (16/32 bit float) com tinyexr e envia como
 * GL_RGB16F_ARB (ou GL_RGB se GL_ARB_texture_float não estiver disponível).
 * Fallback: tenta também versão .jpg caso o .exr falhe.
 */
bool loadEXRNormalMap(const char* exrPath, const char* fallbackJpgPath,
                      GLuint& outTex);

/**
 * Carrega todas as texturas da cena em uma única chamada.
 * Deve ser chamado após a criação do contexto OpenGL (ex: em initScene()).
 */
void loadSceneTextures();

/**
 * Libera todas as texturas carregadas (chamar no encerramento).
 */
void freeSceneTextures();
