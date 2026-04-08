#pragma once

#include <GL/gl.h>

/** Tamanho da textura do portal (quadrado). Mantido pequeno para performance. */
inline constexpr int kPortalTexSize = 256;

/** Estado do frame buffer do portal. */
struct PortalFBO {
    GLuint fbo = 0;
    GLuint tex = 0;
    GLuint depth_rb = 0;
};

/** Inicializa os dois FBOs de portal (A→B e B→A). Retorna true se OK. */
bool initPortalFBOs(PortalFBO& a, PortalFBO& b);

/** Libera recursos dos FBOs. */
void destroyPortalFBO(const PortalFBO& p);

/** Renderiza a cena de dentro do wormhole `idx` olhando para o outro, capturando no FBO. */
void renderPortalView(int idx, const PortalFBO& fbo);

/** Desenha um disco billboard (vira para a câmera) no centro do portal, pintado com a textura do FBO. */
void drawPortalBillboard(int idx, GLuint portalTex);

/** Renderiza ambos os portais antes da cena normal (modo raster). */
void renderPortals();
