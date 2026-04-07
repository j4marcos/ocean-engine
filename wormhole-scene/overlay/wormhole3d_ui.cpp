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

    // Define a margem esquerda fixa para todos os textos
    const int leftX = 18;

    // -------------------------------------------------------------
    // BLOCO SUPERIOR ESQUERDO (Status e Infos)
    // -------------------------------------------------------------
    int currentTopY = gWindowHeight - 22; 
    const int topSpacing = 16;

    // Lambda que desenha e DESCE o cursor 'Y'
    auto printTopLeft = [&](float r, float g, float b, const char* text) {
        glColor3f(r, g, b);
        drawText(leftX, currentTopY, text);
        currentTopY -= topSpacing; 
    };

    // 1. FPS e Modo
    char modeBuf[32];
    buildModeLabel(modeBuf, static_cast<int>(sizeof(modeBuf)));
    char fpsBuf[64];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f  |  Modo: %s", gFpsDisplay, modeBuf);
    printTopLeft(0.95f, 0.95f, 0.95f, fpsBuf);

    // 2. Tempo
    DayNightLighting dn{};
    computeDayNightLighting(dn);
    const int totalSec = gSceneTimeMs / 1000;
    const int hh = totalSec / 3600;
    const int remH = totalSec % 3600;
    const int mm = remH / 60;
    const int ss = remH % 60;
    const char* skyLabel = dn.skyDayFactor >= 0.42f ? "dia" : "noite";
    
    char timeBuf[128];
    std::snprintf(timeBuf, sizeof(timeBuf), "Tempo %02d:%02d:%02d  %s   Ciclo=%d",
        hh, mm, ss, skyLabel, gDayNightEffectiveMs);
    printTopLeft(0.82f, 0.86f, 0.98f, timeBuf);

    // 3. Sol
    char solBuf[40];
    std::snprintf(solBuf, sizeof(solBuf), "Sol: %s  [ / ]  T", gDayNightAuto ? "auto" : "manual");
    printTopLeft(0.72f, 0.78f, 0.95f, solBuf);

    // 4. Veículos
    char vehBuf[48];
    std::snprintf(vehBuf, sizeof(vehBuf), "Carros/barcos: %s  V", gSceneVehiclesEnabled ? "on" : "off");
    printTopLeft(0.78f, 0.82f, 0.96f, vehBuf);


    // -------------------------------------------------------------
    // BLOCO INFERIOR ESQUERDO (Apenas Controles)
    // -------------------------------------------------------------
    int helpY = 14; // Posição Y inicial (perto do chão)
    const int helpSpacing = 14;

    // Lambda que desenha e SOBE o cursor 'Y'
    auto printBottomLeft = [&](const char* text) {
        glColor3f(0.92f, 0.92f, 0.92f);
        drawText(leftX, helpY, text);
        helpY += helpSpacing; 
    };

    printBottomLeft("WASD mover  |  Z/X descer/subir  |  Setas olhar");
    printBottomLeft("1 Raster  |  2 CPU  |  3 GPU");
    printBottomLeft("4 Bezier  |  R/F warp +/-");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
