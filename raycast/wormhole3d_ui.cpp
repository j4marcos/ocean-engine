#include "wormhole3d_ui.h"
#include "wormhole3d_globals.h"

#include <cstdio>

#include <GL/glut.h>

namespace {

void drawText(const int x, const int y, const char* text) {
    glRasterPos2i(x, y);
    for (const char* p = text; *p != '\0'; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
    }
}

} // namespace

void drawOverlay() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.18f, 0.18f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2i(gButtonX, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY + gButtonH);
    glVertex2i(gButtonX, gButtonY + gButtonH);
    glEnd();

    glColor3f(0.70f, 0.70f, 0.74f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(gButtonX, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY + gButtonH);
    glVertex2i(gButtonX, gButtonY + gButtonH);
    glEnd();

    glColor3f(0.95f, 0.95f, 0.95f);
    if (gUseRaycast) {
        drawText(gButtonX + 12, gButtonY + 12, "Modo: Raycast 3D (clique p/ Rasterizacao)");
    } else {
        drawText(gButtonX + 12, gButtonY + 12, "Modo: Rasterizacao Leve (clique para Raycast)");
    }

    const int gpuBtnY = gButtonY + gButtonH + kGpuCpuButtonGap;
    if (gUseRaycast && gRaycastGpuReady) {
        glColor3f(0.18f, 0.18f, 0.20f);
        glBegin(GL_QUADS);
        glVertex2i(gButtonX, gpuBtnY);
        glVertex2i(gButtonX + gButtonW, gpuBtnY);
        glVertex2i(gButtonX + gButtonW, gpuBtnY + gButtonH);
        glVertex2i(gButtonX, gpuBtnY + gButtonH);
        glEnd();
        glColor3f(0.70f, 0.70f, 0.74f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(gButtonX, gpuBtnY);
        glVertex2i(gButtonX + gButtonW, gpuBtnY);
        glVertex2i(gButtonX + gButtonW, gpuBtnY + gButtonH);
        glVertex2i(gButtonX, gpuBtnY + gButtonH);
        glEnd();
        glColor3f(0.95f, 0.95f, 0.95f);
        drawText(
            gButtonX + 12,
            gpuBtnY + 12,
            gUseGpuRaycast ? "Raycast: GPU (clique p/ CPU)" : "Raycast: CPU (clique p/ GPU)");
    } else if (gUseRaycast && !gRaycastGpuReady) {
        glColor3f(0.65f, 0.65f, 0.68f);
        drawText(gButtonX + 12, gpuBtnY + 12, "Raycast: apenas CPU (GPU indisponivel)");
    }

    glColor3f(0.82f, 0.84f, 0.88f);
    drawText(18, gpuBtnY + gButtonH + 14, "WASD: mover | Setas: olhar | C: Bezier | T: Raycast | botao: GPU/CPU");

    char fpsBuf[48];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", gFpsDisplay);
    glColor3f(0.55f, 0.95f, 0.55f);
    drawText(gWindowWidth - 130, gWindowHeight - 22, fpsBuf);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
