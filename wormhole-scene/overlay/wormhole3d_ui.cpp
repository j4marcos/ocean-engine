#include "wormhole3d_ui.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"

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
            *p++ = 'Raster';
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
        *p++ = '-';
        *p++ = 'M';
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

    DayNightLighting dn{};
    computeDayNightLighting(dn);
    const int totalSec = gSceneTimeMs / 1000;
    const int hh = totalSec / 3600;
    const int remH = totalSec % 3600;
    const int mm = remH / 60;
    const int ss = remH % 60;
    const int dayPct = static_cast<int>(dn.skyDayFactor * 100.0f + 0.5f);
    const char* skyLabel = dn.skyDayFactor >= 0.42f ? "dia" : "noite";
    char timeBuf[72];
    std::snprintf(
        timeBuf,
        sizeof(timeBuf),
        "Tempo %02d:%02d:%02d  %s  (%d%%)",
        hh,
        mm,
        ss,
        skyLabel,
        dayPct);
    glColor3f(0.82f, 0.86f, 0.98f);
    drawText(18, hudY - 32, timeBuf);

    char fpsBuf[48];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", gFpsDisplay);
    glColor3f(0.55f, 0.95f, 0.55f);
    drawText(gWindowWidth - 130, hudY, fpsBuf);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
