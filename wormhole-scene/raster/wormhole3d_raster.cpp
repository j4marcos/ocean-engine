#include "wormhole3d_raster.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"
#include "wormhole3d_portals.h"
#include "scene_prefabs.h"
#include "scene_textures.h"

#include <GL/glut.h>

// ─── Helpers de classificação de caixas ───────────────────────────────────────
// Retorna true para postes finos (halfSize.x/z < 0.12)
static inline bool isBoxThinPole(const Aabb& b) {
    return b.halfSize.x < 0.12f && b.halfSize.z < 0.12f;
}
// Retorna true para lajes finas (areia, asfalto, folhagem)
static inline bool isBoxSlab(const Aabb& b) {
    return b.halfSize.y < 0.08f;
}
// Retorna true para objetos minúsculos (carros em pose inicial, placas de farol)
static inline bool isBoxTiny(const Aabb& b) {
    return b.halfSize.x < 0.01f && b.halfSize.y < 0.01f && b.halfSize.z < 0.01f;
}
// Retorna true para prédio: x/z moderados e altura > 1.5
static inline bool isBuilding(const Aabb& b) {
    return b.halfSize.x >= 1.0f && b.halfSize.x <= 3.0f
        && b.halfSize.z >= 1.0f && b.halfSize.z <= 3.0f
        && b.halfSize.y > 1.5f;
}
// Retorna true para montanhas: footprint grande
static inline bool isMountain(const Aabb& b) {
    return b.halfSize.x > 5.0f && b.halfSize.y > 1.0f;
}

// ─── Desenho de caixa com UV tiling manual ────────────────────────────────────
// O cubo é desenhado com halfSizes corretos e UVs proporcionais à dimensão real.
// tileM: quantos metros reais equivalem a 1 repetição UV (tile).
static void drawTexturedBox(const Aabb& box, float tileM) {
    const float hx = box.halfSize.x;
    const float hy = box.halfSize.y;
    const float hz = box.halfSize.z;

    // Escala UV por face: dimensão_real / tileM
    const float uX = hx * 2.0f / tileM;
    const float uY = hy * 2.0f / tileM;
    const float uZ = hz * 2.0f / tileM;

    glBegin(GL_QUADS);

    // +X (direita)
    glTexCoord2f(0.0f, 0.0f); glVertex3f( hx, -hy,  hz);
    glTexCoord2f(uZ,   0.0f); glVertex3f( hx, -hy, -hz);
    glTexCoord2f(uZ,   uY  ); glVertex3f( hx,  hy, -hz);
    glTexCoord2f(0.0f, uY  ); glVertex3f( hx,  hy,  hz);

    // -X (esquerda)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-hx, -hy, -hz);
    glTexCoord2f(uZ,   0.0f); glVertex3f(-hx, -hy,  hz);
    glTexCoord2f(uZ,   uY  ); glVertex3f(-hx,  hy,  hz);
    glTexCoord2f(0.0f, uY  ); glVertex3f(-hx,  hy, -hz);

    // +Z (frente)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-hx, -hy,  hz);
    glTexCoord2f(uX,   0.0f); glVertex3f( hx, -hy,  hz);
    glTexCoord2f(uX,   uY  ); glVertex3f( hx,  hy,  hz);
    glTexCoord2f(0.0f, uY  ); glVertex3f(-hx,  hy,  hz);

    // -Z (trás)
    glTexCoord2f(0.0f, 0.0f); glVertex3f( hx, -hy, -hz);
    glTexCoord2f(uX,   0.0f); glVertex3f(-hx, -hy, -hz);
    glTexCoord2f(uX,   uY  ); glVertex3f(-hx,  hy, -hz);
    glTexCoord2f(0.0f, uY  ); glVertex3f( hx,  hy, -hz);

    // +Y (topo)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-hx,  hy, -hz);
    glTexCoord2f(uX,   0.0f); glVertex3f( hx,  hy, -hz);
    glTexCoord2f(uX,   uZ  ); glVertex3f( hx,  hy,  hz);
    glTexCoord2f(0.0f, uZ  ); glVertex3f(-hx,  hy,  hz);

    // -Y (base)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-hx, -hy,  hz);
    glTexCoord2f(uX,   0.0f); glVertex3f( hx, -hy,  hz);
    glTexCoord2f(uX,   uZ  ); glVertex3f( hx, -hy, -hz);
    glTexCoord2f(0.0f, uZ  ); glVertex3f(-hx, -hy, -hz);

    glEnd();
}

