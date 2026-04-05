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

void buildModeLabel(char* buf, const int cap) {
    char* p = buf;
    char* const end = buf + cap - 1;

    if (!gUseRaycast) {
        if (p < end) {
            *p++ = 'R';
        }
    } else {
        if (p < end) {
            *p++ = 'T';
        }
        if (gRaycastGpuReady && gUseGpuRaycast && p < end) {
            *p++ = ' ';
            *p++ = 'G';
        }
    }

    if (gAnimatingCamera && p < end) {
        *p++ = ' ';
        *p++ = 'C';
    }

    *p = '\0';
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

    const int hudY = gWindowHeight - 22;

    char modeBuf[32];
    buildModeLabel(modeBuf, static_cast<int>(sizeof(modeBuf)));
    glColor3f(0.88f, 0.90f, 0.98f);
    drawText(18, hudY, modeBuf);

    char solBuf[40];
    std::snprintf(solBuf, sizeof(solBuf), "Sol: %s  [ / ]  T",
        gDayNightAuto ? "auto" : "manual");
    glColor3f(0.72f, 0.78f, 0.95f);
    drawText(18, hudY - 16, solBuf);

    char fpsBuf[48];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", gFpsDisplay);
    glColor3f(0.55f, 0.95f, 0.55f);
    drawText(gWindowWidth - 130, hudY, fpsBuf);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
