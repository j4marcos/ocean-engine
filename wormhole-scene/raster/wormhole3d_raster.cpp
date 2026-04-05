#include "wormhole3d_raster.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"

#include <cmath>
#include <GL/glut.h>

namespace {

float islandHeight(const float x, const float z) {
    const float d = x * x + z * z;
    const float mask = std::exp(-d * 0.007f);
    return -1.15f + 0.5f * mask * std::sin(x * 0.42f) * std::cos(z * 0.36f);
}

void islandNormal(const float x, const float z, float* nx, float* ny, float* nz) {
    const float e = 0.05f;
    const float dx = (islandHeight(x + e, z) - islandHeight(x - e, z)) * (0.5f / e);
    const float dz = (islandHeight(x, z + e) - islandHeight(x, z - e)) * (0.5f / e);
    float vx = -dx;
    float vy = 1.0f;
    float vz = -dz;
    const float len = std::sqrt(vx * vx + vy * vy + vz * vz);
    if (len > 1e-6f) {
        vx /= len;
        vy /= len;
        vz /= len;
    }
    *nx = vx;
    *ny = vy;
    *nz = vz;
}

void drawIslandTerrain() {
    glDisable(GL_LIGHTING);
    glColor3f(0.45f, 0.55f, 0.32f);

    const int n = 20;
    const float lo = -10.0f;
    const float hi = 10.0f;
    const float step = (hi - lo) / static_cast<float>(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const float x0 = lo + static_cast<float>(i) * step;
            const float x1 = x0 + step;
            const float z0 = lo + static_cast<float>(j) * step;
            const float z1 = z0 + step;
            const float y00 = islandHeight(x0, z0);
            const float y10 = islandHeight(x1, z0);
            const float y11 = islandHeight(x1, z1);
            const float y01 = islandHeight(x0, z1);
            float nx, ny, nz;
            glBegin(GL_QUADS);
            islandNormal(x0, z0, &nx, &ny, &nz);
            glNormal3f(nx, ny, nz);
            glVertex3f(x0, y00, z0);
            islandNormal(x1, z0, &nx, &ny, &nz);
            glNormal3f(nx, ny, nz);
            glVertex3f(x1, y10, z0);
            islandNormal(x1, z1, &nx, &ny, &nz);
            glNormal3f(nx, ny, nz);
            glVertex3f(x1, y11, z1);
            islandNormal(x0, z1, &nx, &ny, &nz);
            glNormal3f(nx, ny, nz);
            glVertex3f(x0, y01, z1);
            glEnd();
        }
    }
}

void drawStreetStrip() {
    glDisable(GL_LIGHTING);
    glColor3f(0.22f, 0.22f, 0.24f);
    const float y = -1.14f;
    glBegin(GL_QUADS);
    glVertex3f(-1.8f, y, -12.0f);
    glVertex3f(1.8f, y, -12.0f);
    glVertex3f(1.8f, y, 12.0f);
    glVertex3f(-1.8f, y, 12.0f);
    glEnd();
}

void drawWaterPlane() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glColor4f(0.25f, 0.48f, 0.72f, 0.62f);
    const float y = -1.08f;
    glBegin(GL_QUADS);
    glVertex3f(-35.0f, y, 35.0f);
    glVertex3f(35.0f, y, 35.0f);
    glVertex3f(35.0f, y, -35.0f);
    glVertex3f(-35.0f, y, -35.0f);
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawBirdsBezier() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    Vec3 birds[3];
    birdComputePositions(birds);
    for (int b = 0; b < 3; ++b) {
        const Vec3& p = birds[b];
        glColor3f(0.18f, 0.16f, 0.14f);
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glutSolidSphere(0.09, 10, 10);
        glPopMatrix();
    }
}

} // namespace

void drawSkybox() {
    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float t = dn.skyDayFactor;
    const float r = 0.008f + 0.12f * t;
    const float g = 0.012f + 0.18f * t;
    const float b = 0.028f + 0.35f * t;

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glColor3f(r, g, b);

    glPushMatrix();
    glTranslatef(gCamera.position.x, gCamera.position.y, gCamera.position.z);
    gluSphere(sphereQuadric, 80.0, 64, 64);
    glPopMatrix();

    glDepthMask(GL_TRUE);
}

void rasterScene() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);

    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float day = dn.skyDayFactor;

    glClearColor(0.02f + 0.14f * day, 0.03f + 0.18f * day, 0.06f + 0.24f * day, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(gCamera.fovViewDegree, static_cast<double>(gWindowWidth) / static_cast<double>(gWindowHeight), 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const Vec3 f = rayForward();
    const Vec3 target = add3(gCamera.position, f);
    gluLookAt(gCamera.position.x, gCamera.position.y, gCamera.position.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);

    drawSkybox();

    glDisable(GL_LIGHTING);
    glColor3f(0.11f, 0.13f, 0.16f);
    glBegin(GL_QUADS);
    glVertex3f(-40.0f, -1.2f, -40.0f);
    glVertex3f(40.0f, -1.2f, -40.0f);
    glVertex3f(40.0f, -1.2f, 40.0f);
    glVertex3f(-40.0f, -1.2f, 40.0f);
    glEnd();

    drawIslandTerrain();
    drawStreetStrip();

    glDisable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const RGBA& c = gSpheres[i].color;
        glColor3f(c.r, c.g, c.b);
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const RGBA& c = gBoxes[i].color;
        const bool thinPole = gBoxes[i].halfSize.x < 0.12f && gBoxes[i].halfSize.z < 0.12f;
        if (thinPole) {
            glColor3f(c.r * 0.85f + 0.15f, c.g * 0.85f + 0.15f, c.b * 0.85f + 0.15f);
        } else {
            glColor3f(c.r, c.g, c.b);
        }
        glPushMatrix();
        glTranslatef(gBoxes[i].center.x, gBoxes[i].center.y, gBoxes[i].center.z);
        glScalef(gBoxes[i].halfSize.x * 2.0f, gBoxes[i].halfSize.y * 2.0f, gBoxes[i].halfSize.z * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glColor3f(0.08f, 0.60f, 0.95f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutWireSphere(gWormhole.holeA.warpRadius, 24, 24);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutWireSphere(gWormhole.holeB.warpRadius, 24, 24);
    glPopMatrix();

    glColor3f(0.22f, 0.86f, 1.0f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutSolidSphere(gWormhole.holeA.coreRadius, 20, 16);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutSolidSphere(gWormhole.holeB.coreRadius, 20, 16);
    glPopMatrix();

    drawWaterPlane();
    drawBirdsBezier();
}