// ─── Desenha todas as caixas (com textura ou cor sólida) ──────────────────────
// Usado em rasterScene() e renderPortalView()
static void drawAllBoxes() {
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const Aabb& box = gBoxes[i];
        const RGBA& c   = box.color;

        glPushMatrix();
        glTranslatef(box.center.x, box.center.y, box.center.z);

        if (isBoxTiny(box)) {
            // Objetos invisíveis (carros na posição de espera) — não renderiza
            glPopMatrix();
            continue;
        }

        if (isBoxThinPole(box)) {
            // Postes finos: cor sólida levemente clara
            glDisable(GL_TEXTURE_2D);
            glColor3f(c.r * 0.85f + 0.15f, c.g * 0.85f + 0.15f, c.b * 0.85f + 0.15f);
            glScalef(box.halfSize.x * 2.0f, box.halfSize.y * 2.0f, box.halfSize.z * 2.0f);
            glutSolidCube(1.0);
            glPopMatrix();
            continue;
        }

        if (isBoxSlab(box)) {
            // Lajes (areia, asfalto, folhagem de árvore): cor sólida
            glDisable(GL_TEXTURE_2D);
            glColor3f(c.r, c.g, c.b);
            glScalef(box.halfSize.x * 2.0f, box.halfSize.y * 2.0f, box.halfSize.z * 2.0f);
            glutSolidCube(1.0);
            glPopMatrix();
            continue;
        }

        if (isBuilding(box) && gTexBrickDiffuse != 0) {
            // Prédio: textura de tijolo, tiling de ~1.5 m por tile
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, gTexBrickDiffuse);
            glColor3f(1.0f, 1.0f, 1.0f);   // white → a textura define a cor
            drawTexturedBox(box, 1.5f);
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();
            continue;
        }

        if (isMountain(box) && gTexTerrainDiffuse != 0) {
            // Montanha: textura de terreno, tiling de ~4 m por tile (triplanar implícito via tileM)
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, gTexTerrainDiffuse);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawTexturedBox(box, 4.0f);
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();
            continue;
        }

        // Fallback: cor sólida (barco, tronco de árvore, farol, etc.)
        glDisable(GL_TEXTURE_2D);
        glColor3f(c.r, c.g, c.b);
        glScalef(box.halfSize.x * 2.0f, box.halfSize.y * 2.0f, box.halfSize.z * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

// ─── rasterScene ──────────────────────────────────────────────────────────────
void rasterScene() {

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ciclo dia/noite
    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float day = dn.skyDayFactor;
    glClearColor(0.02f + 0.14f * day, 0.03f + 0.18f * day, 0.06f + 0.24f * day, 1.0f);

    // câmera
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(gCamera.fovViewDegree,
                   static_cast<double>(gWindowWidth) / static_cast<double>(gWindowHeight),
                   0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const Vec3 f      = rayForward();
    const Vec3 target = add3(gCamera.position, f);
    gluLookAt(gCamera.position.x, gCamera.position.y, gCamera.position.z,
              target.x, target.y, target.z,
              0.0, 1.0, 0.0);

    // chão — cor sólida
    glDisable(GL_TEXTURE_2D);
    glColor3f(kSceneFloorMaterial.r, kSceneFloorMaterial.g, kSceneFloorMaterial.b);
    const float y = kSceneGroundY;
    const float e = 80.0f;
    glBegin(GL_QUADS);
    glVertex3f(-e, y, -e);
    glVertex3f( e, y, -e);
    glVertex3f( e, y,  e);
    glVertex3f(-e, y,  e);
    glEnd();

    // esferas — cor sólida
    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const RGBA& c = gSpheres[i].color;
        glColor3f(c.r, c.g, c.b);
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    // caixas — texturizadas ou cor sólida conforme tipo
    drawAllBoxes();

    // esfera de distorção do wormhole — wireframe
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColor3f(0.08f, 0.60f, 0.95f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutWireSphere(gWormhole.holeA.warpRadius, 24, 24);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutWireSphere(gWormhole.holeB.warpRadius, 24, 24);
    glPopMatrix();
    glDepthMask(GL_TRUE);

    // portais FBO
    renderPortals();
}
