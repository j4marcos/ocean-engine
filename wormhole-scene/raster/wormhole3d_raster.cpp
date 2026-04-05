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

void ensureReflectTex() {
    static int tw = 0;
    static int th = 0;
    if (gTexReflect == 0) {
        glGenTextures(1, &gTexReflect);
    }
    if (tw != gWindowWidth || th != gWindowHeight) {
        tw = gWindowWidth;
        th = gWindowHeight;
        if (tw < 1 || th < 1) {
            return;
        }
        glBindTexture(GL_TEXTURE_2D, gTexReflect);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void drawIslandTerrain() {
    if (!myTexture) {
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, myTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(0.45f, 0.55f, 0.32f);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

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
            const float u0 = static_cast<float>(i) / static_cast<float>(n) * 4.0f;
            const float u1 = static_cast<float>(i + 1) / static_cast<float>(n) * 4.0f;
            const float v0 = static_cast<float>(j) / static_cast<float>(n) * 4.0f;
            const float v1 = static_cast<float>(j + 1) / static_cast<float>(n) * 4.0f;

            glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(u0, v0);
            glVertex3f(x0, y00, z0);
            glTexCoord2f(u1, v0);
            glVertex3f(x1, y10, z0);
            glTexCoord2f(u1, v1);
            glVertex3f(x1, y11, z1);
            glTexCoord2f(u0, v1);
            glVertex3f(x0, y01, z1);
            glEnd();
        }
    }
    glDisable(GL_TEXTURE_2D);
}

void drawStreetStrip() {
    if (!myTexture) {
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, myTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_LIGHTING);
    glColor3f(0.22f, 0.22f, 0.24f);
    const float y = -1.14f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.8f, y, -12.0f);
    glTexCoord2f(8.0f, 0.0f);
    glVertex3f(1.8f, y, -12.0f);
    glTexCoord2f(8.0f, 2.0f);
    glVertex3f(1.8f, y, 12.0f);
    glTexCoord2f(0.0f, 2.0f);
    glVertex3f(-1.8f, y, 12.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void drawTreesLowPoly() {
    glEnable(GL_LIGHTING);
    const struct {
        float x, z;
    } trees[] = {
        {-5.5f, -3.5f},
        {4.2f, -4.0f},
        {-3.0f, -8.0f},
        {5.5f, -7.0f},
    };
    for (const auto& t : trees) {
        const float yb = islandHeight(t.x, t.z);
        glPushMatrix();
        glTranslatef(t.x, yb + 0.05f, t.z);
        glColor3f(0.28f, 0.2f, 0.12f);
        glPushMatrix();
        glTranslatef(0.0f, 0.15f, 0.0f);
        glScalef(0.12f, 0.35f, 0.12f);
        glutSolidCube(1.0f);
        glPopMatrix();
        glColor3f(0.15f, 0.48f, 0.22f);
        glTranslatef(0.0f, 0.55f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidCone(0.45, 0.9, 10, 10);
        glPopMatrix();
    }
}

void drawWaterReflective() {
    if (!gTexReflect) {
        return;
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gTexReflect);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(0.25f, 0.48f, 0.72f, 0.62f);
    const float y = -1.08f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-35.0f, y, 35.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(35.0f, y, 35.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(35.0f, y, -35.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-35.0f, y, -35.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawBirdsBezier() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    const float t = std::fmod(gSceneTimeSec * 0.12f, 1.0f);
    struct BirdPath {
        Vec3 p0, p1, p2, p3;
    };
    const BirdPath paths[3] = {
        {{-6.0f, 5.0f, -4.0f}, {-2.0f, 7.0f, -5.0f}, {3.0f, 6.0f, -7.0f}, {8.0f, 4.5f, -9.0f}},
        {{5.0f, 6.0f, -6.0f}, {1.0f, 8.0f, -7.0f}, {-4.0f, 7.0f, -8.0f}, {-9.0f, 5.0f, -10.0f}},
        {{0.0f, 4.0f, -3.0f}, {4.0f, 9.0f, -6.0f}, {-3.0f, 8.0f, -9.0f}, {6.0f, 5.0f, -11.0f}},
    };
    for (int b = 0; b < 3; ++b) {
        const float tb = std::fmod(t + static_cast<float>(b) * 0.31f, 1.0f);
        const Vec3 p = calculateBezierPoint(tb, paths[b].p0, paths[b].p1, paths[b].p2, paths[b].p3);
        glColor3f(0.18f, 0.16f, 0.14f);
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glutSolidSphere(0.09, 10, 10);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

} // namespace

void drawSkybox() {
    if (!gTexSky) {
        return;
    }

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gTexSky);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glPushMatrix();
    glTranslatef(gCamera.position.x, gCamera.position.y, gCamera.position.z);
    gluSphere(sphereQuadric, 80.0, 64, 64);
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void rasterScene() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    DayNightLighting dn;
    computeDayNightLighting(dn);
    const GLfloat lightDir[4] = {dn.sunDir.x, dn.sunDir.y, dn.sunDir.z, 0.0f};
    const float day = dn.skyDayFactor;
    const GLfloat lightDiffuse[4] = {0.96f * day, 0.94f * day, 0.88f * day, 1.0f};
    const GLfloat globalAmb[4] = {dn.ambient * 0.85f, dn.ambient * 0.88f, dn.ambient * 0.95f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

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

    ensureReflectTex();

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

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, myTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        GLfloat kd[4] = {gSpheres[i].color.r, gSpheres[i].color.g, gSpheres[i].color.b, 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        glPushMatrix();
        glTranslatef(gBoxes[i].center.x, gBoxes[i].center.y, gBoxes[i].center.z);
        glScalef(gBoxes[i].halfSize.x * 2.0f, gBoxes[i].halfSize.y * 2.0f, gBoxes[i].halfSize.z * 2.0f);
        GLfloat kd[4] = {gBoxes[i].color.r, gBoxes[i].color.g, gBoxes[i].color.b, 1.0f};
        GLfloat emis[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (gBoxes[i].halfSize.x < 0.12f && gBoxes[i].halfSize.z < 0.12f) {
            emis[0] = gBoxes[i].color.r * 0.85f;
            emis[1] = gBoxes[i].color.g * 0.85f;
            emis[2] = gBoxes[i].color.b * 0.85f;
        }
        glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
        glMaterialfv(GL_FRONT, GL_EMISSION, emis);
        glutSolidCube(1.0);
        GLfloat zeroEmis[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, zeroEmis);
        glPopMatrix();
    }

    drawTreesLowPoly();

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

    if (gTexReflect && gWindowWidth > 0 && gWindowHeight > 0) {
        glBindTexture(GL_TEXTURE_2D, gTexReflect);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gWindowWidth, gWindowHeight);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    drawWaterReflective();
    drawBirdsBezier();
}
